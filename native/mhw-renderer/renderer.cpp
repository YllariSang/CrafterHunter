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
#include "selection.hpp"
#include "frame_transport.hpp"
#include "frame_source.hpp"
#include "frame_composite.hpp"
#include "frame_clock.hpp"

using Microsoft::WRL::ComPtr;
namespace sel = crafterhunter::depth;
namespace frameio = crafterhunter::frame;
namespace frames = crafterhunter::frames;
namespace cmp = crafterhunter::composite;
namespace ch = crafterhunter::clock;
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
    // Content measurement: a candidate is only bindable once a read-back has
    // shown it holds geometry, and it is re-measured periodically because
    // content varies by area.
    bool contentKnown = false;
    float covered = 0;
    unsigned long long checkedFrame = 0;
    ComPtr<ID3D11Texture2D> staging;
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
// The frame composite has its own once-per-frame latch. It shares the stone's
// draw point and depth candidate, but not its enablement: a placed stone is a
// debug aid, and Minecraft must not require one to be visible.
bool composedFrame = false;
unsigned composedCount = 0;
unsigned composedFrameCount = 0;
// Fixed-function state shared by both draw paths, created once on first use
// rather than when the stone happens to come up.
bool sharedReady = false;
// Depth selection is a GPU read-back in the worst case, so it is cached for the
// frame: both paths need the same answer and only one call pays for it.
unsigned long long depthSelectionFrame = 0;
bool framePipelineFailed = false;
// Content read-back state: at most one copy in flight, because Map stalls.
int contentPending = -1;
unsigned long long contentCopyFrame = 0;
std::size_t lastSelected = sel::NotFound;
bool skipLogged = false;
// Frame-synchronization trace: remaining composed frames to log.
unsigned frameSyncRemaining = 0;
unsigned long long previousCameraHash = 0;
bool previousCameraValid = false;
ComPtr<ID3D11Buffer> cameraStaging;

// Minecraft frame state. The texture holds the newest uploaded frame, and
// lastUploadedSequence is what makes the upload conditional: uploading 7.8 MB
// every frame when the guest publishes at 20 Hz would cost more than the frame
// itself.
frames::Source frameSource;
ComPtr<ID3D11Texture2D> minecraftTexture;
ComPtr<ID3D11ShaderResourceView> minecraftView;
UINT minecraftWidth = 0, minecraftHeight = 0;
// The sequence whose pixels are already on the texture. Kept apart from the liveness
// memory below because the two answer different questions: this one avoids re-uploading
// the same frame, the other says the guest is still alive.
//
// When the sequence last changed lives in frameMemory.advanceNanos, on our own clock,
// because that is what a frame's age is measured from - the two processes' clocks were
// measured 3,422,487 ms apart and cannot be compared, and frame_clock.hpp records why
// correcting for that does not work either.
unsigned long long lastUploadedSequence = 0;
// Which way this frame's age is judged, decided once from a measurement and
// logged once so the log states which rule is in force rather than leaving it to
// be inferred.
ch::AgeSource ageSource = ch::AgeSource::LocalLiveness;
bool ageSourceChosen = false;
// Last time we re-opened the channel. The guest deletes and recreates its file
// when the window changes size, which leaves us holding a mapping of a file
// nothing writes to any more; the sequence then freezes, and only re-opening
// recovers. Bounded so a genuinely stopped guest costs one failed open per
// interval rather than one per frame.
unsigned long long lastRemapNanos = 0;
unsigned long long minecraftUploaded = 0;
const char* lastFrameRefusal = nullptr;
bool frameRefusalLogged = false;
unsigned long long lastRefusalLogNanos = 0;

// The scene-depth view drawBlock chose this frame, kept so the frame composite
// compares against the same depth the stone did. Recomputing it would risk two
// different candidates and two different verdicts in one frame.
ComPtr<ID3D11ShaderResourceView> sceneDepthView;
// What the reader remembers about the writer behind the channel. Survives a channel
// re-open on purpose: it is what decides whether a frame is fresh, so discarding it on
// every re-open would discard the only defence against a frozen frame.
frames::FrameMemory frameMemory;

// The Minecraft frame's own pipeline state, kept apart from the stone's.
ComPtr<ID3D11VertexShader> frameVertexShader;
ComPtr<ID3D11PixelShader> framePixelShader;
ComPtr<ID3D11Buffer> frameConstants;

// Maps Minecraft's frame onto MHW's backbuffer. Minecraft's window and MHW's
// are almost never the same size, and the two disagreeing is the normal case
// rather than an edge case, so the mapping is explicit and centred: a frame
// narrower than the backbuffer is letterboxed rather than stretched, because
// stretching would shear Steve and break the depth comparison with it.
struct FrameMapping { float uvRect[4]; float frameFlags[4]; };
FrameMapping frameMapping{};

int drawBlock(ID3D11Texture2D* back, ID3D11Buffer* camera, ID3D11Buffer* ui);
bool createFramePipeline();
void STDMETHODCALLTYPE observeDraw(ID3D11DeviceContext* ctx, UINT n, UINT start);
void STDMETHODCALLTYPE observeIndexed(ID3D11DeviceContext* ctx, UINT n, UINT start, INT base);
bool uploadNewestFrame();
int drawFrame(ID3D11Texture2D* back, ID3D11ShaderResourceView* sceneDepth);
bool drawFrameComposite(ID3D11Texture2D* back);
int selectSceneDepth(ComPtr<ID3D11ShaderResourceView>& out);

