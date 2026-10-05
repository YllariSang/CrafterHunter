using System.Diagnostics;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using SharpPluginLoader.Core;
using SharpPluginLoader.Core.Entities;
using SharpPluginLoader.Core.Memory;

namespace CrafterHunter.MHW;

/// <summary>
/// Host terrain queries for milestone 3, stage A: resolve the game's own
/// segment cast behind an executable fingerprint check, then prove it on the
/// host by casting a down-ray from the hunter and comparing where it lands
/// with <c>Model.CollisionPosition</c>.
///
/// Three rules shape this class. Resolution happens once, in
/// <see cref="Initialize"/>, before anything is called: a missing fingerprint
/// or an unresolved pattern disables terrain for the session rather than
/// falling back to a remembered address. Rays run only from
/// <see cref="Tick"/>, which the game thread calls through
/// <c>Plugin.OnUpdate</c>, at most <see cref="RaysPerTick"/> of them per sample.
/// And every native call is wrapped so a failure disables queries instead of
/// escaping into the game process.
///
/// Requests arrive on the endpoint thread and are queued, never cast there: the
/// game's collision routine is game-thread-only. The queue has a fixed depth and
/// its overflow is dropped and counted rather than accumulated, because a guest
/// that outruns the host should lose answers, not stall the game.
public sealed class TerrainAdapter : IDisposable
{
    private const string ExecutableName = "MonsterHunterWorld.exe";
    private const string CollisionSingletonName = "sMhCollision";
    private const string PluginFolder = "nativePC/plugins/CSharp/CrafterHunter";

    private const int ParameterBytes = 0x140;
    private const int TriangleBytes = 0x140;
    private const int SegmentFloats = 8;
    private const int ParameterFilterOffset = 0xF9;
    private const int TriangleNormalOffset = 0xB0;
    private const int TrianglePositionOffset = 0xC0;

    private const float StartAboveMetres = 1.0f;
    private const float DepthMetres = 5.0f;
    private const float AgreementToleranceMetres = 0.5f;

    /// <summary>Rays cast per sample, and requests allowed to wait.</summary>
    private const int RaysPerTick = 2;
    private const int RequestQueueDepth = 8;
    private const int ResultQueueDepth = 32;

    /// <summary>
    /// The sample cadence. The state machine and the queue are cheap, and
    /// twenty hertz bounds a guest's wait for an answer without turning into a
    /// frame spike.
    /// </summary>
    private const double SampleSeconds = 0.05;

    /// <summary>Requests refused because the queue was full, between reports.</summary>
    private const int DropReportInterval = 64;

    private readonly Action<string> _log;

    // Requests cross from the endpoint thread to the game thread through this
    // queue and never anywhere else: the cast may only happen on the game
    // thread, and the endpoint thread may only touch bytes.
    private readonly object _queueLock = new();
    private readonly Queue<(uint Id, Vector3 Start, Vector3 End)> _requests = new();
    private readonly Queue<byte[]> _results = new();
    private int _droppedRequests;
    private int _reportedDrops;
    private int _droppedResults;

    // The three caller-owned buffers the call sequence needs: a filter block,
    // a triangle-info block, and eight segment floats. Each is allocated as
    // size + 16 bytes and then rounded up to a 16-byte boundary, because the
    // game reads them aligned.
    private IntPtr _rawParameter;
    private IntPtr _rawTriangle;
    private IntPtr _rawSegment;
    private IntPtr _parameter;
    private IntPtr _triangle;
    private IntPtr _segment;

    private readonly byte[] _emptyBlock = new byte[Math.Max(ParameterBytes, TriangleBytes)];
    private readonly float[] _segmentValues = new float[SegmentFloats];
    private readonly float[] _hitPosition = new float[3];
    private readonly float[] _hitNormal = new float[3];

    private bool _initialized;
    private bool _disabled;
    private string? _disabledReason;

    private TerrainRay.State _state = TerrainRay.State.Unavailable;
    private long _nextSample;
    private int _selfChecks;
    private bool _singletonFailureLogged;

