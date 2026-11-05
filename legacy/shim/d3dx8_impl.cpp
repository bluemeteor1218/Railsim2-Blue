#include <wincodec.h>
#include <shlwapi.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "../../renderer/core/image_ops.h"
#include "../../renderer/core/x_mesh.h"
#include "d3d8_objects.h"

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace gfx;

namespace {

IWICImagingFactory* WicFactory()
{
    static IWICImagingFactory* f = [] {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        IWICImagingFactory* p = nullptr;
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&p));
        return p;
    }();
    return f;
}

bool DecodeFrame(IWICBitmapDecoder* decoder, rs2::Image* out)
{
    IWICImagingFactory* factory = WicFactory();
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* conv = nullptr;
    bool ok = false;
    if (SUCCEEDED(decoder->GetFrame(0, &frame)) && SUCCEEDED(factory->CreateFormatConverter(&conv)) &&
        SUCCEEDED(conv->Initialize(frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.0,
                                   WICBitmapPaletteTypeCustom))) {
        UINT w = 0, h = 0;
        conv->GetSize(&w, &h);
        if (w && h) {
            out->width = w;
            out->height = h;
            out->rgba.resize((size_t)w * h * 4);
            ok = SUCCEEDED(conv->CopyPixels(nullptr, w * 4, (UINT)out->rgba.size(), out->rgba.data()));
        }
    }
    if (conv) conv->Release();
    if (frame) frame->Release();
    return ok;
}

// Pixels are decoded straight to BGRA (the D3D8 A8R8G8B8 byte order).
bool LoadFile(const char* path, rs2::Image* out)
{
    wchar_t wide[MAX_PATH * 2];
    if (!MultiByteToWideChar(CP_ACP, 0, path, -1, wide, MAX_PATH * 2)) return false;
    IWICBitmapDecoder* decoder = nullptr;
    if (FAILED(WicFactory()->CreateDecoderFromFilename(wide, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand,
                                                       &decoder)))
        return false;
    const bool ok = DecodeFrame(decoder, out);
    decoder->Release();
    return ok;
}

bool LoadResourceBitmap(HMODULE module, const char* name, rs2::Image* out)
{
    HRSRC r = FindResourceA(module, name, RT_BITMAP);
    if (!r) return false;
    HGLOBAL g = LoadResource(module, r);
    const DWORD size = SizeofResource(module, r);
    if (!g || size < sizeof(BITMAPINFOHEADER)) return false;
    const uint8_t* dib = (const uint8_t*)LockResource(g);
    const auto* bih = (const BITMAPINFOHEADER*)dib;
    BITMAPFILEHEADER fh{};
    fh.bfType = 0x4d42;
    fh.bfSize = (DWORD)sizeof(fh) + size;
    DWORD colors = bih->biClrUsed ? bih->biClrUsed : (bih->biBitCount <= 8 ? 1u << bih->biBitCount : 0);
    fh.bfOffBits = (DWORD)sizeof(fh) + bih->biSize + colors * 4;
    std::string bmp((const char*)&fh, sizeof(fh));
    bmp.append((const char*)dib, size);
    IStream* stream = SHCreateMemStream((const BYTE*)bmp.data(), (UINT)bmp.size());
    IWICBitmapDecoder* decoder = nullptr;
    bool ok = false;
    if (stream && SUCCEEDED(WicFactory()->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand,
                                                                  &decoder))) {
        ok = DecodeFrame(decoder, out);
        decoder->Release();
    }
    if (stream) stream->Release();
    return ok;
}