void log(const char* format, ...) {
    CreateDirectoryA(Dir, nullptr);
    FILE* f = std::fopen("nativePC/plugins/CSharp/CrafterHunter/render/renderer.log", "a");
    if (!f) return;
    std::fprintf(f, "[%llu] ", GetTickCount64());
    va_list args; va_start(args, format); std::vfprintf(f, format, args); va_end(args);
    std::fputc('\n', f); std::fclose(f);
}

// Refusals are reported when they change and then once a second while they
// persist.
//
// Logging only on change was a real gap and it cost a verification round: the
// reader refused every frame with "no complete frame", said so once at startup,
// and was then silent for the rest of the session. The log looked identical to a
// reader that had stopped being called at all, which is a distinction nobody
// should have to guess at from a log.
void refuse(const char* why) {
    const unsigned long long now = ch::millisToNanos(GetTickCount64());
    const bool changed = !lastFrameRefusal || std::strcmp(lastFrameRefusal, why) != 0;
    const bool heartbeat = now - lastRefusalLogNanos >= ch::millisToNanos(1000);
    if (changed || heartbeat) {
        log("Minecraft frame refused: %s", why);
        lastFrameRefusal = why;
        lastRefusalLogNanos = now;
        frameRefusalLogged = true;
    }
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
                    depths.push_back({tex, clear, frame, false, 0.0f, 0, nullptr});
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

// Row-vector projection, matching the shader's `mul(float4(hit, 1), vp)`.
bool projectScreen(const float* vp, const float* centre, UINT screenWidth,
    UINT screenHeight, float& sx, float& sy) {
    if (!vp || !centre) return false;
    const float x = centre[0], y = centre[1], z = centre[2];
    float clip[4]{};
    for (int c = 0; c < 4; ++c)
        clip[c] = x * vp[c] + y * vp[4 + c] + z * vp[8 + c] + vp[12 + c];
    if (!(clip[3] > 0.000001f)) return false;
    sx = (clip[0] / clip[3] * 0.5f + 0.5f) * static_cast<float>(screenWidth);
    sy = (0.5f - clip[1] / clip[3] * 0.5f) * static_cast<float>(screenHeight);
    return true;
}

// Records, for a bounded number of frames, what is needed to show that the
// composition used this frame's camera and this frame's depth:
//   - a hash of the GPU camera constants of the very draw being composed, and
//     whether it changed since the previous traced frame (live, not stale);
//   - the block centre projected once with those GPU constants and once with
//     the CPU-side view-projection handed to CH_Frame for this frame - the
//     pixel difference between them is the CPU/GPU frame-sync error;
//   - the depth candidate actually bound, its measured coverage, its age in
//     frames, and how many traced frames remain.
// Bounded because reading a constant buffer back stalls the pipeline.
void recordFrameSync(ID3D11Buffer* camera, int bound) {
    if (!frameSyncRemaining || !context || !device || !camera) return;
    --frameSyncRemaining;
    float host[16]{};
    bool cameraRead = false;
    if (!cameraStaging) {
        D3D11_BUFFER_DESC bd{}; camera->GetDesc(&bd);
        bd.Usage = D3D11_USAGE_STAGING; bd.BindFlags = 0;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ; bd.MiscFlags = 0;
        if (FAILED(device->CreateBuffer(&bd, nullptr, &cameraStaging))) cameraStaging.Reset();
    }
    if (cameraStaging) {
        context->CopyResource(cameraStaging.Get(), camera);
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(context->Map(cameraStaging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) {
            std::memcpy(host, mapped.pData, sizeof(host));
            context->Unmap(cameraStaging.Get(), 0);
            cameraRead = true;
        }
    }
    unsigned long long hash = 0;
    if (cameraRead) {
        hash = 14695981039346656037ULL;  // FNV-1a over the GPU host view-projection
        const auto* bytes = reinterpret_cast<const unsigned char*>(host);
        for (size_t i = 0; i < sizeof(host); ++i) {
            hash ^= bytes[i];
            hash *= 1099511628211ULL;
        }
    }
    const bool changed = previousCameraValid && hash != previousCameraHash;
    previousCameraHash = hash;
    previousCameraValid = cameraRead;

    float gpuX = 0, gpuY = 0, cpuX = 0, cpuY = 0;
    const bool gpuOk = projectScreen(host, parameters.centre, width, height, gpuX, gpuY);
    const bool cpuOk = projectScreen(parameters.projection, parameters.centre, width, height, cpuX, cpuY);

    float covered = 0;
    unsigned long long age = 0;
    bool haveDepth = false;
    {
        std::lock_guard lock(gate);
        if (bound >= 0 && static_cast<size_t>(bound) < depths.size()) {
            covered = depths[bound].covered;
            age = frame - depths[bound].lastFrame;
            haveDepth = true;
        }
    }
    log("framesync frame=%llu depth=%d covered=%.2f%% age=%llu cam=%016llx %s gpu=(%.1f,%.1f) cpu=(%.1f,%.1f) delta=(%.2f,%.2f) left=%u",
        frame, bound, haveDepth ? covered * 100.0f : 0.0f, age, hash,
        cameraRead ? (changed ? "changed" : "same") : "unread",
        gpuX, gpuY, cpuX, cpuY,
        (gpuOk && cpuOk) ? gpuX - cpuX : 0.0f, (gpuOk && cpuOk) ? gpuY - cpuY : 0.0f,
        frameSyncRemaining);
}

void traceDraw(ID3D11DeviceContext* ctx, const char* kind, UINT count) {
    // The stone's gate does not gate the frame. It used to, which meant a placed
    // stone was a precondition for Minecraft being visible at all - a debug aid
    // silently deciding whether the player could exist.
    if (ownDraw || ctx != context.Get()) return;
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
    if (td.Format == DXGI_FORMAT_R11G11B10_FLOAT &&
        bd.RenderTarget[0].SrcBlend == D3D11_BLEND_SRC_ALPHA &&
        bd.RenderTarget[0].DestBlend == D3D11_BLEND_INV_SRC_ALPHA) {
        UINT stride=0, offset=0; ComPtr<ID3D11Buffer> vertex, camera, ui;
        ctx->IAGetVertexBuffers(0,1,&vertex,&stride,&offset);
        ctx->VSGetConstantBuffers(0,1,&camera); ctx->VSGetConstantBuffers(3,1,&ui);
        D3D11_BUFFER_DESC cameraDesc{}, uiDesc{};
        if (camera) camera->GetDesc(&cameraDesc);
        if (ui) ui->GetDesc(&uiDesc);
        if (!composed && drawEnabled && stride == 32 &&
            cameraDesc.ByteWidth == 1072 && uiDesc.ByteWidth == 400) {
            const int bound = drawBlock(back.Get(), camera.Get(), ui.Get());
            composed = true;
            if (++composedCount == 1) log("Composing before MHW UI with current GPU camera constants");
            if (frameSyncRemaining) recordFrameSync(camera.Get(), bound);
        }
        // Minecraft draws after the stone, so a block placed inside Minecraft's
        // geometry stays visible rather than being buried. The order only
        // decides who wins where both would write; neither depends on the other.
        if (!composedFrame) composedFrame = drawFrameComposite(back.Get());
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


// The stone's vertex shader. Kept distinct from FrameVS rather than shared: they
// differ only in name today, but one generates a full-screen triangle for the
// stone and the other for the frame, and a shared body would let a change to one
// silently alter the other.
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
// The Minecraft frame's shaders, in a source string of their own.
//
// Separate from the stone's because both pixel shaders bind t0 and t1 to
// different resources. Sharing one compilation unit makes those two
// declarations overlap, and D3DCompile rejects the whole file with X4500
// "overlapping register semantics". Only Microsoft's compiler sees it: glslang
// compiles each entry point in isolation and accepts this happily, so
// tools/test-shader-source.sh is a floor rather than proof.
constexpr char FrameShader[] = R"hlsl(
cbuffer Parameters : register(b0) {
    row_major float4x4 inverseVP;
    row_major float4x4 vp;
    float4 centre;
    float4 screen;
};
cbuffer FrameMapping : register(b3) {
    float4 uvRect;      // xy = the rectangle's size as a fraction, zw = its origin
    float4 frameFlags;  // x = 1 when the mapping is usable, 0 when it is degenerate
};
Texture2D<float4> minecraftColour : register(t0);
Texture2D<float> sceneDepth : register(t1);
SamplerState nearest : register(s0);
float4 FrameVS(uint id : SV_VertexID) : SV_Position {
    float2 uv = float2((id << 1) & 2, id & 2);
    return float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
}

// One Minecraft pixel, drawn only where MHW has nothing of its own.
//
// This is the interim rule from frame_composite.hpp, decideSkyOnly(). The full
// rule compares Minecraft's depth against MHW's and is unit-tested there; it is
// not implemented here because the transport carries colour and no depth, so
// there is no Minecraft depth to sample. Rather than bind MHW's depth texture
// where Minecraft's was expected and silently compare a surface with itself,
// this draws only against the host's sky.
//
// What that means on screen: Steve appears against MHW's sky and is NOT
// occluded by MHW's terrain. A tree between him and the camera will not hide
// him. Full occlusion needs Minecraft's own depth in the transport, which is a
// second attachment and a linearisation, not a shader change.
//
// The host fact this relies on is confirmed by the resource trace: MHW clears
// scene depth to 0 under reversed Z, so 0 is the far end and means the host pass
// wrote nothing here.
float4 FramePS(float4 pixel : SV_Position) : SV_Target {
    // A degenerate rectangle has no inverse. Checked here as well as on the host
    // because a division by zero yields a NaN, and every comparison against NaN is
    // false - so a NaN uv would sail past the discard below and sample whatever the
    // hardware made of it.
    if (frameFlags.x < 0.5) discard;

    float2 hostUv = pixel.xy / screen.xy;
    // The screen coordinate is brought *into* the guest rectangle, not scaled. The
    // other direction inverts the relationship and draws a region larger than the
    // screen, cropping the frame and stretching it edge to edge instead of
    // letterboxing it. See mapToGuest() in frame_composite.hpp, and its tests.
    float2 uv = (hostUv - uvRect.zw) / uvRect.xy;
    if (any(uv < 0.0) || any(uv > 1.0)) discard;

    float hostZ = sceneDepth.Load(int3(int2(pixel.xy), 0));
    // Any host geometry here wins. This is the check that keeps the frame from
    // covering the hunter or a monster.
    if (hostZ > 0.0) discard;

    // Minecraft's sky is indistinguishable from its geometry without depth, so
    // the letterbox carries Minecraft's sky with it. Stated rather than hidden:
    // it is the visible cost of not having depth.
    return minecraftColour.SampleLevel(nearest, uv, 0);
}
)hlsl";


// The fixed-function objects both draw paths bind: the context-state slot that
// makes a draw invisible to the game, the screen-size constant buffer, and three
// states whose parameters never vary. Split out of createPipeline because the
// frame path needs all of it, and gating it on the stone would mean Minecraft
// could not appear until a stone had been placed.
bool ensureSharedState() {
    if (sharedReady) return true;
    ComPtr<ID3D11Device1> device1;
    if (FAILED(device.As(&device1)) || FAILED(context.As(&context1))) return false;
    const D3D_FEATURE_LEVEL level = device->GetFeatureLevel();
    D3D_FEATURE_LEVEL selected;
    if (FAILED(device1->CreateDeviceContextState(0, &level, 1, D3D11_SDK_VERSION,
        __uuidof(ID3D11Device), &selected, &drawState))) return false;
    D3D11_BUFFER_DESC cbDesc{}; cbDesc.ByteWidth = sizeof(parameters);
    cbDesc.Usage = D3D11_USAGE_DEFAULT; cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(device->CreateBuffer(&cbDesc, nullptr, &constants))) return false;
    D3D11_SAMPLER_DESC sd{}; sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(device->CreateSamplerState(&sd, &sampler))) return false;
    D3D11_RASTERIZER_DESC rd{}; rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE; rd.DepthClipEnable = TRUE;
    if (FAILED(device->CreateRasterizerState(&rd, &rasterizer))) return false;
    D3D11_DEPTH_STENCIL_DESC dd{}; dd.DepthEnable = FALSE;
    if (FAILED(device->CreateDepthStencilState(&dd, &noDepth))) return false;
    sharedReady = true;
    log("Shared draw state ready (context state, constants, sampler, rasterizer, no-depth)");
    return true;
}

bool createPipeline() {
    if (!ensureSharedState()) return false;
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
    D3D11_TEXTURE2D_DESC td{};
    td.Width = td.Height = 16; td.MipLevels = td.ArraySize = td.SampleDesc.Count = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(device->CreateTexture2D(&td, nullptr, &stone)) ||
        FAILED(device->CreateShaderResourceView(stone.Get(), nullptr, &stoneView))) return false;
    pixelsDirty = true;
    log("Native stone pipeline ready (full context-state preservation)");
    return true;
}

