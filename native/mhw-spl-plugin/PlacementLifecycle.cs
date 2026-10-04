using System.Numerics;

namespace CrafterHunter.MHW;

/// <summary>
/// Placement state for the native renderer. An anchor exists only after an
/// explicit place request and is dropped when the camera reports a scene
/// transition. The anchor is never rebuilt automatically: after an area
/// change the stone stays absent until the operator places it again, so a
/// transition can never leave geometry anchored in the previous world.
/// </summary>
internal sealed class PlacementLifecycle
{
    // One MHW metre is 100 game units. Ordinary camera motion never covers
    // this distance between two rendered frames; a scene, cutscene or
    // fast-travel camera cut always does.
    internal const float JumpUnits = 2500.0f;

    // A camera sample that stops being valid for this long is a loading or
    // title sequence rather than a transient glitch, so the anchor's world can
    // no longer be confirmed.
    internal const long CameraTimeoutMilliseconds = 1000;

    private Vector3? _lastCamera;
    private long _lastValidSampleMilliseconds;

    internal Vector3? Anchor { get; private set; }

    /// <summary>Why the anchor was last dropped; null while one is anchored.</summary>
    internal string? InvalidatedBecause { get; private set; }

    /// <summary>
    /// Feeds one camera sample to the lifecycle. Returns true only when an
    /// existing anchor was dropped by this call.
    /// </summary>
    internal bool Observe(Vector3? cameraPosition, long nowMilliseconds)
    {
        if (cameraPosition is { } position && IsFinite(position))
        {
            if (Anchor is not null &&
                _lastCamera is { } previous &&
                Vector3.DistanceSquared(previous, position) > JumpUnits * JumpUnits)
            {
                return Drop("camera jumped over 25 m in a single frame");
            }

            _lastCamera = position;
            _lastValidSampleMilliseconds = nowMilliseconds;
            return false;
        }

        if (Anchor is not null &&
            _lastValidSampleMilliseconds != 0 &&
            nowMilliseconds - _lastValidSampleMilliseconds > CameraTimeoutMilliseconds)
        {
            return Drop("no valid camera sample for over a second");
        }

        return false;
    }

    /// <summary>Records an explicitly requested anchor.</summary>
    internal void Place(Vector3 centre)
    {
        Anchor = centre;
        InvalidatedBecause = null;
    }

    /// <summary>Handles an explicit clear request. True when an anchor existed.</summary>
    internal bool Clear()
    {
        if (Anchor is null)
        {
            return false;
        }

        Drop("explicit clear request");
        return true;
    }

    /// <summary>Returns the lifecycle to its unloaded state.</summary>
    internal void Reset()
    {
        Anchor = null;
        _lastCamera = null;
        _lastValidSampleMilliseconds = 0;
        InvalidatedBecause = null;
    }

    private bool Drop(string reason)
    {
        Anchor = null;
        _lastCamera = null;
        InvalidatedBecause = reason;
        return true;
    }

    private static bool IsFinite(Vector3 value) =>
        float.IsFinite(value.X) && float.IsFinite(value.Y) && float.IsFinite(value.Z);
}
