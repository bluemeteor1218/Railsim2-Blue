// Direct3D 8 API surface used by the legacy RailSim2 code, declared for a Direct3D 12 backend.
// The original d3d8.h is not part of the modern Windows SDK; only what the game uses is declared.
#pragma once

#include <windows.h>
#include <unknwn.h>

#ifndef D3D_SDK_VERSION
#define D3D_SDK_VERSION 220
#endif

#define D3D_OK S_OK
#define D3DERR_DEVICELOST MAKE_HRESULT(1, 0x876, 2152)
#define D3DERR_DEVICENOTRESET MAKE_HRESULT(1, 0x876, 2153)
#define D3DERR_INVALIDCALL MAKE_HRESULT(1, 0x876, 2156)
#define D3DERR_NOTAVAILABLE MAKE_HRESULT(1, 0x876, 2154)

typedef DWORD D3DCOLOR;
#define D3DCOLOR_ARGB(a, r, g, b) \
    ((D3DCOLOR)((((a) & 0xff) << 24) | (((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff)))
#define D3DCOLOR_XRGB(r, g, b) D3DCOLOR_ARGB(0xff, r, g, b)

struct D3DCOLORVALUE {
    float r, g, b, a;
};

#ifndef D3DVECTOR_DEFINED
typedef struct _D3DVECTOR {
    float x, y, z;
} D3DVECTOR;
#define D3DVECTOR_DEFINED
#endif

enum D3DFORMAT {
    D3DFMT_UNKNOWN = 0,
    D3DFMT_R8G8B8 = 20,
    D3DFMT_A8R8G8B8 = 21,
    D3DFMT_X8R8G8B8 = 22,
    D3DFMT_R5G6B5 = 23,
    D3DFMT_X1R5G5B5 = 24,
    D3DFMT_A1R5G5B5 = 25,
    D3DFMT_A4R4G4B4 = 26,
    D3DFMT_R3G3B2 = 27,
    D3DFMT_A8 = 28,
    D3DFMT_A8R3G3B2 = 29,
    D3DFMT_X4R4G4B4 = 30,
    D3DFMT_D16_LOCKABLE = 70,
    D3DFMT_D32 = 71,
    D3DFMT_D15S1 = 73,
    D3DFMT_D24S8 = 75,
    D3DFMT_D16 = 80,
    D3DFMT_D24X8 = 77,
    D3DFMT_D24X4S4 = 79,
};

enum D3DDEVTYPE { D3DDEVTYPE_HAL = 1, D3DDEVTYPE_REF = 2, D3DDEVTYPE_SW = 3 };
enum D3DPOOL { D3DPOOL_DEFAULT = 0, D3DPOOL_MANAGED = 1, D3DPOOL_SYSTEMMEM = 2 };
enum D3DRESOURCETYPE { D3DRTYPE_SURFACE = 1, D3DRTYPE_TEXTURE = 3, D3DRTYPE_VERTEXBUFFER = 6, D3DRTYPE_INDEXBUFFER = 7 };
enum D3DMULTISAMPLE_TYPE { D3DMULTISAMPLE_NONE = 0 };
enum D3DSWAPEFFECT { D3DSWAPEFFECT_DISCARD = 1, D3DSWAPEFFECT_FLIP = 2, D3DSWAPEFFECT_COPY = 3 };

enum D3DPRIMITIVETYPE {
    D3DPT_POINTLIST = 1,
    D3DPT_LINELIST = 2,
    D3DPT_LINESTRIP = 3,
    D3DPT_TRIANGLELIST = 4,
    D3DPT_TRIANGLESTRIP = 5,
    D3DPT_TRIANGLEFAN = 6,
};

enum D3DTRANSFORMSTATETYPE {
    D3DTS_VIEW = 2,
    D3DTS_PROJECTION = 3,
    D3DTS_TEXTURE0 = 16,
    D3DTS_WORLD = 256,
};

enum D3DRENDERSTATETYPE {
    D3DRS_ZENABLE = 7,
    D3DRS_FILLMODE = 8,
    D3DRS_SHADEMODE = 9,
    D3DRS_ZWRITEENABLE = 14,
    D3DRS_ALPHATESTENABLE = 15,
    D3DRS_SRCBLEND = 19,
    D3DRS_DESTBLEND = 20,
    D3DRS_CULLMODE = 22,
    D3DRS_ZFUNC = 23,
    D3DRS_ALPHAREF = 24,
    D3DRS_ALPHAFUNC = 25,
    D3DRS_ALPHABLENDENABLE = 27,
    D3DRS_FOGENABLE = 28,
    D3DRS_SPECULARENABLE = 29,
    D3DRS_FOGCOLOR = 34,
    D3DRS_FOGTABLEMODE = 35,
    D3DRS_FOGSTART = 36,
    D3DRS_FOGEND = 37,
    D3DRS_STENCILFAIL = 53,
    D3DRS_STENCILZFAIL = 54,
    D3DRS_STENCILPASS = 55,
    D3DRS_STENCILFUNC = 56,
    D3DRS_STENCILREF = 57,
    D3DRS_STENCILMASK = 58,
    D3DRS_STENCILWRITEMASK = 59,
    D3DRS_AMBIENT = 139,
    D3DRS_FOGVERTEXMODE = 140,
    D3DRS_STENCILENABLE = 52,
    D3DRS_LIGHTING = 137,
    D3DRS_NORMALIZENORMALS = 143,
    D3DRS_DIFFUSEMATERIALSOURCE = 145,
    D3DRS_AMBIENTMATERIALSOURCE = 147,
};

enum D3DBLEND {
    D3DBLEND_ZERO = 1,
    D3DBLEND_ONE = 2,
    D3DBLEND_SRCCOLOR = 3,
    D3DBLEND_INVSRCCOLOR = 4,
    D3DBLEND_SRCALPHA = 5,
    D3DBLEND_INVSRCALPHA = 6,
    D3DBLEND_DESTCOLOR = 9,
};

enum D3DCMPFUNC {
    D3DCMP_NEVER = 1,
    D3DCMP_LESS = 2,
    D3DCMP_EQUAL = 3,
    D3DCMP_LESSEQUAL = 4,
    D3DCMP_GREATER = 5,
    D3DCMP_NOTEQUAL = 6,
    D3DCMP_GREATEREQUAL = 7,
    D3DCMP_ALWAYS = 8,
};

enum D3DCULL { D3DCULL_NONE = 1, D3DCULL_CW = 2, D3DCULL_CCW = 3 };
enum D3DSHADEMODE { D3DSHADE_FLAT = 1, D3DSHADE_GOURAUD = 2 };
enum D3DFOGMODE { D3DFOG_NONE = 0, D3DFOG_EXP = 1, D3DFOG_EXP2 = 2, D3DFOG_LINEAR = 3 };
enum D3DSTENCILOP { D3DSTENCILOP_KEEP = 1, D3DSTENCILOP_INCR = 7, D3DSTENCILOP_DECR = 8 };
enum D3DMATERIALCOLORSOURCE { D3DMCS_MATERIAL = 0, D3DMCS_COLOR1 = 1, D3DMCS_COLOR2 = 2 };

enum D3DTEXTURESTAGESTATETYPE {
    D3DTSS_COLOROP = 1,
    D3DTSS_COLORARG1 = 2,
    D3DTSS_COLORARG2 = 3,
    D3DTSS_ALPHAOP = 4,
    D3DTSS_ALPHAARG1 = 5,
    D3DTSS_ALPHAARG2 = 6,
    D3DTSS_TEXCOORDINDEX = 11,
    D3DTSS_ADDRESSU = 13,
    D3DTSS_ADDRESSV = 14,
    D3DTSS_MAGFILTER = 16,
    D3DTSS_MINFILTER = 17,
    D3DTSS_MIPFILTER = 18,
    D3DTSS_TEXTURETRANSFORMFLAGS = 24,
};

enum D3DTEXTUREOP {
    D3DTOP_DISABLE = 1,
    D3DTOP_SELECTARG1 = 2,
    D3DTOP_SELECTARG2 = 3,
    D3DTOP_MODULATE = 4,
    D3DTOP_ADDSMOOTH = 11,
};

#define D3DTA_DIFFUSE 0x00000000
#define D3DTA_CURRENT 0x00000001
#define D3DTA_TEXTURE 0x00000002

enum D3DTEXTUREADDRESS {
    D3DTADDRESS_WRAP = 1,
    D3DTADDRESS_MIRROR = 2,
    D3DTADDRESS_CLAMP = 3,
    D3DTADDRESS_BORDER = 4,
    D3DTADDRESS_MIRRORONCE = 5,
};

enum D3DTEXTUREFILTERTYPE { D3DTEXF_NONE = 0, D3DTEXF_POINT = 1, D3DTEXF_LINEAR = 2, D3DTEXF_ANISOTROPIC = 3 };

#define D3DTSS_TCI_PASSTHRU 0x00000000
#define D3DTSS_TCI_CAMERASPACENORMAL 0x00010000
#define D3DTTFF_DISABLE 0
#define D3DTTFF_COUNT2 2

#define D3DFVF_XYZ 0x002
#define D3DFVF_XYZRHW 0x004
#define D3DFVF_NORMAL 0x010
#define D3DFVF_DIFFUSE 0x040
#define D3DFVF_TEX1 0x100
#define D3DFVF_TEX2 0x200

#define D3DCLEAR_TARGET 0x1
#define D3DCLEAR_ZBUFFER 0x2
#define D3DCLEAR_STENCIL 0x4
#define D3DLOCK_READONLY 0x10
#define D3DLOCK_NOSYSLOCK 0x800
#define D3DUSAGE_RENDERTARGET 0x1
#define D3DUSAGE_DEPTHSTENCIL 0x2
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING 0x20
#define D3DCREATE_HARDWARE_VERTEXPROCESSING 0x40
#define D3DADAPTER_DEFAULT 0
#define D3DCS_ALL 0x1ff

#define D3DPRASTERCAPS_FOGVERTEX 0x80
#define D3DPRASTERCAPS_FOGTABLE 0x100
#define D3DPRASTERCAPS_FOGRANGE 0x10000
#define D3DPTEXTURECAPS_ALPHA 0x4
#define D3DPTEXTURECAPS_MIPMAP 0x4000
#define D3DTEXOPCAPS_BUMPENVMAP 0x00200000

enum D3DLIGHTTYPE { D3DLIGHT_POINT = 1, D3DLIGHT_SPOT = 2, D3DLIGHT_DIRECTIONAL = 3 };

struct D3DMATERIAL8 {
    D3DCOLORVALUE Diffuse, Ambient, Specular, Emissive;
    float Power;
};

struct D3DLIGHT8 {
    D3DLIGHTTYPE Type;
    D3DCOLORVALUE Diffuse, Specular, Ambient;
    D3DVECTOR Position, Direction;
    float Range, Falloff, Attenuation0, Attenuation1, Attenuation2, Theta, Phi;
};

struct D3DVIEWPORT8 {
    DWORD X, Y, Width, Height;
    float MinZ, MaxZ;
};

struct D3DCLIPSTATUS8 {
    DWORD ClipUnion, ClipIntersection;
};

struct D3DDISPLAYMODE {
    UINT Width, Height, RefreshRate;
    D3DFORMAT Format;
};

struct D3DADAPTER_IDENTIFIER8 {
    char Driver[512];
    char Description[512];
    LARGE_INTEGER DriverVersion;
    DWORD VendorId, DeviceId, SubSysId, Revision;
    GUID DeviceIdentifier;
    DWORD WHQLLevel;
};

struct D3DCAPS8 {
    DWORD RasterCaps, TextureCaps, TextureOpCaps;
    DWORD MaxTextureWidth, MaxTextureHeight, MaxSimultaneousTextures;
    DWORD MaxPrimitiveCount, MaxActiveLights;
};

struct D3DPRESENT_PARAMETERS {
    UINT BackBufferWidth, BackBufferHeight;
    D3DFORMAT BackBufferFormat;
    UINT BackBufferCount;
    D3DMULTISAMPLE_TYPE MultiSampleType;
    D3DSWAPEFFECT SwapEffect;
    HWND hDeviceWindow;
    BOOL Windowed;
    BOOL EnableAutoDepthStencil;
    D3DFORMAT AutoDepthStencilFormat;
    DWORD Flags;
    UINT FullScreen_RefreshRateInHz;
    UINT FullScreen_PresentationInterval;
};

struct D3DSURFACE_DESC {
    D3DFORMAT Format;
    D3DRESOURCETYPE Type;
    DWORD Usage;
    D3DPOOL Pool;
    UINT Size;
    D3DMULTISAMPLE_TYPE MultiSampleType;
    UINT Width, Height;
};

struct D3DINDEXBUFFER_DESC {
    D3DFORMAT Format;
    D3DRESOURCETYPE Type;
    DWORD Usage;
    D3DPOOL Pool;
    UINT Size;
};

struct D3DLOCKED_RECT {
    INT Pitch;
    void* pBits;
};

struct D3DXMATRIX;

struct IDirect3DDevice8;
struct IDirect3DSurface8;
struct IDirect3DTexture8;
struct IDirect3DBaseTexture8;
struct IDirect3DVertexBuffer8;
struct IDirect3DIndexBuffer8;

struct IDirect3DResource8 : IUnknown {};

struct IDirect3DBaseTexture8 : IDirect3DResource8 {};

struct IDirect3DSurface8 : IDirect3DResource8 {
    virtual HRESULT STDMETHODCALLTYPE GetDesc(D3DSURFACE_DESC* desc) = 0;
    virtual HRESULT STDMETHODCALLTYPE LockRect(D3DLOCKED_RECT* locked, const RECT* rect, DWORD flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockRect() = 0;
};

struct IDirect3DTexture8 : IDirect3DBaseTexture8 {
    virtual DWORD STDMETHODCALLTYPE GetLevelCount() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT level, D3DSURFACE_DESC* desc) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetSurfaceLevel(UINT level, IDirect3DSurface8** surface) = 0;
    virtual HRESULT STDMETHODCALLTYPE LockRect(UINT level, D3DLOCKED_RECT* locked, const RECT* rect, DWORD flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockRect(UINT level) = 0;
};

struct IDirect3DVertexBuffer8 : IDirect3DResource8 {
    virtual HRESULT STDMETHODCALLTYPE Lock(UINT offset, UINT size, BYTE** data, DWORD flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE Unlock() = 0;
};

struct IDirect3DIndexBuffer8 : IDirect3DResource8 {
    virtual HRESULT STDMETHODCALLTYPE Lock(UINT offset, UINT size, BYTE** data, DWORD flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE Unlock() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesc(D3DINDEXBUFFER_DESC* desc) = 0;
};

struct IDirect3DDevice8 : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE TestCooperativeLevel() = 0;
    virtual UINT STDMETHODCALLTYPE GetAvailableTextureMem() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceCaps(D3DCAPS8* caps) = 0;
    virtual HRESULT STDMETHODCALLTYPE Reset(D3DPRESENT_PARAMETERS* params) = 0;
    virtual HRESULT STDMETHODCALLTYPE Present(const RECT* src, const RECT* dst, HWND window, const RGNDATA* dirty) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateTexture(UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format,
                                                    D3DPOOL pool, IDirect3DTexture8** texture) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateVertexBuffer(UINT length, DWORD usage, DWORD fvf, D3DPOOL pool,
                                                         IDirect3DVertexBuffer8** buffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateImageSurface(UINT width, UINT height, D3DFORMAT format,
                                                         IDirect3DSurface8** surface) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilSurface(UINT width, UINT height, D3DFORMAT format,
                                                                D3DMULTISAMPLE_TYPE multisample,
                                                                IDirect3DSurface8** surface) = 0;
    virtual HRESULT STDMETHODCALLTYPE CopyRects(IDirect3DSurface8* src, const RECT* srcRects, UINT rectCount,
                                                IDirect3DSurface8* dst, const POINT* dstPoints) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetRenderTarget(IDirect3DSurface8* target, IDirect3DSurface8* depth) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRenderTarget(IDirect3DSurface8** target) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDepthStencilSurface(IDirect3DSurface8** depth) = 0;
    virtual HRESULT STDMETHODCALLTYPE BeginScene() = 0;
    virtual HRESULT STDMETHODCALLTYPE EndScene() = 0;
    virtual HRESULT STDMETHODCALLTYPE Clear(DWORD count, const RECT* rects, DWORD flags, D3DCOLOR color, float z,
                                            DWORD stencil) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTransform(D3DTRANSFORMSTATETYPE state, const D3DXMATRIX* matrix) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetViewport(const D3DVIEWPORT8* viewport) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewport(D3DVIEWPORT8* viewport) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMaterial(const D3DMATERIAL8* material) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetLight(DWORD index, const D3DLIGHT8* light) = 0;
    virtual HRESULT STDMETHODCALLTYPE LightEnable(DWORD index, BOOL enable) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetRenderState(D3DRENDERSTATETYPE state, DWORD value) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRenderState(D3DRENDERSTATETYPE state, DWORD* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetClipStatus(const D3DCLIPSTATUS8* status) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTexture(DWORD stage, IDirect3DBaseTexture8* texture) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTexture(DWORD stage, IDirect3DBaseTexture8** texture) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVertexShader(DWORD fvf) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetStreamSource(UINT stream, IDirect3DVertexBuffer8* data, UINT stride) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawPrimitive(D3DPRIMITIVETYPE type, UINT startVertex, UINT primitiveCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawPrimitiveUP(D3DPRIMITIVETYPE type, UINT primitiveCount, const void* data,
                                                      UINT stride) = 0;
};

struct IDirect3D8 : IUnknown {
    virtual UINT STDMETHODCALLTYPE GetAdapterCount() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAdapterIdentifier(UINT adapter, DWORD flags, D3DADAPTER_IDENTIFIER8* id) = 0;
    virtual UINT STDMETHODCALLTYPE GetAdapterModeCount(UINT adapter) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumAdapterModes(UINT adapter, UINT mode, D3DDISPLAYMODE* out) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAdapterDisplayMode(UINT adapter, D3DDISPLAYMODE* out) = 0;
    virtual HRESULT STDMETHODCALLTYPE CheckDeviceFormat(UINT adapter, D3DDEVTYPE type, D3DFORMAT adapterFormat,
                                                        DWORD usage, D3DRESOURCETYPE resource, D3DFORMAT check) = 0;
    virtual HRESULT STDMETHODCALLTYPE CheckDepthStencilMatch(UINT adapter, D3DDEVTYPE type, D3DFORMAT adapterFormat,
                                                             D3DFORMAT renderTarget, D3DFORMAT depthStencil) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDevice(UINT adapter, D3DDEVTYPE type, HWND window, DWORD behavior,
                                                   D3DPRESENT_PARAMETERS* params, IDirect3DDevice8** device) = 0;
};

typedef IDirect3D8* LPDIRECT3D8;
typedef IDirect3DDevice8* LPDIRECT3DDEVICE8;
typedef IDirect3DSurface8* LPDIRECT3DSURFACE8;
typedef IDirect3DTexture8* LPDIRECT3DTEXTURE8;
typedef IDirect3DVertexBuffer8* LPDIRECT3DVERTEXBUFFER8;
typedef IDirect3DIndexBuffer8* LPDIRECT3DINDEXBUFFER8;

IDirect3D8* Direct3DCreate8(UINT sdkVersion);