// The Minecraft frame's own pipeline. Separate shaders, sampler and constant
// buffer from the stone's, because they are drawn at different times against
// different bindings; sharing one set would mean rebinding both on every draw and
// would hide which resource each draw actually read.
bool createFramePipeline() {
    if (!ensureSharedState()) return false;
    ComPtr<ID3DBlob> vs, ps, error;
    if (FAILED(D3DCompile(FrameShader, sizeof(FrameShader) - 1, "CrafterHunter", nullptr, nullptr,
            "FrameVS", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &vs, &error))) {
        log("FrameVS compile: %s", error ? static_cast<char*>(error->GetBufferPointer()) : "failed");
        return false;
    }
    error.Reset();
    if (FAILED(D3DCompile(FrameShader, sizeof(FrameShader) - 1, "CrafterHunter", nullptr, nullptr,
            "FramePS", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &ps, &error))) {
        log("FramePS compile: %s", error ? static_cast<char*>(error->GetBufferPointer()) : "failed");
        return false;
    }
    if (FAILED(device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(),
            nullptr, &frameVertexShader)) ||
        FAILED(device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(),
            nullptr, &framePixelShader))) return false;

    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = sizeof(FrameMapping);
    bd.Usage = D3D11_USAGE_DEFAULT; bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(device->CreateBuffer(&bd, nullptr, &frameConstants))) return false;

    // No Minecraft depth texture is created, because the transport carries none.
    // The one that used to sit here was a 1x1 placeholder for a binding the
    // shader no longer reads; leaving it would suggest depth arrives someday
    // without saying what it would cost.
    log("Minecraft frame pipeline ready (colour only: sky-against composite, no depth channel)");
    return true;
}

