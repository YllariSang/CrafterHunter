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

    internal static void Draw(Vector3 cameraPosition, Quaternion cameraRotation)
    {
        var forward = Vector3.Transform(-Vector3.UnitZ, cameraRotation);
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
