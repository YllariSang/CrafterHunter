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

var id = System.Text.Encoding.UTF8.GetBytes(MinecraftBlockAsset.ExpectedBlockId);
var rgbaPayload = new byte[1 + id.Length + 2 + 16 * 16 * 4];
rgbaPayload[0] = (byte)id.Length;
id.CopyTo(rgbaPayload, 1);
rgbaPayload[1 + id.Length] = 16;
rgbaPayload[2 + id.Length] = 16;
rgbaPayload[3 + id.Length] = 0x42;
if (!MinecraftBlockAsset.TryReadPixels(rgbaPayload, out var pixels) ||
    pixels.Length != 1024 || pixels[0] != 0x42)
{
    throw new Exception("Minecraft pixel packet did not round-trip.");
}
rgbaPayload[1 + id.Length] = 32;
if (MinecraftBlockAsset.TryReadPixels(rgbaPayload, out _))
{
    throw new Exception("Wrong-sized Minecraft texture must be rejected.");
}
byte[] pngSignature = [137, 80, 78, 71, 13, 10, 26, 10, 1];
var pngPayload = new byte[1 + id.Length + pngSignature.Length];
pngPayload[0] = (byte)id.Length;
id.CopyTo(pngPayload, 1);
pngSignature.CopyTo(pngPayload, 1 + id.Length);
if (!MinecraftBlockAsset.TryReadPng(pngPayload, out var png) ||
    !png.AsSpan().SequenceEqual(pngSignature))
{
    throw new Exception("Minecraft PNG packet did not round-trip.");
}
if (MinecraftBlockRenderer.Position(Vector3.Zero, Vector3.UnitX, 0).X != -95 ||
    MinecraftBlockRenderer.Position(Vector3.Zero, Vector3.UnitX, 1).X != 0 ||
    MinecraftBlockRenderer.Position(Vector3.Zero, Vector3.UnitX, 2).X != 95 ||
    MinecraftBlockRenderer.Position(Vector3.Zero, Vector3.UnitX, 1).Y != 40)
{
    throw new Exception("A/B/C render methods must have separate stable world positions.");
}
var colorPixels = new byte[16 * 16 * 4];
colorPixels[0] = 0x12;
colorPixels[1] = 0x34;
colorPixels[2] = 0x56;
colorPixels[3] = 0xFF;
if (MinecraftBlockRenderer.PixelColor32(colorPixels, 0, 0) != 0xFF563412)
{
    throw new Exception("Minecraft RGBA pixels must pack to ImGui color without red tint.");
}
Console.WriteLine("Minecraft block packet and A/B/C placement checks passed.");

var lifecycle = new PlacementLifecycle();
long clock = 1_000;
var origin = Vector3.Zero;
var anchor = new Vector3(1000, 200, -400);
if (lifecycle.Observe(origin, clock))
{
    throw new Exception("Observing a camera without an anchor must not report an invalidation.");
}
lifecycle.Place(anchor);
if (lifecycle.Anchor != anchor)
{
    throw new Exception("Place must retain the requested anchor.");
}
clock += 16;
if (lifecycle.Observe(new Vector3(500, 0, 0), clock) || lifecycle.Anchor != anchor)
{
    throw new Exception("Ordinary camera motion must not drop a placed anchor.");
}
clock += 16;
if (!lifecycle.Observe(new Vector3(3500, 0, 0), clock) || lifecycle.Anchor is not null)
{
    throw new Exception("A single-frame camera jump must drop the anchor.");
}
if (lifecycle.InvalidatedBecause is null)
{
    throw new Exception("An invalidation must record why it happened.");
}
clock += 16;
lifecycle.Observe(new Vector3(3600, 0, 0), clock);
if (lifecycle.Anchor is not null)
{
    throw new Exception("The lifecycle must never re-anchor after a scene change.");
}

lifecycle.Reset();
clock = 5_000;
lifecycle.Observe(origin, clock);
lifecycle.Place(anchor);
if (lifecycle.Observe(null, clock + PlacementLifecycle.CameraTimeoutMilliseconds))
{
    throw new Exception("A camera gap inside the grace period must keep the anchor.");
}
if (!lifecycle.Observe(null, clock + PlacementLifecycle.CameraTimeoutMilliseconds + 1) ||
    lifecycle.Anchor is not null)
{
    throw new Exception("A camera gap beyond the grace period must drop the anchor.");
}

lifecycle.Reset();
lifecycle.Observe(origin, clock);
lifecycle.Place(anchor);
if (lifecycle.Observe(new Vector3(float.NaN, 0, 0), clock + 10))
{
    throw new Exception("A single non-finite sample inside the grace period must keep the anchor.");
}
if (!lifecycle.Observe(new Vector3(float.NaN, 0, 0),
        clock + 10 + PlacementLifecycle.CameraTimeoutMilliseconds + 1) ||
    lifecycle.Anchor is not null)
{
    throw new Exception("A non-finite camera must drop the anchor once the grace period expires.");
}

lifecycle.Reset();
lifecycle.Observe(origin, clock);
lifecycle.Place(anchor);
if (!lifecycle.Clear() || lifecycle.Anchor is not null)
{
    throw new Exception("An explicit clear must drop the anchor.");
}
if (lifecycle.Clear())
{
    throw new Exception("Clearing twice must report that there was nothing to clear.");
}
lifecycle.Reset();
if (lifecycle.Anchor is not null || lifecycle.InvalidatedBecause is not null)
{
    throw new Exception("Reset must return the lifecycle to its unloaded state.");
}
Console.WriteLine("Placement lifecycle checks passed.");

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
