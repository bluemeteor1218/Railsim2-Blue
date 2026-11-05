#pragma once

// CPU-side 2D batch: builds textured/colored quads in virtual pixel coordinates and
// groups them into draw ranges by texture. No graphics API dependency.

#include <cstddef>
#include <cstdint>
#include <vector>

#include "viewport_layout.h"

namespace rs2 {

struct SpriteVertex {
    float x, y;
    float u, v;
    uint8_t color[4]; // RGBA
};
static_assert(sizeof(SpriteVertex) == 20, "SpriteVertex must match the UI input layout");

struct DrawRange {
    int texture;
    uint32_t firstVertex;
    uint32_t vertexCount;
};

struct Color {
    uint8_t r = 255, g = 255, b = 255, a = 255;
};

class SpriteBatch {
public:
    void Clear()
    {
        vertices_.clear();
        ranges_.clear();
        texture_ = 0;
    }

    // Texture slot used by following quads. Slot 0 is conventionally a 1x1 white texture.
    void SetTexture(int texture) { texture_ = texture; }

    void AddSolid(const Rect& r, Color c) { AddQuad(r, {0, 0, 0, 0}, c, 0); }

    // Textured quad; uv is in 0..1 texture space.
    void AddQuad(const Rect& r, const Rect& uv, Color c) { AddQuad(r, uv, c, texture_); }

    // Four 1px-scaled edges around r.
    void AddFrame(const Rect& r, float thickness, Color c)
    {
        AddSolid({r.x, r.y, r.w, thickness}, c);
        AddSolid({r.x, r.y + r.h - thickness, r.w, thickness}, c);
        AddSolid({r.x, r.y + thickness, thickness, r.h - 2 * thickness}, c);
        AddSolid({r.x + r.w - thickness, r.y + thickness, thickness, r.h - 2 * thickness}, c);
    }

    const std::vector<SpriteVertex>& Vertices() const { return vertices_; }
    const std::vector<DrawRange>& Ranges() const { return ranges_; }

private:
    void AddQuad(const Rect& r, const Rect& uv, Color c, int texture)
    {
        if (ranges_.empty() || ranges_.back().texture != texture) {
            ranges_.push_back({texture, static_cast<uint32_t>(vertices_.size()), 0});
        }
        const float x0 = r.x, y0 = r.y, x1 = r.x + r.w, y1 = r.y + r.h;
        const float u0 = uv.x, v0 = uv.y, u1 = uv.x + uv.w, v1 = uv.y + uv.h;
        const SpriteVertex tl{x0, y0, u0, v0, {c.r, c.g, c.b, c.a}};
        const SpriteVertex tr{x1, y0, u1, v0, {c.r, c.g, c.b, c.a}};
        const SpriteVertex bl{x0, y1, u0, v1, {c.r, c.g, c.b, c.a}};
        const SpriteVertex br{x1, y1, u1, v1, {c.r, c.g, c.b, c.a}};
        // Two clockwise triangles (top-left origin, y down).
        vertices_.insert(vertices_.end(), {tl, tr, bl, tr, br, bl});
        ranges_.back().vertexCount += 6;
    }

    std::vector<SpriteVertex> vertices_;
    std::vector<DrawRange> ranges_;
    int texture_ = 0;
};

} // namespace rs2
