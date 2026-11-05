#include "d3d8_objects.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace gfx {

Backend* g_backend = nullptr;

uint64_t NextId()
{
    static LONG64 counter = 0;
    return (uint64_t)InterlockedIncrement64(&counter);
}

void NotifyGeometryDestroyed(uint64_t id)
{
    if (g_backend) g_backend->OnGeometryDestroyed(id);
}

Surface8::~Surface8()
{
    if (tex) tex->Release();
}

uint32_t* Surface8::Bits()
{
    if (kind == TextureLevel && tex && level < tex->levels.size()) return tex->levels[level].data();
    return pixels.data();
}

HRESULT Surface8::GetDesc(D3DSURFACE_DESC* d)
{
    d->Format = format;
    d->Type = D3DRTYPE_SURFACE;
    d->Usage = kind == DepthStencil ? D3DUSAGE_DEPTHSTENCIL : (kind == BackBuffer ? D3DUSAGE_RENDERTARGET : 0);
    d->Pool = D3DPOOL_DEFAULT;
    d->Size = width * height * 4;
    d->MultiSampleType = D3DMULTISAMPLE_NONE;
    d->Width = width;
    d->Height = height;
    return S_OK;
}

HRESULT Surface8::LockRect(D3DLOCKED_RECT* locked, const RECT* rect, DWORD flags)
{
    if (kind == DepthStencil) return D3DERR_INVALIDCALL;
    if (kind == BackBuffer || (kind == TextureLevel && tex && tex->renderTarget())) {
        std::vector<uint32_t>& buf = kind == BackBuffer ? pixels : tex->readback;
        buf.resize((size_t)width * height);
        if (g_backend) g_backend->ReadSurface(this, buf.data());
        locked->Pitch = width * 4;
        locked->pBits = buf.data() + (rect ? (size_t)rect->top * width + rect->left : 0);
        return S_OK;
    }
    if (kind == TextureLevel && tex) {
        if (!(flags & D3DLOCK_READONLY)) tex->version++;
        const UINT w = tex->LevelWidth(level);
        locked->Pitch = w * 4;
        locked->pBits = tex->levels[level].data() + (rect ? (size_t)rect->top * w + rect->left : 0);
        return S_OK;
    }
    locked->Pitch = width * 4;
    locked->pBits = pixels.data() + (rect ? (size_t)rect->top * width + rect->left : 0);
    return S_OK;
}

HRESULT Surface8::UnlockRect() { return S_OK; }

Tex8::~Tex8()
{
    if (g_backend) g_backend->OnTextureDestroyed(this);
}

HRESULT Tex8::GetLevelDesc(UINT level, D3DSURFACE_DESC* d)
{
    if (level >= levels.size()) return D3DERR_INVALIDCALL;
    d->Format = format;
    d->Type = D3DRTYPE_SURFACE;
    d->Usage = usage;
    d->Pool = D3DPOOL_MANAGED;
    d->Size = LevelWidth(level) * LevelHeight(level) * 4;
    d->MultiSampleType = D3DMULTISAMPLE_NONE;
    d->Width = LevelWidth(level);
    d->Height = LevelHeight(level);
    return S_OK;
}

HRESULT Tex8::GetSurfaceLevel(UINT level, IDirect3DSurface8** out)
{
    if (level >= levels.size() || !out) return D3DERR_INVALIDCALL;
    auto* s = new Surface8();
    s->kind = Surface8::TextureLevel;
    s->width = LevelWidth(level);
    s->height = LevelHeight(level);
    s->format = format;
    s->tex = this;
    s->level = level;
    AddRef();
    *out = s;
    return S_OK;
}

