#pragma once

// Draws a SpriteBatch with Direct3D 12: alpha-blended, no depth, one SRV slot per texture.

#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <string>

#include "sprite_batch.h"
#include "viewport_layout.h"

namespace rs2 {

inline const char kUiShader[] = R"hlsl(
cbuffer Ui : register(b0)
{
    float2 scale; // virtual pixels -> NDC
    float2 padding;
};

Texture2D tex : register(t0);
SamplerState samplerLinear : register(s0);

struct VSInput
{
    float2 position : POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position.x * scale.x - 1.0f, input.position.y * scale.y + 1.0f, 0.0f, 1.0f);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    return input.color * tex.Sample(samplerLinear, input.uv);
}
)hlsl";

class UiRenderer {
public:
    static constexpr UINT kMaxTextures = 4;
    static constexpr UINT kMaxVertices = 65536;
    static constexpr UINT kFrames = 2;

    bool Initialize(ID3D12Device* device, DXGI_FORMAT renderTargetFormat, DXGI_FORMAT depthFormat, std::string* error)
    {
        device_ = device;

        D3D12_DESCRIPTOR_RANGE range{};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors = 1;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER parameters[2]{};
        parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        parameters[0].Constants.Num32BitValues = 4;
        parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameters[1].DescriptorTable.NumDescriptorRanges = 1;
        parameters[1].DescriptorTable.pDescriptorRanges = &range;
        parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC sampler{};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC description{};
        description.NumParameters = 2;
        description.pParameters = parameters;
        description.NumStaticSamplers = 1;
        description.pStaticSamplers = &sampler;
        description.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Microsoft::WRL::ComPtr<ID3DBlob> blob, errors;
        if (FAILED(D3D12SerializeRootSignature(&description, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &errors)) ||
            FAILED(device_->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
                                                IID_PPV_ARGS(&rootSignature_)))) {
            return Fail(error, "UI root signature", errors.Get());
        }

        Microsoft::WRL::ComPtr<ID3DBlob> vs, ps;
        if (FAILED(D3DCompile(kUiShader, sizeof(kUiShader) - 1, "ui.hlsl", nullptr, nullptr, "VSMain", "vs_5_0",
                              D3DCOMPILE_ENABLE_STRICTNESS, 0, &vs, &errors)) ||
            FAILED(D3DCompile(kUiShader, sizeof(kUiShader) - 1, "ui.hlsl", nullptr, nullptr, "PSMain", "ps_5_0",
                              D3DCOMPILE_ENABLE_STRICTNESS, 0, &ps, &errors))) {
            return Fail(error, "UI shader", errors.Get());
        }