    private NativeAction<IntPtr, uint, uint, uint, uint, uint, ulong, byte, uint, uint, byte> _parameterConstructor;
    private NativeAction<IntPtr, uint, uint, uint, byte> _parameterSetAttribute;
    private NativeAction<IntPtr> _parameterDestructor;
    private NativeAction<IntPtr> _triangleConstructor;
    private NativeAction<IntPtr> _triangleReset;
    private NativeFunction<IntPtr, IntPtr, byte, IntPtr, IntPtr, int> _checkSegment;
    private NativeFunction<IntPtr, uint, uint> _triangleAttribute;

    public TerrainAdapter(Action<string> log)
    {
        _log = log;
    }

    /// <summary>The state machine's current position, for tests and diagnostics.</summary>
    public TerrainRay.State State => _state;

    /// <summary>True once a fingerprint or signature failure has latched.</summary>
    public bool Disabled => _disabled;

    /// <summary>The first failure that latched <see cref="Disabled"/>.</summary>
    public string? DisabledReason => _disabledReason;

    /// <summary>
    /// Whether the optional surface-filter setter resolved. Stage A casts
    /// without a filter, so the delegate is bound but unused; the property
    /// keeps that visible instead of leaving a resolved pattern unexplained.
    /// </summary>
    public bool FilterSetterResolved => _parameterSetAttribute.NativePointer != IntPtr.Zero;

    /// <summary>
    /// Fingerprint the executable, resolve all seven patterns against the
    /// loaded image, and allocate the call buffers. Any failure latches
    /// <see cref="Disabled"/>; nothing later can move it back.
    /// </summary>
    public void Initialize()
    {
        if (_initialized || _disabled)
        {
            return;
        }

        try
        {
            if (!TryFingerprintExecutable(out var fingerprintReason))
            {
                Disable(fingerprintReason!);
                return;
            }

            foreach (var (name, pattern) in TerrainRay.Signatures)
            {
                IntPtr address;
                try
                {
                    address = PatternScanner.FindFirst(pattern, cache: true);
                }
                catch (Exception exception)
                {
                    Disable($"pattern {name} could not be parsed: {exception.Message}");
                    return;
                }

                if (address == IntPtr.Zero)
                {
                    Disable($"pattern {name} did not resolve in the loaded image");
                    return;
                }

                if (!Bind(name, address))
                {
                    Disable($"pattern {name} is not part of the adapter's call sequence");
                    return;
                }
            }

            _parameter = AllocateAligned(ParameterBytes, out _rawParameter);
            _triangle = AllocateAligned(TriangleBytes, out _rawTriangle);
            _segment = AllocateAligned(SegmentFloats * sizeof(float), out _rawSegment);

            _initialized = true;
            _log(
                $"Terrain adapter ready: all {TerrainRay.Signatures.Length} signatures resolved " +
                "against the pinned executable.");
        }
        catch (Exception exception)
        {
            Disable($"initialization failed with {exception.GetType().FullName}: {exception.Message}");
        }
    }

    /// <summary>
    /// Publish the state machine, spend the sample's ray budget on queued
    /// requests, and fall back to the request file's self-check when the queue is
    /// empty. The caller is <c>Plugin.OnUpdate</c> on the game thread. State is
    /// published whether or not a ray runs, so a loading screen reads
    /// "unavailable" rather than the last answer the ray gave, and a request
    /// that arrives while the world is not ready is answered "no terrain" rather
    /// than dropped.
    /// </summary>
    public unsafe void Tick(long now)
    {
        _nextSample = now + Ticks(SampleSeconds);
        if (_disabled || !_initialized)
        {
            return;
        }

        try
        {
            var hunter = Player.MainPlayer;
            var collisionPresent = TryGetCollisionSingleton(out var collision);
            var state = TerrainRay.Classify(
                fingerprintMatched: true,
                signaturesResolved: true,
                collisionSingletonPresent: collisionPresent,
                hunterPresent: hunter is not null);

            if (state != _state)
            {
                _state = state;
                _log($"Terrain state changed to {_state}.");
            }

            var spent = 0;
            while (spent < RaysPerTick && TryTakeRequest(out var request))
            {
                spent++;
                if (state == TerrainRay.State.Ready)
                {
                    Answer(collision, request);
                }
                else
                {
                    // Unavailable is a state, not a silence: the guest is told
                    // now rather than left to time out holding an old hit.
                    Publish(
                        TerrainPacket.EncodeResult(
                            request.Id, TerrainPacket.StatusNoTerrain, Vector3.Zero, Vector3.Zero, 0));
                }
            }

            if (spent == 0 && TerrainRequest.ConsumeCheck(PluginFolder))
            {
                SelfCheck(hunter, collision, state);
            }
        }
        catch (Exception exception)
        {
            // A native failure must not escape an in-process callback and
            // terminate MHW; terrain queries switch off for the session and
            // the diagnostic log keeps the exception for the next run.
            Disable($"terrain ray failed with {exception.GetType().FullName}: {exception.Message}");
        }
    }

