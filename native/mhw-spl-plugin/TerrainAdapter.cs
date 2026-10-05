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
/// <c>Plugin.OnUpdate</c>, and only when a request is pending and at most one
/// per sample. And every native call is wrapped so a failure disables queries
/// instead of escaping into the game process.
/// </summary>
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

    /// <summary>
    /// The state machine and the request file are sampled once a second; at
    /// most one ray is cast per sample, and only on request. The queue that
    /// guest requests will arrive on in stage B spends the same budget rather
    /// than a second, larger one.
    /// </summary>
    private const double SampleSeconds = 1.0;

    private readonly Action<string> _log;

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
    /// Publish the state machine once a second and cast at most one ray, only
    /// when a request is pending. The caller is <c>Plugin.OnUpdate</c> on the
    /// game thread. State is published whether or not a ray runs, so a loading
    /// screen reads "unavailable" rather than the last answer the ray gave, and
    /// a request that arrives while the world is not ready is refused loudly
    /// instead of quietly.
    /// </summary>
    public unsafe void Tick(long now)
    {
        if (_disabled || !_initialized)
        {
            return;
        }

        if (now < _nextSample)
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

            if (!TerrainRequest.ConsumeCheck(PluginFolder))
            {
                _nextSample = now + Ticks(SampleSeconds);
                return;
            }

            _selfChecks++;
            if (state != TerrainRay.State.Ready)
            {
                _log(
                    $"Terrain self-check {_selfChecks} refused: the state is {_state}, " +
                    "so no ray was cast.");
                _nextSample = now + Ticks(SampleSeconds);
                return;
            }

            CastDownRay(hunter!, collision);
            _nextSample = now + Ticks(SampleSeconds);
        }
        catch (Exception exception)
        {
            // A native failure must not escape an in-process callback and
            // terminate MHW; terrain queries switch off for the session and
            // the diagnostic log keeps the exception for the next run.
            Disable($"terrain ray failed with {exception.GetType().FullName}: {exception.Message}");
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
    /// Cast one ray from above the hunter down through the ground, then compare
    /// where it landed with the hunter's own collision point and report the
    /// numbers. That comparison is milestone 3's ground-truth rule: the ray has
    /// to hit the ground the hunter stands on, flat and on slopes.
    /// </summary>
    private unsafe void CastDownRay(Player hunter, IntPtr collision)
    {
        var position = hunter.Position;
        TerrainRay.DownSegment(
            position, StartAboveMetres, DepthMetres, TerrainRay.UnitsPerMetre, out var start, out var end);
        if (!IsFinite(start) || !IsFinite(end))
        {
            return;
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
            if (hits > 0)
            {
                Marshal.Copy(_triangle + TrianglePositionOffset, _hitPosition, 0, 3);
                Marshal.Copy(_triangle + TriangleNormalOffset, _hitNormal, 0, 3);
                Report(hunter, position, hit: true, hits, attribute: _triangleAttribute.Invoke(_triangle, 0));
            }
            else
            {
                Report(hunter, position, hit: false, hits, attribute: 0);
            }
        }
        finally
        {
            _triangleReset.Invoke(_triangle);
            _parameterDestructor.Invoke(_parameter);
        }
    }

    private void Report(Player hunter, Vector3 position, bool hit, int hits, uint attribute)
    {
        if (!hit)
        {
            _log($"Terrain self-check {_selfChecks}: no hit in {DepthMetres:F1}m below the hunter.");
            return;
        }

        var collisionY = hunter.CollisionPosition.Y;
        var rayY = _hitPosition[1];
        var agree = TerrainRay.Agrees(
            rayY, collisionY, AgreementToleranceMetres, TerrainRay.UnitsPerMetre);
        _log(
            $"Terrain self-check {_selfChecks}: hit={hits} agree={agree} " +
            $"rayY={rayY / TerrainRay.UnitsPerMetre:F3}m " +
            $"collisionY={collisionY / TerrainRay.UnitsPerMetre:F3}m " +
            $"delta={MathF.Abs(rayY - collisionY) / TerrainRay.UnitsPerMetre:F3}m " +
            $"positionY={position.Y / TerrainRay.UnitsPerMetre:F3}m " +
            $"normal=({_hitNormal[0]:F2}, {_hitNormal[1]:F2}, {_hitNormal[2]:F2}) " +
            $"attr={attribute}");
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
