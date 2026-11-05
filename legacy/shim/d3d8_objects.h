// CPU-side implementation objects behind the D3D8 shim. The graphics backend (Direct3D 12) only sees these.
#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include "d3d8.h"
#include "d3dx8.h"

namespace gfx {

template <class I>
struct RefCounted : I {
    LONG refs = 1;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** out) override
    {
        if (out) *out = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&refs); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const LONG n = InterlockedDecrement(&refs);
        if (n == 0) delete this;
        return n;
    }
    virtual ~RefCounted() = default;
};

struct Tex8;

// Surfaces are all 32-bit BGRA (A8R8G8B8) in system memory.
struct Surface8 : RefCounted<IDirect3DSurface8> {
    enum Kind { Image, BackBuffer, DepthStencil, TextureLevel } kind = Image;
    UINT width = 0, height = 0;
    D3DFORMAT format = D3DFMT_A8R8G8B8;
    Tex8* tex = nullptr; // for TextureLevel (holds a reference)
    UINT level = 0;
    std::vector<uint32_t> pixels; // Image only

    ~Surface8() override;
    uint32_t* Bits();
    HRESULT STDMETHODCALLTYPE GetDesc(D3DSURFACE_DESC* desc) override;
    HRESULT STDMETHODCALLTYPE LockRect(D3DLOCKED_RECT* locked, const RECT* rect, DWORD flags) override;
    HRESULT STDMETHODCALLTYPE UnlockRect() override;
};

struct Tex8 : RefCounted<IDirect3DTexture8> {
    UINT width = 0, height = 0;
    D3DFORMAT format = D3DFMT_A8R8G8B8;
    DWORD usage = 0;
    std::vector<std::vector<uint32_t>> levels;
    uint32_t version = 1;  // bumped whenever CPU contents change
    void* gpu = nullptr;   // owned by the backend
    std::vector<uint32_t> readback; // render-target contents fetched on demand
    bool renderTarget() const { return (usage & D3DUSAGE_RENDERTARGET) != 0; }

    ~Tex8() override;
    UINT LevelWidth(UINT l) const { return width >> l ? width >> l : 1; }
    UINT LevelHeight(UINT l) const { return height >> l ? height >> l : 1; }
    DWORD STDMETHODCALLTYPE GetLevelCount() override { return (DWORD)levels.size(); }
    HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT level, D3DSURFACE_DESC* desc) override;
    HRESULT STDMETHODCALLTYPE GetSurfaceLevel(UINT level, IDirect3DSurface8** surface) override;
    HRESULT STDMETHODCALLTYPE LockRect(UINT level, D3DLOCKED_RECT* locked, const RECT* rect, DWORD flags) override;
    HRESULT STDMETHODCALLTYPE UnlockRect(UINT level) override;
};

struct VB8 : RefCounted<IDirect3DVertexBuffer8> {
    std::vector<uint8_t> data;
    DWORD fvf = 0;
    HRESULT STDMETHODCALLTYPE Lock(UINT offset, UINT size, BYTE** out, DWORD) override
    {
        if (!out) return D3DERR_INVALIDCALL;
        *out = data.data() + offset;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Unlock() override { return S_OK; }
};

struct IB8 : RefCounted<IDirect3DIndexBuffer8> {
    std::vector<uint8_t> data;
    HRESULT STDMETHODCALLTYPE Lock(UINT offset, UINT size, BYTE** out, DWORD) override
    {
        if (!out) return D3DERR_INVALIDCALL;
        *out = data.data() + offset;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Unlock() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE GetDesc(D3DINDEXBUFFER_DESC* d) override
    {
        d->Format = D3DFMT_UNKNOWN;
        d->Type = D3DRTYPE_INDEXBUFFER;
        d->Usage = 0;
        d->Pool = D3DPOOL_SYSTEMMEM;
        d->Size = (UINT)data.size();
        return S_OK;
    }
};

// Snapshot of the fixed-function pipeline handed to the backend with each draw.
struct State {
    D3DXMATRIX world, view, proj, tex0;
    D3DVIEWPORT8 viewport{};
    D3DMATERIAL8 material{};
    D3DLIGHT8 lights[8]{};
    bool lightOn[8]{};
    DWORD rs[256]{};
    DWORD tss[4][32]{};
    Tex8* tex[4]{};
    DWORD fvf = 0;
    Surface8* renderTarget = nullptr;
    Surface8* depthStencil = nullptr;
};

class Backend {
public:
    virtual ~Backend() = default;
    virtual bool Init(HWND window, const D3DPRESENT_PARAMETERS& params) = 0;
    virtual bool Resize(const D3DPRESENT_PARAMETERS& params) = 0;
    virtual void BeginScene() = 0;
    virtual void EndScene() = 0;
    virtual void Clear(const State& s, DWORD flags, D3DCOLOR color, float z, DWORD stencil) = 0;
    // `vertices` points at vertexCount vertices of `stride` bytes; indices is null for non-indexed draws.
    virtual void Draw(const State& s, D3DPRIMITIVETYPE type, const void* vertices, UINT vertexCount, UINT stride,
                      const uint16_t* indices, UINT indexCount) = 0;
    virtual void Present() = 0;
    virtual void ReadSurface(Surface8* surface, uint32_t* out) = 0; // render target / backbuffer readback
    virtual void OnTextureDestroyed(Tex8* tex) = 0;
};

extern Backend* g_backend;
HRESULT DrawIndexed(IDirect3DDevice8* dev, DWORD fvf, const void* verts, UINT vertexCount, const uint16_t* indices,
                    UINT indexCount);
UINT FvfVertexSize(DWORD fvf);
IDirect3D8* CreateD3D8();
Backend* CreateNullBackend();
Backend* CreateD3D12Backend();

} // namespace gfx