    /// <summary>
    /// Accept a request from the guest. Called on the endpoint thread: it only
    /// parses and queues. A full queue drops the request and counts it, because
    /// a guest that outruns the host must lose answers rather than make the
    /// game wait.
    /// </summary>
    public void Enqueue(uint id, Vector3 start, Vector3 end)
    {
        lock (_queueLock)
        {
            if (_requests.Count >= RequestQueueDepth)
            {
                _droppedRequests++;
                if (_droppedRequests - _reportedDrops >= DropReportInterval)
                {
                    _reportedDrops = _droppedRequests;
                    _log(
                        $"Terrain requests dropped: {_droppedRequests} in total, " +
                        $"queue depth {RequestQueueDepth}.");
                }

                return;
            }

            _requests.Enqueue((id, start, end));
        }
    }

    /// <summary>Take one answer for the endpoint thread to send.</summary>
    public bool TryTakeResult(out byte[] payload)
    {
        lock (_queueLock)
        {
            if (_results.Count == 0)
            {
                payload = Array.Empty<byte>();
                return false;
            }

            payload = _results.Dequeue();
            return true;
        }
    }

    private bool TryTakeRequest(out (uint Id, Vector3 Start, Vector3 End) request)
    {
        lock (_queueLock)
        {
            if (_requests.Count == 0)
            {
                request = default;
                return false;
            }

            request = _requests.Dequeue();
            return true;
        }
    }

    private void Publish(byte[] payload)
    {
        lock (_queueLock)
        {
            if (_results.Count >= ResultQueueDepth)
            {
                // The endpoint thread is behind, not the guest: drop the newest
                // answer and count it rather than growing without bound.
                _droppedResults++;
                if (_droppedResults % DropReportInterval == 0)
                {
                    _log($"Terrain answers dropped: {_droppedResults} in total.");
                }

                return;
            }

            _results.Enqueue(payload);
        }
    }

    public void Dispose()
    {
        FreeAligned(_rawParameter, _parameter);
        FreeAligned(_rawTriangle, _triangle);
        FreeAligned(_rawSegment, _segment);
        _rawParameter = IntPtr.Zero;
        _rawTriangle = IntPtr.Zero;
        _rawSegment = IntPtr.Zero;
        _parameter = IntPtr.Zero;
        _triangle = IntPtr.Zero;
        _segment = IntPtr.Zero;
        _initialized = false;
    }

    /// <summary>
    /// Answer one queued request: cast the guest's segment and publish the
    /// result, with a miss carrying no position so it cannot be read as a hit at
    /// the origin.
    /// </summary>
    private unsafe void Answer(IntPtr collision, (uint Id, Vector3 Start, Vector3 End) request)
    {
        var hit = TryCast(collision, request.Start, request.End, out var position, out var normal, out var attribute);
        Publish(
            TerrainPacket.EncodeResult(
                request.Id,
                hit ? TerrainPacket.StatusHit : TerrainPacket.StatusMiss,
                hit ? position : Vector3.Zero,
                hit ? normal : Vector3.Zero,
                hit ? attribute : 0u));
    }

