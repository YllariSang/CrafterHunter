using System.Numerics;
using CrafterHunter.MHW;

var eye = new Vector3(25, 10, 9);
Verify(eye, -Vector3.UnitZ);
Verify(eye, Vector3.UnitX);
Verify(eye, Vector3.UnitX, reversedDepth: true);
if (RenderProbe.TryGetViewRay(default, Matrix4x4.Identity, out _, out _))
{
    throw new Exception("A singular view matrix must not produce a probe ray.");
}
Console.WriteLine("Render probe view-ray checks passed.");

static void Verify(Vector3 eye, Vector3 expectedForward, bool reversedDepth = false)
{
    var view = Matrix4x4.CreateLookAt(eye, eye + expectedForward, Vector3.UnitY);
    var projection = Matrix4x4.CreatePerspectiveFieldOfView(MathF.PI / 3, 16f / 9f, 10, 10000);
    if (reversedDepth)
    {
        projection.M33 = 10f / (10000f - 10f);
        projection.M43 = 10f * 10000f / (10000f - 10f);
    }
    if (!RenderProbe.TryGetViewRay(view, projection, out var origin, out var forward))
    {
        throw new Exception("Could not unproject a valid perspective camera.");
    }
    if (Vector3.Distance(origin, eye) > 0.01f ||
        Vector3.Dot(forward, expectedForward) < 0.999f)
    {
        throw new Exception($"Wrong view ray: origin={origin}, direction={forward}");
    }
}
