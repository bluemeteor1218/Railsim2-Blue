#pragma once

// API-neutral replacement for the legacy dev* render-state wrappers
// (lib/render.h). Setters mirror the old helpers one-to-one so call sites can
// be ported mechanically. The state splits into:
//   * PipelineKey  - values that select a Direct3D 12 pipeline state object
//   * FrameConstants - values uploaded to a constant buffer for HLSL

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "vertex_format.h"

namespace rs2 {

enum class CompareFunc : uint8_t { Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always };
enum class BlendFactor : uint8_t { Zero, One, SrcColor, InvSrcColor, SrcAlpha, InvSrcAlpha, DestColor };
enum class CullMode : uint8_t { None, Back };

// Stage 1 of the legacy fixed-function pipeline (texture coordinate set 1).
//   Modulate  : devTEX_LIGHTMAP  (D3DTOP_MODULATE)   result = current * tex1
//   AddSmooth : devTEX_ENVMAP    (D3DTOP_ADDSMOOTH)  result = current + tex1 - current * tex1
enum class SecondTexture : uint8_t { None, Modulate, AddSmooth, EnvMap };

// Mirrors the legacy devBLEND_* presets.
struct BlendState {
    bool enable = false;
    BlendFactor src = BlendFactor::One;
    BlendFactor dst = BlendFactor::Zero;

    bool operator==(const BlendState& o) const { return enable == o.enable && src == o.src && dst == o.dst; }
};

// Everything that requires a different PSO.
struct PipelineKey {
    BlendState blend;
    CullMode cull = CullMode::Back;
    CompareFunc depthFunc = CompareFunc::LessEqual;
    bool depthRead = true;
    bool depthWrite = true;
    bool alphaTest = false;
    bool lighting = false;
    bool fog = false;
    bool textured = false;
    VertexFormat vertexFormat = VertexFormat::NX;
    SecondTexture secondTexture = SecondTexture::None;
    bool specular = false;
    bool flatShading = false;

    bool operator==(const PipelineKey& o) const
    {
        return blend == o.blend && cull == o.cull && depthFunc == o.depthFunc && depthRead == o.depthRead &&
               depthWrite == o.depthWrite && alphaTest == o.alphaTest && lighting == o.lighting && fog == o.fog &&
               textured == o.textured && vertexFormat == o.vertexFormat && secondTexture == o.secondTexture &&
               specular == o.specular && flatShading == o.flatShading;
    }
    bool operator!=(const PipelineKey& o) const { return !(*this == o); }

    // FNV-1a over the individual fields (no reliance on struct padding).
    size_t Hash() const
    {
        const uint8_t bytes[] = {
            static_cast<uint8_t>(blend.enable), static_cast<uint8_t>(blend.src), static_cast<uint8_t>(blend.dst),
            static_cast<uint8_t>(cull), static_cast<uint8_t>(depthFunc), static_cast<uint8_t>(depthRead),
            static_cast<uint8_t>(depthWrite), static_cast<uint8_t>(alphaTest), static_cast<uint8_t>(lighting),
            static_cast<uint8_t>(fog), static_cast<uint8_t>(textured), static_cast<uint8_t>(vertexFormat),
            static_cast<uint8_t>(secondTexture), static_cast<uint8_t>(specular), static_cast<uint8_t>(flatShading)};
        uint64_t h = 1469598103934665603ull;
        for (uint8_t b : bytes) {
            h = (h ^ b) * 1099511628211ull;
        }
        return static_cast<size_t>(h);
    }
};

struct PipelineKeyHasher {
    size_t operator()(const PipelineKey& k) const { return k.Hash(); }
};

// 16-byte aligned groups so the layout matches an HLSL cbuffer exactly.
constexpr int kMaxLights = 4;

// Directional light (the only kind the legacy renderer needs).
struct DirectionalLight {
    float direction[4] = {0, -1, 0, 0}; // world space, direction the light travels
    float diffuse[4] = {1, 1, 1, 1};
};

struct FrameConstants {
    float ambient[4] = {0, 0, 0, 1};
    float fogColor[4] = {0, 0, 0, 1};
    float fogStartEnd[2] = {0.0f, 1.0f};
    float alphaRef = 0.0f; // 0..1
    uint32_t lightMask = 0; // bit n = light n enabled
    DirectionalLight lights[kMaxLights];
    float viewDirection[4] = {0, 0, 1, 0}; // world-space camera forward (infinite-viewer specular)
};
static_assert(sizeof(FrameConstants) == 192, "FrameConstants must match the HLSL cbuffer layout");

// Mirrors D3DMATERIAL8. Diffuse multiplies the vertex color; specular.w is the power.
struct Material {
    float diffuse[4] = {1, 1, 1, 1};
    float ambient[4] = {1, 1, 1, 1};
    float emissive[4] = {0, 0, 0, 0};
    float specular[4] = {0, 0, 0, 16};
};
static_assert(sizeof(Material) == 64, "Material is uploaded as 16 root constants");

// Unpacks a legacy D3DCOLOR (0xAARRGGBB) into RGBA floats.
inline void UnpackColor(uint32_t argb, float out[4])
{
    out[0] = static_cast<float>((argb >> 16) & 0xFF) / 255.0f;
    out[1] = static_cast<float>((argb >> 8) & 0xFF) / 255.0f;
    out[2] = static_cast<float>(argb & 0xFF) / 255.0f;
    out[3] = static_cast<float>((argb >> 24) & 0xFF) / 255.0f;
}

class RenderState {
public:
    const PipelineKey& Pipeline() const { return pipeline_; }
    const FrameConstants& Constants() const { return constants_; }

