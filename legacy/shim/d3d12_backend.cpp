// Direct3D 12 backend for the D3D8 shim: fixed-function emulation with a single uber shader.
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "d3d8_objects.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace gfx {
namespace {

constexpr UINT kFrames = 3;
constexpr UINT64 kUploadBytes = 96ull << 20;
constexpr UINT kSrvCpuSlots = 8192;
constexpr UINT kSrvGpuPerFrame = 16384;
constexpr UINT kMaxRtv = 128;
constexpr DXGI_FORMAT kColorFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
constexpr DXGI_FORMAT kDepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

struct StageC {
    uint32_t colorOp, colorArg1, colorArg2, alphaOp;
    uint32_t alphaArg1, alphaArg2, coordIndex, flags;
};
struct LightC {
    float dir[4], diffuse[4], ambient[4], specular[4];
};
struct SceneC {
    float mvp[16], world[16], texMat[2][16], worldView[16];
    float matDiffuse[4], matAmbient[4], matEmissive[4], matSpecular[4];
    float globalAmbient[4], fogColor[4], fogParams[4];
    float eye[4], viewport[4];
    uint32_t flags[4];  // lighting, specular, diffuseFromVertex, ambientFromVertex
    uint32_t flags2[4]; // fogMode, alphaTest, lightMask, alphaFunc
    StageC stage[2];
    LightC lights[8];
};
static_assert(sizeof(SceneC) % 16 == 0, "cbuffer layout");

const char kShader[] = R"hlsl(
struct StageC { uint colorOp, colorArg1, colorArg2, alphaOp, alphaArg1, alphaArg2, coordIndex, flags; };
struct LightC { float4 dir, diffuse, ambient, specular; };
cbuffer Scene : register(b0)
{
    row_major float4x4 mvp;
    row_major float4x4 world;
    row_major float4x4 texMat0;
    row_major float4x4 texMat1;
    row_major float4x4 worldView;
    float4 matDiffuse, matAmbient, matEmissive, matSpecular;
    float4 globalAmbient, fogColor, fogParams;
    float4 eye, viewport;
    uint4 flags;
    uint4 flags2;
    StageC stage[2];
    LightC lights[8];
};

Texture2D tex0 : register(t0);
Texture2D tex1 : register(t1);
SamplerState samWrap : register(s0);
SamplerState samClamp : register(s1);
SamplerState samMirror : register(s2);

struct VSInput
{
    float3 position : POSITION;
#if HAS_RHW
    float rhw : RHW;
#endif
#if HAS_NORMAL
    float3 normal : NORMAL;
#endif
#if HAS_COLOR
    float4 color : COLOR;
#endif
#if HAS_UV0
    float2 uv0 : TEXCOORD0;
#endif
#if HAS_UV1
    float2 uv1 : TEXCOORD1;
#endif
};

struct VSOutput
{
    float4 position : SV_POSITION;
    INTERP float4 color : COLOR;
    INTERP float3 spec : TEXCOORD3;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float viewDepth : TEXCOORD2;
};

float2 GenCoord(StageC st, float4x4 m, VSInput i, float3 viewNormal)
{
    float2 uv = float2(0, 0);
    uint idx = st.coordIndex & 0xffff;
    if (st.coordIndex & 0x10000) {
        float4 c = float4(viewNormal, 1);
        if (st.flags & 2) c = mul(c, m);
        return c.xy;
    }
#if HAS_UV1
    if (idx == 1) uv = i.uv1; else
#endif
    {
#if HAS_UV0
        uv = i.uv0;
#endif
    }
    if (st.flags & 2) uv = mul(float4(uv, 1, 0), m).xy;
    return uv;
}

VSOutput VSMain(VSInput i)
{
    VSOutput o;
#if HAS_RHW
    o.position = float4(i.position.x / viewport.x * 2.0f - 1.0f, 1.0f - i.position.y / viewport.y * 2.0f, i.position.z, 1.0f);
    o.viewDepth = 0;
#else
    o.position = mul(float4(i.position, 1.0f), mvp);
    o.viewDepth = o.position.w;
#endif
#if HAS_COLOR
    float4 vcol = i.color;
#else
    float4 vcol = float4(1, 1, 1, 1);
#endif
    o.spec = float3(0, 0, 0);
    float3 viewNormal = float3(0, 0, 0);
#if HAS_NORMAL
    viewNormal = normalize(mul(i.normal, (float3x3)worldView));
#endif
#if LIGHTING
    float3 n = normalize(mul(i.normal, (float3x3)world));
    float3 wpos = mul(float4(i.position, 1.0f), world).xyz;
    float3 toEye = normalize(eye.xyz - wpos);
    float4 diffMat = flags.z ? vcol : matDiffuse;
    float4 ambMat = flags.w ? vcol : matAmbient;
    float3 amb = globalAmbient.rgb;
    float3 dif = float3(0, 0, 0);
    [unroll]
    for (uint k = 0; k < 8; ++k)
    {
        if (flags2.z & (1u << k))
        {
            float3 toLight = -lights[k].dir.xyz;
            float ndl = saturate(dot(n, toLight));
            amb += lights[k].ambient.rgb;
            dif += lights[k].diffuse.rgb * ndl;
            if (flags.y != 0 && ndl > 0.0f)
            {
                float3 h = normalize(toLight + toEye);
                o.spec += lights[k].specular.rgb * matSpecular.rgb * pow(saturate(dot(n, h)), max(matSpecular.w, 0.0001f));
            }
        }
    }
    o.spec = saturate(o.spec);
    o.color.rgb = saturate(matEmissive.rgb + ambMat.rgb * amb + diffMat.rgb * dif);
    o.color.a = diffMat.a;
#else
    o.color = vcol;
#endif
    o.uv0 = GenCoord(stage[0], texMat0, i, viewNormal);
    o.uv1 = GenCoord(stage[1], texMat1, i, viewNormal);
    return o;
}

float4 SampleTex(uint s, uint samplerIdx, float2 uv)
{
    if (s == 0)
    {
        [branch] if (samplerIdx == 1) return tex0.Sample(samClamp, uv);
        else if (samplerIdx == 2) return tex0.Sample(samMirror, uv);
        return tex0.Sample(samWrap, uv);
    }
    [branch] if (samplerIdx == 1) return tex1.Sample(samClamp, uv);
    else if (samplerIdx == 2) return tex1.Sample(samMirror, uv);
    return tex1.Sample(samWrap, uv);
}

float4 Arg(uint a, float4 t, float4 d, float4 c)
{
    uint sel = a & 0xf;
    if (sel == 0) return d;
    if (sel == 1) return c;
    return t;
}

float3 Op3(uint op, float3 a1, float3 a2)
{
    if (op == 2) return a1;
    if (op == 3) return a2;
    if (op == 5) return saturate(a1 * a2 * 2);
    if (op == 6) return saturate(a1 * a2 * 4);
    if (op == 7) return saturate(a1 + a2);
    if (op == 11) return a1 + a2 - a1 * a2;
    return a1 * a2;
}

float Op1(uint op, float a1, float a2)
{
    if (op == 2) return a1;
    if (op == 3) return a2;
    if (op == 5) return saturate(a1 * a2 * 2);
    if (op == 6) return saturate(a1 * a2 * 4);
    if (op == 7) return saturate(a1 + a2);
    if (op == 11) return a1 + a2 - a1 * a2;
    return a1 * a2;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    float4 diffuse = input.color;
    float4 current = diffuse;
    [unroll]
    for (uint s = 0; s < 2; ++s)
    {
        StageC st = stage[s];
        if (st.colorOp == 1) break;
        bool hasTex = (st.flags & 1) != 0;
        bool usesTex = ((st.colorArg1 & 0xf) == 2) || ((st.colorArg2 & 0xf) == 2) || ((st.alphaArg1 & 0xf) == 2) || ((st.alphaArg2 & 0xf) == 2);
        if (!hasTex && usesTex) break;
        float4 t = float4(1, 1, 1, 1);
        if (hasTex) t = SampleTex(s, (st.flags >> 8) & 0xff, s == 0 ? input.uv0 : input.uv1);
        float4 c1 = Arg(st.colorArg1, t, diffuse, current);
        float4 c2 = Arg(st.colorArg2, t, diffuse, current);
        float3 rgb = Op3(st.colorOp, c1.rgb, c2.rgb);
        float a = current.a;
        if (st.alphaOp != 1)
        {
            float4 a1 = Arg(st.alphaArg1, t, diffuse, current);
            float4 a2 = Arg(st.alphaArg2, t, diffuse, current);
            a = Op1(st.alphaOp, a1.a, a2.a);
        }
        current = float4(rgb, a);
    }
    float4 color = current;
    if (flags.y != 0) color.rgb = saturate(color.rgb + input.spec);
    if (flags2.y != 0)
    {
        float r = fogParams.w;
        float a = color.a;
        bool ok = true;
        uint f = flags2.w;
        if (f == 1) ok = false;
        else if (f == 2) ok = a < r;
        else if (f == 3) ok = a == r;
        else if (f == 4) ok = a <= r;
        else if (f == 5) ok = a > r;
        else if (f == 6) ok = a != r;
        else if (f == 7) ok = a >= r;
        if (!ok) discard;
    }
    uint fm = flags2.x;
    if (fm != 0)
    {
        float d = input.viewDepth;
        float fog = 1.0f;
        if (fm == 3) fog = saturate((fogParams.y - d) / max(fogParams.y - fogParams.x, 0.0001f));
        else if (fm == 1) fog = saturate(exp(-d * fogParams.z));
        else if (fm == 2) fog = saturate(exp(-(d * fogParams.z) * (d * fogParams.z)));
        color.rgb = lerp(fogColor.rgb, color.rgb, fog);
    }
    return color;
}
)hlsl";

