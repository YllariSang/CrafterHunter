using System.Numerics;
using CrafterHunter.MHW;

var eye = new Vector3(25, 10, 9);
Verify(eye, eye - Vector3.UnitZ * 500, -Vector3.UnitZ);
Verify(eye, eye + Vector3.UnitX * 200, Vector3.UnitX);
if (RenderProbe.TryGetTargetRay(eye, eye, out _) ||
    RenderProbe.TryGetTargetRay(eye, new Vector3(float.NaN, 0, 0), out _))
{
    throw new Exception("Invalid camera targets must not produce a probe ray.");
}
Console.WriteLine("Render probe target-ray checks passed.");

var placement = new RenderProbePlacement();
if (placement.TryAnchor(eye, eye) || placement.Centre.HasValue)
{
    throw new Exception("Invalid target must not anchor the probe.");
}
if (!placement.TryAnchor(eye, eye - Vector3.UnitZ * 500))
{
    throw new Exception("Valid target failed to anchor the probe.");
}
var fixedCentre = placement.Centre;
if (!placement.TryAnchor(eye + Vector3.UnitX * 1000, eye + Vector3.UnitX * 1500) ||
    placement.Centre != fixedCentre)
{
    throw new Exception("Probe moved with the camera instead of staying in world space.");
}
if (!placement.TryAnchor(eye + Vector3.UnitX * 5000, eye + Vector3.UnitX * 5500) ||
    placement.Centre != eye + Vector3.UnitX * 5300)
{
    throw new Exception("Probe did not reanchor after a large scene jump.");
}
placement.Reset();
if (placement.Centre.HasValue)
{
    throw new Exception("Probe anchor survived reset.");
}
Console.WriteLine("Render probe world-anchor checks passed.");

static void Verify(Vector3 eye, Vector3 target, Vector3 expectedForward)
{
    if (!RenderProbe.TryGetTargetRay(eye, target, out var forward))
    {
        throw new Exception("Could not normalize a valid camera target.");
    }
    if (Vector3.Dot(forward, expectedForward) < 0.999f ||
        Vector3.Distance(RenderProbe.Centre(eye, forward), eye + expectedForward * 300) > 0.01f)
    {
        throw new Exception($"Wrong target ray: direction={forward}");
    }
}
