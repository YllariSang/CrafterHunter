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

    internal static bool TryGetViewRay(
        Matrix4x4 view,
        Matrix4x4 projection,
        out Vector3 origin,
        out Vector3 forward
    )
    {
        origin = default;
        forward = default;
        if (!Matrix4x4.Invert(view, out var world) ||
            !Matrix4x4.Invert(view * projection, out var inverseViewProjection))
        {
            return false;
        }

        var first = Vector4.Transform(new Vector4(0, 0, 0.1f, 1), inverseViewProjection);
        var second = Vector4.Transform(new Vector4(0, 0, 0.9f, 1), inverseViewProjection);
        if (!float.IsFinite(first.W) || !float.IsFinite(second.W) ||
            MathF.Abs(first.W) < 1e-6f || MathF.Abs(second.W) < 1e-6f)
        {
            return false;
        }

        origin = world.Translation;
        var a = new Vector3(first.X, first.Y, first.Z) / first.W;
        var b = new Vector3(second.X, second.Y, second.Z) / second.W;
        var aDistance = Vector3.DistanceSquared(a, origin);
        var bDistance = Vector3.DistanceSquared(b, origin);
        var outward = aDistance < bDistance ? b - a : a - b;
        if (!float.IsFinite(origin.X) || !float.IsFinite(origin.Y) ||
            !float.IsFinite(origin.Z) || !float.IsFinite(outward.X) ||
            !float.IsFinite(outward.Y) || !float.IsFinite(outward.Z) ||
            outward.LengthSquared() < 1e-8f)
        {
            return false;
        }

        forward = Vector3.Normalize(outward);
        return true;
    }

    internal static void Draw(Vector3 cameraPosition, Vector3 forward)
    {
        var centre = cameraPosition + forward * 300.0f;
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
