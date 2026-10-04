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