HRESULT BuildTexture(IDirect3DDevice8* dev, rs2::Image img, UINT mipLevels, DWORD usage, D3DFORMAT fmt, D3DCOLOR key,
                     IDirect3DTexture8** out)
{
    auto* px = reinterpret_cast<uint32_t*>(img.rgba.data());
    if (key) {
        for (size_t i = 0; i < (size_t)img.width * img.height; ++i)
            if ((px[i] & 0xffffff) == (key & 0xffffff)) px[i] = 0;
    }
    std::vector<rs2::Image> chain;
    if (mipLevels == 1) {
        chain.push_back(std::move(img));
    } else {
        chain = rs2::GenerateMipChain(img);
        if (mipLevels != 0 && mipLevels != D3DX_DEFAULT && chain.size() > mipLevels) chain.resize(mipLevels);
    }
    IDirect3DTexture8* t = nullptr;
    HRESULT hr = dev->CreateTexture(chain[0].width, chain[0].height, (UINT)chain.size(), usage, fmt, D3DPOOL_MANAGED, &t);
    if (FAILED(hr)) return hr;
    auto* tex = static_cast<Tex8*>(t);
    for (size_t l = 0; l < chain.size(); ++l) memcpy(tex->levels[l].data(), chain[l].rgba.data(), chain[l].rgba.size());
    *out = t;
    return D3D_OK;
}

// ---- meshes ----

struct Mesh8 : RefCounted<ID3DXMesh> {
    IDirect3DDevice8* device = nullptr;
    DWORD fvf = 0;
    UINT stride = 0;
    std::vector<uint8_t> vertices;
    std::vector<uint16_t> indices;
    std::vector<DWORD> attributes; // per face
    std::map<DWORD, std::vector<uint16_t>> subsetCache;

    explicit Mesh8(IDirect3DDevice8* d) : device(d) {}
    DWORD STDMETHODCALLTYPE GetNumFaces() override { return (DWORD)indices.size() / 3; }
    DWORD STDMETHODCALLTYPE GetNumVertices() override { return (DWORD)(vertices.size() / stride); }
    DWORD STDMETHODCALLTYPE GetFVF() override { return fvf; }
    HRESULT STDMETHODCALLTYPE GetIndexBuffer(IDirect3DIndexBuffer8**) override { return D3DERR_NOTAVAILABLE; }
    HRESULT STDMETHODCALLTYPE GetVertexBuffer(IDirect3DVertexBuffer8** out) override
    {
        auto* v = new VB8();
        v->data = vertices;
        v->fvf = fvf;
        *out = v;
        return D3D_OK;
    }
    HRESULT STDMETHODCALLTYPE LockVertexBuffer(DWORD, BYTE** d) override { *d = vertices.data(); return D3D_OK; }
    HRESULT STDMETHODCALLTYPE UnlockVertexBuffer() override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE LockIndexBuffer(DWORD, BYTE** d) override { *d = (BYTE*)indices.data(); return D3D_OK; }
    HRESULT STDMETHODCALLTYPE UnlockIndexBuffer() override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE LockAttributeBuffer(DWORD, DWORD** d) override { *d = attributes.data(); return D3D_OK; }
    HRESULT STDMETHODCALLTYPE UnlockAttributeBuffer() override { subsetCache.clear(); return D3D_OK; }
    HRESULT STDMETHODCALLTYPE DrawSubset(DWORD attr) override
    {
        auto it = subsetCache.find(attr);
        if (it == subsetCache.end()) {
            std::vector<uint16_t> idx;
            for (size_t f = 0; f < attributes.size(); ++f)
                if (attributes[f] == attr) idx.insert(idx.end(), &indices[f * 3], &indices[f * 3] + 3);
            it = subsetCache.emplace(attr, std::move(idx)).first;
        }
        if (it->second.empty()) return D3D_OK;
        return DrawIndexed(device, fvf, vertices.data(), GetNumVertices(), it->second.data(), (UINT)it->second.size());
    }
    HRESULT STDMETHODCALLTYPE Optimize(DWORD, const DWORD*, DWORD* adjOut, DWORD* faceRemap, ID3DXBuffer** vertRemap,
                                       ID3DXMesh** out) override
    {
        if (adjOut || faceRemap || vertRemap) return D3DERR_NOTAVAILABLE;
        auto* m = new Mesh8(device);
        m->fvf = fvf;
        m->stride = stride;
        m->vertices = vertices;
        m->indices = indices;
        m->attributes = attributes;
        *out = m;
        return D3D_OK;
    }
};

