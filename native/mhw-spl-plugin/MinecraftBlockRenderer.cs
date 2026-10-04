using System.Numerics;
using ImGuiNET;
using SharpPluginLoader.Core;
using SharpPluginLoader.Core.Rendering;

namespace CrafterHunter.MHW;

// Three render paths consume the same pixels supplied by Minecraft. A and B
// use SPL's 3D primitive pass; C uses its image overlay. None is presumed to
// have correct host-depth interaction until the in-game comparison confirms it.
internal sealed class MinecraftBlockRenderer
{
    private const float HalfSize = 35.0f;
    private const float Separation = 95.0f;
    private const float VerticalLift = 40.0f;
    private readonly List<(MeshHandle Handle, Vector4 Color)> _meshes = [];
    private MinecraftBlockAsset? _meshAsset;
    internal static Vector3 Position(Vector3 anchor, Vector3 right, int method) =>
        anchor + Vector3.UnitY * VerticalLift + right * ((method - 1) * Separation);

    internal void DrawMesh(MinecraftBlockAsset asset, Vector3 anchor, Vector3 right)
    {
        if (!ReferenceEquals(_meshAsset, asset))
        {
            BuildMeshes(asset);
        }
        var meshPosition = Position(anchor, right, 0);
        var transform = Matrix4x4.CreateTranslation(meshPosition);
        foreach (var (handle, color) in _meshes)
        {
            Primitives.RenderMesh(handle, transform, color);
        }

    }

    internal void DrawLines(MinecraftBlockAsset asset, Vector3 anchor, Vector3 right)
    {
        var linePosition = Position(anchor, right, 1);
        var activeCamera = CameraSystem.MainViewport.Camera;
        if (activeCamera is null) return;
        var camera = activeCamera.Position;
        for (var face = 0; face < 6; face++)
        {
            if (!FaceVisible(face, camera - linePosition)) continue;
            for (var y = 0; y < MinecraftBlockAsset.Size; y++)
            {
                for (var x = 0; x < MinecraftBlockAsset.Size; x++)
                {
                    var color = PixelColor(asset.Rgba, x, y);
                    if (color.W < 0.01f) continue;
                    var start = linePosition + FacePoint(face, x / 16.0f, (y + 0.5f) / 16.0f);
                    var end = linePosition + FacePoint(face, (x + 1) / 16.0f, (y + 0.5f) / 16.0f);
                    Primitives.RenderLine(start, end, color);
                }
            }
        }
        // Full-length boundaries keep the 3D line method legible even when
        // individual Minecraft texels are subpixel at the current distance.
        var edgeColor = PixelColor(asset.Rgba, 8, 8);
        for (var axis = 0; axis < 3; axis++)
        {
            for (var sideA = -1; sideA <= 1; sideA += 2)
            {
                for (var sideB = -1; sideB <= 1; sideB += 2)
                {
                    Vector3 a, b;
                    if (axis == 0)
                    {
                        a = new(-HalfSize, sideA * HalfSize, sideB * HalfSize);
                        b = new(HalfSize, sideA * HalfSize, sideB * HalfSize);
                    }
                    else if (axis == 1)
                    {
                        a = new(sideA * HalfSize, -HalfSize, sideB * HalfSize);
                        b = new(sideA * HalfSize, HalfSize, sideB * HalfSize);
                    }
                    else
                    {
                        a = new(sideA * HalfSize, sideB * HalfSize, -HalfSize);
                        b = new(sideA * HalfSize, sideB * HalfSize, HalfSize);
                    }
                    Primitives.RenderLine(linePosition + a, linePosition + b, edgeColor);
                }
            }
        }
    }

    internal void DrawOverlay(MinecraftBlockAsset asset, Vector3 anchor, Vector3 right)
    {
        // The block is still an overlay, but place it below ImGui windows and
        // the software cursor. ForegroundDrawList obscured the mouse pointer.
        var blockDraw = ImGui.GetBackgroundDrawList();
        var labelDraw = ImGui.GetForegroundDrawList();
        var activeCamera = CameraSystem.MainViewport.Camera;
        if (activeCamera is null) return;
        var camera = activeCamera.Position;
        var centres = new[]
        {
            Position(anchor, right, 0), Position(anchor, right, 1), Position(anchor, right, 2)
        };
        var labels = new[] { "A: 3D mesh", "B: 3D lines", "C: pixel quads (no depth)" };
        for (var method = 0; method < centres.Length; method++)
        {
            if (CameraSystem.MainViewport.WorldToScreen(
                    centres[method] + new Vector3(0, 50, 0), out var labelPosition))
            {
                labelDraw.AddText(labelPosition, 0xFFFFFFFF, labels[method]);
            }
        }

        var position = centres[2];
        // Painter order only resolves the block's own faces. This overlay has
        // no MHW scene depth, so its draw-through is an intentional control.
        var faces = Enumerable.Range(0, 6)
            .Where(face => FaceVisible(face, camera - position))
            .OrderByDescending(face => Vector3.DistanceSquared(
                camera, position + FacePoint(face, 0.5f, 0.5f)));
        foreach (var face in faces)
        {
            if (!TryProjectFace(position, face, out var points)) continue;
            for (var y = 0; y < MinecraftBlockAsset.Size; y++)
            {
                for (var x = 0; x < MinecraftBlockAsset.Size; x++)
                {
                    var color = PixelColor32(asset.Rgba, x, y);
                    if ((color & 0xFF000000) == 0) continue;
                    var u0 = x / 16.0f;
                    var u1 = (x + 1) / 16.0f;
                    var v0 = y / 16.0f;
                    var v1 = (y + 1) / 16.0f;
                    blockDraw.AddQuadFilled(
                        Interpolate(points, u0, v0), Interpolate(points, u1, v0),
                        Interpolate(points, u1, v1), Interpolate(points, u0, v1), color);
                }
            }
        }
    }

