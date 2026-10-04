// D3D11 resource observer. No retail code patches or guessed global addresses.
// sMhRender field layout follows SPL D3DModule::common_initialize and
// initialize_for_d3d11. The managed caller gates access to the pinned game hash.
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <MinHook.h>
#include <array>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <mutex>
#include <vector>

using Microsoft::WRL::ComPtr;
namespace {
constexpr char Dir[] = "nativePC/plugins/CSharp/CrafterHunter/render";
std::mutex gate;
ComPtr<ID3D11Device> device;
ComPtr<ID3D11DeviceContext> context;
IDXGISwapChain* swapchain = nullptr; // Borrowed only; do not obstruct ResizeBuffers.
UINT width = 0, height = 0;
unsigned long long frame = 0;
bool initialized = false;
struct Depth {
    ComPtr<ID3D11Texture2D> texture;
    float clear = 1;
    unsigned long long lastFrame = 0;
};
std::vector<Depth> depths;
using ClearFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11DepthStencilView*, UINT, FLOAT, UINT8);
ClearFn originalClear = nullptr;
void* clearTarget = nullptr;

void log(const char* format, ...) {
    CreateDirectoryA(Dir, nullptr);
    FILE* f = std::fopen("nativePC/plugins/CSharp/CrafterHunter/render/renderer.log", "a");
    if (!f) return;
    std::fprintf(f, "[%llu] ", GetTickCount64());
    va_list args; va_start(args, format); std::vfprintf(f, format, args); va_end(args);
    std::fputc('\n', f); std::fclose(f);
}

bool readPointer(const void* base, size_t offset, void** output) {
    SIZE_T copied = 0;
    return base && ReadProcessMemory(GetCurrentProcess(),
        static_cast<const char*>(base) + offset, output, sizeof(void*), &copied) &&
        copied == sizeof(void*) && *output;
}

void STDMETHODCALLTYPE observeClear(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* view,
    UINT flags, FLOAT clear, UINT8 stencil) {
    if (ctx == context.Get() && view && (flags & D3D11_CLEAR_DEPTH)) {
        ComPtr<ID3D11Resource> resource;
        view->GetResource(&resource);
        ComPtr<ID3D11Texture2D> tex;
        if (resource && SUCCEEDED(resource.As(&tex))) {
            D3D11_TEXTURE2D_DESC desc{}; tex->GetDesc(&desc);
            if (desc.Width == width && desc.Height == height && desc.SampleDesc.Count == 1 && desc.ArraySize == 1) {
                std::lock_guard lock(gate);
                bool known = false;
                for (auto& depth : depths) if (depth.texture.Get() == tex.Get()) {
                    depth.clear = clear; depth.lastFrame = frame; known = true; break;
                }
                if (!known && depths.size() < 8) {
                    log("depth[%zu] %ux%u format=%u bind=%u clear=%.3f", depths.size(),
                        desc.Width, desc.Height, desc.Format, desc.BindFlags, clear);
                    depths.push_back({tex, clear, frame});
                }
            }
        }
    }
    originalClear(ctx, view, flags, clear, stencil);
}

bool initialize(void* singleton) {
    void* renderer = nullptr; void* rawChain = nullptr; void* vtable = nullptr;
    // These same fields are used by the installed loader, after its DX11 check.
    if (!readPointer(singleton, 0x78, &renderer) ||
        !readPointer(renderer, 0x1488, &rawChain) || !readPointer(rawChain, 0, &vtable)) return false;
    auto* chain = static_cast<IDXGISwapChain*>(rawChain);
    if (FAILED(chain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(device.GetAddressOf())))) return false;
    DXGI_SWAP_CHAIN_DESC sd{};
    if (FAILED(chain->GetDesc(&sd)) || !sd.OutputWindow) { device.Reset(); return false; }
    device->GetImmediateContext(&context);
    swapchain = chain;
    auto status = MH_Initialize();
    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) return false;
    clearTarget = (*reinterpret_cast<void***>(context.Get()))[53];
    status = MH_CreateHook(clearTarget, reinterpret_cast<void*>(&observeClear), reinterpret_cast<void**>(&originalClear));
    if (status != MH_OK) { log("Clear hook creation failed %d", status); return false; }
    if (MH_EnableHook(clearTarget) != MH_OK) { MH_RemoveHook(clearTarget); return false; }
    initialized = true;
    log("D3D11 observer initialized; scene rendering is unchanged");
    return true;
}

