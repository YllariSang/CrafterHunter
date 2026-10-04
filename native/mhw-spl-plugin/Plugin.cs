using System.Buffers.Binary;
using System.Diagnostics;
using System.Net.Sockets;
using System.Numerics;
using System.Reflection;
using System.Text;
using SharpPluginLoader.Core;

namespace CrafterHunter.MHW;

public sealed class Plugin : IPlugin
{
    private const string BridgeHost = "127.0.0.1";
    private const int BridgePort = 38470;
    private const ushort ProtocolVersion = 1;
    private const byte MhwSource = 2;
    private const ushort HelloKind = 1;
    private const ushort HeartbeatKind = 3;
    private const ushort CameraStateKind = 10;
    private const ushort BlockPixelsKind = 20;
    private const ushort BlockPngKind = 21;
    private const int HeaderLength = 24;
    private const int MaximumPayloadLength = 1200;
    private const int CameraPayloadLength = 44;
    private const float MhwUnitsPerMetre = 100.0f;
    private static readonly long CameraStartupDelayTicks = Stopwatch.Frequency * 10;
    private static readonly byte[] HelloPayload =
        Encoding.UTF8.GetBytes("crafterhunter-mhw-spl/0.3.1");
    private static readonly object DiagnosticLock = new();

    private readonly object _lifecycleLock = new();
    private readonly RenderProbePlacement _probePlacement = new();
    private readonly MinecraftBlockRenderer _blockRenderer = new();
    private CancellationTokenSource? _cancellation;
    private Task? _endpointTask;
    private byte[]? _latestCameraPayload;
    private MinecraftBlockAsset? _blockAsset;
    private byte[]? _pendingPixels;
    private byte[]? _pendingPng;
    private long _lastBlockAssetTimestamp;
    private bool _renderProbeEnabled =
        Environment.GetEnvironmentVariable("CRAFTERHUNTER_CUBE_PROBE") == "1";
    private bool _blockCompareEnabled =
        Environment.GetEnvironmentVariable("CRAFTERHUNTER_BLOCK_COMPARE") == "1";
    private RenderCamera? _renderCamera;
    private int _renderFailureLogged;
    private bool _meshEnabled = true;
    private bool _linesEnabled = true;
    private bool _overlayEnabled = true;
    private int _bridgeReady;
    private int _cameraAttemptLogged;
    private int _cameraSuccessLogged;
    private long _cameraEnableTimestamp;
    private long _nextCameraSampleTimestamp;
    private long _nextRenderDiagnosticTimestamp;
    private int _probeDiagnosticsRemaining = 12;

    public string Name => "CrafterHunter MHW Endpoint";
    public string Author => "CrafterHunter contributors";

    public void OnLoad()
    {
        WriteDiagnostic("OnLoad entered.");

        lock (_lifecycleLock)
        {
            if (_endpointTask is not null)
            {
                WriteDiagnostic("OnLoad ignored because the endpoint is already running.");
                return;
            }

            Volatile.Write(ref _bridgeReady, 0);
            Volatile.Write(ref _renderCamera, null);
            Volatile.Write(ref _blockAsset, null);
            _probePlacement.Reset();
            _probeDiagnosticsRemaining = 12;
            Volatile.Write(
                ref _cameraEnableTimestamp,
                Stopwatch.GetTimestamp() + CameraStartupDelayTicks
            );
            var cancellation = new CancellationTokenSource();
            _cancellation = cancellation;
            _endpointTask = Task.Run(() => RunEndpointAsync(cancellation.Token));
        }

        WriteDiagnostic("Endpoint task started; camera reads remain disabled until bridge ACK + 10s grace.");
        if (_renderProbeEnabled)
        {
            WriteDiagnostic("Opt-in cube rendering probe enabled.");
        }
        if (_blockCompareEnabled)
        {
            WriteDiagnostic("Minecraft stone A/B/C comparison enabled; waiting for actual block asset.");
        }
    }

    public void OnUnload()
    {
        WriteDiagnostic("OnUnload entered.");
        Task? task;
        CancellationTokenSource? cancellation;

        lock (_lifecycleLock)
        {
            task = _endpointTask;
            cancellation = _cancellation;
            _endpointTask = null;
            _cancellation = null;
        }

        cancellation?.Cancel();
        try
        {
            task?.Wait(TimeSpan.FromSeconds(2));
        }
        catch (AggregateException exception)
            when (exception.InnerExceptions.All(inner => inner is TaskCanceledException))
        {
            // Cancellation is the expected unload path.
        }
        finally
        {
            Volatile.Write(ref _bridgeReady, 0);
            Volatile.Write(ref _renderCamera, null);
            Volatile.Write(ref _blockAsset, null);
            cancellation?.Dispose();
            WriteDiagnostic("Endpoint stopped.");
        }
    }

