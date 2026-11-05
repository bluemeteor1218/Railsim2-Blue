#pragma once

// Rasterizes printable ASCII with GDI (grayscale anti-aliasing) into an RGBA atlas
// (white color, glyph coverage in alpha) and fills a GlyphTable for text_layout.h.

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <set>
#include <vector>

#include "image_ops.h"
#include "text_layout.h"

namespace rs2 {

constexpr uint32_t kAtlasSize = 512;
constexpr int kGlyphPadding = 1;

inline bool BuildFontAtlasSized(const wchar_t* faceName, int pixelHeight, const std::set<char32_t>& extraChars,
                                uint32_t atlasSize, Image* atlas, GlyphTable* table)
{
    pixelHeight = std::max(pixelHeight, 6);

    HDC dc = CreateCompatibleDC(nullptr);
    if (dc == nullptr) {
        return false;
    }
    HFONT font = CreateFontW(-pixelHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                             faceName);
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = static_cast<LONG>(atlasSize);
    info.bmiHeader.biHeight = -static_cast<LONG>(atlasSize); // top-down
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (font == nullptr || bitmap == nullptr || bits == nullptr) {
        if (font != nullptr) DeleteObject(font);
        if (bitmap != nullptr) DeleteObject(bitmap);
        DeleteDC(dc);
        return false;
    }

    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    HGDIOBJ oldFont = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    GdiFlush();
    std::memset(bits, 0, static_cast<size_t>(atlasSize) * atlasSize * 4);

    TEXTMETRICW metrics{};
    GetTextMetricsW(dc, &metrics);
    const int cellHeight = metrics.tmHeight + kGlyphPadding * 2;
    table->lineHeight = static_cast<float>(metrics.tmHeight);

    int penX = 0;
    int penY = 0;
    bool ok = true;
    const float inv = 1.0f / static_cast<float>(atlasSize);
    std::vector<char32_t> chars;
    for (char32_t ch = GlyphTable::kFirst; ch <= GlyphTable::kLast; ++ch) {
        chars.push_back(ch);
    }
    for (const char32_t ch : extraChars) {
        if (ch > GlyphTable::kLast && ch <= 0xFFFF) {
            chars.push_back(ch);
        }
    }
    for (const char32_t ch : chars) {
        int advance = 0;
        const wchar_t wide = static_cast<wchar_t>(ch);
        GetCharWidth32W(dc, wide, wide, &advance);
        const int cellWidth = advance + kGlyphPadding * 2;
        if (penX + cellWidth > static_cast<int>(atlasSize)) {
            penX = 0;
            penY += cellHeight;
        }
        if (penY + cellHeight > static_cast<int>(atlasSize)) {
            ok = false;
            break;
        }
        TextOutW(dc, penX + kGlyphPadding, penY + kGlyphPadding, &wide, 1);

        Glyph glyph;
        glyph.valid = true;
        glyph.advance = static_cast<float>(advance);
        glyph.width = static_cast<float>(advance);
        glyph.height = static_cast<float>(metrics.tmHeight);
        glyph.u0 = static_cast<float>(penX + kGlyphPadding) * inv;
        glyph.v0 = static_cast<float>(penY + kGlyphPadding) * inv;
        glyph.u1 = static_cast<float>(penX + kGlyphPadding + advance) * inv;
        glyph.v1 = static_cast<float>(penY + kGlyphPadding + metrics.tmHeight) * inv;
        table->Set(ch, glyph);
        penX += cellWidth;
    }
    GdiFlush();

    if (ok) {
        atlas->width = atlas->height = atlasSize;
        atlas->rgba.resize(static_cast<size_t>(atlasSize) * atlasSize * 4);
        const uint8_t* src = static_cast<const uint8_t*>(bits);
        for (size_t i = 0; i < static_cast<size_t>(atlasSize) * atlasSize; ++i) {
            // Grayscale AA: the green channel is the coverage.
            atlas->rgba[i * 4 + 0] = 255;
            atlas->rgba[i * 4 + 1] = 255;
            atlas->rgba[i * 4 + 2] = 255;
            atlas->rgba[i * 4 + 3] = src[i * 4 + 1];
        }
    }

    SelectObject(dc, oldFont);
    SelectObject(dc, oldBitmap);
    DeleteObject(font);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return ok;
}

// Rasterizes ASCII plus `extraChars` (BMP only). The atlas grows (512 -> 2048) until everything fits.
inline bool BuildFontAtlas(const wchar_t* faceName, int pixelHeight, const std::set<char32_t>& extraChars,
                           Image* atlas, GlyphTable* table)
{
    for (uint32_t size = 512; size <= 2048; size *= 2) {
        *table = GlyphTable();
        if (BuildFontAtlasSized(faceName, pixelHeight, extraChars, size, atlas, table)) {
            return true;
        }
    }
    return false;
}

} // namespace rs2
