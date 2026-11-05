#pragma once

// Direct3D 12 pipeline setup for the fixed-function replacement: HLSL shaders,
// the shared root signature, and a PSO cache keyed by rs2::PipelineKey.

#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <string>
#include <unordered_map>

#include "render_state.h"

namespace rs2 {

// Root parameters: 0 = ObjectConstants (b0, root constants), 1 = FrameConstants (b1, root CBV), 2 = SRV table.
struct ObjectConstants {
    float mvp[16];
    float world[16];
    Material material;
};
static_assert(sizeof(ObjectConstants) == 176, "ObjectConstants must match the HLSL cbuffer");

struct Vertex {
    float position[3];
    float normal[3];
    float uv[2];
    uint8_t color[4]; // RGBA
};
static_assert(sizeof(Vertex) == 36, "Vertex must match the input layout");

// Matrices are stored row-major and used as row vectors, like D3D8/D3DX.
inline const char kSceneShader[] = R"hlsl(
cbuffer Object : register(b0)
{
    row_major float4x4 mvp;
    row_major float4x4 world;
    float4 matDiffuse;
    float4 matAmbient;
    float4 matEmissive;
};

struct Light
{
    float4 direction;
    float4 diffuse;
};

cbuffer Frame : register(b1)
{
    float4 ambient;
    float4 fogColor;
    float2 fogStartEnd;
    float alphaRef;
    uint lightMask;
    Light lights[4];
};

Texture2D baseTexture : register(t0);
SamplerState baseSampler : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD1;
    float viewDepth : TEXCOORD0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), mvp);
    output.viewDepth = output.position.w;
    output.uv = input.uv;
    output.color = input.color;
#if LIGHTING
    float3 n = normalize(mul(float4(input.normal, 0.0f), world).xyz);
    float3 lit = matEmissive.rgb + ambient.rgb * matAmbient.rgb;
    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        if (lightMask & (1u << i))
        {
            lit += lights[i].diffuse.rgb * saturate(dot(n, -lights[i].direction.xyz));
        }
    }
    output.color.rgb = saturate(input.color.rgb * matDiffuse.rgb * lit);
    output.color.a = input.color.a * matDiffuse.a;
#endif
    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    float4 color = input.color;
#if TEXTURED
    color *= baseTexture.Sample(baseSampler, input.uv);
#endif
#if ALPHATEST
    clip(color.a - alphaRef);
#endif
#if FOG
    float f = saturate((fogStartEnd.y - input.viewDepth) / (fogStartEnd.y - fogStartEnd.x));
    color.rgb = lerp(fogColor.rgb, color.rgb, f);
#endif
    return color;
}
)hlsl";

inline D3D12_BLEND ToD3D12(BlendFactor f)
{
    switch (f) {
    case BlendFactor::Zero: return D3D12_BLEND_ZERO;
    case BlendFactor::One: return D3D12_BLEND_ONE;
    case BlendFactor::SrcColor: return D3D12_BLEND_SRC_COLOR;
    case BlendFactor::InvSrcColor: return D3D12_BLEND_INV_SRC_COLOR;
    case BlendFactor::SrcAlpha: return D3D12_BLEND_SRC_ALPHA;
    case BlendFactor::InvSrcAlpha: return D3D12_BLEND_INV_SRC_ALPHA;
    case BlendFactor::DestColor: return D3D12_BLEND_DEST_COLOR;
    }
    return D3D12_BLEND_ONE;
}

