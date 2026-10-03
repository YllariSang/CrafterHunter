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
    private const float HalfSize = 50.0f;
    private const float Separation = 180.0f;
    private static readonly Vector2 Uv0 = new(0, 0);
    private static readonly Vector2 Uv1 = new(1, 0);
    private static readonly Vector2 Uv2 = new(1, 1);
    private static readonly Vector2 Uv3 = new(0, 1);
    private readonly List<(MeshHandle Handle, Vector4 Color)> _meshes = [];
    private MinecraftBlockAsset? _meshAsset;
    private MinecraftBlockAsset? _textureAsset;
    private TextureHandle _texture = TextureHandle.Invalid;

    internal static Vector3 Position(Vector3 anchor, Vector3 right, int method) =>
        anchor + right * ((method - 1) * Separation);

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
    }

    internal void DrawOverlay(MinecraftBlockAsset asset, Vector3 anchor, Vector3 right)
    {
        EnsureTexture(asset);
        var draw = ImGui.GetForegroundDrawList();
        var activeCamera = CameraSystem.MainViewport.Camera;
        if (activeCamera is null) return;
        var camera = activeCamera.Position;
        var centres = new[]
        {
            Position(anchor, right, 0), Position(anchor, right, 1), Position(anchor, right, 2)
        };
        var labels = new[] { "A: 3D mesh", "B: 3D lines", "C: image quads" };
        for (var method = 0; method < centres.Length; method++)
        {
            if (CameraSystem.MainViewport.WorldToScreen(
                    centres[method] + new Vector3(0, 75, 0), out var labelPosition))
            {
                draw.AddText(labelPosition, 0xFFFFFFFF, labels[method]);
            }
        }

        if (_texture == TextureHandle.Invalid) return;
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
            draw.AddImageQuad(_texture, points[0], points[1], points[2], points[3],
                Uv0, Uv1, Uv2, Uv3, 0xFFFFFFFF);
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

    private void EnsureTexture(MinecraftBlockAsset asset)
    {
        if (ReferenceEquals(_textureAsset, asset)) return;
        _textureAsset = asset;
        _texture = TextureHandle.Invalid;
        var path = Path.Combine(Path.GetTempPath(),
            $"crafterhunter-{Guid.NewGuid():N}.png");
        try
        {
            File.WriteAllBytes(path, asset.Png);
            _texture = Renderer.LoadTexture(path, out var width, out var height);
            Log.Info($"[CrafterHunter block] C loaded {asset.BlockId} PNG: {width}x{height}");
        }
        finally
        {
            // Never leave a copied Minecraft texture on disk after GPU upload.
            if (File.Exists(path)) File.Delete(path);
        }
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
        0 => new(-HalfSize + u * 100, HalfSize - v * 100, HalfSize),
        1 => new(HalfSize - u * 100, HalfSize - v * 100, -HalfSize),
        2 => new(HalfSize, HalfSize - v * 100, HalfSize - u * 100),
        3 => new(-HalfSize, HalfSize - v * 100, -HalfSize + u * 100),
        4 => new(-HalfSize + u * 100, HalfSize, -HalfSize + v * 100),
        _ => new(-HalfSize + u * 100, -HalfSize, HalfSize - v * 100)
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
