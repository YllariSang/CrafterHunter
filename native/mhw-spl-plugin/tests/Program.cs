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

// Terrain: the signature table is what the adapter resolves in the game, so a
// typo or a mangled wildcard here would silently disable terrain queries on
// the next run. Parse every pattern the way the loader's scanner does.
if (TerrainRay.Signatures.Length != 7)
{
    throw new Exception($"The terrain adapter expects 7 signatures, found {TerrainRay.Signatures.Length}.");
}
foreach (var (name, pattern) in TerrainRay.Signatures)
{
    if (string.IsNullOrWhiteSpace(name) || string.IsNullOrWhiteSpace(pattern))
    {
        throw new Exception("Every terrain signature needs both a name and a pattern.");
    }

    try
    {
        SharpPluginLoader.Core.Memory.Pattern.FromString(pattern);
    }
    catch (Exception exception)
    {
        throw new Exception($"Terrain signature {name} is not a parseable pattern: {exception.Message}");
    }
}
if (TerrainRay.PinnedExecutableSha256.Length != 64 ||
    !TerrainRay.PinnedExecutableSha256.All(character => Uri.IsHexDigit(character)))
{
    throw new Exception("The pinned executable fingerprint must be 64 hex digits.");
}
if (TerrainRay.PinnedExecutableSha256 != TerrainRay.PinnedExecutableSha256.ToLowerInvariant())
{
    throw new Exception("The pinned executable fingerprint must be stored in lowercase.");
}
Console.WriteLine("Terrain signature checks passed: seven names, parseable patterns, pinned hash.");

// The self-check segment starts above the hunter and ends below the ground
// line, in the same units the game reports positions in.
TerrainRay.DownSegment(
    new Vector3(10, 2000, -5), 1f, 5f, TerrainRay.UnitsPerMetre, out var rayStart, out var rayEnd);
if (rayStart.X != 10 || rayStart.Z != -5)
{
    throw new Exception("A down-ray must not move the hunter horizontally.");
}
if (rayStart.Y != 2100 || rayEnd.Y != 1500)
{
    throw new Exception($"Unexpected down-ray heights: start={rayStart.Y} end={rayEnd.Y}.");
}
if (!(rayStart.Y > 2000 && rayEnd.Y < 2000))
{
    throw new Exception("A down-ray must straddle the hunter's own height.");
}
Console.WriteLine("Terrain down-segment checks passed.");

// The agreement rule: inside tolerance agrees, outside does not, and a
// non-finite measurement never counts as a verdict.
if (!TerrainRay.Agrees(2000f, 2000f, 0.5f, 100f))
{
    throw new Exception("A ray that lands exactly on the collision point must agree.");
}
if (!TerrainRay.Agrees(2050f, 2000f, 0.5f, 100f))
{
    throw new Exception("A ray at exactly the tolerance must agree.");
}
if (TerrainRay.Agrees(2051f, 2000f, 0.5f, 100f))
{
    throw new Exception("A ray beyond the tolerance must not agree.");
}
if (TerrainRay.Agrees(float.NaN, 2000f, 0.5f, 100f) ||
    TerrainRay.Agrees(2000f, float.PositiveInfinity, 0.5f, 100f))
{
    throw new Exception("A non-finite measurement must never agree.");
}
Console.WriteLine("Terrain agreement checks passed.");

// The state machine: a fingerprint or signature failure dominates everything.
VerifyState(TerrainRay.Classify(false, true, true, true), TerrainRay.State.Disabled);
VerifyState(TerrainRay.Classify(true, false, true, true), TerrainRay.State.Disabled);
VerifyState(TerrainRay.Classify(false, false, false, false), TerrainRay.State.Disabled);
VerifyState(TerrainRay.Classify(true, true, false, true), TerrainRay.State.Unavailable);
VerifyState(TerrainRay.Classify(true, true, true, false), TerrainRay.State.Unavailable);
VerifyState(TerrainRay.Classify(true, true, false, false), TerrainRay.State.Unavailable);
VerifyState(TerrainRay.Classify(true, true, true, true), TerrainRay.State.Ready);
Console.WriteLine("Terrain state-machine checks passed: disabled, unavailable, ready.");

// The request channel is the only thing that makes the adapter cast: a
// pending request fires exactly once, and a cleared request never fires.
var terrainFolder = Path.Combine(Path.GetTempPath(), $"crafterhunter-terrain-{Guid.NewGuid():N}");
try
{
    if (TerrainRequest.ConsumeCheck(terrainFolder))
    {
        throw new Exception("An absent request file must not produce a self-check.");
    }

    Directory.CreateDirectory(Path.Combine(terrainFolder, TerrainRequest.FolderName));
    File.WriteAllText(TerrainRequest.CheckPath(terrainFolder), "check\n");
    if (!TerrainRequest.ConsumeCheck(terrainFolder) ||
        File.Exists(TerrainRequest.CheckPath(terrainFolder)))
    {
        throw new Exception("A pending self-check request must be taken exactly once.");
    }

    File.WriteAllText(TerrainRequest.CheckPath(terrainFolder), "check\n");
    TerrainRequest.Clear(terrainFolder);
    if (File.Exists(TerrainRequest.CheckPath(terrainFolder)))
    {
        throw new Exception("Clear must drop a pending request.");
    }

    if (TerrainRequest.ConsumeCheck(terrainFolder))
    {
        throw new Exception("A cleared request must never fire.");
    }
}
finally
{
    if (Directory.Exists(terrainFolder))
    {
        Directory.Delete(terrainFolder, recursive: true);
    }
}
Console.WriteLine("Terrain request-channel checks passed.");

static void VerifyState(TerrainRay.State actual, TerrainRay.State expected)
{
    if (actual != expected)
    {
        throw new Exception($"Terrain state {actual} should have been {expected}.");
    }
}

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