HRESULT Tex8::LockRect(UINT level, D3DLOCKED_RECT* locked, const RECT* rect, DWORD flags)
{
    if (level >= levels.size()) return D3DERR_INVALIDCALL;
    if (renderTarget()) {
        IDirect3DSurface8* s = nullptr;
        GetSurfaceLevel(level, &s);
        const HRESULT hr = s->LockRect(locked, rect, flags);
        s->Release(); // pixels live in the temporary surface; see Surface8::LockRect
        return hr;
    }
    if (!(flags & D3DLOCK_READONLY)) version++;
    const UINT w = LevelWidth(level);
    locked->Pitch = w * 4;
    locked->pBits = levels[level].data() + (rect ? (size_t)rect->top * w + rect->left : 0);
    return S_OK;
}

HRESULT Tex8::UnlockRect(UINT) { return S_OK; }

namespace {

D3DXMATRIX Identity() { return D3DXMATRIX(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1); }

UINT VertexCount(D3DPRIMITIVETYPE t, UINT n)
{
    switch (t) {
    case D3DPT_POINTLIST: return n;
    case D3DPT_LINELIST: return n * 2;
    case D3DPT_LINESTRIP: return n + 1;
    case D3DPT_TRIANGLELIST: return n * 3;
    default: return n + 2;
    }
}

UINT FvfStride(DWORD fvf)
{
    UINT s = 0;
    if (fvf & D3DFVF_XYZ) s += 12;
    if (fvf & D3DFVF_XYZRHW) s += 16;
    if (fvf & D3DFVF_NORMAL) s += 12;
    if (fvf & D3DFVF_DIFFUSE) s += 4;
    if (fvf & D3DFVF_TEX1) s += 8;
    if (fvf & D3DFVF_TEX2) s += 16;
    return s;
}

class Device8 : public RefCounted<IDirect3DDevice8> {
public:
    D3DPRESENT_PARAMETERS pp{};
    HWND window = nullptr;
    State st;
    Surface8* backBuffer = nullptr;
    Surface8* depthBuffer = nullptr;
    VB8* stream = nullptr;
    UINT streamStride = 0;
    bool inScene = false;

    ~Device8() override
    {
        if (stream) stream->Release();
        if (st.renderTarget) st.renderTarget->Release();
        if (st.depthStencil) st.depthStencil->Release();
        if (backBuffer) backBuffer->Release();
        if (depthBuffer) depthBuffer->Release();
        for (auto*& t : st.tex)
            if (t) t->Release();
        delete g_backend;
        g_backend = nullptr;
    }

    void MakeBuffers()
    {
        if (st.renderTarget) st.renderTarget->Release();
        if (st.depthStencil) st.depthStencil->Release();
        if (backBuffer) backBuffer->Release();
        if (depthBuffer) depthBuffer->Release();
        backBuffer = new Surface8();
        backBuffer->kind = Surface8::BackBuffer;
        backBuffer->width = pp.BackBufferWidth;
        backBuffer->height = pp.BackBufferHeight;
        depthBuffer = new Surface8();
        depthBuffer->kind = Surface8::DepthStencil;
        depthBuffer->width = pp.BackBufferWidth;
        depthBuffer->height = pp.BackBufferHeight;
        depthBuffer->format = pp.AutoDepthStencilFormat;
        backBuffer->AddRef();
        depthBuffer->AddRef();
        st.renderTarget = backBuffer;
        st.depthStencil = depthBuffer;
        st.viewport = {0, 0, pp.BackBufferWidth, pp.BackBufferHeight, 0.0f, 1.0f};
    }

