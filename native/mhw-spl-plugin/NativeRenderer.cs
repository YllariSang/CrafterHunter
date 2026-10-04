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
    private static FrameDelegate? _frame;
    private static StopDelegate? _stop;
    private static bool _checked;
    internal static bool Enabled { get; private set; }

    internal static void Initialize()
    {
        if (_checked) return;
        _checked = true;
        // SPL loads managed assemblies from bytes; Assembly.Location is empty.
        var folder = Path.GetFullPath("nativePC/plugins/CSharp/CrafterHunter");
        if (!File.Exists(Path.Combine(folder, "native-renderer.enabled"))) return;
        using var exe = File.OpenRead("MonsterHunterWorld.exe");
        if (Convert.ToHexString(SHA256.HashData(exe)) !=
            "C2EBBBD2C49F216D484E31A5219BED419EB1E5E7D206D02CBA040A3AB79D90EA")
            throw new InvalidOperationException("Native renderer disabled: unsupported MHW executable hash.");
        var library = NativeLibrary.Load(Path.Combine(folder, "CrafterHunter.Render.dll"));
        _frame = Marshal.GetDelegateForFunctionPointer<FrameDelegate>(NativeLibrary.GetExport(library, "CH_Frame"));
        _stop = Marshal.GetDelegateForFunctionPointer<StopDelegate>(NativeLibrary.GetExport(library, "CH_Stop"));
        Enabled = true;
    }

    internal static void Frame()
    {
        if (!Enabled || Renderer.IsDirectX12) return;
        var render = SingletonManager.GetSingleton("sMhRender");
        if (render is null || SingletonManager.GetSingleton("sMhCamera") is null) return;
        var vp = CameraSystem.MainViewport;
        var matrix = vp.ViewMatrix * vp.ProjectionMatrix;
        _frame!(render.Instance, in matrix);
    }

    internal static void Stop() { _stop?.Invoke(); Enabled = false; }
}