// Measures whether a candidate actually holds scene geometry, so selection can
// reject the fresh-but-empty buffer measured on 2026-10-05. One copy in flight
// at a time and only for candidates fresh enough to be selectable: Map stalls
// the GPU, so this happens on first sight and then only every
// sel::ContentRecheckFrames frames. The result is cached on the candidate.
// Caller holds `gate`.
void serviceContentChecks() {
    constexpr unsigned long long CopyTimeoutFrames = 90;
    if (contentPending >= static_cast<int>(depths.size())) contentPending = -1;
    if (contentPending >= 0) {
        Depth& pending = depths[contentPending];
        if (!pending.staging || frame - contentCopyFrame > CopyTimeoutFrames) {
            contentPending = -1;
        } else {
            D3D11_MAPPED_SUBRESOURCE mapped{};
            if (SUCCEEDED(context->Map(pending.staging.Get(), 0, D3D11_MAP_READ,
                    D3D11_MAP_FLAG_DO_NOT_WAIT, &mapped))) {
                const float measured = sel::coverage([&](unsigned x, unsigned y) {
                    const auto* row = reinterpret_cast<const float*>(
                        static_cast<const unsigned char*>(mapped.pData) +
                        static_cast<size_t>(y) * mapped.RowPitch);
                    return row[x];
                }, width, height);
                context->Unmap(pending.staging.Get(), 0);
                const bool changed = !pending.contentKnown ||
                    (measured >= sel::ContentThreshold) != (pending.covered >= sel::ContentThreshold);
                pending.contentKnown = true;
                pending.covered = measured;
                pending.checkedFrame = frame;
                if (changed)
                    log("depth[%d] content %.2f%% %s (age=%llu)", contentPending, measured * 100.0f,
                        measured >= sel::ContentThreshold ? "holds geometry" : "empty",
                        frame - pending.lastFrame);
                contentPending = -1;
            }
        }
    }
    if (contentPending >= 0) return;
    // Next candidate to measure: anything unmeasured first, then the oldest
    // measured one that is due a re-check. Stale ones are skipped, since
    // measuring a buffer we would refuse to bind tells us nothing useful.
    int chosen = -1;
    unsigned long long oldest = 0;
    for (size_t i = 0; i < depths.size(); ++i) {
        Depth& candidate = depths[i];
        if (frame - candidate.lastFrame > sel::FreshFrames) continue;
        if (!candidate.contentKnown) { chosen = static_cast<int>(i); break; }
        const unsigned long long due = frame - candidate.checkedFrame;
        if (due >= sel::ContentRecheckFrames && (chosen < 0 || due > oldest)) {
            chosen = static_cast<int>(i); oldest = due;
        }
    }
    if (chosen < 0) return;
    Depth& candidate = depths[chosen];
    if (!candidate.staging) {
        D3D11_TEXTURE2D_DESC td{};
        candidate.texture->GetDesc(&td);
        td.Usage = D3D11_USAGE_STAGING; td.BindFlags = 0;
        td.CPUAccessFlags = D3D11_CPU_ACCESS_READ; td.MiscFlags = 0;
        if (FAILED(device->CreateTexture2D(&td, nullptr, &candidate.staging))) return;
    }
    context->CopyResource(candidate.staging.Get(), candidate.texture.Get());
    contentPending = chosen;
    contentCopyFrame = frame;
}

