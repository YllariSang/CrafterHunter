// Depth-tested stone rendering at the observed pre-UI D3D11 pass.
// No retail code patches or guessed global addresses.
// sMhRender field layout follows SPL D3DModule::common_initialize and
// initialize_for_d3d11. The managed caller gates access to the pinned game hash.
#include <windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
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
bool drawEnabled = false;
bool pipelineFailed = false;
ComPtr<ID3D11DeviceContext1> context1;
ComPtr<ID3DDeviceContextState> drawState;
ComPtr<ID3D11VertexShader> vertexShader;
ComPtr<ID3D11PixelShader> pixelShader;
ComPtr<ID3D11Buffer> constants;
ComPtr<ID3D11Texture2D> stone;
ComPtr<ID3D11ShaderResourceView> stoneView;
ComPtr<ID3D11SamplerState> sampler;
ComPtr<ID3D11RasterizerState> rasterizer;
ComPtr<ID3D11DepthStencilState> noDepth;
std::array<unsigned char, 1024> pixels{};
bool pixelsDirty = false;
struct alignas(16) Parameters {
    float inverse[16]{};
    float projection[16]{};
    float centre[4]{};
    float screen[4]{};
} parameters;
struct Depth {
    ComPtr<ID3D11Texture2D> texture;
    float clear = 1;
    unsigned long long lastFrame = 0;
};
std::vector<Depth> depths;
using ClearFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11DepthStencilView*, UINT, FLOAT, UINT8);
ClearFn originalClear = nullptr;
void* clearTarget = nullptr;
using DrawFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT);
using IndexedFn = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT, INT);
DrawFn originalDraw = nullptr;
IndexedFn originalIndexed = nullptr;
void* drawTarget = nullptr;
void* indexedTarget = nullptr;
bool traceDraws = false;
bool ownDraw = false;
unsigned traceCount = 0;
bool composed = false;
unsigned composedCount = 0;
void drawBlock(ID3D11Texture2D* back, ID3D11Buffer* camera, ID3D11Buffer* ui);
void traceDraw(ID3D11DeviceContext* ctx, const char* kind, UINT count);
void STDMETHODCALLTYPE observeDraw(ID3D11DeviceContext* ctx, UINT n, UINT start) {
    traceDraw(ctx, "Draw", n); originalDraw(ctx, n, start);
}
void STDMETHODCALLTYPE observeIndexed(ID3D11DeviceContext* ctx, UINT n, UINT start, INT base) {
    traceDraw(ctx, "Indexed", n); originalIndexed(ctx, n, start, base);
}

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
    drawTarget = (*reinterpret_cast<void***>(context.Get()))[13];
    indexedTarget = (*reinterpret_cast<void***>(context.Get()))[12];
    if (MH_CreateHook(drawTarget, reinterpret_cast<void*>(&observeDraw), reinterpret_cast<void**>(&originalDraw)) == MH_OK)
        MH_EnableHook(drawTarget);
    if (MH_CreateHook(indexedTarget, reinterpret_cast<void*>(&observeIndexed), reinterpret_cast<void**>(&originalIndexed)) == MH_OK)
        MH_EnableHook(indexedTarget);
    initialized = true;
    log("D3D11 renderer initialized; waiting for a verified pre-UI pass");
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