    /// <summary>
    /// The request-file self-check: cast from above the hunter down through the
    /// ground, then compare where it landed with the hunter's own collision
    /// point and report the numbers. That comparison is milestone 3's
    /// ground-truth rule: the ray has to hit the ground the hunter stands on,
    /// flat and on slopes.
    /// </summary>
    private unsafe void SelfCheck(Player? hunter, IntPtr collision, TerrainRay.State state)
    {
        _selfChecks++;
        if (hunter is null || state != TerrainRay.State.Ready)
        {
            _log(
                $"Terrain self-check {_selfChecks} refused: the state is {_state}, " +
                "so no ray was cast.");
            return;
        }

        var position = hunter.Position / TerrainRay.UnitsPerMetre;
        TerrainRay.DownSegment(position, StartAboveMetres, DepthMetres, out var start, out var end);
        if (!IsFinite(start) || !IsFinite(end))
        {
            _log($"Terrain self-check {_selfChecks} refused: the hunter's position is not finite.");
            return;
        }

        if (!TryCast(collision, start, end, out var hit, out var normal, out var attribute))
        {
            _log($"Terrain self-check {_selfChecks}: no hit in {DepthMetres:F1}m below the hunter.");
            return;
        }

        var collisionY = hunter.CollisionPosition.Y / TerrainRay.UnitsPerMetre;
        _log(
            $"Terrain self-check {_selfChecks}: hit=True agree=" +
            $"{TerrainRay.Agrees(hit.Y, collisionY, AgreementToleranceMetres)} " +
            $"rayY={hit.Y:F3}m collisionY={collisionY:F3}m " +
            $"delta={MathF.Abs(hit.Y - collisionY):F3}m positionY={position.Y:F3}m " +
            $"normal=({normal.X:F2}, {normal.Y:F2}, {normal.Z:F2}) attr={attribute}");
    }

    /// <summary>
    /// Cast one segment and release both buffers before returning, whatever the
    /// result. Metres go in and come out; the game only ever sees its own units.
    /// </summary>
    private unsafe bool TryCast(
        IntPtr collision,
        Vector3 startMetres,
        Vector3 endMetres,
        out Vector3 hitPosition,
        out Vector3 hitNormal,
        out uint attribute)
    {
        hitPosition = Vector3.Zero;
        hitNormal = Vector3.Zero;
        attribute = 0;

        var start = startMetres * TerrainRay.UnitsPerMetre;
        var end = endMetres * TerrainRay.UnitsPerMetre;
        if (!IsFinite(start) || !IsFinite(end))
        {
            return false;
        }

        _segmentValues[0] = start.X;
        _segmentValues[1] = start.Y;
        _segmentValues[2] = start.Z;
        _segmentValues[3] = 0;
        _segmentValues[4] = end.X;
        _segmentValues[5] = end.Y;
        _segmentValues[6] = end.Z;
        _segmentValues[7] = 0;
        Marshal.Copy(_segmentValues, 0, _segment, SegmentFloats);

        Marshal.Copy(_emptyBlock, 0, _parameter, ParameterBytes);
        Marshal.Copy(_emptyBlock, 0, _triangle, TriangleBytes);

        // The call sequence from docs/terrain-query.md, in the order the prior
        // art established for this build: build the filter, build the triangle
        // buffer, cast, read, then release both buffers before returning.
        _parameterConstructor.Invoke(
            _parameter,
            0x7FFFFFFFu,
            0x3FFFFFFFu,
            0u,
            0u,
            0xAu,
            0UL,
            (byte)1,
            1u,
            0u,
            (byte)1);
        Marshal.WriteByte(_parameter, ParameterFilterOffset, 0);
        _triangleConstructor.Invoke(_triangle);

        var hits = _checkSegment.Invoke(collision, _segment, 1, _triangle, _parameter);
        try
        {
            if (hits <= 0)
            {
                return false;
            }

            Marshal.Copy(_triangle + TrianglePositionOffset, _hitPosition, 0, 3);
            Marshal.Copy(_triangle + TriangleNormalOffset, _hitNormal, 0, 3);
            attribute = _triangleAttribute.Invoke(_triangle, 0);
            hitPosition = new Vector3(
                _hitPosition[0] / TerrainRay.UnitsPerMetre,
                _hitPosition[1] / TerrainRay.UnitsPerMetre,
                _hitPosition[2] / TerrainRay.UnitsPerMetre);
            hitNormal = new Vector3(
                _hitNormal[0] / TerrainRay.UnitsPerMetre,
                _hitNormal[1] / TerrainRay.UnitsPerMetre,
                _hitNormal[2] / TerrainRay.UnitsPerMetre);
            return true;
        }
        finally
        {
            _triangleReset.Invoke(_triangle);
            _parameterDestructor.Invoke(_parameter);
        }
    }

