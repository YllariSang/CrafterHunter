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
    private const int HeaderLength = 24;
    private const int MaximumPayloadLength = 1200;
    private const int CameraPayloadLength = 44;
    private const float MhwUnitsPerMetre = 100.0f;
    private static readonly long CameraStartupDelayTicks = Stopwatch.Frequency * 10;
    private static readonly byte[] HelloPayload =
        Encoding.UTF8.GetBytes("crafterhunter-mhw-spl/0.2.0");
    private static readonly object DiagnosticLock = new();

    private readonly object _lifecycleLock = new();
    private CancellationTokenSource? _cancellation;
    private Task? _endpointTask;
    private byte[]? _latestCameraPayload;
    private bool _renderProbeEnabled =
        Environment.GetEnvironmentVariable("CRAFTERHUNTER_CUBE_PROBE") == "1";
    private RenderCamera? _renderCamera;
    private int _renderFailureLogged;
    private int _bridgeReady;
    private int _cameraAttemptLogged;
    private int _cameraSuccessLogged;
    private long _cameraEnableTimestamp;
    private long _nextCameraSampleTimestamp;

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
            if (!TryCaptureCameraPayload(out var payload, out var cameraPosition, out var cameraRotation))
            {
                Volatile.Write(ref _renderCamera, null);
                return;
            }

            Volatile.Write(ref _renderCamera, new RenderCamera(cameraPosition, cameraRotation));
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
        if (!_renderProbeEnabled || Volatile.Read(ref _bridgeReady) == 0 || camera is null)
        {
            return;
        }

        try
        {
            RenderProbe.Draw(camera.Position, camera.Rotation);
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

    private sealed record RenderCamera(Vector3 Position, Quaternion Rotation);

    private static bool TryCaptureCameraPayload(
        out byte[] payload,
        out Vector3 cameraPosition,
        out Quaternion cameraRotation
    )
    {
        payload = Array.Empty<byte>();
        cameraPosition = default;
        cameraRotation = default;

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

        cameraPosition = camera.Position;
        cameraRotation = rotation;
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

            var line = $"{DateTimeOffset.Now:O} [CrafterHunter.MHW 0.2.0] {message}{Environment.NewLine}";
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