    public void OnUpdate(float deltaTime)
    {
        _ = deltaTime;
        var now = Stopwatch.GetTimestamp();
        if (Volatile.Read(ref _bridgeReady) == 0 ||
            now < Volatile.Read(ref _cameraEnableTimestamp))
        {
            return;
        }
        if (now < Volatile.Read(ref _nextCameraSampleTimestamp))
        {
            return;
        }
        Volatile.Write(
            ref _nextCameraSampleTimestamp,
            now + Stopwatch.Frequency / 20
        );

        if (Interlocked.Exchange(ref _cameraAttemptLogged, 1) == 0)
        {
            WriteDiagnostic("Beginning the first guarded camera read.");
        }

        try
        {
            if (!TryCaptureCameraPayload(
                    _renderProbeEnabled || _blockCompareEnabled,
                    out var payload,
                    out var renderPosition,
                    out var renderTarget))
            {
                Volatile.Write(ref _renderCamera, null);
                return;
            }

            RenderCamera? renderCamera = null;
            if ((_renderProbeEnabled || _blockCompareEnabled) &&
                _probePlacement.TryAnchor(renderPosition, renderTarget) &&
                _probePlacement.Centre is { } anchor)
            {
                renderCamera = new RenderCamera(anchor, _probePlacement.Right);
            }
            Volatile.Write(ref _renderCamera, renderCamera);
            if (renderCamera is not null && _probeDiagnosticsRemaining > 0 &&
                now >= _nextRenderDiagnosticTimestamp)
            {
                _nextRenderDiagnosticTimestamp = now + Stopwatch.Frequency * 5;
                _probeDiagnosticsRemaining--;
                TryLogRenderProbe(renderPosition, renderTarget, renderCamera.Anchor);
            }
            Interlocked.Exchange(ref _latestCameraPayload, payload);
            if (Interlocked.Exchange(ref _cameraSuccessLogged, 1) == 0)
            {
                WriteDiagnostic("First guarded camera read succeeded.");
            }
        }
        catch (Exception exception)
        {
            // A managed API failure must not escape an in-process plugin callback and
            // terminate MHW. Disable further camera reads for this plugin lifetime;
            // the diagnostic log preserves the actual exception for the next run.
            Volatile.Write(ref _bridgeReady, 0);
            Volatile.Write(ref _renderCamera, null);
            WriteDiagnostic($"Camera capture disabled after {exception.GetType().FullName}: {exception.Message}");
        }
    }

    public void OnRender()
    {
        var camera = Volatile.Read(ref _renderCamera);
        if ((!_renderProbeEnabled && !_blockCompareEnabled) ||
            Volatile.Read(ref _bridgeReady) == 0 || camera is null)
        {
            return;
        }

        try
        {
            if (_blockCompareEnabled)
            {
                var asset = Volatile.Read(ref _blockAsset);
                if (asset is not null && BlockAssetFresh())
                {
                    if (_meshEnabled)
                    {
                        try { _blockRenderer.DrawMesh(asset, camera.Anchor, camera.Right); }
                        catch (Exception exception)
                        {
                            _meshEnabled = false;
                            WriteDiagnostic($"Block method A failed: {exception}");
                        }
                    }
                    if (_linesEnabled)
                    {
                        try { _blockRenderer.DrawLines(asset, camera.Anchor, camera.Right); }
                        catch (Exception exception)
                        {
                            _linesEnabled = false;
                            WriteDiagnostic($"Block method B failed: {exception}");
                        }
                    }
                }
            }
            else
            {
                RenderProbe.Draw(camera.Anchor);
            }
        }
        catch (Exception exception)
        {
            _renderProbeEnabled = false;
            if (Interlocked.Exchange(ref _renderFailureLogged, 1) == 0)
            {
                WriteDiagnostic($"Rendering probe disabled after {exception.GetType().FullName}: {exception.Message}");
            }
        }
    }

    public void OnImGuiFreeRender()
    {
        if (!_blockCompareEnabled || !_overlayEnabled || Volatile.Read(ref _bridgeReady) == 0 ||
            Volatile.Read(ref _renderCamera) is not { } camera ||
            Volatile.Read(ref _blockAsset) is not { } asset || !BlockAssetFresh())
        {
            return;
        }
        try
        {
            _blockRenderer.DrawOverlay(asset, camera.Anchor, camera.Right);
        }
        catch (Exception exception)
        {
            _overlayEnabled = false;
            WriteDiagnostic($"Block method C failed: {exception}");
        }
    }

    private bool BlockAssetFresh() =>
        Stopwatch.GetTimestamp() - Volatile.Read(ref _lastBlockAssetTimestamp) <
        Stopwatch.Frequency * 6;

