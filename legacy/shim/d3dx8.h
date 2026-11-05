// D3DX8 subset used by the legacy RailSim2 code. Math comes from renderer/compat/d3dx_math.h.
#pragma once

#include "d3d8.h"
#include "dxfile.h"
#include "../../renderer/compat/d3dx_math.h"

#define D3DX_DEFAULT ((UINT)-1)

#define D3DXMESH_SYSTEMMEM 0x110
#define D3DXMESHOPT_COMPACT 0x01000000
#define D3DXMESHOPT_ATTRSORT 0x02000000
#define D3DXMESHOPT_VERTEXCACHE 0x04000000

struct D3DXMATERIAL {
    D3DMATERIAL8 MatD3D;
    char* pTextureFilename;
};

struct D3DXIMAGE_INFO;

struct ID3DXBuffer : IUnknown {
    virtual void* STDMETHODCALLTYPE GetBufferPointer() = 0;
    virtual DWORD STDMETHODCALLTYPE GetBufferSize() = 0;
};

struct ID3DXMesh : IUnknown {
    virtual DWORD STDMETHODCALLTYPE GetNumFaces() = 0;
    virtual DWORD STDMETHODCALLTYPE GetNumVertices() = 0;
    virtual DWORD STDMETHODCALLTYPE GetFVF() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetIndexBuffer(IDirect3DIndexBuffer8** buffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVertexBuffer(IDirect3DVertexBuffer8** buffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE LockVertexBuffer(DWORD flags, BYTE** data) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockVertexBuffer() = 0;
    virtual HRESULT STDMETHODCALLTYPE LockIndexBuffer(DWORD flags, BYTE** data) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockIndexBuffer() = 0;
    virtual HRESULT STDMETHODCALLTYPE LockAttributeBuffer(DWORD flags, DWORD** data) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockAttributeBuffer() = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawSubset(DWORD attribute) = 0;
    virtual HRESULT STDMETHODCALLTYPE Optimize(DWORD flags, const DWORD* adjacencyIn, DWORD* adjacencyOut,
                                               DWORD* faceRemap, ID3DXBuffer** vertexRemap, ID3DXMesh** optimized) = 0;
};

struct ID3DXSprite : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Begin() = 0;
    virtual HRESULT STDMETHODCALLTYPE End() = 0;
    virtual HRESULT STDMETHODCALLTYPE Draw(IDirect3DTexture8* texture, const RECT* source, const D3DXVECTOR2* scaling,
                                           const D3DXVECTOR2* rotationCenter, float rotation,
                                           const D3DXVECTOR2* translation, D3DCOLOR color) = 0;
};

struct ID3DXFont : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Begin() = 0;
    virtual HRESULT STDMETHODCALLTYPE End() = 0;
    virtual int STDMETHODCALLTYPE DrawTextA(const char* text, int count, RECT* rect, DWORD format, D3DCOLOR color) = 0;
};

typedef ID3DXBuffer* LPD3DXBUFFER;
typedef ID3DXMesh* LPD3DXMESH;
typedef ID3DXSprite* LPD3DXSPRITE;
typedef ID3DXFont* LPD3DXFONT;

HRESULT D3DXCreateTextureFromFileExA(IDirect3DDevice8* device, const char* file, UINT width, UINT height, UINT mipLevels,
                                     DWORD usage, D3DFORMAT format, D3DPOOL pool, DWORD filter, DWORD mipFilter,
                                     D3DCOLOR colorKey, D3DXIMAGE_INFO* info, PALETTEENTRY* palette,
                                     IDirect3DTexture8** texture);
HRESULT D3DXCreateTextureFromResourceExA(IDirect3DDevice8* device, HMODULE module, const char* resource, UINT width,
                                         UINT height, UINT mipLevels, DWORD usage, D3DFORMAT format, D3DPOOL pool,
                                         DWORD filter, DWORD mipFilter, D3DCOLOR colorKey, D3DXIMAGE_INFO* info,
                                         PALETTEENTRY* palette, IDirect3DTexture8** texture);
HRESULT D3DXCreateSprite(IDirect3DDevice8* device, ID3DXSprite** sprite);
HRESULT D3DXCreateFontIndirect(IDirect3DDevice8* device, const LOGFONTA* font, ID3DXFont** result);
HRESULT D3DXCreateSphere(IDirect3DDevice8* device, float radius, UINT slices, UINT stacks, ID3DXMesh** mesh,
                         ID3DXBuffer** adjacency);
HRESULT D3DXCreateBox(IDirect3DDevice8* device, float width, float height, float depth, ID3DXMesh** mesh,
                      ID3DXBuffer** adjacency);
HRESULT D3DXCreateTeapot(IDirect3DDevice8* device, ID3DXMesh** mesh, ID3DXBuffer** adjacency);
HRESULT D3DXLoadMeshFromX(const char* file, DWORD options, IDirect3DDevice8* device, ID3DXBuffer** adjacency,
                          ID3DXBuffer** materials, DWORD* materialCount, ID3DXMesh** mesh);
HRESULT D3DXLoadMeshFromXof(IDirectXFileData* data, DWORD options, IDirect3DDevice8* device, ID3DXBuffer** adjacency,
                            ID3DXBuffer** materials, DWORD* materialCount, ID3DXMesh** mesh);
UINT D3DXGetFVFVertexSize(DWORD fvf);
HRESULT D3DXComputeBoundingBox(const void* firstPosition, DWORD vertexCount, DWORD fvf, D3DXVECTOR3* minimum,
                               D3DXVECTOR3* maximum);
HRESULT D3DXComputeBoundingSphere(const void* firstPosition, DWORD vertexCount, DWORD fvf, D3DXVECTOR3* center,
                                  float* radius);
