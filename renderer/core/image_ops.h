#pragma once

// CPU-side image helpers that do not depend on any graphics API.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace rs2 {

struct Image {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> rgba; // width * height * 4, tightly packed
};

// Builds the full mip chain (level 0 is a copy of the source) using a 2x2 box
// filter. Odd sizes clamp at the edge. Stops at 1x1.
inline std::vector<Image> GenerateMipChain(const Image& source)
{
    std::vector<Image> chain;
    if (source.width == 0 || source.height == 0 || source.rgba.size() != static_cast<size_t>(source.width) * source.height * 4) {
        return chain;
    }
    chain.push_back(source);
    while (chain.back().width > 1 || chain.back().height > 1) {
        const Image& src = chain.back();
        Image dst;
        dst.width = std::max(1u, src.width / 2);
        dst.height = std::max(1u, src.height / 2);
        dst.rgba.resize(static_cast<size_t>(dst.width) * dst.height * 4);
        for (uint32_t y = 0; y < dst.height; ++y) {
            const uint32_t y0 = std::min(y * 2, src.height - 1);
            const uint32_t y1 = std::min(y * 2 + 1, src.height - 1);
            for (uint32_t x = 0; x < dst.width; ++x) {
                const uint32_t x0 = std::min(x * 2, src.width - 1);
                const uint32_t x1 = std::min(x * 2 + 1, src.width - 1);
                for (uint32_t c = 0; c < 4; ++c) {
                    const uint32_t sum = src.rgba[(static_cast<size_t>(y0) * src.width + x0) * 4 + c] +
                                         src.rgba[(static_cast<size_t>(y0) * src.width + x1) * 4 + c] +
                                         src.rgba[(static_cast<size_t>(y1) * src.width + x0) * 4 + c] +
                                         src.rgba[(static_cast<size_t>(y1) * src.width + x1) * 4 + c];
                    dst.rgba[(static_cast<size_t>(y) * dst.width + x) * 4 + c] = static_cast<uint8_t>((sum + 2) / 4);
                }
            }
        }
        chain.push_back(std::move(dst));
    }
    return chain;
}

// 8x8-cell checkerboard used when an asset cannot be loaded.
inline Image MakeCheckerboard(uint32_t size, uint32_t cell)
{
    Image image;
    image.width = image.height = size;
    image.rgba.resize(static_cast<size_t>(size) * size * 4);
    for (uint32_t y = 0; y < size; ++y) {
        for (uint32_t x = 0; x < size; ++x) {
            const uint8_t v = (((x / cell) + (y / cell)) & 1) ? 230 : 60;
            uint8_t* p = &image.rgba[(static_cast<size_t>(y) * size + x) * 4];
            p[0] = p[1] = p[2] = v;
            p[3] = 255;
        }
    }
    return image;
}

// Radial falloff (white at the center, black at the edge): a stand-in light map.
inline Image MakeRadialGradient(uint32_t size)
{
    Image image;
    image.width = image.height = size;
    image.rgba.resize(static_cast<size_t>(size) * size * 4);
    const float half = static_cast<float>(size) * 0.5f;
    for (uint32_t y = 0; y < size; ++y) {
        for (uint32_t x = 0; x < size; ++x) {
            const float dx = (static_cast<float>(x) + 0.5f - half) / half;
            const float dy = (static_cast<float>(y) + 0.5f - half) / half;
            float t = 1.0f - std::sqrt(dx * dx + dy * dy);
            t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
            const uint8_t v = static_cast<uint8_t>(255.0f * t + 0.5f);
            uint8_t* p = &image.rgba[(static_cast<size_t>(y) * size + x) * 4];
            p[0] = p[1] = p[2] = v;
            p[3] = 255;
        }
    }
    return image;
}

} // namespace rs2
