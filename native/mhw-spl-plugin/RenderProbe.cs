using System.Numerics;
using SharpPluginLoader.Core.Rendering;

namespace CrafterHunter.MHW;

// A bounded rendering oracle. One MHW metre is 100 game units; this cube is
// 1 m wide and centred 3 m ahead of the most recent valid camera sample.
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

    internal static void Draw(Vector3 cameraPosition, Vector3 forward)
    {
        var centre = Centre(cameraPosition, forward);
        // The centre sphere separates camera-position failure from line-renderer
        // failure when only a single cube edge survives the host's render pass.
        Primitives.RenderSphere(centre, 20.0f, new Vector4(0.95f, 0.15f, 0.9f, 1.0f));
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