class PipelineCache {
public:
    bool Initialize(ID3D12Device* device, DXGI_FORMAT renderTargetFormat, DXGI_FORMAT depthFormat, std::string* error)
    {
        device_ = device;
        renderTargetFormat_ = renderTargetFormat;
        depthFormat_ = depthFormat;

        D3D12_DESCRIPTOR_RANGE range{};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors = 1;
        range.BaseShaderRegister = 0;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER parameters[3]{};
        parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        parameters[0].Constants.ShaderRegister = 0;
        parameters[0].Constants.Num32BitValues = sizeof(ObjectConstants) / 4;
        parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        parameters[1].Descriptor.ShaderRegister = 1;
        parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameters[2].DescriptorTable.NumDescriptorRanges = 1;
        parameters[2].DescriptorTable.pDescriptorRanges = &range;
        parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // Trilinear + 16x anisotropic: the main sharpness win over the D3D8 point/bilinear default.
        D3D12_STATIC_SAMPLER_DESC sampler{};
        sampler.Filter = D3D12_FILTER_ANISOTROPIC;
        sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.MaxAnisotropy = 16;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC description{};
        description.NumParameters = 3;
        description.pParameters = parameters;
        description.NumStaticSamplers = 1;
        description.pStaticSamplers = &sampler;
        description.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Microsoft::WRL::ComPtr<ID3DBlob> blob;
        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        HRESULT result = D3D12SerializeRootSignature(&description, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &errors);
        if (FAILED(result)) {
            return Fail(error, "D3D12SerializeRootSignature", errors.Get());
        }
        result = device_->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
                                              IID_PPV_ARGS(&rootSignature_));
        if (FAILED(result)) {
            return Fail(error, "CreateRootSignature", nullptr);
        }
        return true;
    }

    ID3D12RootSignature* RootSignature() const { return rootSignature_.Get(); }
    size_t Size() const { return cache_.size(); }

    ID3D12PipelineState* Get(const PipelineKey& key, std::string* error)
    {
        const auto found = cache_.find(key);
        if (found != cache_.end()) {
            return found->second.Get();
        }

        Microsoft::WRL::ComPtr<ID3DBlob> vs, ps;
        if (!Compile(key, "VSMain", "vs_5_0", &vs, error) || !Compile(key, "PSMain", "ps_5_0", &ps, error)) {
            return nullptr;
        }

        static const D3D12_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
        desc.pRootSignature = rootSignature_.Get();
        desc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
        desc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
        desc.InputLayout = {layout, static_cast<UINT>(sizeof(layout) / sizeof(layout[0]))};
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = renderTargetFormat_;
        desc.DSVFormat = depthFormat_;
        desc.SampleMask = UINT_MAX;
        desc.SampleDesc.Count = 1;

        desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        desc.RasterizerState.CullMode = key.cull == CullMode::Back ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE;
        desc.RasterizerState.DepthClipEnable = TRUE;

        D3D12_RENDER_TARGET_BLEND_DESC& blend = desc.BlendState.RenderTarget[0];
        blend.BlendEnable = key.blend.enable;
        blend.SrcBlend = ToD3D12(key.blend.src);
        blend.DestBlend = ToD3D12(key.blend.dst);
        blend.BlendOp = D3D12_BLEND_OP_ADD;
        blend.SrcBlendAlpha = D3D12_BLEND_ONE;
        blend.DestBlendAlpha = D3D12_BLEND_ZERO;
        blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        desc.DepthStencilState.DepthEnable = key.depthRead;
        desc.DepthStencilState.DepthWriteMask = key.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        // CompareFunc is declared in the same order as D3D12_COMPARISON_FUNC, offset by one.
        desc.DepthStencilState.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(static_cast<int>(key.depthFunc) + 1);

        Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
        const HRESULT result = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
        if (FAILED(result)) {
            Fail(error, "CreateGraphicsPipelineState", nullptr);
            return nullptr;
        }
        return cache_.emplace(key, std::move(pso)).first->second.Get();
    }

private:
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

    bool Compile(const PipelineKey& key, const char* entry, const char* target,
                 Microsoft::WRL::ComPtr<ID3DBlob>* out, std::string* error)
    {
        const D3D_SHADER_MACRO macros[] = {
            {"LIGHTING", key.lighting ? "1" : "0"},
            {"FOG", key.fog ? "1" : "0"},
            {"ALPHATEST", key.alphaTest ? "1" : "0"},
            {"TEXTURED", key.textured ? "1" : "0"},
            {nullptr, nullptr},
        };
        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        const HRESULT result = D3DCompile(kSceneShader, sizeof(kSceneShader) - 1, "scene.hlsl", macros, nullptr,
                                          entry, target, D3DCOMPILE_ENABLE_STRICTNESS, 0, out->GetAddressOf(),
                                          &errors);
        if (FAILED(result)) {
            return Fail(error, entry, errors.Get());
        }
        return true;
    }

    ID3D12Device* device_ = nullptr;
    DXGI_FORMAT renderTargetFormat_ = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT depthFormat_ = DXGI_FORMAT_UNKNOWN;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    std::unordered_map<PipelineKey, Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineKeyHasher> cache_;
};

} // namespace rs2