void traceDraw(ID3D11DeviceContext* ctx, const char* kind, UINT count) {
    if (ownDraw || ctx != context.Get() || (!traceDraws && (!drawEnabled || composed))) return;
    ComPtr<ID3D11RenderTargetView> rtv;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);
    if (!rtv) return;
    ComPtr<ID3D11Resource> resource; rtv->GetResource(&resource);
    ComPtr<ID3D11Texture2D> back;
    if (FAILED(resource.As(&back))) return;
    D3D11_TEXTURE2D_DESC td{}; back->GetDesc(&td);
    if (td.Width != width || td.Height != height) return;
    ComPtr<ID3D11BlendState> blend; FLOAT factor[4]; UINT mask;
    ctx->OMGetBlendState(&blend, factor, &mask);
    D3D11_BLEND_DESC bd{}; if (blend) blend->GetDesc(&bd);
    ComPtr<ID3D11DepthStencilState> depth; UINT ref;
    ctx->OMGetDepthStencilState(&depth, &ref);
    D3D11_DEPTH_STENCIL_DESC dd{}; if (depth) depth->GetDesc(&dd); else dd.DepthEnable = TRUE;
    if (dd.DepthEnable || !bd.RenderTarget[0].BlendEnable) return;
    // Captured on pinned MHW 421810: UI begins on the final R11G11B10 scene
    // target, alpha blended, stride 32, camera b0=1072 and UI b3=400 bytes.
    // The shader also validates the UI's pixel-to-clip scale on the GPU.
    if (!composed && drawEnabled && td.Format == DXGI_FORMAT_R11G11B10_FLOAT &&
        bd.RenderTarget[0].SrcBlend == D3D11_BLEND_SRC_ALPHA &&
        bd.RenderTarget[0].DestBlend == D3D11_BLEND_INV_SRC_ALPHA) {
        UINT stride=0, offset=0; ComPtr<ID3D11Buffer> vertex, camera, ui;
        ctx->IAGetVertexBuffers(0,1,&vertex,&stride,&offset);
        ctx->VSGetConstantBuffers(0,1,&camera); ctx->VSGetConstantBuffers(3,1,&ui);
        D3D11_BUFFER_DESC cameraDesc{}, uiDesc{};
        if (camera) camera->GetDesc(&cameraDesc);
        if (ui) ui->GetDesc(&uiDesc);
        if (stride == 32 && cameraDesc.ByteWidth == 1072 && uiDesc.ByteWidth == 400) {
            drawBlock(back.Get(), camera.Get(), ui.Get()); composed = true;
            if (++composedCount == 1) log("Composing before MHW UI with current GPU camera constants");
        }
    }
    if (!traceDraws || traceCount >= 80) return;
    log("color trace %u %s count=%u format=%u blend=%u target=%p", traceCount, kind, count, td.Format, bd.RenderTarget[0].BlendEnable, back.Get());
    if (traceCount < 6) {
        UINT stride = 0, offset = 0; ComPtr<ID3D11Buffer> vertex;
        ctx->IAGetVertexBuffers(0, 1, &vertex, &stride, &offset);
        log("trace %u vertex stride=%u blend=%u/%u op=%u", traceCount, stride,
            bd.RenderTarget[0].SrcBlend, bd.RenderTarget[0].DestBlend, bd.RenderTarget[0].BlendOp);
        for (unsigned slot = 0; slot < 4; ++slot) {
            ComPtr<ID3D11Buffer> cb; ctx->VSGetConstantBuffers(slot, 1, &cb);
            if (!cb) continue;
            D3D11_BUFFER_DESC desc{}; cb->GetDesc(&desc);
            log("trace %u VS cb%u bytes=%u", traceCount, slot, desc.ByteWidth);
            desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; desc.MiscFlags = 0;
            ComPtr<ID3D11Buffer> staging;
            if (SUCCEEDED(device->CreateBuffer(&desc, nullptr, &staging))) {
                ctx->CopyResource(staging.Get(), cb.Get()); D3D11_MAPPED_SUBRESOURCE map{};
                if (SUCCEEDED(ctx->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &map))) {
                    char path[MAX_PATH]; std::snprintf(path,sizeof(path),"%s/trace-%u-vs%u.bin",Dir,traceCount,slot);
                    if (FILE* f=std::fopen(path,"wb")) { std::fwrite(map.pData,1,desc.ByteWidth,f); std::fclose(f); }
                    ctx->Unmap(staging.Get(),0);
                }
            }
        }
        void* stack[12]{}; const auto n = CaptureStackBackTrace(0, 12, stack, nullptr);
        const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        for (USHORT i = 0; i < n; ++i) log("trace %u stack %u rva=%llx", traceCount, i,
            static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(stack[i]) - base));
    }
    if (traceCount < 6) {
        char path[MAX_PATH]; std::snprintf(path, sizeof(path), "%s/trace-%u-before.bin", Dir, traceCount);
        dumpTexture(back.Get(), path);
    }
    ++traceCount;
}

