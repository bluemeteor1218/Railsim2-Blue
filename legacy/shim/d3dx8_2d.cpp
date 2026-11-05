// ID3DXSprite / ID3DXFont on top of the shim device (screen-space XYZRHW quads).
#include <algorithm>
#include <cmath>
#include <list>
#include <string>
#include <unordered_map>

#include "d3d8_objects.h"

using namespace gfx;

namespace {

struct RhwVtx {
    float x, y, z, rhw;
    DWORD color;
    float u, v;
};
constexpr DWORD kRhwFvf = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;

struct StateGuard {
    IDirect3DDevice8* dev;
    DWORD rs[6];
    DWORD fvf;
    explicit StateGuard(IDirect3DDevice8* d) : dev(d)
    {
        static const D3DRENDERSTATETYPE ids[6] = {D3DRS_ALPHABLENDENABLE, D3DRS_SRCBLEND, D3DRS_DESTBLEND,
                                                  D3DRS_ZENABLE, D3DRS_LIGHTING, D3DRS_CULLMODE};
        for (int i = 0; i < 6; ++i) dev->GetRenderState(ids[i], &rs[i]);
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        dev->SetRenderState(D3DRS_ZENABLE, FALSE);
        dev->SetRenderState(D3DRS_LIGHTING, FALSE);
        dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    }
    ~StateGuard()
    {
        static const D3DRENDERSTATETYPE ids[6] = {D3DRS_ALPHABLENDENABLE, D3DRS_SRCBLEND, D3DRS_DESTBLEND,
                                                  D3DRS_ZENABLE, D3DRS_LIGHTING, D3DRS_CULLMODE};
        for (int i = 0; i < 6; ++i) dev->SetRenderState(ids[i], rs[i]);
    }
};

void DrawQuad(IDirect3DDevice8* dev, IDirect3DTexture8* tex, const float p[4][2], const float uv[4][2], D3DCOLOR c)
{
    RhwVtx v[4];
    for (int i = 0; i < 4; ++i) v[i] = {p[i][0], p[i][1], 0.0f, 1.0f, c, uv[i][0], uv[i][1]};
    const RhwVtx tris[6] = {v[0], v[1], v[2], v[0], v[2], v[3]};
    dev->SetTexture(0, tex);
    dev->SetVertexShader(kRhwFvf);
    dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tris, sizeof(RhwVtx));
}

struct Sprite8 : RefCounted<ID3DXSprite> {
    IDirect3DDevice8* dev;
    explicit Sprite8(IDirect3DDevice8* d) : dev(d) {}
    HRESULT STDMETHODCALLTYPE Begin() override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE End() override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE Draw(IDirect3DTexture8* tex, const RECT* src, const D3DXVECTOR2* scale,
                                   const D3DXVECTOR2* rotCenter, float rot, const D3DXVECTOR2* trans,
                                   D3DCOLOR color) override
    {
        auto* t = static_cast<Tex8*>(tex);
        RECT r = src ? *src : RECT{0, 0, (LONG)t->width, (LONG)t->height};
        const float w = float(r.right - r.left), h = float(r.bottom - r.top);
        const float sx = scale ? scale->x : 1.0f, sy = scale ? scale->y : 1.0f;
        const float cx = rotCenter ? rotCenter->x : 0.0f, cy = rotCenter ? rotCenter->y : 0.0f;
        const float tx = trans ? trans->x : 0.0f, ty = trans ? trans->y : 0.0f;
        const float cs = cosf(rot), sn = sinf(rot);
        const float corners[4][2] = {{0, 0}, {w, 0}, {w, h}, {0, h}};
        float p[4][2], uv[4][2];
        for (int i = 0; i < 4; ++i) {
            const float x = (corners[i][0] - cx) * sx, y = (corners[i][1] - cy) * sy;
            p[i][0] = x * cs - y * sn + cx + tx;
            p[i][1] = x * sn + y * cs + cy + ty;
            uv[i][0] = (r.left + corners[i][0]) / t->width;
            uv[i][1] = (r.top + corners[i][1]) / t->height;
        }
        StateGuard g(dev);
        DrawQuad(dev, tex, p, uv, color);
        return D3D_OK;
    }
};

struct Font8 : RefCounted<ID3DXFont> {
    IDirect3DDevice8* dev;
    HFONT font = nullptr;
    HDC dc = nullptr;
    struct Entry { Tex8* tex; int w, h; };
    std::list<std::pair<std::string, Entry>> lru;
    std::unordered_map<std::string, decltype(lru)::iterator> cache;