struct Buffer8 : RefCounted<ID3DXBuffer> {
    std::vector<uint8_t> data;
    void* STDMETHODCALLTYPE GetBufferPointer() override { return data.data(); }
    DWORD STDMETHODCALLTYPE GetBufferSize() override { return (DWORD)data.size(); }
};

struct Vtx {
    float p[3], n[3];
};

// D3D8 front faces are clockwise; flip triangles whose geometric normal disagrees with the vertex normal.
void FixWinding(const std::vector<Vtx>& v, std::vector<uint16_t>& idx)
{
    for (size_t i = 0; i + 2 < idx.size(); i += 3) {
        const Vtx &a = v[idx[i]], &b = v[idx[i + 1]], &c = v[idx[i + 2]];
        const float e1[3] = {b.p[0] - a.p[0], b.p[1] - a.p[1], b.p[2] - a.p[2]};
        const float e2[3] = {c.p[0] - a.p[0], c.p[1] - a.p[1], c.p[2] - a.p[2]};
        const float cx = e1[1] * e2[2] - e1[2] * e2[1], cy = e1[2] * e2[0] - e1[0] * e2[2],
                    cz = e1[0] * e2[1] - e1[1] * e2[0];
        const float dot = cx * (a.n[0] + b.n[0] + c.n[0]) + cy * (a.n[1] + b.n[1] + c.n[1]) +
                          cz * (a.n[2] + b.n[2] + c.n[2]);
        if (dot < 0) std::swap(idx[i + 1], idx[i + 2]);
    }
}

Mesh8* FromPN(IDirect3DDevice8* dev, const std::vector<Vtx>& v, std::vector<uint16_t> idx)
{
    FixWinding(v, idx);
    auto* m = new Mesh8(dev);
    m->fvf = D3DFVF_XYZ | D3DFVF_NORMAL;
    m->stride = sizeof(Vtx);
    m->vertices.assign((const uint8_t*)v.data(), (const uint8_t*)(v.data() + v.size()));
    m->indices = std::move(idx);
    m->attributes.assign(m->indices.size() / 3, 0);
    return m;
}

HRESULT MeshFromX(const rs2::XMesh& xm, IDirect3DDevice8* dev, ID3DXBuffer** adj, ID3DXBuffer** mats, DWORD* matCount,
                  ID3DXMesh** out)
{
    auto* m = new Mesh8(dev);
    m->fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;
    m->stride = 32;
    m->vertices.resize(xm.vertices.size() * 32);
    for (size_t i = 0; i < xm.vertices.size(); ++i) {
        float* d = (float*)(m->vertices.data() + i * 32);
        memcpy(d, xm.vertices[i].position, 12);
        memcpy(d + 3, xm.vertices[i].normal, 12);
        memcpy(d + 6, xm.vertices[i].uv, 8);
    }
    m->indices.resize(xm.indices.size());
    for (size_t i = 0; i < xm.indices.size(); ++i) m->indices[i] = (uint16_t)xm.indices[i];
    m->attributes.assign(m->indices.size() / 3, 0);
    for (const auto& s : xm.subsets)
        for (uint32_t f = s.indexStart / 3; f < (s.indexStart + s.indexCount) / 3; ++f) m->attributes[f] = s.material;

    const size_t n = std::max<size_t>(xm.materials.size(), 1);
    if (matCount) *matCount = (DWORD)n;
    if (mats) {
        size_t bytes = n * sizeof(D3DXMATERIAL);
        for (const auto& mt : xm.materials) bytes += mt.texture.size() + 1;
        auto* b = new Buffer8();
        b->data.assign(bytes, 0);
        auto* arr = (D3DXMATERIAL*)b->data.data();
        char* strings = (char*)(arr + n);
        for (size_t i = 0; i < n; ++i) {
            D3DMATERIAL8& d = arr[i].MatD3D;
            if (i < xm.materials.size()) {
                const auto& mt = xm.materials[i];
                d.Diffuse = {mt.diffuse[0], mt.diffuse[1], mt.diffuse[2], mt.diffuse[3]};
                d.Specular = {mt.specular[0], mt.specular[1], mt.specular[2], 1.0f};
                d.Emissive = {mt.emissive[0], mt.emissive[1], mt.emissive[2], 1.0f};
                d.Power = mt.power;
                if (!mt.texture.empty()) {
                    memcpy(strings, mt.texture.c_str(), mt.texture.size() + 1);
                    arr[i].pTextureFilename = strings;
                    strings += mt.texture.size() + 1;
                }
            } else {
                d.Diffuse = {1, 1, 1, 1};
            }
            d.Ambient = d.Diffuse;
        }
        *mats = b;
    }
    if (adj) {
        auto* b = new Buffer8();
        b->data.assign(m->indices.size() * sizeof(DWORD), 0xff);
        *adj = b;
    }
    *out = m;
    return D3D_OK;
}

