#pragma once

// WIC image decoding and Direct3D 12 texture upload (with a CPU-built mip chain).

#include <d3d12.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <cstring>
#include <vector>

#include "image_ops.h"

namespace rs2 {

// Decodes any WIC-supported format (PNG, BMP, JPEG, ...) to tightly packed RGBA8.
// COM must already be initialized on the calling thread.
inline bool LoadImageWic(const wchar_t* path, Image* out)
{
    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) {
        return false;
    }
    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand,
                                                  &decoder))) {
        return false;
    }
    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame))) {
        return false;
    }
    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    if (FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0,
                                     WICBitmapPaletteTypeCustom))) {
        return false;
    }
    UINT width = 0, height = 0;
    if (FAILED(converter->GetSize(&width, &height)) || width == 0 || height == 0) {
        return false;
    }
    out->width = width;
    out->height = height;
    out->rgba.resize(static_cast<size_t>(width) * height * 4);
    return SUCCEEDED(converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(out->rgba.size()),
                                           out->rgba.data()));
}

// Records the upload into `commandList` (which must be open). `uploadHeap` must stay
// alive until the GPU has finished executing the list.
inline bool CreateTextureFromMips(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
                                  const std::vector<Image>& mips, Microsoft::WRL::ComPtr<ID3D12Resource>* texture,
                                  Microsoft::WRL::ComPtr<ID3D12Resource>* uploadHeap)
{
    if (mips.empty()) {
        return false;
    }
    const UINT mipCount = static_cast<UINT>(mips.size());

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = mips[0].width;
    desc.Height = mips[0].height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = static_cast<UINT16>(mipCount);
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    D3D12_HEAP_PROPERTIES defaultHeap{};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;
    if (FAILED(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &desc,
                                               D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                               IID_PPV_ARGS(texture->GetAddressOf())))) {
        return false;
    }

    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(mipCount);
    std::vector<UINT> rows(mipCount);
    std::vector<UINT64> rowSizes(mipCount);
    UINT64 totalSize = 0;
    device->GetCopyableFootprints(&desc, 0, mipCount, 0, footprints.data(), rows.data(), rowSizes.data(), &totalSize);

    D3D12_HEAP_PROPERTIES uploadProps{};
    uploadProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bufferDesc{};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = totalSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (FAILED(device->CreateCommittedResource(&uploadProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                               IID_PPV_ARGS(uploadHeap->GetAddressOf())))) {
        return false;
    }

    uint8_t* mapped = nullptr;
    const D3D12_RANGE noRead{0, 0};
    if (FAILED((*uploadHeap)->Map(0, &noRead, reinterpret_cast<void**>(&mapped)))) {
        return false;
    }
    for (UINT level = 0; level < mipCount; ++level) {
        const Image& image = mips[level];
        const size_t rowBytes = static_cast<size_t>(image.width) * 4;
        for (UINT row = 0; row < rows[level]; ++row) {
            std::memcpy(mapped + footprints[level].Offset + static_cast<size_t>(row) * footprints[level].Footprint.RowPitch,
                        image.rgba.data() + static_cast<size_t>(row) * rowBytes, rowBytes);
        }
    }
    (*uploadHeap)->Unmap(0, nullptr);

    for (UINT level = 0; level < mipCount; ++level) {
        D3D12_TEXTURE_COPY_LOCATION dst{};
        dst.pResource = texture->Get();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = level;
        D3D12_TEXTURE_COPY_LOCATION src{};
        src.pResource = uploadHeap->Get();
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint = footprints[level];
        commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    }

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture->Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &barrier);
    return true;
}

} // namespace rs2