    private sealed record RenderCamera(Vector3 Anchor, Vector3 Right);

    private static void TryLogRenderProbe(Vector3 position, Vector3 target, Vector3 anchor)
    {
        try
        {
            var visible = CameraSystem.MainViewport.WorldToScreen(anchor, out var screen);
            Log.Info($"[CrafterHunter probe] camera={position} target={target} anchor={anchor} " +
                     $"screenVisible={visible} screen={screen}");
        }
        catch
        {
            // Diagnostics never determine whether camera telemetry or rendering runs.
        }
    }

    private static bool TryCaptureCameraPayload(
        bool captureRenderTarget,
        out byte[] payload,
        out Vector3 renderPosition,
        out Vector3 renderTarget
    )
    {
        payload = Array.Empty<byte>();
        renderPosition = default;
        renderTarget = default;

        // CameraSystem.MainViewport assumes sMhCamera is non-null and immediately
        // dereferences it. Check the singleton explicitly before using that helper.
        if (SingletonManager.GetSingleton("sMhCamera") is null)
        {
            return false;
        }

        var viewport = CameraSystem.MainViewport;
        var camera = viewport.Camera;
        if (camera is null || !Matrix4x4.Invert(viewport.ViewMatrix, out var cameraWorld))
        {
            return false;
        }
        if (!Matrix4x4.Decompose(cameraWorld, out _, out var rotation, out _))
        {
            return false;
        }

        var position = camera.Position / MhwUnitsPerMetre;
        rotation = Quaternion.Normalize(rotation);
        var fieldOfView = camera.FieldOfView;
        if (fieldOfView > MathF.PI)
        {
            // MHW build 421810 stores this field in degrees despite older API docs
            // describing radians.
            fieldOfView *= MathF.PI / 180.0f;
        }
        var aspectRatio = camera.AspectRatio;
        var nearPlane = camera.NearClip / MhwUnitsPerMetre;
        var farPlane = camera.FarClip / MhwUnitsPerMetre;

        Span<float> values =
        [
            position.X,
            position.Y,
            position.Z,
            rotation.X,
            rotation.Y,
            rotation.Z,
            rotation.W,
            fieldOfView,
            aspectRatio,
            nearPlane,
            farPlane,
        ];
        if (!AllFinite(values) || aspectRatio <= 0 || nearPlane <= 0 || farPlane <= nearPlane)
        {
            return false;
        }

        if (captureRenderTarget)
        {
            renderPosition = camera.Position;
            renderTarget = camera.GetTargetWorld();
        }
        payload = new byte[CameraPayloadLength];
        for (var index = 0; index < values.Length; index++)
        {
            BinaryPrimitives.WriteInt32LittleEndian(
                payload.AsSpan(index * sizeof(float), sizeof(float)),
                BitConverter.SingleToInt32Bits(values[index])
            );
        }
        return true;
    }

    private async Task RunEndpointAsync(CancellationToken cancellationToken)
    {
        uint sequence = 0;

        while (!cancellationToken.IsCancellationRequested)
        {
            try
            {
                using var client = new UdpClient();
                client.Connect(BridgeHost, BridgePort);
                client.Client.ReceiveTimeout = 1000;

                Send(client, HelloKind, ++sequence, HelloPayload);
                var remoteEndpoint = new System.Net.IPEndPoint(System.Net.IPAddress.Any, 0);
                var acknowledgement = client.Receive(ref remoteEndpoint);
                if (!IsHelloAcknowledgement(acknowledgement))
                {
                    WriteDiagnostic("Bridge returned an invalid HelloAck; retrying.");
                    await RetryDelay(cancellationToken).ConfigureAwait(false);
                    continue;
                }

                Volatile.Write(ref _bridgeReady, 1);
                WriteDiagnostic("Bridge HelloAck received; camera reads are armed after the startup grace.");
                var nextHeartbeat = Stopwatch.GetTimestamp();

                while (!cancellationToken.IsCancellationRequested)
                {
                    while (client.Available > 0)
                    {
                        var inbound = client.Receive(ref remoteEndpoint);
                        HandleMinecraftAssetPacket(inbound);
                    }
                    var cameraPayload = Interlocked.Exchange(ref _latestCameraPayload, null);
                    if (cameraPayload is not null)
                    {
                        Send(client, CameraStateKind, ++sequence, cameraPayload);
                    }

                    var now = Stopwatch.GetTimestamp();
                    if (now >= nextHeartbeat)
                    {
                        Send(client, HeartbeatKind, ++sequence, Array.Empty<byte>());
                        nextHeartbeat = now + Stopwatch.Frequency;
                    }

                    await Task.Delay(TimeSpan.FromMilliseconds(10), cancellationToken)
                        .ConfigureAwait(false);
                }
            }
            catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
            {
                return;
            }
            catch (SocketException)
            {
                await RetryDelay(cancellationToken).ConfigureAwait(false);
            }
            catch (Exception exception)
            {
                WriteDiagnostic($"Endpoint retry after {exception.GetType().FullName}: {exception.Message}");
                await RetryDelay(cancellationToken).ConfigureAwait(false);
            }
            finally
            {
                Volatile.Write(ref _bridgeReady, 0);
            }
        }
    }