// Returns the index of the depth candidate bound for this frame, or -1 when the
// frame is skipped. Selection never falls back to drawing without depth: an
// unmeasured, stale or empty candidate means no composition, not draw-through.
// Uploads the newest published Minecraft frame, if there is one. Returns true
// only when a frame is on the texture and ready to draw.
//
// The upload is conditional on the sequence, not on the frame rate: the guest
// publishes at 20-60 Hz, and uploading 7.8 MB on every MHW frame would cost more
// than the frame itself. A frame whose geometry changed means the guest resized
// its window, and the texture is rebuilt rather than stretched.
// Re-opens the channel, at most once per RemapIntervalNanos.
//
// Every way of losing the channel ends the same way - no frame arrives - and none of
// them is visible from inside the mapping. The file can be unlinked while we hold it
// (a verification script did exactly that), recreated at a new size after a window
// change, or replaced by a restarted guest whose sequence numbers begin again at one.
// In each case the mapping still describes something real and something *past*, and
// only re-opening tells us so.
//
// Returns whether a frame is available afterwards, which is what the callers want to
// know and what keeps the retry loop in one place.
bool recoverChannel(bool haveFrame) {
    const unsigned long long now = ch::millisToNanos(GetTickCount64());
    if (!frames::shouldRemap(haveFrame, now, lastRemapNanos)) return false;
    lastRemapNanos = now;

    const bool wasMapped = frameSource.mapped();
    frameSource.close();
    if (!frameSource.open()) {
        log("Minecraft frame channel still absent after re-opening it");
        return false;
    }

    const char* why = nullptr;
    const frameio::FrameView view = frameSource.newestFrame(now, &why);
    // Forget what we were holding *only* if this is a different writer, not merely a
    // different file handle.
    //
    // Re-opening a channel whose file still exists always succeeds, including when that
    // file is the same frozen one. Forgetting unconditionally therefore meant that once
    // per second the reader zeroed its liveness clock, saw the same unmoving sequence as
    // if it were fresh progress, and drew a frame from a guest that had gone. Recovery
    // was defeating the stall detection meant to stop exactly that.
    const bool headerOk = frameSource.headerValid();
    const frames::Generation generation = frames::classifyGeneration(
        frameMemory, headerOk, headerOk, view.sequence, view.width, view.height);
    const bool forget = frames::shouldForget(generation);
    log("re-opened the Minecraft frame channel (%s, size %zu): %s, memory %s",
        wasMapped ? "was mapped" : "was absent", frameSource.length(),
        generation == frames::Generation::Same ? "same writer"
            : generation == frames::Generation::Restarted ? "restarted"
            : generation == frames::Generation::Resized ? "resized" : "replaced",
        forget ? "cleared" : "kept");

    if (forget) {
        lastUploadedSequence = 0;
        frameMemory.established = false;
        ageSourceChosen = false;
    }
    return view.pixels != nullptr;
    log("Minecraft frame channel still absent after re-opening it");
    return false;
}