constexpr char Shader[] = R"hlsl(
cbuffer Parameters : register(b0) {
    row_major float4x4 inverseVP;
    row_major float4x4 vp;
    float4 centre;
    float4 screen;
};
cbuffer HostCamera : register(b1) {
    row_major float4x4 hostVP;
    row_major float4x4 hostView;
    row_major float4x4 hostProjection;
    row_major float4x4 hostInverseView;
    row_major float4x4 hostInverseProjection;
    row_major float4x4 hostInverseVP;
};
cbuffer HostUI : register(b2) { float4 uiScale; };
Texture2D<float4> stone : register(t0);
Texture2D<float> sceneDepth : register(t1);
SamplerState nearest : register(s0);
float4 VS(uint id : SV_VertexID) : SV_Position {
    float2 uv = float2((id << 1) & 2, id & 2);
    return float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
}
float4 PS(float4 pixel : SV_Position) : SV_Target {
    float2 ndc = pixel.xy / screen.xy * float2(2, -2) + float2(-1, 1);
    if (any(abs(uiScale.xy - float2(2, -2) / screen.xy) > 0.000001)) discard;
    // Use this very draw's GPU camera: no stale CPU pose or convention conversion.
    float4 nearH = mul(float4(ndc, 1, 1), hostInverseVP);
    float4 farH = mul(float4(ndc, 0.00001, 1), hostInverseVP);
    float3 origin = nearH.xyz / nearH.w;
    float3 direction = normalize(farH.xyz / farH.w - origin);
    float3 safeDir = (step(0, direction) * 2 - 1) * max(abs(direction), 0.0000001);
    float3 a = (centre.xyz - centre.www - origin) / safeDir;
    float3 b = (centre.xyz + centre.www - origin) / safeDir;
    float3 lo = min(a, b), hi = max(a, b);
    float enter = max(max(lo.x, lo.y), lo.z);
    float leave = min(min(hi.x, hi.y), hi.z);
    if (leave < max(enter, 0)) discard;
    float3 hit = origin + direction * (enter >= 0 ? enter : leave);
    float4 clip = mul(float4(hit, 1), hostVP);
    if (clip.w <= 0) discard;
    float depth = clip.z / clip.w;
    if (depth < 0 || depth > 1) discard;
    float host = sceneDepth.Load(int3(int2(pixel.xy), 0));
    // Confirmed by the resource trace: MHW clears scene depth to 0 (reversed Z).
    if (depth + 0.0000001 < host) discard;
    float3 p = (hit - centre.xyz) / centre.w;
    float3 ap = abs(p);
    float2 uv;
    float shade;
    if (ap.x >= ap.y && ap.x >= ap.z) { uv = float2(p.x > 0 ? -p.z : p.z, -p.y); shade = 0.8; }
    else if (ap.y >= ap.z) { uv = float2(p.x, p.z); shade = p.y > 0 ? 1 : 0.5; }
    else { uv = float2(p.z > 0 ? p.x : -p.x, -p.y); shade = 0.65; }
    float4 color = stone.SampleLevel(nearest, saturate(uv * 0.5 + 0.5), 0);
    if (color.a < 0.5) discard;
    return float4(color.rgb * shade, 1);
}
)hlsl";

bool createPipeline() {
    ComPtr<ID3D11Device1> device1;
    if (FAILED(device.As(&device1)) || FAILED(context.As(&context1))) return false;
    const D3D_FEATURE_LEVEL level = device->GetFeatureLevel();
    D3D_FEATURE_LEVEL selected;
    if (FAILED(device1->CreateDeviceContextState(0, &level, 1, D3D11_SDK_VERSION,
        __uuidof(ID3D11Device), &selected, &drawState))) return false;
    ComPtr<ID3DBlob> vs, ps, error;
    auto hr = D3DCompile(Shader, sizeof(Shader) - 1, "CrafterHunter", nullptr, nullptr,
        "VS", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &vs, &error);
    if (FAILED(hr)) { log("VS compile: %s", error ? static_cast<char*>(error->GetBufferPointer()) : "failed"); return false; }
    error.Reset();
    hr = D3DCompile(Shader, sizeof(Shader) - 1, "CrafterHunter", nullptr, nullptr,
        "PS", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &ps, &error);
    if (FAILED(hr)) { log("PS compile: %s", error ? static_cast<char*>(error->GetBufferPointer()) : "failed"); return false; }
    if (FAILED(device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &vertexShader)) ||
        FAILED(device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &pixelShader))) return false;
    D3D11_BUFFER_DESC bd{}; bd.ByteWidth = sizeof(parameters);
    bd.Usage = D3D11_USAGE_DEFAULT; bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(device->CreateBuffer(&bd, nullptr, &constants))) return false;
    D3D11_TEXTURE2D_DESC td{};
    td.Width = td.Height = 16; td.MipLevels = td.ArraySize = td.SampleDesc.Count = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(device->CreateTexture2D(&td, nullptr, &stone)) ||
        FAILED(device->CreateShaderResourceView(stone.Get(), nullptr, &stoneView))) return false;
    D3D11_SAMPLER_DESC sd{}; sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(device->CreateSamplerState(&sd, &sampler))) return false;
    D3D11_RASTERIZER_DESC rd{}; rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE; rd.DepthClipEnable = TRUE;
    if (FAILED(device->CreateRasterizerState(&rd, &rasterizer))) return false;
    D3D11_DEPTH_STENCIL_DESC dd{}; dd.DepthEnable = FALSE;
    if (FAILED(device->CreateDepthStencilState(&dd, &noDepth))) return false;
    pixelsDirty = true;
    log("Native stone pipeline ready (full context-state preservation)");
    return true;
}