    private void BuildMeshes(MinecraftBlockAsset asset)
    {
        _meshes.Clear();
        var byColor = new Dictionary<uint, MeshBuilder>();
        for (var face = 0; face < 6; face++)
        {
            for (var y = 0; y < MinecraftBlockAsset.Size; y++)
            {
                for (var x = 0; x < MinecraftBlockAsset.Size; x++)
                {
                    var offset = (y * MinecraftBlockAsset.Size + x) * 4;
                    var key = ((uint)asset.Rgba[offset] << 24) |
                              ((uint)asset.Rgba[offset + 1] << 16) |
                              ((uint)asset.Rgba[offset + 2] << 8) |
                              asset.Rgba[offset + 3];
                    if ((key & 0xFF) == 0) continue;
                    if (!byColor.TryGetValue(key, out var builder))
                    {
                        builder = new MeshBuilder();
                        byColor.Add(key, builder);
                    }
                    var u0 = x / 16.0f;
                    var u1 = (x + 1) / 16.0f;
                    var v0 = y / 16.0f;
                    var v1 = (y + 1) / 16.0f;
                    builder.AddQuad(FacePoint(face, u0, v0), FacePoint(face, u1, v0),
                        FacePoint(face, u1, v1), FacePoint(face, u0, v1));
                }
            }
        }
        foreach (var (key, builder) in byColor)
        {
            var handle = Primitives.SupplyCustomMesh(
                builder.Vertices.ToArray().AsSpan(), builder.Indices.ToArray().AsSpan());
            if (handle != MeshHandle.Invalid)
            {
                _meshes.Add((handle, new Vector4(
                    ((key >> 24) & 0xFF) / 255.0f,
                    ((key >> 16) & 0xFF) / 255.0f,
                    ((key >> 8) & 0xFF) / 255.0f,
                    (key & 0xFF) / 255.0f)));
            }
        }
        _meshAsset = asset;
        Log.Info($"[CrafterHunter block] A registered {_meshes.Count} color meshes from {asset.BlockId}");
    }

    private static Vector2 Interpolate(Vector2[] corners, float u, float v) =>
        Vector2.Lerp(Vector2.Lerp(corners[0], corners[1], u),
            Vector2.Lerp(corners[3], corners[2], u), v);

    internal static uint PixelColor32(byte[] rgba, int x, int y)
    {
        var offset = (y * MinecraftBlockAsset.Size + x) * 4;
        return (uint)rgba[offset] | ((uint)rgba[offset + 1] << 8) |
            ((uint)rgba[offset + 2] << 16) | ((uint)rgba[offset + 3] << 24);
    }

    private static bool TryProjectFace(Vector3 position, int face, out Vector2[] points)
    {
        points = new Vector2[4];
        var corners = new[]
        {
            FacePoint(face, 0, 0), FacePoint(face, 1, 0),
            FacePoint(face, 1, 1), FacePoint(face, 0, 1)
        };
        for (var index = 0; index < 4; index++)
        {
            if (!CameraSystem.MainViewport.WorldToScreen(position + corners[index],
                    out points[index])) return false;
        }
        return true;
    }

    private static bool FaceVisible(int face, Vector3 toCamera) =>
        Vector3.Dot(face switch
        {
            0 => Vector3.UnitZ,
            1 => -Vector3.UnitZ,
            2 => Vector3.UnitX,
            3 => -Vector3.UnitX,
            4 => Vector3.UnitY,
            _ => -Vector3.UnitY
        }, toCamera) > 0;

    // Unit cube point in MHW's native centimetre-like units. All six faces use
    // Minecraft's own stone pixels, rather than a painted proxy texture.
    private static Vector3 FacePoint(int face, float u, float v) => face switch
    {
        0 => new(-HalfSize + u * 2 * HalfSize, HalfSize - v * 2 * HalfSize, HalfSize),
        1 => new(HalfSize - u * 2 * HalfSize, HalfSize - v * 2 * HalfSize, -HalfSize),
        2 => new(HalfSize, HalfSize - v * 2 * HalfSize, HalfSize - u * 2 * HalfSize),
        3 => new(-HalfSize, HalfSize - v * 2 * HalfSize, -HalfSize + u * 2 * HalfSize),
        4 => new(-HalfSize + u * 2 * HalfSize, HalfSize, -HalfSize + v * 2 * HalfSize),
        _ => new(-HalfSize + u * 2 * HalfSize, -HalfSize, HalfSize - v * 2 * HalfSize)
    };

    private static Vector4 PixelColor(byte[] rgba, int x, int y)
    {
        var offset = (y * MinecraftBlockAsset.Size + x) * 4;
        return new Vector4(rgba[offset] / 255.0f, rgba[offset + 1] / 255.0f,
            rgba[offset + 2] / 255.0f, rgba[offset + 3] / 255.0f);
    }

    private sealed class MeshBuilder
    {
        internal List<Vector4> Vertices { get; } = [];
        internal List<uint> Indices { get; } = [];

        internal void AddQuad(Vector3 a, Vector3 b, Vector3 c, Vector3 d)
        {
            var start = (uint)Vertices.Count;
            Vertices.Add(new Vector4(a, 1));
            Vertices.Add(new Vector4(b, 1));
            Vertices.Add(new Vector4(c, 1));
            Vertices.Add(new Vector4(d, 1));
            // Both windings make the comparison independent of the loader's
            // face-culling state; duplicate indices are registered only once.
            Indices.AddRange([
                start, start + 1, start + 2, start, start + 2, start + 3,
                start + 2, start + 1, start, start + 3, start + 2, start
            ]);
        }
    }
}