// ---- DirectX file enumeration (text .x, top level mesh only) ----

struct XData : RefCounted<IDirectXFileData> {
    std::shared_ptr<rs2::XMesh> mesh;
    HRESULT STDMETHODCALLTYPE GetType(const GUID** t) override
    {
        *t = &TID_D3DRMMesh;
        return DXFILE_OK;
    }
};

struct XEnum : RefCounted<IDirectXFileEnumObject> {
    std::shared_ptr<rs2::XMesh> mesh;
    bool done = false;
    HRESULT STDMETHODCALLTYPE GetNextDataObject(IDirectXFileData** out) override
    {
        if (done || !mesh) return E_FAIL;
        done = true;
        auto* d = new XData();
        d->mesh = mesh;
        *out = d;
        return DXFILE_OK;
    }
};

struct XFile : RefCounted<IDirectXFile> {
    HRESULT STDMETHODCALLTYPE RegisterTemplates(const void*, DWORD) override { return DXFILE_OK; }
    HRESULT STDMETHODCALLTYPE CreateEnumObject(const void* src, DXFILELOADOPTIONS opt,
                                               IDirectXFileEnumObject** out) override
    {
        std::string text;
        if (opt == DXFILELOAD_FROMFILE) {
            if (!ReadFileText((const char*)src, &text)) return E_FAIL;
        } else {
            const auto* r = (const DXFILELOADRESOURCE*)src;
            HRSRC h = FindResourceA(r->hModule, r->lpName, r->lpType);
            if (!h) return E_FAIL;
            HGLOBAL g = LoadResource(r->hModule, h);
            text.assign((const char*)LockResource(g), SizeofResource(r->hModule, h));
        }
        auto mesh = std::make_shared<rs2::XMesh>();
        if (!rs2::ParseXMesh(text, mesh.get())) return E_FAIL;
        auto* e = new XEnum();
        e->mesh = mesh;
        *out = e;
        return DXFILE_OK;
    }
    static bool ReadFileText(const char* path, std::string* out)
    {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;
        std::ostringstream ss;
        ss << f.rdbuf();
        *out = ss.str();
        return true;
    }
};

} // namespace

HRESULT D3DXCreateTextureFromFileExA(IDirect3DDevice8* dev, const char* file, UINT, UINT, UINT mipLevels, DWORD usage,
                                     D3DFORMAT fmt, D3DPOOL, DWORD, DWORD, D3DCOLOR key, D3DXIMAGE_INFO*,
                                     PALETTEENTRY*, IDirect3DTexture8** out)
{
    rs2::Image img;
    if (!LoadFile(file, &img)) return D3DERR_INVALIDCALL;
    return BuildTexture(dev, std::move(img), mipLevels, usage, fmt, key, out);
}