bool uploadNewestFrame() {
    if (!frameSource.mapped() && !recoverChannel(false)) {
        refuse("channel not present (guest not publishing)");
        return false;
    }

    // Our own clock, in nanoseconds. GetTickCount64 counts milliseconds and the
    // conversion is a multiplication by a million - an earlier version divided as
    // well and produced a number ten thousand times too small, which made every
    // frame look like it had been captured in the future.
    const unsigned long long now = ch::millisToNanos(GetTickCount64());
    const char* reason = nullptr;
    frameio::FrameView view = frameSource.newestFrame(now, &reason);
    if (!view.pixels) {
        // Recover before refusing. Returning here is what left the reader holding a
        // mapping of a file nothing was writing to, for the rest of the session.
        if (recoverChannel(false)) {
            view = frameSource.newestFrame(now, &reason);
        }
        if (!view.pixels) {
            if (reason) refuse(reason);
            return false;
        }
    }

    // Decide once, from a measurement, how this frame's age will be judged, and
    // say so in the log. The guest's stamp and ours were measured 3,422,487 ms
    // apart, so the transport's own 50 ms freshness rule cannot apply here.
    if (!ageSourceChosen) {
        const std::int64_t divergence = ch::divergenceNanos(now, view.capturedNanos);
        ageSource = ch::chooseAgeSource(divergence, frameio::MaxAgeNanos);
        ageSourceChosen = true;
        log("frame age source=%s (guest stamp is %.3f s from ours, tolerance %.0f ms)",
            ageSource == ch::AgeSource::GuestStamp ? "guest-stamp" : "local-liveness",
            static_cast<double>(divergence) / 1e9,
            static_cast<double>(frameio::MaxAgeNanos) / 1e6);
    }

    // Liveness is measured here, on our own clock, because nothing else can be. The
    // memory struct is what survives a channel re-open, so the two are kept as one.
    if (view.sequence != frameMemory.sequence) {
        frameMemory.sequence = view.sequence;
        frameMemory.advanceNanos = now;
    }
    frameMemory.width = view.width;
    frameMemory.height = view.height;
    frameMemory.established = true;
    const unsigned long long lastAdvanceNanos = frameMemory.advanceNanos;

    if (!ch::frameIsFresh(ageSource, now, view.capturedNanos, lastAdvanceNanos,
                          ch::StallBoundNanos, frameio::MaxAgeNanos)) {
        // The sequence has stopped advancing on a mapping that may describe the
        // past: an unlinked file, a recreated one, or one from before a guest
        // restart. This used to carry its own remap under a different interval; it now
        // goes through the same rate-limited routine as every other refusal, because
        // two bounds for one remedy is one bound too many.
        if (recoverChannel(false)) return uploadNewestFrame();
        refuse("frame is not being published");
        return false;
    }
    frameRefusalLogged = false;

    if (view.sequence == lastUploadedSequence && minecraftView) return true;
    if (view.width != minecraftWidth || view.height != minecraftHeight) {
        D3D11_TEXTURE2D_DESC td{};
        td.Width = view.width; td.Height = view.height;
        td.MipLevels = td.ArraySize = td.SampleDesc.Count = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        td.CPUAccessFlags = 0;
        minecraftTexture.Reset();
        minecraftView.Reset();
        if (FAILED(device->CreateTexture2D(&td, nullptr, &minecraftTexture))) return false;
        if (FAILED(device->CreateShaderResourceView(minecraftTexture.Get(), nullptr, &minecraftView))) return false;
        minecraftWidth = view.width; minecraftHeight = view.height;
        log("Minecraft frame texture %ux%u", minecraftWidth, minecraftHeight);
    }

    D3D11_TEXTURE2D_DESC staged{};
    minecraftTexture->GetDesc(&staged);
    // The guest's rows are tight; D3D's default pitch is the row's own size, so
    // no repacking is needed. Pitch is read back rather than assumed.
    context->UpdateSubresource(minecraftTexture.Get(), 0, nullptr, view.pixels,
        static_cast<UINT>(staged.Width * 4), static_cast<UINT>(staged.Height));

    // Ask again whether that is still the frame we just uploaded. The upload
    // copied from shared memory the guest may already be refilling, and a torn
    // frame is indistinguishable from a rendering fault. Dropping it costs one
    // frame of latency, which nothing can see.
    if (!frameSource.stillHolds(view.sequence)) {
        refuse("slot recycled during upload; frame dropped");
        return false;
    }
    frameRefusalLogged = false;
    lastUploadedSequence = view.sequence;
    ++minecraftUploaded;
    if (minecraftUploaded == 1) {
        log("First Minecraft frame uploaded: seq=%llu %ux%u",
            static_cast<unsigned long long>(view.sequence), view.width, view.height);
    }
    lastFrameRefusal = nullptr;
    frameRefusalLogged = false;
    lastRefusalLogNanos = 0;
    return true;
}
int drawFrame(ID3D11Texture2D* back, ID3D11ShaderResourceView* sceneDepth);
void traceDraw(ID3D11DeviceContext* ctx, const char* kind, UINT count);
void STDMETHODCALLTYPE observeDraw(ID3D11DeviceContext* ctx, UINT n, UINT start) {
    traceDraw(ctx, "Draw", n); originalDraw(ctx, n, start);
}
void STDMETHODCALLTYPE observeIndexed(ID3D11DeviceContext* ctx, UINT n, UINT start, INT base) {
    traceDraw(ctx, "Indexed", n); originalIndexed(ctx, n, start, base);
}

