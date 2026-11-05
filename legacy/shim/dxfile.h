// DirectX file (.x) API subset used by the legacy RailSim2 code; implemented with the text .x loader.
#pragma once

#include <windows.h>
#include <unknwn.h>

#define DXFILE_OK 0
#define D3DRM_XTEMPLATES ((void*)0)
#define D3DRM_XTEMPLATE_BYTES 0

enum DXFILELOADOPTIONS { DXFILELOAD_FROMFILE = 0, DXFILELOAD_FROMRESOURCE = 1 };

struct DXFILELOADRESOURCE {
    HMODULE hModule;
    const char* lpName;
    const char* lpType;
};

// {3D82AB44-62DA-11CF-AB39-0020AF71E433}
static const GUID TID_D3DRMMesh = {0x3d82ab44, 0x62da, 0x11cf, {0xab, 0x39, 0x00, 0x20, 0xaf, 0x71, 0xe4, 0x33}};

struct IDirectXFileData : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetType(const GUID** type) = 0;
};

struct IDirectXFileEnumObject : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetNextDataObject(IDirectXFileData** data) = 0;
};

struct IDirectXFile : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE CreateEnumObject(const void* source, DXFILELOADOPTIONS options,
                                                       IDirectXFileEnumObject** result) = 0;
    virtual HRESULT STDMETHODCALLTYPE RegisterTemplates(const void* data, DWORD size) = 0;
};

typedef IDirectXFile* LPDIRECTXFILE;
typedef IDirectXFileData* LPDIRECTXFILEDATA;
typedef IDirectXFileEnumObject* LPDIRECTXFILEENUMOBJECT;

HRESULT DirectXFileCreate(IDirectXFile** file);