void drawBlock(ID3D11Texture2D* back, ID3D11Buffer* camera, ID3D11Buffer* ui) {
    if (!drawEnabled || pipelineFailed) return;
    if (!pixelShader && !createPipeline()) { pipelineFailed = true; log("Pipeline unavailable; drawing disabled"); return; }
    ComPtr<ID3D11ShaderResourceView> depthView;
    {
        std::lock_guard lock(gate);
        // Candidate 0 was verified in a read-only capture to contain scene geometry.
        // Reject stale/unsupported resources; never fall back to draw-through.
        if (depths.empty() || frame - depths[0].lastFrame > 1 || depths[0].clear != 0) return;
        D3D11_TEXTURE2D_DESC d{}; depths[0].texture->GetDesc(&d);
        if (d.Format != DXGI_FORMAT_R32_TYPELESS || d.Width != width || d.Height != height) return;
        D3D11_SHADER_RESOURCE_VIEW_DESC desc{}; desc.Format = DXGI_FORMAT_R32_FLOAT;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; desc.Texture2D.MipLevels = 1;
        if (FAILED(device->CreateShaderResourceView(depths[0].texture.Get(), &desc, &depthView))) return;
    }
    ComPtr<ID3D11RenderTargetView> target;
    if (FAILED(device->CreateRenderTargetView(back, nullptr, &target))) return;
    ComPtr<ID3DDeviceContextState> saved;
    context1->SwapDeviceContextState(drawState.Get(), &saved);
    if (pixelsDirty) { context->UpdateSubresource(stone.Get(), 0, nullptr, pixels.data(), 64, 1024); pixelsDirty = false; }
    parameters.screen[0] = static_cast<float>(width); parameters.screen[1] = static_cast<float>(height);
    context->UpdateSubresource(constants.Get(), 0, nullptr, &parameters, 0, 0);
    context->OMSetRenderTargets(1, target.GetAddressOf(), nullptr);
    context->OMSetDepthStencilState(noDepth.Get(), 0);
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0); context->DSSetShader(nullptr, nullptr, 0); context->GSSetShader(nullptr, nullptr, 0);
    context->PSSetConstantBuffers(0, 1, constants.GetAddressOf());
    context->PSSetConstantBuffers(1, 1, &camera);
    context->PSSetConstantBuffers(2, 1, &ui);
    ID3D11ShaderResourceView* views[] = {stoneView.Get(), depthView.Get()};
    context->PSSetShaderResources(0, 2, views);
    context->PSSetSamplers(0, 1, sampler.GetAddressOf());
    context->RSSetState(rasterizer.Get());
    D3D11_VIEWPORT viewport{0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1};
    context->RSSetViewports(1, &viewport);
    ownDraw = true; context->Draw(3, 0); ownDraw = false;
    ID3D11ShaderResourceView* empty[2]{}; context->PSSetShaderResources(0, 2, empty);
    context->OMSetRenderTargets(0, nullptr, nullptr);
    context1->SwapDeviceContextState(saved.Get(), nullptr);
}
}

extern "C" __declspec(dllexport) void CH_Block(const float* inverse, const float* centre,
    const unsigned char* rgba, int enabled) {
    drawEnabled = enabled != 0;
    if (!drawEnabled) return;
    std::memcpy(parameters.inverse, inverse, sizeof(parameters.inverse));
    std::memcpy(parameters.centre, centre, sizeof(parameters.centre));
    if (std::memcmp(pixels.data(), rgba, pixels.size())) {
        std::memcpy(pixels.data(), rgba, pixels.size()); pixelsDirty = true;
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
    std::memcpy(parameters.projection, viewProjection, sizeof(parameters.projection));
    composed = false;
    traceDraws = false;
    constexpr char TraceRequest[] = "nativePC/plugins/CSharp/CrafterHunter/render/trace.request";
    if (GetFileAttributesA(TraceRequest) != INVALID_FILE_ATTRIBUTES && DeleteFileA(TraceRequest)) {
        traceDraws = true; traceCount = 0;
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
        std::snprintf(path, sizeof(path), "%s/%llu-parameters.bin", Dir, stamp);
        if (FILE* f = std::fopen(path, "wb")) { std::fwrite(&parameters, sizeof(parameters), 1, f); std::fclose(f); }
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
    if (initialized) {
        MH_DisableHook(clearTarget); MH_RemoveHook(clearTarget);
        MH_DisableHook(drawTarget); MH_RemoveHook(drawTarget);
        MH_DisableHook(indexedTarget); MH_RemoveHook(indexedTarget);
    }
    std::lock_guard lock(gate); depths.clear(); context.Reset(); device.Reset();
    drawState.Reset(); context1.Reset(); vertexShader.Reset(); pixelShader.Reset();
    constants.Reset(); stoneView.Reset(); stone.Reset(); sampler.Reset();
    rasterizer.Reset(); noDepth.Reset(); drawEnabled = false; pipelineFailed = false;
    initialized = false; swapchain = nullptr;
}