// The scene-depth candidate for this frame, as a shader-readable view, or -1
// when none is eligible. Shared by both draw paths so that a stone and a Steve
// in the same frame can never disagree about what is in front of them, and
// cached per frame because the content measurement behind it can stall the GPU.
//
// Deliberately independent of the stone: occlusion is needed by both, and
// neither is the reason the other may exist.
int selectSceneDepth(ComPtr<ID3D11ShaderResourceView>& out) {
    if (depthSelectionFrame == frame && sceneDepthView) {
        out = sceneDepthView;
        return static_cast<int>(lastSelected);
    }
    ComPtr<ID3D11ShaderResourceView> depthView;
    std::size_t chosen = sel::NotFound;
    {
        std::lock_guard lock(gate);
        serviceContentChecks();
        std::array<sel::CandidateView, 8> candidates{};
        const size_t count = depths.size() < candidates.size() ? depths.size() : candidates.size();
        for (size_t i = 0; i < count; ++i) {
            D3D11_TEXTURE2D_DESC d{}; depths[i].texture->GetDesc(&d);
            candidates[i] = sel::CandidateView{depths[i].clear, depths[i].lastFrame, frame,
                d.Format == DXGI_FORMAT_R32_TYPELESS && d.Width == width && d.Height == height,
                depths[i].contentKnown, depths[i].covered};
        }
        chosen = sel::select(candidates.data(), count);
        if (chosen == sel::NotFound) {
            lastSelected = sel::NotFound;
            sceneDepthView.Reset();
            depthSelectionFrame = frame;
            if (!skipLogged) {
                skipLogged = true;
                log("no eligible depth candidate (%zu observed; unmeasured, stale or empty) - composition skipped",
                    count);
            }
            return -1;
        }
        if (chosen != lastSelected) {
            lastSelected = chosen;
            log("depth select=%zu covered=%.2f%% age=%llu", chosen,
                depths[chosen].covered * 100.0f, frame - depths[chosen].lastFrame);
        }
        skipLogged = false;
        D3D11_SHADER_RESOURCE_VIEW_DESC desc{}; desc.Format = DXGI_FORMAT_R32_FLOAT;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; desc.Texture2D.MipLevels = 1;
        if (FAILED(device->CreateShaderResourceView(depths[chosen].texture.Get(), &desc, &depthView))) return -1;
        sceneDepthView = depthView;
        depthSelectionFrame = frame;
    }
    out = depthView;
    return static_cast<int>(chosen);
}

int drawBlock(ID3D11Texture2D* back, ID3D11Buffer* camera, ID3D11Buffer* ui) {    if (!drawEnabled || pipelineFailed) return -1;
    if (!pixelShader && !createPipeline()) { pipelineFailed = true; log("Pipeline unavailable; drawing disabled"); return -1; }
    ComPtr<ID3D11ShaderResourceView> depthView;
    const int chosen = selectSceneDepth(depthView);
    if (chosen < 0) return -1;
    ComPtr<ID3D11RenderTargetView> target;
    if (FAILED(device->CreateRenderTargetView(back, nullptr, &target))) return -1;
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
    return chosen;
}

// Creates the frame pipeline on first use and reports failure once, so a
// machine without shader model 5 sees one line rather than one per frame.
bool ensureFramePipeline() {
    if (framePixelShader) return true;
    if (framePipelineFailed) return false;
    if (!createFramePipeline()) {
        framePipelineFailed = true;
        log("Minecraft frame pipeline unavailable; frame composite disabled");
        return false;
    }
    return true;
}

// Draws the guest's frame into the pre-UI target. Returns true only when a frame
// was actually drawn, which is what the once-per-frame latch keys on.
//
// The order inside is deliberate: pipeline, then pixels, then depth. Uploading
// before the depth is known means a frame arrives even in a frame where no
// depth is usable, so the next one has something to show immediately.
bool drawFrameComposite(ID3D11Texture2D* back) {
    if (!ensureFramePipeline()) return false;
    if (!uploadNewestFrame()) return false;
    ComPtr<ID3D11ShaderResourceView> depthView;
    if (selectSceneDepth(depthView) < 0) return false;
    if (drawFrame(back, depthView.Get()) == 0) return false;
    if (++composedFrameCount == 1) log("Compositing Minecraft frame with MHW scene depth");
    return true;
}

