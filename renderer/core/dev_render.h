#pragma once

// Drop-in replacement for the dev* wrappers of lib/render.h. The legacy code
// called IDirect3DDevice8 directly; here every call is recorded into a
// RenderState, which the Direct3D 12 renderer turns into PSOs and constants.
// D3D8 constant values are reproduced so ported call sites compile unchanged.

#include <cstdint>
#include <cstring>

#include "render_state.h"

namespace rs2 {
namespace d3d8 {

enum : uint32_t {
    D3DBLEND_ZERO = 1, D3DBLEND_ONE = 2, D3DBLEND_SRCCOLOR = 3, D3DBLEND_INVSRCCOLOR = 4,
    D3DBLEND_SRCALPHA = 5, D3DBLEND_INVSRCALPHA = 6, D3DBLEND_DESTCOLOR = 9,
};
enum : uint32_t { D3DSHADE_FLAT = 1, D3DSHADE_GOURAUD = 2 };
enum : uint32_t { D3DCMP_NEVER = 1, D3DCMP_ALWAYS = 8, D3DCMP_GREATEREQUAL = 7 };

} // namespace d3d8

class DevRender {
public:
    RenderState& State() { return state_; }
    const RenderState& State() const { return state_; }

    void SetLighting(bool f) { state_.SetLighting(f); }
    bool GetLighting() const { return state_.GetLighting(); }
    void SetLight(int n, bool f) { state_.SetLight(n, f); }
    void SetAmbient(uint32_t argb) { state_.SetAmbient(argb); }
    void SetMaterial(const Material& m) { state_.SetMaterial(m); }
    void ResetMaterial() { state_.ResetMaterial(); }
    void SetSpecular(bool f) { state_.SetSpecular(f); }
    void SetCulling(bool f) { state_.SetCulling(f); }
    void SetShading(uint32_t mode) { state_.SetShadingFlat(mode == d3d8::D3DSHADE_FLAT); }
    void SetZRead(bool f) { state_.SetZRead(f); }
    bool GetZRead() const { return state_.GetZRead(); }
    void SetZWrite(bool f) { state_.SetZWrite(f); }
    bool GetZWrite() const { return state_.GetZWrite(); }
    void SetNormalize(bool) {} // normals are always renormalized in the vertex shader

    // The legacy code only ever uses D3DCMP_GREATEREQUAL; other functions are treated as "pass".
    void SetAlphaTest(bool f, uint32_t func, uint32_t ref)
    {
        state_.SetAlphaTest(f && func == d3d8::D3DCMP_GREATEREQUAL, ref);
    }
    void AlphaPass() { SetAlphaTest(false, d3d8::D3DCMP_ALWAYS, 0); }
    void AlphaGreat(uint32_t ref) { SetAlphaTest(true, d3d8::D3DCMP_GREATEREQUAL, ref); }

    void SetBlend(bool flag, uint32_t src, uint32_t dst)
    {
        if (!flag) {
            state_.BlendNone();
            return;
        }
        state_.SetBlend(true, ToFactor(src), ToFactor(dst));
    }

    // Vertex fog and pixel fog both become the same linear pixel-shader fog.
    void SetFog(bool flag, uint32_t argb, float start, float end) { state_.SetFog(flag, argb, start, end); }
    void SetPixelFog(bool flag, uint32_t argb, float start, float end) { state_.SetFog(flag, argb, start, end); }

    // devSetTexture(0, ...) and the stage-1 light map / environment map modes.
    void SetTexture(bool textured) { state_.SetTextured(textured); }
    void SetLightMap(bool f) { state_.SetSecondTexture(f ? SecondTexture::Modulate : SecondTexture::None); }
    void SetEnvMap(bool f) { state_.SetSecondTexture(f ? SecondTexture::EnvMap : SecondTexture::None); }

private:
    static BlendFactor ToFactor(uint32_t d3d8Value)
    {
        switch (d3d8Value) {
        case d3d8::D3DBLEND_ZERO: return BlendFactor::Zero;
        case d3d8::D3DBLEND_SRCCOLOR: return BlendFactor::SrcColor;
        case d3d8::D3DBLEND_INVSRCCOLOR: return BlendFactor::InvSrcColor;
        case d3d8::D3DBLEND_SRCALPHA: return BlendFactor::SrcAlpha;
        case d3d8::D3DBLEND_INVSRCALPHA: return BlendFactor::InvSrcAlpha;
        case d3d8::D3DBLEND_DESTCOLOR: return BlendFactor::DestColor;
        default: return BlendFactor::One;
        }
    }

    RenderState state_;
};

} // namespace rs2
