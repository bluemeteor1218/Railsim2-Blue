#pragma once

// Per-frame ring of 256-byte constant buffer slots in a persistently mapped
// upload heap. Each draw that needs different FrameConstants takes a slot and
// binds it as a root CBV.

#include <d3d12.h>
#include <wrl/client.h>

#include <cstring>

namespace rs2 {

class ConstantRing {
public:
    static constexpr UINT kSlotSize = 256; // CBV address alignment
    static constexpr UINT kSlotsPerFrame = 1024;
    static constexpr UINT kFrames = 3;

    bool Initialize(ID3D12Device* device)
    {
        D3D12_HEAP_PROPERTIES heap{};
        heap.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = static_cast<UINT64>(kSlotSize) * kSlotsPerFrame * kFrames;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if (FAILED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                                                    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                    IID_PPV_ARGS(&buffer_))) ||
            FAILED(buffer_->Map(0, nullptr, reinterpret_cast<void**>(&mapped_)))) {
            return false;
        }
        return true;
    }

    void BeginFrame(UINT frameIndex)
    {
        frame_ = frameIndex % kFrames;
        used_ = 0;
    }

    // Returns the GPU address of a slot holding a copy of `data`, or 0 when the frame's slots are exhausted.
    template <typename T>
    D3D12_GPU_VIRTUAL_ADDRESS Upload(const T& data)
    {
        static_assert(sizeof(T) <= kSlotSize, "constants must fit one slot");
        if (used_ >= kSlotsPerFrame) {
            return 0;
        }
        const UINT64 offset = (static_cast<UINT64>(frame_) * kSlotsPerFrame + used_++) * kSlotSize;
        std::memcpy(mapped_ + offset, &data, sizeof(T));
        return buffer_->GetGPUVirtualAddress() + offset;
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> buffer_;
    unsigned char* mapped_ = nullptr;
    UINT frame_ = 0;
    UINT used_ = 0;
};

} // namespace rs2