HRESULT D3DXCreateTextureFromResourceExA(IDirect3DDevice8* dev, HMODULE module, const char* name, UINT, UINT,
                                         UINT mipLevels, DWORD usage, D3DFORMAT fmt, D3DPOOL, DWORD, DWORD,
                                         D3DCOLOR key, D3DXIMAGE_INFO*, PALETTEENTRY*, IDirect3DTexture8** out)
{
    rs2::Image img;
    if (!LoadResourceBitmap(module, name, &img)) return D3DERR_INVALIDCALL;
    return BuildTexture(dev, std::move(img), mipLevels, usage, fmt, key, out);
}

HRESULT D3DXCreateSphere(IDirect3DDevice8* dev, float radius, UINT slices, UINT stacks, ID3DXMesh** mesh, ID3DXBuffer**)
{
    slices = std::max(slices, 3u);
    stacks = std::max(stacks, 2u);
    std::vector<Vtx> v;
    std::vector<uint16_t> idx;
    const float pi = 3.14159265f;
    v.push_back({{0, 0, radius}, {0, 0, 1}});
    for (UINT s = 1; s < stacks; ++s) {
        const float phi = pi * s / stacks;
        for (UINT l = 0; l < slices; ++l) {
            const float th = 2 * pi * l / slices;
            const float nx = sinf(phi) * cosf(th), ny = sinf(phi) * sinf(th), nz = cosf(phi);
            v.push_back({{nx * radius, ny * radius, nz * radius}, {nx, ny, nz}});
        }
    }
    v.push_back({{0, 0, -radius}, {0, 0, -1}});
    const uint16_t last = (uint16_t)(v.size() - 1);
    for (UINT l = 0; l < slices; ++l) {
        const UINT n = (l + 1) % slices;
        idx.insert(idx.end(), {0, (uint16_t)(1 + l), (uint16_t)(1 + n)});
        const UINT base = 1 + (stacks - 2) * slices;
        idx.insert(idx.end(), {last, (uint16_t)(base + n), (uint16_t)(base + l)});
    }
    for (UINT s = 0; s + 2 < stacks; ++s)
        for (UINT l = 0; l < slices; ++l) {
            const UINT n = (l + 1) % slices;
            const uint16_t a = (uint16_t)(1 + s * slices + l), b = (uint16_t)(1 + s * slices + n);
            const uint16_t c = (uint16_t)(1 + (s + 1) * slices + l), d = (uint16_t)(1 + (s + 1) * slices + n);
            idx.insert(idx.end(), {a, c, b, b, c, d});
        }
    *mesh = FromPN(dev, v, std::move(idx));
    return D3D_OK;
}

HRESULT D3DXCreateBox(IDirect3DDevice8* dev, float w, float h, float d, ID3DXMesh** mesh, ID3DXBuffer**)
{
    const float x = w / 2, y = h / 2, z = d / 2;
    struct Face { float n[3]; float c[4][3]; };
    const Face faces[6] = {
        {{1, 0, 0}, {{x, -y, -z}, {x, y, -z}, {x, y, z}, {x, -y, z}}},
        {{-1, 0, 0}, {{-x, -y, z}, {-x, y, z}, {-x, y, -z}, {-x, -y, -z}}},
        {{0, 1, 0}, {{-x, y, -z}, {-x, y, z}, {x, y, z}, {x, y, -z}}},
        {{0, -1, 0}, {{-x, -y, z}, {-x, -y, -z}, {x, -y, -z}, {x, -y, z}}},
        {{0, 0, 1}, {{-x, -y, z}, {x, -y, z}, {x, y, z}, {-x, y, z}}},
        {{0, 0, -1}, {{x, -y, -z}, {-x, -y, -z}, {-x, y, -z}, {x, y, -z}}},
    };
    std::vector<Vtx> v;
    std::vector<uint16_t> idx;
    for (const auto& f : faces) {
        const uint16_t b = (uint16_t)v.size();
        for (const auto& c : f.c) v.push_back({{c[0], c[1], c[2]}, {f.n[0], f.n[1], f.n[2]}});
        idx.insert(idx.end(), {b, (uint16_t)(b + 1), (uint16_t)(b + 2), b, (uint16_t)(b + 2), (uint16_t)(b + 3)});
    }
    *mesh = FromPN(dev, v, std::move(idx));
    return D3D_OK;
}