    private static bool IsHelloAcknowledgement(ReadOnlySpan<byte> packet)
    {
        return packet.Length >= HeaderLength &&
               packet[..4].SequenceEqual("CHNT"u8) &&
               BinaryPrimitives.ReadUInt16LittleEndian(packet.Slice(4, 2)) == ProtocolVersion &&
               BinaryPrimitives.ReadUInt16LittleEndian(packet.Slice(6, 2)) == 2 &&
               packet[8] == 1 &&
               packet[9] == 0 &&
               packet[10] == 0 &&
               packet[11] == 0 &&
               BinaryPrimitives.ReadUInt32LittleEndian(packet.Slice(16, 4)) ==
                   (uint)(packet.Length - HeaderLength);
    }

    private void HandleMinecraftAssetPacket(ReadOnlySpan<byte> packet)
    {
        if (packet.Length < HeaderLength || packet.Length > HeaderLength + MaximumPayloadLength ||
            !packet[..4].SequenceEqual("CHNT"u8) ||
            BinaryPrimitives.ReadUInt16LittleEndian(packet.Slice(4, 2)) != ProtocolVersion ||
            packet[8] != 3 || packet[9] != 0 || packet[10] != 0 || packet[11] != 0 ||
            BinaryPrimitives.ReadUInt32LittleEndian(packet.Slice(16, 4)) !=
                (uint)(packet.Length - HeaderLength))
        {
            return;
        }

        var kind = BinaryPrimitives.ReadUInt16LittleEndian(packet.Slice(6, 2));
        var payload = packet[HeaderLength..];
        if (kind == BlockPixelsKind &&
            MinecraftBlockAsset.TryReadPixels(payload, out var pixels))
        {
            _pendingPixels = pixels;
        }
        else if (kind == BlockPngKind &&
                 MinecraftBlockAsset.TryReadPng(payload, out var png))
        {
            _pendingPng = png;
        }
        else
        {
            return;
        }

        if (_pendingPixels is not { } rgba || _pendingPng is not { } image)
        {
            return;
        }
        var current = Volatile.Read(ref _blockAsset);
        if (current is null || !current.Rgba.AsSpan().SequenceEqual(rgba) ||
            !current.Png.AsSpan().SequenceEqual(image))
        {
            Volatile.Write(ref _blockAsset,
                new MinecraftBlockAsset(MinecraftBlockAsset.ExpectedBlockId, rgba, image));
            WriteDiagnostic("Received actual minecraft:stone pixels and PNG from Minecraft.");
        }
        Volatile.Write(ref _lastBlockAssetTimestamp, Stopwatch.GetTimestamp());
    }

    private static bool AllFinite(ReadOnlySpan<float> values)
    {
        foreach (var value in values)
        {
            if (!float.IsFinite(value))
            {
                return false;
            }
        }
        return true;
    }

    private static async Task RetryDelay(CancellationToken cancellationToken)
    {
        try
        {
            await Task.Delay(TimeSpan.FromSeconds(2), cancellationToken).ConfigureAwait(false);
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            // Let the outer loop observe cancellation.
        }
    }

    private static void Send(
        UdpClient client,
        ushort kind,
        uint sequence,
        byte[] payload
    )
    {
        if (payload.Length > MaximumPayloadLength)
        {
            throw new ArgumentOutOfRangeException(nameof(payload));
        }

        var packet = new byte[HeaderLength + payload.Length];
        Encoding.ASCII.GetBytes("CHNT", packet);
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4, 2), ProtocolVersion);
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(6, 2), kind);
        packet[8] = MhwSource;
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(12, 4), sequence);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(16, 4), (uint)payload.Length);
        payload.CopyTo(packet, HeaderLength);
        client.Send(packet, packet.Length);
    }

    private static void WriteDiagnostic(string message)
    {
        try
        {
            var assemblyDirectory = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            if (string.IsNullOrEmpty(assemblyDirectory))
            {
                return;
            }

            var line = $"{DateTimeOffset.Now:O} [CrafterHunter.MHW 0.3.1] {message}{Environment.NewLine}";
            lock (DiagnosticLock)
            {
                File.AppendAllText(Path.Combine(assemblyDirectory, "CrafterHunter.runtime.log"), line);
            }
        }
        catch
        {
            // Diagnostics are best-effort and must never affect the host process.
        }
    }
}