bool dumpTexture(ID3D11Texture2D* tex, const char* path) {
    D3D11_TEXTURE2D_DESC d{}; tex->GetDesc(&d);
    d.Usage = D3D11_USAGE_STAGING; d.BindFlags = 0;
    d.CPUAccessFlags = D3D11_CPU_ACCESS_READ; d.MiscFlags = 0;
    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(device->CreateTexture2D(&d, nullptr, &staging))) return false;
    context->CopyResource(staging.Get(), tex);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return false;
    FILE* f = std::fopen(path, "wb");
    bool ok = f != nullptr;
    if (f) {
        // Header: width,height,DXGI_FORMAT,row pitch; then raw pitched rows.
        unsigned header[] = {d.Width, d.Height, static_cast<unsigned>(d.Format), mapped.RowPitch};
        ok = std::fwrite(header, sizeof(header), 1, f) == 1 &&
            std::fwrite(mapped.pData, mapped.RowPitch, d.Height, f) == d.Height;
        std::fclose(f);
    }
    context->Unmap(staging.Get(), 0);
    return ok;
}
}

extern "C" __declspec(dllexport) int CH_Frame(void* singleton, const float* viewProjection) {
    if (!initialized && !initialize(singleton)) return 0;
    ++frame;
    ComPtr<ID3D11Texture2D> back;
    if (FAILED(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(back.GetAddressOf())))) return 0;
    D3D11_TEXTURE2D_DESC d{}; back->GetDesc(&d);
    if (width != d.Width || height != d.Height) {
        std::lock_guard lock(gate); depths.clear(); width = d.Width; height = d.Height;
        log("backbuffer %ux%u format=%u", width, height, d.Format);
    }
    constexpr char Request[] = "nativePC/plugins/CSharp/CrafterHunter/render/capture.request";
    if (GetFileAttributesA(Request) != INVALID_FILE_ATTRIBUTES && DeleteFileA(Request)) {
        std::lock_guard lock(gate);
        const auto stamp = GetTickCount64();
        char path[MAX_PATH];
        std::snprintf(path, sizeof(path), "%s/%llu-color.bin", Dir, stamp);
        const bool colorOk = dumpTexture(back.Get(), path);
        std::snprintf(path, sizeof(path), "%s/%llu-matrices.bin", Dir, stamp);
        if (FILE* f = std::fopen(path, "wb")) { std::fwrite(viewProjection, sizeof(float), 16, f); std::fclose(f); }
        for (size_t i = 0; i < depths.size(); ++i) {
            std::snprintf(path, sizeof(path), "%s/%llu-depth%zu.bin", Dir, stamp, i);
            const bool ok = dumpTexture(depths[i].texture.Get(), path);
            log("capture=%llu candidate=%zu age=%llu clear=%.3f success=%d", stamp, i,
                frame - depths[i].lastFrame, depths[i].clear, ok);
        }
        log("capture=%llu color=%d complete", stamp, colorOk);
    }
    return static_cast<int>(depths.size());
}

extern "C" __declspec(dllexport) void CH_Stop() {
    // DLL stays loaded until process exit so no callback can target freed code.
    if (initialized) { MH_DisableHook(clearTarget); MH_RemoveHook(clearTarget); }
    std::lock_guard lock(gate); depths.clear(); context.Reset(); device.Reset();
    initialized = false; swapchain = nullptr;
}