    void InitState()
    {
        st.world = Identity();
        st.view = Identity();
        st.proj = Identity();
        for (auto& m : st.texMat) m = Identity();
        st.rs[D3DRS_ZENABLE] = 1;
        st.rs[D3DRS_ZWRITEENABLE] = 1;
        st.rs[D3DRS_SHADEMODE] = D3DSHADE_GOURAUD;
        st.rs[D3DRS_SRCBLEND] = D3DBLEND_ONE;
        st.rs[D3DRS_DESTBLEND] = D3DBLEND_ZERO;
        st.rs[D3DRS_CULLMODE] = D3DCULL_CCW;
        st.rs[D3DRS_ZFUNC] = D3DCMP_LESSEQUAL;
        st.rs[D3DRS_ALPHAFUNC] = D3DCMP_ALWAYS;
        st.rs[D3DRS_LIGHTING] = 1;
        st.rs[D3DRS_FOGTABLEMODE] = D3DFOG_NONE;
        st.rs[D3DRS_FOGVERTEXMODE] = D3DFOG_NONE;
        st.rs[D3DRS_FOGSTART] = 0;
        float one = 1.0f;
        memcpy(&st.rs[D3DRS_FOGEND], &one, 4);
        st.rs[D3DRS_STENCILFUNC] = D3DCMP_ALWAYS;
        st.rs[D3DRS_STENCILFAIL] = st.rs[D3DRS_STENCILZFAIL] = st.rs[D3DRS_STENCILPASS] = D3DSTENCILOP_KEEP;
        st.rs[D3DRS_STENCILMASK] = st.rs[D3DRS_STENCILWRITEMASK] = 0xffffffff;
        st.rs[D3DRS_DIFFUSEMATERIALSOURCE] = D3DMCS_COLOR1;
        st.rs[D3DRS_AMBIENTMATERIALSOURCE] = D3DMCS_MATERIAL;
        for (int i = 0; i < 4; ++i) {
            st.tss[i][D3DTSS_COLOROP] = i == 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE;
            st.tss[i][D3DTSS_COLORARG1] = D3DTA_TEXTURE;
            st.tss[i][D3DTSS_COLORARG2] = D3DTA_CURRENT;
            st.tss[i][D3DTSS_ALPHAOP] = i == 0 ? D3DTOP_SELECTARG1 : D3DTOP_DISABLE;
            st.tss[i][D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
            st.tss[i][D3DTSS_ALPHAARG2] = D3DTA_CURRENT;
            st.tss[i][D3DTSS_TEXCOORDINDEX] = i;
            st.tss[i][D3DTSS_ADDRESSU] = st.tss[i][D3DTSS_ADDRESSV] = D3DTADDRESS_WRAP;
            st.tss[i][D3DTSS_MAGFILTER] = st.tss[i][D3DTSS_MINFILTER] = D3DTEXF_POINT;
            st.tss[i][D3DTSS_MIPFILTER] = D3DTEXF_NONE;
        }
    }

    // IDirect3DDevice8
    HRESULT STDMETHODCALLTYPE TestCooperativeLevel() override { return D3D_OK; }
    UINT STDMETHODCALLTYPE GetAvailableTextureMem() override { return 1024u * 1024u * 1024u; }
    HRESULT STDMETHODCALLTYPE GetDeviceCaps(D3DCAPS8* c) override
    {
        memset(c, 0, sizeof(*c));
        c->RasterCaps = D3DPRASTERCAPS_FOGVERTEX | D3DPRASTERCAPS_FOGTABLE | D3DPRASTERCAPS_FOGRANGE;
        c->TextureCaps = D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_MIPMAP;
        c->TextureOpCaps = D3DTEXOPCAPS_BUMPENVMAP;
        c->MaxTextureWidth = c->MaxTextureHeight = 16384;
        c->MaxSimultaneousTextures = 2;
        c->MaxPrimitiveCount = 0xffff;
        c->MaxActiveLights = 8;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE Reset(D3DPRESENT_PARAMETERS* p) override
    {
        pp = *p;
        if (!pp.hDeviceWindow) pp.hDeviceWindow = window;
        MakeBuffers();
        return g_backend->Resize(pp) ? D3D_OK : D3DERR_INVALIDCALL;
    }
    HRESULT STDMETHODCALLTYPE Present(const RECT*, const RECT*, HWND, const RGNDATA*) override
    {
        g_backend->Present();
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE CreateTexture(UINT w, UINT h, UINT levels, DWORD usage, D3DFORMAT fmt, D3DPOOL,
                                            IDirect3DTexture8** out) override
    {
        if (!w || !h || !out) return D3DERR_INVALIDCALL;
        UINT maxLevels = 1;
        for (UINT m = std::max(w, h); m > 1; m >>= 1) ++maxLevels;
        if (levels == 0 || levels > maxLevels) levels = maxLevels;
        auto* t = new Tex8();
        t->width = w;
        t->height = h;
        t->format = fmt;
        t->usage = usage;
        t->levels.resize(levels);
        for (UINT l = 0; l < levels; ++l) t->levels[l].assign((size_t)t->LevelWidth(l) * t->LevelHeight(l), 0);
        *out = t;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE CreateVertexBuffer(UINT len, DWORD, DWORD fvf, D3DPOOL,
                                                 IDirect3DVertexBuffer8** out) override
    {
        auto* v = new VB8();
        v->data.assign(len, 0);
        v->fvf = fvf;
        *out = v;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE CreateImageSurface(UINT w, UINT h, D3DFORMAT fmt, IDirect3DSurface8** out) override
    {
        auto* s = new Surface8();
        s->width = w;
        s->height = h;
        s->format = fmt;
        s->pixels.assign((size_t)w * h, 0);
        *out = s;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE CreateDepthStencilSurface(UINT w, UINT h, D3DFORMAT fmt, D3DMULTISAMPLE_TYPE,
                                                        IDirect3DSurface8** out) override
    {
        auto* s = new Surface8();
        s->kind = Surface8::DepthStencil;
        s->width = w;
        s->height = h;
        s->format = fmt;
        *out = s;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE CopyRects(IDirect3DSurface8* src, const RECT* rects, UINT count, IDirect3DSurface8* dst,
                                        const POINT* points) override
    {
        auto* s = static_cast<Surface8*>(src);
        auto* d = static_cast<Surface8*>(dst);
        std::vector<uint32_t> readback;
        const uint32_t* sp;
        if (s->kind == Surface8::BackBuffer || (s->kind == Surface8::TextureLevel && s->tex && s->tex->renderTarget())) {
            readback.resize((size_t)s->width * s->height);
            g_backend->ReadSurface(s, readback.data());
            sp = readback.data();
        } else {
            sp = s->Bits();
        }
        uint32_t* dp = d->Bits();
        const UINT dw = d->kind == Surface8::TextureLevel ? d->tex->LevelWidth(d->level) : d->width;
        RECT whole = {0, 0, (LONG)s->width, (LONG)s->height};
        const UINT n = rects ? count : 1;
        for (UINT i = 0; i < n; ++i) {
            const RECT r = rects ? rects[i] : whole;
            const POINT p = points ? points[i] : POINT{r.left, r.top};
            for (LONG y = r.top; y < r.bottom; ++y) {
                if (p.y + (y - r.top) >= (LONG)d->height) break;
                const LONG w = std::min<LONG>(r.right - r.left, (LONG)dw - p.x);
                if (w > 0) memcpy(dp + (size_t)(p.y + y - r.top) * dw + p.x, sp + (size_t)y * s->width + r.left, w * 4);
            }
        }
        if (d->kind == Surface8::TextureLevel && d->tex) d->tex->version++;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE SetRenderTarget(IDirect3DSurface8* target, IDirect3DSurface8* depth) override
    {
        if (target) {
            target->AddRef();
            if (st.renderTarget) st.renderTarget->Release();
            st.renderTarget = static_cast<Surface8*>(target);
        }
        if (depth) {
            depth->AddRef();
            if (st.depthStencil) st.depthStencil->Release();
            st.depthStencil = static_cast<Surface8*>(depth);
        }
        if (st.renderTarget) st.viewport = {0, 0, st.renderTarget->width, st.renderTarget->height, 0.0f, 1.0f};
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE GetRenderTarget(IDirect3DSurface8** out) override
    {
        st.renderTarget->AddRef();
        *out = st.renderTarget;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE GetDepthStencilSurface(IDirect3DSurface8** out) override
    {
        st.depthStencil->AddRef();
        *out = st.depthStencil;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE BeginScene() override
    {
        inScene = true;
        g_backend->BeginScene();
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE EndScene() override
    {
        inScene = false;
        g_backend->EndScene();
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE Clear(DWORD, const RECT*, DWORD flags, D3DCOLOR color, float z, DWORD stencil) override
    {
        g_backend->Clear(st, flags, color, z, stencil);
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE SetTransform(D3DTRANSFORMSTATETYPE type, const D3DXMATRIX* m) override
    {
        switch (type) {
        case D3DTS_VIEW: st.view = *m; break;
        case D3DTS_PROJECTION: st.proj = *m; break;
        
        case D3DTS_WORLD: st.world = *m; break;
        default:
            if ((int)type >= D3DTS_TEXTURE0 && (int)type < D3DTS_TEXTURE0 + 4) st.texMat[(int)type - D3DTS_TEXTURE0] = *m;
            break;
        }
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE SetViewport(const D3DVIEWPORT8* v) override
    {
        st.viewport = *v;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE GetViewport(D3DVIEWPORT8* v) override
    {
        *v = st.viewport;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE SetMaterial(const D3DMATERIAL8* m) override
    {
        st.material = *m;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE SetLight(DWORD i, const D3DLIGHT8* l) override
    {
        if (i < 8) st.lights[i] = *l;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE LightEnable(DWORD i, BOOL e) override
    {
        if (i < 8) st.lightOn[i] = e != FALSE;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE SetRenderState(D3DRENDERSTATETYPE s, DWORD v) override
    {
        if ((UINT)s < 256) st.rs[s] = v;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE GetRenderState(D3DRENDERSTATETYPE s, DWORD* v) override
    {
        *v = (UINT)s < 256 ? st.rs[s] : 0;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE SetClipStatus(const D3DCLIPSTATUS8*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetTexture(DWORD stage, IDirect3DBaseTexture8* t) override
    {
        if (stage >= 4) return D3DERR_INVALIDCALL;
        auto* n = static_cast<Tex8*>(static_cast<IDirect3DTexture8*>(t));
        if (n) n->AddRef();
        if (st.tex[stage]) st.tex[stage]->Release();
        st.tex[stage] = n;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE GetTexture(DWORD stage, IDirect3DBaseTexture8** out) override
    {
        Tex8* t = stage < 4 ? st.tex[stage] : nullptr;
        if (t) t->AddRef();
        *out = t;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD v) override
    {
        if (stage < 4 && (UINT)type < 32) st.tss[stage][type] = v;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE SetVertexShader(DWORD fvf) override
    {
        st.fvf = fvf;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE SetStreamSource(UINT, IDirect3DVertexBuffer8* d, UINT stride) override
    {
        auto* v = static_cast<VB8*>(d);
        if (v) v->AddRef();
        if (stream) stream->Release();
        stream = v;
        streamStride = stride;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE DrawPrimitive(D3DPRIMITIVETYPE type, UINT start, UINT count) override
    {
        if (!stream) return D3DERR_INVALIDCALL;
        Geometry g;
        g.vertices = stream->data.data() + (size_t)start * streamStride;
        g.vertexCount = VertexCount(type, count);
        g.stride = streamStride;
        g.vbId = stream->id;
        g.vbVersion = stream->version;
        g.vbBase = stream->data.data();
        g.vbBytes = stream->data.size();
        g_backend->Draw(st, type, g);
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE DrawPrimitiveUP(D3DPRIMITIVETYPE type, UINT count, const void* data, UINT stride) override
    {
        Geometry g;
        g.vertices = data;
        g.vertexCount = VertexCount(type, count);
        g.stride = stride;
        g_backend->Draw(st, type, g);
        return D3D_OK;
    }
};

class Direct3D8 : public RefCounted<IDirect3D8> {
public:
    UINT STDMETHODCALLTYPE GetAdapterCount() override { return 1; }
    HRESULT STDMETHODCALLTYPE GetAdapterIdentifier(UINT, DWORD, D3DADAPTER_IDENTIFIER8* id) override
    {
        memset(id, 0, sizeof(*id));
        strcpy_s(id->Driver, "d3d12");
        strcpy_s(id->Description, "Direct3D 12 backend");
        return D3D_OK;
    }
    UINT STDMETHODCALLTYPE GetAdapterModeCount(UINT) override
    {
        return (UINT)Modes().size();
    }
    HRESULT STDMETHODCALLTYPE EnumAdapterModes(UINT, UINT i, D3DDISPLAYMODE* out) override
    {
        auto m = Modes();
        if (i >= m.size()) return D3DERR_INVALIDCALL;
        *out = m[i];
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE GetAdapterDisplayMode(UINT, D3DDISPLAYMODE* out) override
    {
        out->Width = GetSystemMetrics(SM_CXSCREEN);
        out->Height = GetSystemMetrics(SM_CYSCREEN);
        out->RefreshRate = 60;
        out->Format = D3DFMT_X8R8G8B8;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE CheckDeviceFormat(UINT, D3DDEVTYPE, D3DFORMAT, DWORD, D3DRESOURCETYPE, D3DFORMAT) override
    {
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE CheckDepthStencilMatch(UINT, D3DDEVTYPE, D3DFORMAT, D3DFORMAT, D3DFORMAT) override
    {
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE CreateDevice(UINT, D3DDEVTYPE, HWND window, DWORD, D3DPRESENT_PARAMETERS* p,
                                           IDirect3DDevice8** out) override
    {
        auto* d = new Device8();
        d->window = window;
        d->pp = *p;
        if (!d->pp.hDeviceWindow) d->pp.hDeviceWindow = window;
        if (!d->pp.BackBufferWidth || !d->pp.BackBufferHeight) {
            RECT rc;
            GetClientRect(window, &rc);
            d->pp.BackBufferWidth = rc.right - rc.left;
            d->pp.BackBufferHeight = rc.bottom - rc.top;
        }
        *p = d->pp;
        if (!d->pp.Windowed) {
            // Fullscreen is emulated with a borderless window.
            SetWindowLongA(window, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowPos(window, HWND_TOP, 0, 0, (int)d->pp.BackBufferWidth, (int)d->pp.BackBufferHeight,
                         SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            UpdateWindow(window);
        }
        delete g_backend;
        g_backend = getenv("RS2_NULL_BACKEND") ? CreateNullBackend() : CreateD3D12Backend();
        if (!g_backend->Init(window, d->pp)) {
            d->Release();
            return D3DERR_NOTAVAILABLE;
        }
        d->InitState();
        d->MakeBuffers();
        *out = d;
        return D3D_OK;
    }

private:
    static std::vector<D3DDISPLAYMODE> Modes()
    {
        std::vector<D3DDISPLAYMODE> v;
        DEVMODEA dm{};
        dm.dmSize = sizeof(dm);
        for (DWORD i = 0; EnumDisplaySettingsA(nullptr, i, &dm); ++i) {
            if (dm.dmBitsPerPel != 32) continue;
            D3DDISPLAYMODE m{dm.dmPelsWidth, dm.dmPelsHeight, dm.dmDisplayFrequency, D3DFMT_X8R8G8B8};
            bool dup = false;
            for (auto& e : v) dup |= e.Width == m.Width && e.Height == m.Height && e.RefreshRate == m.RefreshRate;
            if (!dup) v.push_back(m);
        }
        return v;
    }
};

} // namespace

HRESULT DrawIndexed(IDirect3DDevice8* dev, DWORD fvf, const Geometry& g)
{
    auto* d = static_cast<Device8*>(dev);
    const DWORD saved = d->st.fvf;
    d->st.fvf = fvf;
    g_backend->Draw(d->st, D3DPT_TRIANGLELIST, g);
    d->st.fvf = saved;
    return D3D_OK;
}

UINT FvfVertexSize(DWORD fvf) { return FvfStride(fvf); }

IDirect3D8* CreateD3D8() { return new Direct3D8(); }

} // namespace gfx

IDirect3D8* Direct3DCreate8(UINT) { return gfx::CreateD3D8(); }
