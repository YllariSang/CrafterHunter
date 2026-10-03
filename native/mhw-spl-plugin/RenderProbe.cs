using System.Numerics;
using SharpPluginLoader.Core.Rendering;

namespace CrafterHunter.MHW;

// A bounded rendering oracle. One MHW metre is 100 game units; this cube is
// 1 m wide and anchored 3 m ahead of the first valid camera sample.
internal static class RenderProbe
{
    private static readonly (int A, int B)[] Edges =
    [
        (0, 1), (1, 3), (3, 2), (2, 0),
        (4, 5), (5, 7), (7, 6), (6, 4),
        (0, 4), (1, 5), (2, 6), (3, 7),
    ];
    private static readonly Vector4 Color = new(0.1f, 0.95f, 0.3f, 1.0f);

    internal static bool TryGetTargetRay(Vector3 origin, Vector3 target, out Vector3 forward)
    {
        forward = default;
        var outward = target - origin;
        if (!float.IsFinite(origin.X) || !float.IsFinite(origin.Y) || !float.IsFinite(origin.Z) ||
            !float.IsFinite(outward.X) || !float.IsFinite(outward.Y) ||
            !float.IsFinite(outward.Z) || outward.LengthSquared() < 1e-8f)
        {
            return false;
        }

        forward = Vector3.Normalize(outward);
        return true;
    }

    internal static Vector3 Centre(Vector3 cameraPosition, Vector3 forward) =>
        cameraPosition + forward * 300.0f;

    internal static void Draw(Vector3 centre)
    {
        Span<Vector3> corners = stackalloc Vector3[8];
        for (var corner = 0; corner < corners.Length; corner++)
        {
            corners[corner] = centre + new Vector3(
                (corner & 1) == 0 ? -50.0f : 50.0f,
                (corner & 2) == 0 ? -50.0f : 50.0f,
                (corner & 4) == 0 ? -50.0f : 50.0f
            );
        }

        foreach (var (a, b) in Edges)
        {
            Primitives.RenderLine(corners[a], corners[b], Color);
        }
    }
}

// A camera-relative centre would stay glued to the screen. Capture the first
// usable ray once, then retain its world-space location as the camera moves.
internal sealed class RenderProbePlacement
{
    // The game can change scenes after the plugin's ten-second startup grace.
    // Treat a camera jump beyond 25 m as a new placement scene; ordinary test
    // movements keep the original world anchor.
    private const float ReanchorDistanceSquared = 2500.0f * 2500.0f;
    internal Vector3? Centre { get; private set; }

    internal bool TryAnchor(Vector3 cameraPosition, Vector3 target)
    {
        if (Centre is { } centre &&
            Vector3.DistanceSquared(cameraPosition, centre) <= ReanchorDistanceSquared)
        {
            return true;
        }
        if (!RenderProbe.TryGetTargetRay(cameraPosition, target, out var forward))
        {
            return false;
        }
        Centre = RenderProbe.Centre(cameraPosition, forward);
        return true;
    }

    internal void Reset() => Centre = null;
}