        static const D3D12_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
        desc.pRootSignature = rootSignature_.Get();
        desc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
        desc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
        desc.InputLayout = {layout, static_cast<UINT>(sizeof(layout) / sizeof(layout[0]))};
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = renderTargetFormat;
        desc.DSVFormat = depthFormat;
        desc.SampleMask = UINT_MAX;
        desc.SampleDesc.Count = 1;
        desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        desc.RasterizerState.DepthClipEnable = TRUE;
        D3D12_RENDER_TARGET_BLEND_DESC& blend = desc.BlendState.RenderTarget[0];
        blend.BlendEnable = TRUE;
        blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blend.BlendOp = D3D12_BLEND_OP_ADD;
        blend.SrcBlendAlpha = D3D12_BLEND_ONE;
        blend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        desc.DepthStencilState.DepthEnable = FALSE;
        if (FAILED(device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline_)))) {
            return Fail(error, "UI pipeline state", nullptr);
        }

        D3D12_DESCRIPTOR_HEAP_DESC heapDescription{};
        heapDescription.NumDescriptors = kMaxTextures;
        heapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (FAILED(device_->CreateDescriptorHeap(&heapDescription, IID_PPV_ARGS(&srvHeap_)))) {
            return Fail(error, "UI descriptor heap", nullptr);
        }
        srvSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        for (auto& buffer : vertexBuffers_) {
            D3D12_HEAP_PROPERTIES heap{};
            heap.Type = D3D12_HEAP_TYPE_UPLOAD;
            D3D12_RESOURCE_DESC bufferDesc{};
            bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            bufferDesc.Width = static_cast<UINT64>(kMaxVertices) * sizeof(SpriteVertex);
            bufferDesc.Height = 1;
            bufferDesc.DepthOrArraySize = 1;
            bufferDesc.MipLevels = 1;
            bufferDesc.SampleDesc.Count = 1;
            bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            if (FAILED(device_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                        IID_PPV_ARGS(&buffer.resource))) ||
                FAILED(buffer.resource->Map(0, nullptr, reinterpret_cast<void**>(&buffer.mapped)))) {
                return Fail(error, "UI vertex buffer", nullptr);
            }
        }
        return true;
    }

    // (Re)binds a texture to a slot. The caller keeps the resource alive and not in use by the GPU.
    void SetTexture(UINT slot, ID3D12Resource* texture)
    {
        if (slot >= kMaxTextures) {
            return;
        }
        const D3D12_RESOURCE_DESC desc = texture->GetDesc();
        D3D12_SHADER_RESOURCE_VIEW_DESC view{};
        view.Format = desc.Format;
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view.Texture2D.MipLevels = desc.MipLevels;
        D3D12_CPU_DESCRIPTOR_HANDLE handle = srvHeap_->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<SIZE_T>(slot) * srvSize_;
        device_->CreateShaderResourceView(texture, &view, handle);
    }

    // Records draw commands. Geometry beyond kMaxVertices is dropped.
    void Draw(ID3D12GraphicsCommandList* commandList, UINT frameIndex, const SpriteBatch& batch,
              const ViewportLayout& layout)
    {
        const auto& vertices = batch.Vertices();
        if (vertices.empty()) {
            return;
        }
        FrameBuffer& frame = vertexBuffers_[frameIndex % kFrames];
        const UINT count = static_cast<UINT>(std::min<size_t>(vertices.size(), kMaxVertices));
        std::memcpy(frame.mapped, vertices.data(), static_cast<size_t>(count) * sizeof(SpriteVertex));

        const Rect& vp = layout.Viewport();
        const D3D12_VIEWPORT viewport{vp.x, vp.y, vp.w, vp.h, 0.0f, 1.0f};
        const D3D12_RECT scissor{static_cast<LONG>(vp.x), static_cast<LONG>(vp.y),
                                 static_cast<LONG>(vp.x + vp.w), static_cast<LONG>(vp.y + vp.h)};
        const float constants[4] = {2.0f / layout.VirtualWidth(), -2.0f / layout.VirtualHeight(), 0.0f, 0.0f};

        commandList->SetPipelineState(pipeline_.Get());
        commandList->SetGraphicsRootSignature(rootSignature_.Get());
        ID3D12DescriptorHeap* heaps[] = {srvHeap_.Get()};
        commandList->SetDescriptorHeaps(1, heaps);
        commandList->SetGraphicsRoot32BitConstants(0, 4, constants, 0);
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissor);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        const D3D12_VERTEX_BUFFER_VIEW view{frame.resource->GetGPUVirtualAddress(),
                                            static_cast<UINT>(kMaxVertices * sizeof(SpriteVertex)),
                                            static_cast<UINT>(sizeof(SpriteVertex))};
        commandList->IASetVertexBuffers(0, 1, &view);

        for (const DrawRange& range : batch.Ranges()) {
            if (range.firstVertex >= count || range.texture < 0 || static_cast<UINT>(range.texture) >= kMaxTextures) {
                continue;
            }
            const UINT n = std::min(range.vertexCount, count - range.firstVertex);
            D3D12_GPU_DESCRIPTOR_HANDLE table = srvHeap_->GetGPUDescriptorHandleForHeapStart();
            table.ptr += static_cast<UINT64>(range.texture) * srvSize_;
            commandList->SetGraphicsRootDescriptorTable(1, table);
            commandList->DrawInstanced(n, 1, range.firstVertex, 0);
        }
    }

private:
    struct FrameBuffer {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        uint8_t* mapped = nullptr;
    };

    static bool Fail(std::string* error, const char* what, ID3DBlob* details)
    {
        if (error != nullptr) {
            *error = what;
            if (details != nullptr) {
                error->append(": ");
                error->append(static_cast<const char*>(details->GetBufferPointer()), details->GetBufferSize());
            }
        }
        return false;
    }

    ID3D12Device* device_ = nullptr;
    UINT srvSize_ = 0;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
    std::array<FrameBuffer, kFrames> vertexBuffers_;
};

} // namespace rs2