    // devSetBlend and the devBLEND_* presets. Factors are ignored when disabled,
    // so equivalent states compare equal and share a PSO.
    void SetBlend(bool enable, BlendFactor src, BlendFactor dst)
    {
        pipeline_.blend = enable ? BlendState{true, src, dst} : BlendState{};
    }
    void BlendNone() { SetBlend(false, BlendFactor::One, BlendFactor::Zero); }
    void BlendAlpha() { SetBlend(true, BlendFactor::SrcAlpha, BlendFactor::InvSrcAlpha); }
    void BlendRgb() { SetBlend(true, BlendFactor::SrcColor, BlendFactor::InvSrcColor); }
    void BlendAdd() { SetBlend(true, BlendFactor::One, BlendFactor::One); }
    void BlendAdd2() { SetBlend(true, BlendFactor::SrcAlpha, BlendFactor::One); }
    void BlendMod() { SetBlend(true, BlendFactor::DestColor, BlendFactor::Zero); }

    void SetCulling(bool enable) { pipeline_.cull = enable ? CullMode::Back : CullMode::None; }
    void SetZRead(bool enable) { pipeline_.depthRead = enable; }
    void SetZWrite(bool enable) { pipeline_.depthWrite = enable; }
    void SetLighting(bool enable) { pipeline_.lighting = enable; }
    void SetTextured(bool enable) { pipeline_.textured = enable; }
    void SetVertexFormat(VertexFormat format) { pipeline_.vertexFormat = format; }
    void SetSecondTexture(SecondTexture mode) { pipeline_.secondTexture = mode; }
    void SetSpecular(bool enable) { pipeline_.specular = enable; }
    void SetShadingFlat(bool enable) { pipeline_.flatShading = enable; }
    void SetViewDirection(float x, float y, float z)
    {
        const float length = std::sqrt(x * x + y * y + z * z);
        const float inv = length > 0.0f ? 1.0f / length : 0.0f;
        constants_.viewDirection[0] = x * inv;
        constants_.viewDirection[1] = y * inv;
        constants_.viewDirection[2] = z * inv;
    }
    bool GetLighting() const { return pipeline_.lighting; }
    bool GetZRead() const { return pipeline_.depthRead; }
    bool GetZWrite() const { return pipeline_.depthWrite; }

    // The legacy code only uses GREATEREQUAL (devALPHA_GREAT), ref in 0..255.
    void SetAlphaTest(bool enable, uint32_t ref)
    {
        pipeline_.alphaTest = enable;
        constants_.alphaRef = enable ? static_cast<float>(ref > 255 ? 255 : ref) / 255.0f : 0.0f;
    }

    void SetAmbient(uint32_t argb) { UnpackColor(argb, constants_.ambient); }

    void SetLight(int index, bool enable)
    {
        if (index < 0 || index >= 32) {
            return;
        }
        const uint32_t bit = 1u << index;
        constants_.lightMask = enable ? (constants_.lightMask | bit) : (constants_.lightMask & ~bit);
    }

    // devSetLight's D3DLIGHT8 setup for directional lights. Direction is normalized here.
    void SetDirectionalLight(int index, float dx, float dy, float dz, uint32_t diffuseArgb)
    {
        if (index < 0 || index >= kMaxLights) {
            return;
        }
        const float length = std::sqrt(dx * dx + dy * dy + dz * dz);
        const float inv = length > 0.0f ? 1.0f / length : 0.0f;
        DirectionalLight& light = constants_.lights[index];
        light.direction[0] = dx * inv;
        light.direction[1] = dy * inv;
        light.direction[2] = dz * inv;
        light.direction[3] = 0.0f;
        UnpackColor(diffuseArgb, light.diffuse);
    }

    const Material& GetMaterial() const { return material_; }
    void SetMaterial(const Material& material) { material_ = material; }
    void ResetMaterial() { material_ = Material(); }

    // Vertex and pixel fog both collapse to one linear pixel-shader fog.
    void SetFog(bool enable, uint32_t argb, float start, float end)
    {
        pipeline_.fog = enable;
        if (enable) {
            UnpackColor(argb, constants_.fogColor);
            constants_.fogStartEnd[0] = start;
            constants_.fogStartEnd[1] = end;
        }
    }

private:
    PipelineKey pipeline_;
    FrameConstants constants_;
    Material material_;
};

} // namespace rs2