void Log(const char* fmt, const char* s = "")
{
    FILE* f = fopen("d3d12_backend.log", "a");
    if (!f) return;
    fprintf(f, fmt, s);
    fputc(10, f);
    fclose(f);
}

UINT Align(UINT64 v, UINT a) { return (UINT)((v + a - 1) / a * a); }

struct GpuTex {
    ComPtr<ID3D12Resource> res;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    uint32_t version = 0;
    UINT srv = 0;
    UINT rtv = UINT_MAX;
    UINT width = 0, height = 0;
};

struct CachedBuffer {
    ComPtr<ID3D12Resource> res;
    uint32_t version = 0;
    size_t bytes = 0;
};

struct PsoKey {
    uint32_t fvf = 0;
    uint8_t topology = 0, blend = 0, src = 0, dst = 0, cull = 0, depthEnable = 0, depthWrite = 0, depthFunc = 0;
    uint8_t stencilEnable = 0, stencilFunc = 0, stencilFail = 0, stencilZFail = 0, stencilPass = 0, wire = 0,
            flat = 0, lighting = 0;
    uint32_t stencilRead = 0, stencilWrite = 0;
    bool operator==(const PsoKey& o) const { return memcmp(this, &o, sizeof(*this)) == 0; }
};
struct PsoHash {
    size_t operator()(const PsoKey& k) const
    {
        uint64_t h = 1469598103934665603ull;
        const uint8_t* p = (const uint8_t*)&k;
        for (size_t i = 0; i < sizeof(k); ++i) h = (h ^ p[i]) * 1099511628211ull;
        return (size_t)h;
    }
};

class D3D12Backend : public Backend {
public:
    ~D3D12Backend() override
    {
        if (queue_) WaitIdle();
        if (event_) CloseHandle(event_);
    }