    private static long Ticks(double seconds) => (long)(Stopwatch.Frequency * seconds);

    private static bool IsFinite(Vector3 value) =>
        float.IsFinite(value.X) && float.IsFinite(value.Y) && float.IsFinite(value.Z);

    private static IntPtr AllocateAligned(int size, out IntPtr raw)
    {
        raw = Marshal.AllocHGlobal(size + 16);
        var aligned = (raw.ToInt64() + 15) & ~15L;
        return (IntPtr)aligned;
    }

    private static void FreeAligned(IntPtr raw, IntPtr aligned)
    {
        _ = aligned;
        if (raw != IntPtr.Zero)
        {
            Marshal.FreeHGlobal(raw);
        }
    }

    private bool TryFingerprintExecutable(out string? reason)
    {
        var path = Path.GetFullPath(ExecutableName);
        if (!File.Exists(path))
        {
            reason = $"{ExecutableName} was not found at {path}";
            return false;
        }

        byte[] hash;
        using (var stream = File.OpenRead(path))
        {
            hash = SHA256.HashData(stream);
        }

        if (!Convert.ToHexString(hash)
                .Equals(TerrainRay.PinnedExecutableSha256, StringComparison.OrdinalIgnoreCase))
        {
            reason = $"{ExecutableName} does not match the pinned SHA-256";
            return false;
        }

        reason = null;
        return true;
    }

    private bool Bind(string name, IntPtr address)
    {
        switch (name)
        {
            case "sCollision::CheckSegment":
                _checkSegment = new NativeFunction<IntPtr, IntPtr, byte, IntPtr, IntPtr, int>(address);
                return true;
            case "TriangleInfo::ctor":
                _triangleConstructor = new NativeAction<IntPtr>(address);
                return true;
            case "TriangleInfo::reset":
                _triangleReset = new NativeAction<IntPtr>(address);
                return true;
            case "Param::ctor":
                _parameterConstructor =
                    new NativeAction<IntPtr, uint, uint, uint, uint, uint, ulong, byte, uint, uint, byte>(address);
                return true;
            case "Param::setAttr":
                _parameterSetAttribute = new NativeAction<IntPtr, uint, uint, uint, byte>(address);
                return true;
            case "Param::dtor":
                _parameterDestructor = new NativeAction<IntPtr>(address);
                return true;
            case "TriangleInfo::attr":
                _triangleAttribute = new NativeFunction<IntPtr, uint, uint>(address);
                return true;
            default:
                return false;
        }
    }

    private bool TryGetCollisionSingleton(out IntPtr instance)
    {
        instance = IntPtr.Zero;
        try
        {
            var singleton = SingletonManager.GetSingleton(CollisionSingletonName);
            if (singleton is null)
            {
                return false;
            }

            instance = singleton.Instance;
            return instance != IntPtr.Zero;
        }
        catch (Exception exception)
        {
            if (!_singletonFailureLogged)
            {
                _singletonFailureLogged = true;
                _log(
                    "Terrain collision singleton lookup failed with " +
                    $"{exception.GetType().FullName}: {exception.Message}");
            }

            instance = IntPtr.Zero;
            return false;
        }
    }

    private void Disable(string reason)
    {
        if (_disabled)
        {
            return;
        }

        _disabled = true;
        _disabledReason = reason;
        _state = TerrainRay.State.Disabled;
        _log($"Terrain queries disabled for this session: {reason}");
    }
}
