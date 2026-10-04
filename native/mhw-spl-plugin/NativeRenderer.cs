using System.Numerics;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using SharpPluginLoader.Core;
using SharpPluginLoader.Core.Rendering;

namespace CrafterHunter.MHW;

internal static class NativeRenderer
{
    private delegate int FrameDelegate(nint singleton, in Matrix4x4 viewProjection);
    private delegate void StopDelegate();
    private delegate void BlockDelegate(in Matrix4x4 inverse, in Vector4 centre, byte[] pixels, int enabled);
    private static FrameDelegate? _frame;
    private static StopDelegate? _stop;
    private static BlockDelegate? _block;
    private static bool _checked;
    private static string _folder = "";
    private static long _nextReloadCheck;
    private static Vector3? _placement;
    internal static bool Enabled { get; private set; }
    internal static bool Requested { get; private set; }

    internal static void Initialize()
    {
        if (_checked) return;
        _checked = true;
        // SPL loads managed assemblies from bytes; Assembly.Location is empty.
        var folder = Path.GetFullPath("nativePC/plugins/CSharp/CrafterHunter");
        Requested = File.Exists(Path.Combine(folder, "native-renderer.enabled"));
        if (!Requested) return;
        using var exe = File.OpenRead("MonsterHunterWorld.exe");
        if (Convert.ToHexString(SHA256.HashData(exe)) !=
            "C2EBBBD2C49F216D484E31A5219BED419EB1E5E7D206D02CBA040A3AB79D90EA")
            throw new InvalidOperationException("Native renderer disabled: unsupported MHW executable hash.");
        _folder = folder;
        Load();
    }

    private static void Load()
    {
        Directory.CreateDirectory(Path.Combine(_folder, "render"));
        // Keep retired DLLs mapped until process exit: hook callbacks can never
        // jump into freed code. Each build gets its own loader identity.
        var copy = Path.Combine(_folder, "render", $"renderer-{Guid.NewGuid():N}.bin");
        File.Copy(Path.Combine(_folder, "CrafterHunter.Render.dll"), copy);
        var library = NativeLibrary.Load(copy);
        _frame = Marshal.GetDelegateForFunctionPointer<FrameDelegate>(NativeLibrary.GetExport(library, "CH_Frame"));
        _stop = Marshal.GetDelegateForFunctionPointer<StopDelegate>(NativeLibrary.GetExport(library, "CH_Stop"));
        _block = Marshal.GetDelegateForFunctionPointer<BlockDelegate>(NativeLibrary.GetExport(library, "CH_Block"));
        Enabled = true;
    }

    internal static void Frame(MinecraftBlockAsset? asset, Vector3? centre)
    {
        if (!Enabled || Renderer.IsDirectX12) return;
        if (Environment.TickCount64 > _nextReloadCheck)
        {
            _nextReloadCheck = Environment.TickCount64 + 1000;
            var request = Path.Combine(_folder, "native-renderer.reload");
            if (File.Exists(request))
            {
                File.Delete(request);
                _stop?.Invoke();
                Load();
            }
        }
        var render = SingletonManager.GetSingleton("sMhRender");
        if (render is null || SingletonManager.GetSingleton("sMhCamera") is null) return;
        var vp = CameraSystem.MainViewport;
        // Explicitly arm in a loaded world; never anchor to title/loading cameras.
        var placementRequest = Path.Combine(_folder, "render", "place.request");
        if (File.Exists(placementRequest) && vp.Camera is { } camera &&
            RenderProbe.TryGetTargetRay(camera.Position, camera.GetTargetWorld(), out var forward))
        {
            _placement = RenderProbe.Centre(camera.Position, forward);
            File.Delete(placementRequest);
        }
        centre = _placement;
        var matrix = vp.ViewMatrix * vp.ProjectionMatrix;
        if (asset is not null && centre is { } position && Matrix4x4.Invert(matrix, out var inverse))
        {
            var bounds = new Vector4(position, 35);
            _block!(in inverse, in bounds, asset.Rgba, 1);
        }
        else
        {
            var identity = Matrix4x4.Identity;
            var bounds = Vector4.Zero;
            _block!(in identity, in bounds, Array.Empty<byte>(), 0);
        }
        _frame!(render.Instance, in matrix);
    }

    internal static void Stop() { _stop?.Invoke(); Enabled = false; }
}