    bool Init(HWND window, const D3DPRESENT_PARAMETERS& pp) override
    {
        window_ = window;
        UINT factoryFlags = 0;
        if (getenv("RS2_D3D12_DEBUG")) {
            ComPtr<ID3D12Debug> dbg;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg)))) {
                dbg->EnableDebugLayer();
                factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
            }
        }
        if (FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory_)))) return false;
        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; factory_->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC1 d;
            adapter->GetDesc1(&d);
            if (!(d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
                SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&dev_))))
                break;
        }
        if (!dev_) return false;

        D3D12_COMMAND_QUEUE_DESC qd{};
        qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        if (FAILED(dev_->CreateCommandQueue(&qd, IID_PPV_ARGS(&queue_)))) return false;
        if (FAILED(dev_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)))) return false;
        event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        D3D12_DESCRIPTOR_HEAP_DESC hd{};
        hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        hd.NumDescriptors = kMaxRtv;
        dev_->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&rtvHeap_));
        hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        hd.NumDescriptors = 32;
        dev_->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&dsvHeap_));
        hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        hd.NumDescriptors = kSrvCpuSlots;
        dev_->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&srvCpu_));
        hd.NumDescriptors = kSrvGpuPerFrame * kFrames;
        hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        dev_->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&srvGpu_));
        rtvInc_ = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        dsvInc_ = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        srvInc_ = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        for (UINT i = 0; i < kFrames; ++i) {
            dev_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frames_[i].alloc));
            D3D12_HEAP_PROPERTIES hp{D3D12_HEAP_TYPE_UPLOAD};
            D3D12_RESOURCE_DESC rd = BufferDesc(kUploadBytes);
            if (FAILED(dev_->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_GENERIC_READ,
                                                     nullptr, IID_PPV_ARGS(&frames_[i].upload))))
                return false;
            frames_[i].upload->Map(0, nullptr, (void**)&frames_[i].mapped);
        }
        dev_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frames_[0].alloc.Get(), nullptr,
                                IID_PPV_ARGS(&list_));
        list_->Close();

        if (!CreateRootSignature()) return false;
        for (UINT i = 0; i < kSrvCpuSlots; ++i) freeSrv_.push_back(kSrvCpuSlots - 1 - i);
        for (UINT i = kFrames; i < kMaxRtv; ++i) freeRtv_.push_back(kMaxRtv - 1 - (i - kFrames));
        CreateWhiteTexture();
        if (!CreateSwapChain(pp)) return false;
        BeginFrame();
        return true;
    }

    bool Resize(const D3DPRESENT_PARAMETERS& pp) override
    {
        WaitIdle();
        for (auto& b : backBuffers_) b.Reset();
        mainDepth_.Reset();
        offscreenDepth_.clear();
        if (swap_) {
            if (FAILED(swap_->ResizeBuffers(kFrames, pp.BackBufferWidth, pp.BackBufferHeight, kColorFormat, 0)))
                return false;
        }
        width_ = pp.BackBufferWidth;
        height_ = pp.BackBufferHeight;
        CreateBackBuffers();
        // The command list was recorded against the old buffers; restart the frame.
        frameOpen_ = false;
        BeginFrame();
        return true;
    }

    void BeginScene() override {}
    void EndScene() override {}

    void Clear(const State& s, DWORD flags, D3DCOLOR color, float z, DWORD stencil) override
    {
        if (!BindTarget(s)) return;
        if (flags & D3DCLEAR_TARGET) {
            const float c[4] = {((color >> 16) & 0xff) / 255.0f, ((color >> 8) & 0xff) / 255.0f,
                                (color & 0xff) / 255.0f, ((color >> 24) & 0xff) / 255.0f};
            list_->ClearRenderTargetView(curRtv_, c, 0, nullptr);
        }
        if (flags & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL)) {
            D3D12_CLEAR_FLAGS cf = (D3D12_CLEAR_FLAGS)0;
            if (flags & D3DCLEAR_ZBUFFER) cf |= D3D12_CLEAR_FLAG_DEPTH;
            if (flags & D3DCLEAR_STENCIL) cf |= D3D12_CLEAR_FLAG_STENCIL;
            list_->ClearDepthStencilView(curDsv_, cf, z, (UINT8)stencil, 0, nullptr);
        }
    }

    void Draw(const State& s, D3DPRIMITIVETYPE type, const Geometry& g) override
    {
        if (g.vertexCount == 0) return;
        if (!BindTarget(s)) return;

        PsoKey key = MakeKey(s, type);
        ID3D12PipelineState* pso = GetPso(key);
        if (!pso) return;

        // Geometry
        D3D12_VERTEX_BUFFER_VIEW vbv{};
        vbv.StrideInBytes = g.stride;
        if (g.vbId) {
            CachedBuffer& cb = GetCached(g.vbId, g.vbVersion, g.vbBase, g.vbBytes);
            const size_t offset = (const uint8_t*)g.vertices - (const uint8_t*)g.vbBase;
            vbv.BufferLocation = cb.res->GetGPUVirtualAddress() + offset;
            vbv.SizeInBytes = (UINT)(g.vbBytes - offset);
        } else {
            const UINT bytes = g.vertexCount * g.stride;
            D3D12_GPU_VIRTUAL_ADDRESS addr;
            void* dst = Alloc(bytes, 16, &addr);
            if (!dst) return;
            memcpy(dst, g.vertices, bytes);
            vbv.BufferLocation = addr;
            vbv.SizeInBytes = bytes;
        }

        D3D12_INDEX_BUFFER_VIEW ibv{};
        bool indexed = g.indices != nullptr;
        std::vector<uint16_t> fan;
        const uint16_t* indices = g.indices;
        UINT indexCount = g.indexCount;
        if (type == D3DPT_TRIANGLEFAN && !indexed) {
            for (UINT i = 1; i + 1 < g.vertexCount; ++i) fan.insert(fan.end(), {0, (uint16_t)i, (uint16_t)(i + 1)});
            indices = fan.data();
            indexCount = (UINT)fan.size();
            indexed = true;
        }
        if (indexed) {
            ibv.Format = DXGI_FORMAT_R16_UINT;
            ibv.SizeInBytes = indexCount * 2;
            if (g.ibId) {
                CachedBuffer& cb = GetCached(g.ibId, g.ibVersion, indices, indexCount * 2);
                ibv.BufferLocation = cb.res->GetGPUVirtualAddress();
            } else {
                D3D12_GPU_VIRTUAL_ADDRESS addr;
                void* dst = Alloc(indexCount * 2, 4, &addr);
                if (!dst) return;
                memcpy(dst, indices, indexCount * 2);
                ibv.BufferLocation = addr;
            }
        }

        // Pipeline and state
        if (pso != curPso_) {
            list_->SetPipelineState(pso);
            curPso_ = pso;
        }
        D3D_PRIMITIVE_TOPOLOGY topo = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        switch (type) {
        case D3DPT_POINTLIST: topo = D3D_PRIMITIVE_TOPOLOGY_POINTLIST; break;
        case D3DPT_LINELIST: topo = D3D_PRIMITIVE_TOPOLOGY_LINELIST; break;
        case D3DPT_LINESTRIP: topo = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
        case D3DPT_TRIANGLESTRIP: topo = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
        default: break;
        }
        if (type == D3DPT_TRIANGLESTRIP && indexed) topo = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        list_->IASetPrimitiveTopology(topo);
        list_->OMSetStencilRef(s.rs[D3DRS_STENCILREF]);

        SceneC c;
        FillScene(s, &c);
        if (!hasLastScene_ || memcmp(&c, &lastScene_, sizeof(c)) != 0) {
            D3D12_GPU_VIRTUAL_ADDRESS addr;
            void* dst = Alloc(sizeof(SceneC), 256, &addr);
            if (!dst) return;
            memcpy(dst, &c, sizeof(c));
            list_->SetGraphicsRootConstantBufferView(0, addr);
            lastScene_ = c;
            hasLastScene_ = true;
        }
        BindTextures(s);

        list_->IASetVertexBuffers(0, 1, &vbv);
        if (indexed) {
            list_->IASetIndexBuffer(&ibv);
            list_->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
        } else {
            list_->DrawInstanced(g.vertexCount, 1, 0, 0);
        }
    }

    void Present() override
    {
        OpenFrame();
        // Return the backbuffer to PRESENT.
        Transition(backBuffers_[bbIndex_].Get(), bbState_, D3D12_RESOURCE_STATE_PRESENT);
        bbState_ = D3D12_RESOURCE_STATE_PRESENT;
        list_->Close();
        frameOpen_ = false;
        ID3D12CommandList* lists[] = {list_.Get()};
        queue_->ExecuteCommandLists(1, lists);
        swap_->Present(1, 0);
        frames_[frameIndex_].fenceValue = ++fenceValue_;
        queue_->Signal(fence_.Get(), fenceValue_);
        bbIndex_ = swap_->GetCurrentBackBufferIndex();
        frameIndex_ = (frameIndex_ + 1) % kFrames;
        bbState_ = D3D12_RESOURCE_STATE_PRESENT;
        BeginFrame();
    }

    void ReadSurface(Surface8* surface, uint32_t* out) override
    {
        OpenFrame();
        ID3D12Resource* res = nullptr;
        D3D12_RESOURCE_STATES* state = nullptr;
        if (surface->kind == Surface8::BackBuffer) {
            res = backBuffers_[bbIndex_].Get();
            state = &bbState_;
        } else if (surface->tex && surface->tex->gpu) {
            auto* t = (GpuTex*)surface->tex->gpu;
            res = t->res.Get();
            state = &t->state;
        }
        if (!res) {
            memset(out, 0, (size_t)surface->width * surface->height * 4);
            return;
        }
        const UINT pitch = Align(surface->width * 4, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
        D3D12_HEAP_PROPERTIES hp{D3D12_HEAP_TYPE_READBACK};
        D3D12_RESOURCE_DESC rd = BufferDesc((UINT64)pitch * surface->height);
        ComPtr<ID3D12Resource> rb;
        dev_->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                      IID_PPV_ARGS(&rb));
        const D3D12_RESOURCE_STATES prev = *state;
        Transition(res, prev, D3D12_RESOURCE_STATE_COPY_SOURCE);
        D3D12_TEXTURE_COPY_LOCATION dst{}, src{};
        dst.pResource = rb.Get();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dst.PlacedFootprint.Footprint = {kColorFormat, surface->width, surface->height, 1, pitch};
        src.pResource = res;
        src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        list_->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        Transition(res, D3D12_RESOURCE_STATE_COPY_SOURCE, prev);
        // Flush so the copy is visible, then continue recording.
        list_->Close();
        frameOpen_ = false;
        ID3D12CommandList* lists[] = {list_.Get()};
        queue_->ExecuteCommandLists(1, lists);
        WaitIdle();
        list_->Reset(frames_[frameIndex_].alloc.Get(), nullptr);
        ReopenState();
        uint8_t* p = nullptr;
        rb->Map(0, nullptr, (void**)&p);
        for (UINT y = 0; y < surface->height; ++y) memcpy(out + (size_t)y * surface->width, p + (size_t)y * pitch, surface->width * 4);
        rb->Unmap(0, nullptr);
    }

    void OnTextureDestroyed(Tex8* tex) override
    {
        if (!tex->gpu) return;
        auto* t = (GpuTex*)tex->gpu;
        Retire(t->res);
        freeSrv_.push_back(t->srv);
        if (t->rtv != UINT_MAX) freeRtv_.push_back(t->rtv);
        delete t;
        tex->gpu = nullptr;
    }

    void OnGeometryDestroyed(uint64_t id) override
    {
        auto it = cache_.find(id);
        if (it == cache_.end()) return;
        Retire(it->second.res);
        cache_.erase(it);
    }

