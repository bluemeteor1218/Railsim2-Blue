#pragma once

// Lays out text as sprite quads from a glyph table. Independent of how the atlas is built.

#include <cstdint>
#include <string>

#include "sprite_batch.h"

namespace rs2 {

struct Glyph {
    float u0 = 0, v0 = 0, u1 = 0, v1 = 0; // atlas rectangle, 0..1
    float width = 0, height = 0;          // bitmap size in pixels at the atlas font size
    float offsetX = 0, offsetY = 0;       // bitmap origin relative to the pen / line top
    float advance = 0;                    // pen advance in pixels
    bool valid = false;
};

class GlyphTable {
public:
    static constexpr char32_t kFirst = 32;
    static constexpr char32_t kLast = 126;

    void Set(char32_t ch, const Glyph& glyph)
    {
        if (ch >= kFirst && ch <= kLast) {
            glyphs_[ch - kFirst] = glyph;
        }
    }

    // Falls back to '?' for characters outside the table.
    const Glyph& Get(char32_t ch) const
    {
        if (ch >= kFirst && ch <= kLast && glyphs_[ch - kFirst].valid) {
            return glyphs_[ch - kFirst];
        }
        return glyphs_['?' - kFirst];
    }

    float lineHeight = 0;

private:
    Glyph glyphs_[kLast - kFirst + 1];
};

struct TextMetrics {
    float width = 0;
    float height = 0;
};

// Measures (and optionally draws) `text`. `scale` multiplies the atlas pixel size, so the
// caller passes ViewportLayout-based scale divided by the atlas rasterization scale.
inline TextMetrics LayoutText(const GlyphTable& table, const std::string& text, float x, float y, float scale,
                              Color color, SpriteBatch* batch)
{
    float penX = x;
    float penY = y;
    float maxX = x;
    for (const char ch : text) {
        if (ch == '\n') {
            maxX = penX > maxX ? penX : maxX;
            penX = x;
            penY += table.lineHeight * scale;
            continue;
        }
        const Glyph& g = table.Get(static_cast<unsigned char>(ch));
        if (batch != nullptr && g.width > 0 && g.height > 0) {
            batch->AddQuad({penX + g.offsetX * scale, penY + g.offsetY * scale, g.width * scale, g.height * scale},
                           {g.u0, g.v0, g.u1 - g.u0, g.v1 - g.v0}, color);
        }
        penX += g.advance * scale;
    }
    maxX = penX > maxX ? penX : maxX;
    return {maxX - x, penY + table.lineHeight * scale - y};
}

} // namespace rs2