// The teapot is only a debug primitive; a sphere stands in for it.
HRESULT D3DXCreateTeapot(IDirect3DDevice8* dev, ID3DXMesh** mesh, ID3DXBuffer** adj)
{
    return D3DXCreateSphere(dev, 1.0f, 16, 12, mesh, adj);
}

HRESULT D3DXLoadMeshFromX(const char* file, DWORD, IDirect3DDevice8* dev, ID3DXBuffer** adj, ID3DXBuffer** mats,
                          DWORD* matCount, ID3DXMesh** mesh)
{
    std::string text;
    if (!XFile::ReadFileText(file, &text)) return E_FAIL;
    rs2::XMesh xm;
    if (!rs2::ParseXMesh(text, &xm)) return E_FAIL;
    return MeshFromX(xm, dev, adj, mats, matCount, mesh);
}

HRESULT D3DXLoadMeshFromXof(IDirectXFileData* data, DWORD, IDirect3DDevice8* dev, ID3DXBuffer** adj, ID3DXBuffer** mats,
                            DWORD* matCount, ID3DXMesh** mesh)
{
    auto* d = static_cast<XData*>(data);
    return MeshFromX(*d->mesh, dev, adj, mats, matCount, mesh);
}

HRESULT DirectXFileCreate(IDirectXFile** file)
{
    *file = new XFile();
    return DXFILE_OK;
}

UINT D3DXGetFVFVertexSize(DWORD fvf) { return gfx::FvfVertexSize(fvf); }

HRESULT D3DXComputeBoundingBox(const void* first, DWORD count, DWORD fvf, D3DXVECTOR3* mn, D3DXVECTOR3* mx)
{
    const UINT stride = gfx::FvfVertexSize(fvf);
    if (!count) return D3DERR_INVALIDCALL;
    const float* p = (const float*)first;
    *mn = *mx = D3DXVECTOR3(p[0], p[1], p[2]);
    for (DWORD i = 1; i < count; ++i) {
        p = (const float*)((const uint8_t*)first + (size_t)i * stride);
        mn->x = std::min(mn->x, p[0]); mn->y = std::min(mn->y, p[1]); mn->z = std::min(mn->z, p[2]);
        mx->x = std::max(mx->x, p[0]); mx->y = std::max(mx->y, p[1]); mx->z = std::max(mx->z, p[2]);
    }
    return D3D_OK;
}

HRESULT D3DXComputeBoundingSphere(const void* first, DWORD count, DWORD fvf, D3DXVECTOR3* center, float* radius)
{
    D3DXVECTOR3 mn, mx;
    if (FAILED(D3DXComputeBoundingBox(first, count, fvf, &mn, &mx))) return D3DERR_INVALIDCALL;
    *center = D3DXVECTOR3((mn.x + mx.x) / 2, (mn.y + mx.y) / 2, (mn.z + mx.z) / 2);
    const UINT stride = gfx::FvfVertexSize(fvf);
    float r2 = 0;
    for (DWORD i = 0; i < count; ++i) {
        const float* p = (const float*)((const uint8_t*)first + (size_t)i * stride);
        const float dx = p[0] - center->x, dy = p[1] - center->y, dz = p[2] - center->z;
        r2 = std::max(r2, dx * dx + dy * dy + dz * dz);
    }
    *radius = sqrtf(r2);
    return D3D_OK;
}
