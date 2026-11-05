#pragma once

// Lays out text as sprite quads from a glyph table. Independent of how the atlas is built.

#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>

#include "sprite_batch.h"

namespace rs2 {

// Decodes one UTF-8 sequence starting at text[i] and advances i. Malformed bytes decode to U+FFFD.
inline char32_t DecodeUtf8(const std::string& text, size_t* i)
{
    const unsigned char c = static_cast<unsigned char>(text[*i]);
    int extra = 0;
    char32_t code = c;
    if (c >= 0xF0 && c < 0xF8) {
        extra = 3;
        code = c & 0x07;
    } else if (c >= 0xE0) {
        extra = 2;
        code = c & 0x0F;
    } else if (c >= 0xC0) {
        extra = 1;
        code = c & 0x1F;
    } else if (c >= 0x80) {
        ++*i;
        return 0xFFFD;
    }
    ++*i;
    for (int k = 0; k < extra; ++k) {
        if (*i >= text.size() || (static_cast<unsigned char>(text[*i]) & 0xC0) != 0x80) {
            return 0xFFFD;
        }
        code = (code << 6) | (static_cast<unsigned char>(text[*i]) & 0x3F);
        ++*i;
    }
    return code;
}

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
        } else {
            extra_[ch] = glyph;
        }
    }

    bool Has(char32_t ch) const
    {
        if (ch >= kFirst && ch <= kLast) {
            return glyphs_[ch - kFirst].valid;
        }
        return extra_.count(ch) != 0;
    }

    // Falls back to '?' for characters that are not in the atlas.
    const Glyph& Get(char32_t ch) const
    {
        if (ch >= kFirst && ch <= kLast && glyphs_[ch - kFirst].valid) {
            return glyphs_[ch - kFirst];
        }
        const auto it = extra_.find(ch);
        if (it != extra_.end()) {
            return it->second;
        }
        return glyphs_['?' - kFirst];
    }

    float lineHeight = 0;

private:
    Glyph glyphs_[kLast - kFirst + 1];
    std::unordered_map<char32_t, Glyph> extra_;
};

// Adds every non-ASCII code point of `text` to `out` (ASCII is always rasterized).
inline void CollectExtraCodepoints(const std::string& text, std::set<char32_t>* out)
{
    for (size_t i = 0; i < text.size();) {
        const char32_t ch = DecodeUtf8(text, &i);
        if (ch > GlyphTable::kLast) {
            out->insert(ch);
        }
    }
}

struct TextMetrics {
    float width = 0;
    float height = 0;
};

// Measures (and optionally draws) UTF-8 `text`. `scale` multiplies the atlas pixel size, so the
// caller passes ViewportLayout-based scale divided by the atlas rasterization scale.
inline TextMetrics LayoutText(const GlyphTable& table, const std::string& text, float x, float y, float scale,
                              Color color, SpriteBatch* batch)
{
    float penX = x;
    float penY = y;
    float maxX = x;
    for (size_t i = 0; i < text.size();) {
        const char32_t ch = DecodeUtf8(text, &i);
        if (ch == '\n') {
            maxX = penX > maxX ? penX : maxX;
            penX = x;
            penY += table.lineHeight * scale;
            continue;
        }
        const Glyph& g = table.Get(ch);
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