private:
    struct Frame {
        ComPtr<ID3D12CommandAllocator> alloc;
        ComPtr<ID3D12Resource> upload;
        uint8_t* mapped = nullptr;
        UINT64 fenceValue = 0;
        UINT64 offset = 0;
        UINT srvOffset = 0;
    };
    struct Retired {
        ComPtr<ID3D12Resource> res;
        UINT64 fence;
    };

    static D3D12_RESOURCE_DESC BufferDesc(UINT64 bytes)
    {
        D3D12_RESOURCE_DESC rd{};
        rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        rd.Width = bytes;
        rd.Height = 1;
        rd.DepthOrArraySize = 1;
        rd.MipLevels = 1;
        rd.SampleDesc.Count = 1;
        rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        return rd;
    }

    void Transition(ID3D12Resource* r, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
    {
        if (from == to) return;
        D3D12_RESOURCE_BARRIER b{};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource = r;
        b.Transition.StateBefore = from;
        b.Transition.StateAfter = to;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        list_->ResourceBarrier(1, &b);
    }

    void WaitIdle()
    {
        queue_->Signal(fence_.Get(), ++fenceValue_);
        fence_->SetEventOnCompletion(fenceValue_, event_);
        WaitForSingleObject(event_, INFINITE);
    }

    void Retire(ComPtr<ID3D12Resource>& r)
    {
        if (r) retired_.push_back({r, fenceValue_ + kFrames});
        r.Reset();
    }

    void ReleaseRetired()
    {
        const UINT64 done = fence_->GetCompletedValue();
        size_t w = 0;
        for (size_t i = 0; i < retired_.size(); ++i)
            if (retired_[i].fence > done) retired_[w++] = std::move(retired_[i]);
        retired_.resize(w);
    }

    bool CreateRootSignature()
    {
        D3D12_DESCRIPTOR_RANGE range{};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors = 2;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        D3D12_ROOT_PARAMETER p[2]{};
        p[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        p[0].Descriptor.ShaderRegister = 0;
        p[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        p[1].DescriptorTable.NumDescriptorRanges = 1;
        p[1].DescriptorTable.pDescriptorRanges = &range;
        p[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_STATIC_SAMPLER_DESC sm[3]{};
        const D3D12_TEXTURE_ADDRESS_MODE modes[3] = {D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                     D3D12_TEXTURE_ADDRESS_MODE_MIRROR};
        for (int i = 0; i < 3; ++i) {
            sm[i].Filter = D3D12_FILTER_ANISOTROPIC;
            sm[i].AddressU = sm[i].AddressV = sm[i].AddressW = modes[i];
            sm[i].MaxAnisotropy = 16;
            sm[i].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            sm[i].MaxLOD = D3D12_FLOAT32_MAX;
            sm[i].ShaderRegister = i;
            sm[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        }
        D3D12_ROOT_SIGNATURE_DESC d{};
        d.NumParameters = 2;
        d.pParameters = p;
        d.NumStaticSamplers = 3;
        d.pStaticSamplers = sm;
        d.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        ComPtr<ID3DBlob> blob, err;
        if (FAILED(D3D12SerializeRootSignature(&d, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &err))) return false;
        return SUCCEEDED(dev_->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
                                                   IID_PPV_ARGS(&rootSig_)));
    }

    bool CreateSwapChain(const D3DPRESENT_PARAMETERS& pp)
    {
        width_ = pp.BackBufferWidth;
        height_ = pp.BackBufferHeight;
        DXGI_SWAP_CHAIN_DESC1 sd{};
        sd.Width = width_;
        sd.Height = height_;
        sd.Format = kColorFormat;
        sd.SampleDesc.Count = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = kFrames;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        ComPtr<IDXGISwapChain1> sc;
        if (FAILED(factory_->CreateSwapChainForHwnd(queue_.Get(), window_, &sd, nullptr, nullptr, &sc))) return false;
        sc.As(&swap_);
        factory_->MakeWindowAssociation(window_, DXGI_MWA_NO_ALT_ENTER);
        CreateBackBuffers();
        return true;
    }

    void CreateBackBuffers()
    {
        for (UINT i = 0; i < kFrames; ++i) {
            swap_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i]));
            D3D12_CPU_DESCRIPTOR_HANDLE h = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
            h.ptr += (SIZE_T)i * rtvInc_;
            dev_->CreateRenderTargetView(backBuffers_[i].Get(), nullptr, h);
        }
        bbIndex_ = swap_->GetCurrentBackBufferIndex();
        bbState_ = D3D12_RESOURCE_STATE_PRESENT;
        mainDepth_ = CreateDepth(width_, height_);
        D3D12_CPU_DESCRIPTOR_HANDLE h = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
        dev_->CreateDepthStencilView(mainDepth_.Get(), nullptr, h);
    }

    ComPtr<ID3D12Resource> CreateDepth(UINT w, UINT h)
    {
        D3D12_HEAP_PROPERTIES hp{D3D12_HEAP_TYPE_DEFAULT};
        D3D12_RESOURCE_DESC rd{};
        rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        rd.Width = w;
        rd.Height = h;
        rd.DepthOrArraySize = 1;
        rd.MipLevels = 1;
        rd.Format = kDepthFormat;
        rd.SampleDesc.Count = 1;
        rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        D3D12_CLEAR_VALUE cv{};
        cv.Format = kDepthFormat;
        cv.DepthStencil.Depth = 1.0f;
        ComPtr<ID3D12Resource> r;
        dev_->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_DEPTH_WRITE, &cv,
                                      IID_PPV_ARGS(&r));
        return r;
    }

    void CreateWhiteTexture()
    {
        Tex8 dummy;
        dummy.width = dummy.height = 1;
        dummy.levels.assign(1, std::vector<uint32_t>(1, 0xffffffffu));
        white_ = CreateGpuTex(dummy);
    }

    GpuTex* CreateGpuTex(Tex8& t)
    {
        auto* g = new GpuTex();
        g->width = t.width;
        g->height = t.height;
        D3D12_HEAP_PROPERTIES hp{D3D12_HEAP_TYPE_DEFAULT};
        D3D12_RESOURCE_DESC rd{};
        rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        rd.Width = t.width;
        rd.Height = t.height;
        rd.DepthOrArraySize = 1;
        rd.MipLevels = (UINT16)t.levels.size();
        rd.Format = kColorFormat;
        rd.SampleDesc.Count = 1;
        D3D12_CLEAR_VALUE cv{};
        cv.Format = kColorFormat;
        D3D12_RESOURCE_STATES initial = D3D12_RESOURCE_STATE_COPY_DEST;
        if (t.renderTarget()) {
            rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            rd.MipLevels = 1;
            initial = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        }
        dev_->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, initial,
                                      t.renderTarget() ? &cv : nullptr, IID_PPV_ARGS(&g->res));
        g->state = initial;
        g->srv = freeSrv_.back();
        freeSrv_.pop_back();
        D3D12_SHADER_RESOURCE_VIEW_DESC sv{};
        sv.Format = kColorFormat;
        sv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        sv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        sv.Texture2D.MipLevels = rd.MipLevels;
        D3D12_CPU_DESCRIPTOR_HANDLE h = srvCpu_->GetCPUDescriptorHandleForHeapStart();
        h.ptr += (SIZE_T)g->srv * srvInc_;
        dev_->CreateShaderResourceView(g->res.Get(), &sv, h);
        if (t.renderTarget()) {
            g->rtv = freeRtv_.back();
            freeRtv_.pop_back();
            D3D12_CPU_DESCRIPTOR_HANDLE r = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
            r.ptr += (SIZE_T)g->rtv * rtvInc_;
            dev_->CreateRenderTargetView(g->res.Get(), nullptr, r);
            g->version = t.version;
        } else {
            OpenFrame();
            UploadTexture(g, t);
        }
        return g;
    }

    void UploadTexture(GpuTex* g, Tex8& t)
    {
        Transition(g->res.Get(), g->state, D3D12_RESOURCE_STATE_COPY_DEST);
        for (UINT l = 0; l < t.levels.size(); ++l) {
            const UINT w = t.LevelWidth(l), h = t.LevelHeight(l);
            const UINT pitch = Align(w * 4, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
            D3D12_GPU_VIRTUAL_ADDRESS addr;
            uint8_t* dst = (uint8_t*)Alloc((UINT64)pitch * h, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT, &addr);
            if (!dst) break;
            for (UINT y = 0; y < h; ++y) memcpy(dst + (size_t)y * pitch, &t.levels[l][(size_t)y * w], w * 4);
            D3D12_TEXTURE_COPY_LOCATION dl{}, sl{};
            dl.pResource = g->res.Get();
            dl.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dl.SubresourceIndex = l;
            sl.pResource = frames_[frameIndex_].upload.Get();
            sl.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            sl.PlacedFootprint.Offset = addr - frames_[frameIndex_].upload->GetGPUVirtualAddress();
            sl.PlacedFootprint.Footprint = {kColorFormat, w, h, 1, pitch};
            list_->CopyTextureRegion(&dl, 0, 0, 0, &sl, nullptr);
        }
        Transition(g->res.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        g->state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        g->version = t.version;
    }

    GpuTex* Resolve(Tex8* t)
    {
        if (!t) return white_;
        if (!t->gpu) t->gpu = CreateGpuTex(*t);
        auto* g = (GpuTex*)t->gpu;
        if (!t->renderTarget() && g->version != t->version) {
            OpenFrame();
            UploadTexture(g, *t);
        }
        if (g->state != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
            Transition(g->res.Get(), g->state, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            g->state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        }
        return g;
    }

    void BindTextures(const State& s)
    {
        GpuTex* a = Resolve(s.tex[0]);
        GpuTex* b = Resolve(s.tex[1]);
        if (a == lastTex_[0] && b == lastTex_[1] && lastHeapFrame_ == frameCounter_) return;
        Frame& f = frames_[frameIndex_];
        if (f.srvOffset + 2 > kSrvGpuPerFrame) f.srvOffset = 0; // unlikely: reuse (risky, but bounded)
        const UINT base = frameIndex_ * kSrvGpuPerFrame + f.srvOffset;
        f.srvOffset += 2;
        D3D12_CPU_DESCRIPTOR_HANDLE dst = srvGpu_->GetCPUDescriptorHandleForHeapStart();
        dst.ptr += (SIZE_T)base * srvInc_;
        D3D12_CPU_DESCRIPTOR_HANDLE src = srvCpu_->GetCPUDescriptorHandleForHeapStart();
        D3D12_CPU_DESCRIPTOR_HANDLE s0{src.ptr + (SIZE_T)a->srv * srvInc_}, s1{src.ptr + (SIZE_T)b->srv * srvInc_};
        dev_->CopyDescriptorsSimple(1, dst, s0, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE dst2{dst.ptr + srvInc_};
        dev_->CopyDescriptorsSimple(1, dst2, s1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_GPU_DESCRIPTOR_HANDLE gh = srvGpu_->GetGPUDescriptorHandleForHeapStart();
        gh.ptr += (UINT64)base * srvInc_;
        list_->SetGraphicsRootDescriptorTable(1, gh);
        lastTex_[0] = a;
        lastTex_[1] = b;
        lastHeapFrame_ = frameCounter_;
    }

    CachedBuffer& GetCached(uint64_t id, uint32_t version, const void* data, size_t bytes)
    {
        CachedBuffer& cb = cache_[id];
        if (!cb.res || cb.version != version || cb.bytes != bytes) {
            Retire(cb.res);
            D3D12_HEAP_PROPERTIES hp{D3D12_HEAP_TYPE_UPLOAD};
            D3D12_RESOURCE_DESC rd = BufferDesc(std::max<size_t>(bytes, 16));
            dev_->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                          IID_PPV_ARGS(&cb.res));
            void* p = nullptr;
            cb.res->Map(0, nullptr, &p);
            memcpy(p, data, bytes);
            cb.res->Unmap(0, nullptr);
            cb.version = version;
            cb.bytes = bytes;
        }
        return cb;
    }

    void* Alloc(UINT64 bytes, UINT align, D3D12_GPU_VIRTUAL_ADDRESS* addr)
    {
        Frame& f = frames_[frameIndex_];
        const UINT64 off = (f.offset + align - 1) / align * align;
        if (off + bytes > kUploadBytes) return nullptr;
        f.offset = off + bytes;
        *addr = f.upload->GetGPUVirtualAddress() + off;
        return f.mapped + off;
    }

    void BeginFrame()
    {
        Frame& f = frames_[frameIndex_];
        if (f.fenceValue && fence_->GetCompletedValue() < f.fenceValue) {
            fence_->SetEventOnCompletion(f.fenceValue, event_);
            WaitForSingleObject(event_, INFINITE);
        }
        ReleaseRetired();
        f.alloc->Reset();
        f.offset = 0;
        f.srvOffset = 0;
        list_->Reset(f.alloc.Get(), nullptr);
        frameOpen_ = true;
        ++frameCounter_;
        ReopenState();
    }

    void ReopenState()
    {
        frameOpen_ = true;
        ID3D12DescriptorHeap* heaps[] = {srvGpu_.Get()};
        list_->SetDescriptorHeaps(1, heaps);
        list_->SetGraphicsRootSignature(rootSig_.Get());
        curPso_ = nullptr;
        hasLastScene_ = false;
        lastTex_[0] = lastTex_[1] = nullptr;
        curTargetKey_ = nullptr;
        vpValid_ = false;
    }

    void OpenFrame()
    {
        if (frameOpen_) return;
        list_->Reset(frames_[frameIndex_].alloc.Get(), nullptr);
        ReopenState();
    }

    struct Target {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv{}, dsv{};
        UINT w = 0, h = 0;
    };

    bool BindTarget(const State& s)
    {
        OpenFrame();
        Surface8* rt = s.renderTarget;
        if (!rt) return false;
        const void* key = rt;
        const bool back = rt->kind == Surface8::BackBuffer;
        GpuTex* offTex = nullptr;
        UINT w = rt->width, h = rt->height;
        D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
        if (back) {
            if (bbState_ != D3D12_RESOURCE_STATE_RENDER_TARGET) {
                Transition(backBuffers_[bbIndex_].Get(), bbState_, D3D12_RESOURCE_STATE_RENDER_TARGET);
                bbState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
            }
            rtv = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
            rtv.ptr += (SIZE_T)bbIndex_ * rtvInc_;
        } else if (rt->kind == Surface8::TextureLevel && rt->tex) {
            if (!rt->tex->gpu) rt->tex->gpu = CreateGpuTex(*rt->tex);
            offTex = (GpuTex*)rt->tex->gpu;
            if (offTex->state != D3D12_RESOURCE_STATE_RENDER_TARGET) {
                Transition(offTex->res.Get(), offTex->state, D3D12_RESOURCE_STATE_RENDER_TARGET);
                offTex->state = D3D12_RESOURCE_STATE_RENDER_TARGET;
            }
            rtv = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
            rtv.ptr += (SIZE_T)offTex->rtv * rtvInc_;
            dsv = OffscreenDsv(w, h);
            key = offTex;
        } else {
            return false;
        }
        if (curTargetKey_ != key || curBack_ != back) {
            curRtv_ = rtv;
            curDsv_ = dsv;
            list_->OMSetRenderTargets(1, &curRtv_, FALSE, &curDsv_);
            curTargetKey_ = key;
            curBack_ = back;
            vpValid_ = false;
            curW_ = w;
            curH_ = h;
        }
        const D3DVIEWPORT8& v = s.viewport;
        if (!vpValid_ || memcmp(&v, &curVp_, sizeof(v)) != 0) {
            D3D12_VIEWPORT vp{(float)v.X, (float)v.Y, (float)v.Width, (float)v.Height, v.MinZ, v.MaxZ};
            D3D12_RECT sc{0, 0, (LONG)curW_, (LONG)curH_};
            list_->RSSetViewports(1, &vp);
            list_->RSSetScissorRects(1, &sc);
            curVp_ = v;
            vpValid_ = true;
        }
        return true;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE OffscreenDsv(UINT w, UINT h)
    {
        const uint64_t key = ((uint64_t)w << 32) | h;
        auto it = offscreenDepth_.find(key);
        if (it == offscreenDepth_.end()) {
            OffDepth od;
            od.res = CreateDepth(w, h);
            od.slot = (UINT)offscreenDepth_.size() + 1;
            D3D12_CPU_DESCRIPTOR_HANDLE hd = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
            hd.ptr += (SIZE_T)od.slot * dsvInc_;
            dev_->CreateDepthStencilView(od.res.Get(), nullptr, hd);
            it = offscreenDepth_.emplace(key, std::move(od)).first;
        }
        D3D12_CPU_DESCRIPTOR_HANDLE hd = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
        hd.ptr += (SIZE_T)it->second.slot * dsvInc_;
        return hd;
    }

    PsoKey MakeKey(const State& s, D3DPRIMITIVETYPE type)
    {
        PsoKey k;
        memset(&k, 0, sizeof(k));
        k.fvf = s.fvf;
        k.topology = type == D3DPT_POINTLIST ? 1 : (type == D3DPT_LINELIST || type == D3DPT_LINESTRIP) ? 2 : 3;
        k.blend = s.rs[D3DRS_ALPHABLENDENABLE] != 0;
        k.src = k.blend ? (uint8_t)s.rs[D3DRS_SRCBLEND] : 0;
        k.dst = k.blend ? (uint8_t)s.rs[D3DRS_DESTBLEND] : 0;
        k.cull = (uint8_t)s.rs[D3DRS_CULLMODE];
        k.depthEnable = s.rs[D3DRS_ZENABLE] != 0;
        k.depthWrite = s.rs[D3DRS_ZWRITEENABLE] != 0;
        k.depthFunc = (uint8_t)s.rs[D3DRS_ZFUNC];
        k.stencilEnable = s.rs[D3DRS_STENCILENABLE] != 0;
        if (k.stencilEnable) {
            k.stencilFunc = (uint8_t)s.rs[D3DRS_STENCILFUNC];
            k.stencilFail = (uint8_t)s.rs[D3DRS_STENCILFAIL];
            k.stencilZFail = (uint8_t)s.rs[D3DRS_STENCILZFAIL];
            k.stencilPass = (uint8_t)s.rs[D3DRS_STENCILPASS];
            k.stencilRead = s.rs[D3DRS_STENCILMASK] & 0xff;
            k.stencilWrite = s.rs[D3DRS_STENCILWRITEMASK] & 0xff;
        }
        k.wire = s.rs[D3DRS_FILLMODE] == 2;
        k.flat = s.rs[D3DRS_SHADEMODE] == D3DSHADE_FLAT;
        k.lighting = (s.rs[D3DRS_LIGHTING] != 0) && (s.fvf & D3DFVF_NORMAL) && !(s.fvf & D3DFVF_XYZRHW);
        return k;
    }

    ID3D12PipelineState* GetPso(const PsoKey& k)
    {
        auto it = pso_.find(k);
        if (it != pso_.end()) return it->second.Get();

        const bool rhw = (k.fvf & D3DFVF_XYZRHW) != 0;
        const bool normal = (k.fvf & D3DFVF_NORMAL) != 0;
        const bool color = (k.fvf & D3DFVF_DIFFUSE) != 0;
        const UINT texCount = (k.fvf & D3DFVF_TEX2) ? 2 : ((k.fvf & D3DFVF_TEX1) ? 1 : 0);
        const D3D_SHADER_MACRO macros[] = {
            {"HAS_RHW", rhw ? "1" : "0"},
            {"HAS_NORMAL", normal ? "1" : "0"},
            {"HAS_COLOR", color ? "1" : "0"},
            {"HAS_UV0", texCount > 0 ? "1" : "0"},
            {"HAS_UV1", texCount > 1 ? "1" : "0"},
            {"LIGHTING", k.lighting ? "1" : "0"},
            {"INTERP", k.flat ? "nointerpolation" : ""},
            {nullptr, nullptr},
        };
        ComPtr<ID3DBlob> vs, ps, err;
        if (FAILED(D3DCompile(kShader, sizeof(kShader) - 1, "scene.hlsl", macros, nullptr, "VSMain", "vs_5_0",
                              D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vs, &err))) {
            if (err) Log("%s", (const char*)err->GetBufferPointer());
            return nullptr;
        }
        if (FAILED(D3DCompile(kShader, sizeof(kShader) - 1, "scene.hlsl", macros, nullptr, "PSMain", "ps_5_0",
                              D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &ps, &err))) {
            if (err) Log("%s", (const char*)err->GetBufferPointer());
            return nullptr;
        }

        std::vector<D3D12_INPUT_ELEMENT_DESC> layout;
        UINT off = 0;
        layout.push_back({"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, off, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
        off += 12;
        if (rhw) {
            layout.push_back({"RHW", 0, DXGI_FORMAT_R32_FLOAT, 0, off, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
            off += 4;
        }
        if (normal) {
            layout.push_back({"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, off, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
            off += 12;
        }
        if (color) {
            layout.push_back({"COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, off, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
            off += 4;
        }
        for (UINT i = 0; i < texCount; ++i) {
            layout.push_back({"TEXCOORD", i, DXGI_FORMAT_R32G32_FLOAT, 0, off, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
            off += 8;
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC d{};
        d.pRootSignature = rootSig_.Get();
        d.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
        d.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
        d.InputLayout = {layout.data(), (UINT)layout.size()};
        d.PrimitiveTopologyType = k.topology == 1   ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT
                                  : k.topology == 2 ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE
                                                    : D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        d.NumRenderTargets = 1;
        d.RTVFormats[0] = kColorFormat;
        d.DSVFormat = kDepthFormat;
        d.SampleMask = UINT_MAX;
        d.SampleDesc.Count = 1;
        d.RasterizerState.FillMode = k.wire ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
        d.RasterizerState.CullMode = k.cull == D3DCULL_CCW ? D3D12_CULL_MODE_BACK
                                     : k.cull == D3DCULL_CW ? D3D12_CULL_MODE_FRONT
                                                            : D3D12_CULL_MODE_NONE;
        d.RasterizerState.DepthClipEnable = TRUE;
        auto& bl = d.BlendState.RenderTarget[0];
        bl.BlendEnable = k.blend;
        bl.SrcBlend = k.blend ? (D3D12_BLEND)k.src : D3D12_BLEND_ONE;
        bl.DestBlend = k.blend ? (D3D12_BLEND)k.dst : D3D12_BLEND_ZERO;
        bl.BlendOp = D3D12_BLEND_OP_ADD;
        bl.SrcBlendAlpha = D3D12_BLEND_ONE;
        bl.DestBlendAlpha = D3D12_BLEND_ZERO;
        bl.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        bl.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        d.DepthStencilState.DepthEnable = k.depthEnable;
        d.DepthStencilState.DepthWriteMask = k.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        d.DepthStencilState.DepthFunc = k.depthFunc ? (D3D12_COMPARISON_FUNC)k.depthFunc : D3D12_COMPARISON_FUNC_LESS_EQUAL;
        d.DepthStencilState.StencilEnable = k.stencilEnable;
        d.DepthStencilState.StencilReadMask = (UINT8)k.stencilRead;
        d.DepthStencilState.StencilWriteMask = (UINT8)k.stencilWrite;
        D3D12_DEPTH_STENCILOP_DESC so{};
        so.StencilFailOp = (D3D12_STENCIL_OP)(k.stencilFail ? k.stencilFail : 1);
        so.StencilDepthFailOp = (D3D12_STENCIL_OP)(k.stencilZFail ? k.stencilZFail : 1);
        so.StencilPassOp = (D3D12_STENCIL_OP)(k.stencilPass ? k.stencilPass : 1);
        so.StencilFunc = (D3D12_COMPARISON_FUNC)(k.stencilFunc ? k.stencilFunc : 8);
        d.DepthStencilState.FrontFace = so;
        d.DepthStencilState.BackFace = so;

        ComPtr<ID3D12PipelineState> p;
        if (FAILED(dev_->CreateGraphicsPipelineState(&d, IID_PPV_ARGS(&p)))) {
            Log("CreateGraphicsPipelineState failed");
            return nullptr;
        }
        return pso_.emplace(k, std::move(p)).first->second.Get();
    }

    static void CopyMatrix(float* dst, const D3DXMATRIX& m) { memcpy(dst, &m, 64); }

    void FillScene(const State& s, SceneC* c)
    {
        memset(c, 0, sizeof(*c));
        D3DXMATRIX wv = s.world * s.view;
        D3DXMATRIX wvp = wv * s.proj;
        CopyMatrix(c->mvp, wvp);
        CopyMatrix(c->world, s.world);
        CopyMatrix(c->worldView, wv);
        CopyMatrix(c->texMat[0], s.texMat[0]);
        CopyMatrix(c->texMat[1], s.texMat[1]);
        auto color = [](const D3DCOLORVALUE& v, float* o) { o[0] = v.r; o[1] = v.g; o[2] = v.b; o[3] = v.a; };
        color(s.material.Diffuse, c->matDiffuse);
        color(s.material.Ambient, c->matAmbient);
        color(s.material.Emissive, c->matEmissive);
        color(s.material.Specular, c->matSpecular);
        c->matSpecular[3] = s.material.Power;
        auto argb = [](DWORD v, float* o) {
            o[0] = ((v >> 16) & 0xff) / 255.0f; o[1] = ((v >> 8) & 0xff) / 255.0f;
            o[2] = (v & 0xff) / 255.0f; o[3] = ((v >> 24) & 0xff) / 255.0f;
        };
        argb(s.rs[D3DRS_AMBIENT], c->globalAmbient);
        argb(s.rs[D3DRS_FOGCOLOR], c->fogColor);
        float fs, fe, fd = 1.0f;
        memcpy(&fs, &s.rs[D3DRS_FOGSTART], 4);
        memcpy(&fe, &s.rs[D3DRS_FOGEND], 4);
        c->fogParams[0] = fs;
        c->fogParams[1] = fe;
        c->fogParams[2] = fd;
        c->fogParams[3] = (s.rs[D3DRS_ALPHAREF] & 0xff) / 255.0f;
        D3DXMATRIX inv;
        D3DXMatrixInverse(&inv, nullptr, &s.view);
        c->eye[0] = inv._41; c->eye[1] = inv._42; c->eye[2] = inv._43;
        c->viewport[0] = (float)s.viewport.Width;
        c->viewport[1] = (float)s.viewport.Height;
        c->flags[0] = s.rs[D3DRS_LIGHTING] != 0;
        c->flags[1] = s.rs[D3DRS_SPECULARENABLE] != 0;
        c->flags[2] = s.rs[D3DRS_DIFFUSEMATERIALSOURCE] == D3DMCS_COLOR1 && (s.fvf & D3DFVF_DIFFUSE);
        c->flags[3] = s.rs[D3DRS_AMBIENTMATERIALSOURCE] == D3DMCS_COLOR1 && (s.fvf & D3DFVF_DIFFUSE);
        DWORD fogMode = 0;
        if (s.rs[D3DRS_FOGENABLE] && !(s.fvf & D3DFVF_XYZRHW))
            fogMode = s.rs[D3DRS_FOGTABLEMODE] ? s.rs[D3DRS_FOGTABLEMODE] : s.rs[D3DRS_FOGVERTEXMODE];
        c->flags2[0] = fogMode;
        c->flags2[1] = s.rs[D3DRS_ALPHATESTENABLE] != 0;
        DWORD mask = 0;
        for (int i = 0; i < 8; ++i) {
            if (!s.lightOn[i] || s.lights[i].Type != D3DLIGHT_DIRECTIONAL) continue;
            mask |= 1u << i;
            const D3DLIGHT8& l = s.lights[i];
            float len = sqrtf(l.Direction.x * l.Direction.x + l.Direction.y * l.Direction.y + l.Direction.z * l.Direction.z);
            if (len < 1e-6f) len = 1.0f;
            c->lights[i].dir[0] = l.Direction.x / len;
            c->lights[i].dir[1] = l.Direction.y / len;
            c->lights[i].dir[2] = l.Direction.z / len;
            color(l.Diffuse, c->lights[i].diffuse);
            color(l.Ambient, c->lights[i].ambient);
            color(l.Specular, c->lights[i].specular);
        }
        c->flags2[2] = mask;
        c->flags2[3] = s.rs[D3DRS_ALPHAFUNC];
        for (int i = 0; i < 2; ++i) {
            StageC& st = c->stage[i];
            st.colorOp = s.tss[i][D3DTSS_COLOROP];
            st.colorArg1 = s.tss[i][D3DTSS_COLORARG1];
            st.colorArg2 = s.tss[i][D3DTSS_COLORARG2];
            st.alphaOp = s.tss[i][D3DTSS_ALPHAOP];
            st.alphaArg1 = s.tss[i][D3DTSS_ALPHAARG1];
            st.alphaArg2 = s.tss[i][D3DTSS_ALPHAARG2];
            st.coordIndex = s.tss[i][D3DTSS_TEXCOORDINDEX];
            DWORD addr = s.tss[i][D3DTSS_ADDRESSU];
            DWORD sampler = addr == D3DTADDRESS_CLAMP || addr == D3DTADDRESS_BORDER ? 1 : (addr == D3DTADDRESS_MIRROR ? 2 : 0);
            st.flags = (s.tex[i] ? 1u : 0u) | (s.tss[i][D3DTSS_TEXTURETRANSFORMFLAGS] ? 2u : 0u) | (sampler << 8);
        }
    }

    ComPtr<IDXGIFactory4> factory_;
    ComPtr<ID3D12Device> dev_;
    ComPtr<ID3D12CommandQueue> queue_;
    ComPtr<IDXGISwapChain3> swap_;
    ComPtr<ID3D12Fence> fence_;
    HANDLE event_ = nullptr;
    UINT64 fenceValue_ = 0;
    ComPtr<ID3D12GraphicsCommandList> list_;
    Frame frames_[kFrames];
    UINT frameIndex_ = 0;
    UINT64 frameCounter_ = 0;
    bool frameOpen_ = false;
    ComPtr<ID3D12RootSignature> rootSig_;
    ComPtr<ID3D12DescriptorHeap> rtvHeap_, dsvHeap_, srvCpu_, srvGpu_;
    UINT rtvInc_ = 0, dsvInc_ = 0, srvInc_ = 0;
    std::vector<UINT> freeSrv_, freeRtv_;
    ComPtr<ID3D12Resource> backBuffers_[kFrames];
    ComPtr<ID3D12Resource> mainDepth_;
    struct OffDepth {
        ComPtr<ID3D12Resource> res;
        UINT slot = 0;
    };
    std::unordered_map<uint64_t, OffDepth> offscreenDepth_;
    UINT bbIndex_ = 0;
    D3D12_RESOURCE_STATES bbState_ = D3D12_RESOURCE_STATE_PRESENT;
    UINT width_ = 0, height_ = 0;
    HWND window_ = nullptr;

    GpuTex* white_ = nullptr;
    std::unordered_map<uint64_t, CachedBuffer> cache_;
    std::vector<Retired> retired_;
    std::unordered_map<PsoKey, ComPtr<ID3D12PipelineState>, PsoHash> pso_;

    ID3D12PipelineState* curPso_ = nullptr;
    const void* curTargetKey_ = nullptr;
    bool curBack_ = false;
    D3D12_CPU_DESCRIPTOR_HANDLE curRtv_{}, curDsv_{};
    UINT curW_ = 0, curH_ = 0;
    D3DVIEWPORT8 curVp_{};
    bool vpValid_ = false;
    SceneC lastScene_{};
    bool hasLastScene_ = false;
    GpuTex* lastTex_[2] = {nullptr, nullptr};
    UINT64 lastHeapFrame_ = 0;
};

} // namespace

Backend* CreateD3D12Backend() { return new D3D12Backend(); }

} // namespace gfx