// The Minecraft frame, drawn through the per-pixel rule in frame_composite.hpp.
//
// At most one upload per published frame: the guest publishes at 20-60 Hz while
// MHW renders at its own rate, so the cost here is bounded by the guest rather
// than by MHW's frame count.
int drawFrame(ID3D11Texture2D* back, ID3D11ShaderResourceView* sceneDepth) {
    if (!minecraftView || !framePixelShader) return 0;

    // The letterbox, computed by the function frame_composite.hpp's tests cover. The
    // arithmetic used to live here untested, and was inverted: it scaled the screen
    // coordinate instead of mapping it into the rectangle, so the region drawn was
    // larger than the backbuffer and Minecraft was cropped and stretched rather than
    // letterboxed. There is now nowhere for that mistake to hide.
    const cmp::Letterbox box = cmp::letterbox(static_cast<float>(width),
        static_cast<float>(height), static_cast<float>(minecraftWidth),
        static_cast<float>(minecraftHeight));
    frameMapping.uvRect[0] = box.scaleX;
    frameMapping.uvRect[1] = box.scaleY;
    frameMapping.uvRect[2] = box.offsetX;
    frameMapping.uvRect[3] = box.offsetY;
    // Zero when degenerate, so the shader discards instead of dividing by zero.
    frameMapping.frameFlags[0] = (box.scaleX > 0.0f && box.scaleY > 0.0f) ? 1.0f : 0.0f;
    frameMapping.frameFlags[1] = 0.0f;
    frameMapping.frameFlags[2] = 0.0f;
    frameMapping.frameFlags[3] = 0.0f;

    ComPtr<ID3D11RenderTargetView> target;
    if (FAILED(device->CreateRenderTargetView(back, nullptr, &target))) return 0;
    ComPtr<ID3DDeviceContextState> saved;
    context1->SwapDeviceContextState(drawState.Get(), &saved);

    parameters.screen[0] = static_cast<float>(width);
    parameters.screen[1] = static_cast<float>(height);
    context->UpdateSubresource(constants.Get(), 0, nullptr, &parameters, 0, 0);
    context->UpdateSubresource(frameConstants.Get(), 0, nullptr, &frameMapping, 0, 0);
    context->OMSetRenderTargets(1, target.GetAddressOf(), nullptr);
    context->OMSetDepthStencilState(noDepth.Get(), 0);
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(frameVertexShader.Get(), nullptr, 0);
    context->PSSetShader(framePixelShader.Get(), nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0); context->DSSetShader(nullptr, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->PSSetConstantBuffers(0, 1, constants.GetAddressOf());
    context->PSSetConstantBuffers(3, 1, frameConstants.GetAddressOf());
    ID3D11ShaderResourceView* frameViews[] = {minecraftView.Get(), sceneDepth};
    context->PSSetShaderResources(0, 2, frameViews);
    context->PSSetSamplers(0, 1, sampler.GetAddressOf());
    context->RSSetState(rasterizer.Get());
    D3D11_VIEWPORT viewport{0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1};
    context->RSSetViewports(1, &viewport);
    ownDraw = true; context->Draw(3, 0); ownDraw = false;
    ID3D11ShaderResourceView* empty[2]{}; context->PSSetShaderResources(0, 2, empty);
    context->OMSetRenderTargets(0, nullptr, nullptr);
    context1->SwapDeviceContextState(saved.Get(), nullptr);
    return 1;
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
        contentPending = -1; lastSelected = sel::NotFound;
        log("backbuffer %ux%u format=%u", width, height, d.Format);
    }
    std::memcpy(parameters.projection, viewProjection, sizeof(parameters.projection));
    composed = false;
    composedFrame = false;
    traceDraws = false;
    constexpr char TraceRequest[] = "nativePC/plugins/CSharp/CrafterHunter/render/trace.request";
    if (GetFileAttributesA(TraceRequest) != INVALID_FILE_ATTRIBUTES && DeleteFileA(TraceRequest)) {
        traceDraws = true; traceCount = 0;
    }
    constexpr char FrameSyncRequest[] = "nativePC/plugins/CSharp/CrafterHunter/render/framesync.request";
    if (GetFileAttributesA(FrameSyncRequest) != INVALID_FILE_ATTRIBUTES && DeleteFileA(FrameSyncRequest)) {
        frameSyncRemaining = 60; previousCameraValid = false;
        log("frame-sync trace armed for up to 60 composed frames");
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
    rasterizer.Reset(); noDepth.Reset(); cameraStaging.Reset();
    contentPending = -1; lastSelected = sel::NotFound;
    frameSyncRemaining = 0; previousCameraValid = false;
    drawEnabled = false; pipelineFailed = false;
    composed = false; composedFrame = false; composedFrameCount = 0;
    sharedReady = false; framePipelineFailed = false; depthSelectionFrame = 0;
    sceneDepthView.Reset(); minecraftView.Reset(); minecraftTexture.Reset();
    frameVertexShader.Reset(); framePixelShader.Reset(); frameConstants.Reset();
    lastUploadedSequence = 0; frameMemory = frames::FrameMemory{};
    ageSourceChosen = false; lastRemapNanos = 0;
    minecraftUploaded = 0; minecraftWidth = 0; minecraftHeight = 0;
    frameSource.close();
    initialized = false; swapchain = nullptr;
}