    Font8(IDirect3DDevice8* d, const LOGFONTA& lf) : dev(d)
    {
        LOGFONTA f = lf;
        f.lfQuality = ANTIALIASED_QUALITY;
        font = CreateFontIndirectA(&f);
        dc = CreateCompatibleDC(nullptr);
        SelectObject(dc, font);
    }
    ~Font8() override
    {
        for (auto& e : lru) e.second.tex->Release();
        DeleteDC(dc);
        DeleteObject(font);
    }
    HRESULT STDMETHODCALLTYPE Begin() override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE End() override { return D3D_OK; }

    // Renders the text as a white mask (alpha = coverage) and caches the texture.
    Entry* Mask(const std::string& text, RECT layout, DWORD fmt)
    {
        const std::string key = std::to_string(fmt) + ':' + std::to_string(layout.right - layout.left) + ':' + text;
        auto it = cache.find(key);
        if (it != cache.end()) {
            lru.splice(lru.begin(), lru, it->second);
            return &lru.front().second;
        }
        RECT calc = layout;
        ::DrawTextA(dc, text.c_str(), (int)text.size(), &calc, fmt | DT_CALCRECT);
        const int w = std::max<int>(calc.right - calc.left, 1), h = std::max<int>(calc.bottom - calc.top, 1);
        BITMAPINFO bi{};
        bi.bmiHeader = {sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB, 0, 0, 0, 0, 0};
        void* bits = nullptr;
        HBITMAP bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
        HGDIOBJ old = SelectObject(dc, bmp);
        memset(bits, 0, (size_t)w * h * 4);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(255, 255, 255));
        RECT r = {0, 0, w, h};
        ::DrawTextA(dc, text.c_str(), (int)text.size(), &r, fmt & ~(DT_CALCRECT | DT_RIGHT | DT_CENTER | DT_VCENTER | DT_BOTTOM));
        GdiFlush();
        IDirect3DTexture8* t = nullptr;
        dev->CreateTexture(w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t);
        auto* tex = static_cast<Tex8*>(t);
        const uint32_t* src = (const uint32_t*)bits;
        for (int i = 0; i < w * h; ++i) {
            const uint32_t s = src[i];
            const uint32_t a = std::max({s & 0xff, (s >> 8) & 0xff, (s >> 16) & 0xff});
            tex->levels[0][i] = (a << 24) | 0xffffff;
        }
        SelectObject(dc, old);
        DeleteObject(bmp);
        lru.emplace_front(key, Entry{tex, w, h});
        cache[key] = lru.begin();
        if (lru.size() > 256) {
            lru.back().second.tex->Release();
            cache.erase(lru.back().first);
            lru.pop_back();
        }
        return &lru.front().second;
    }

    int STDMETHODCALLTYPE DrawTextA(const char* text, int count, RECT* rect, DWORD fmt, D3DCOLOR color) override
    {
        std::string s = count < 0 ? std::string(text) : std::string(text, count);
        if (fmt & DT_CALCRECT) return ::DrawTextA(dc, s.c_str(), (int)s.size(), rect, fmt);
        Entry* e = Mask(s, *rect, fmt);
        float x = float(rect->left), y = float(rect->top);
        const int bw = rect->right - rect->left, bh = rect->bottom - rect->top;
        if (fmt & DT_RIGHT) x = float(rect->right - e->w);
        else if (fmt & DT_CENTER) x = rect->left + (bw - e->w) / 2.0f;
        if (fmt & DT_BOTTOM) y = float(rect->bottom - e->h);
        else if (fmt & DT_VCENTER) y = rect->top + (bh - e->h) / 2.0f;
        const float p[4][2] = {{x, y}, {x + e->w, y}, {x + e->w, y + e->h}, {x, y + e->h}};
        const float uv[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        StateGuard g(dev);
        DrawQuad(dev, e->tex, p, uv, color);
        return e->h;
    }
};

} // namespace

HRESULT D3DXCreateSprite(IDirect3DDevice8* dev, ID3DXSprite** out)
{
    *out = new Sprite8(dev);
    return D3D_OK;
}

HRESULT D3DXCreateFontIndirect(IDirect3DDevice8* dev, const LOGFONTA* lf, ID3DXFont** out)
{
    *out = new Font8(dev, *lf);
    return D3D_OK;
}
