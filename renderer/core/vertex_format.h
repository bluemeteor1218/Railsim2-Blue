#pragma once

// API-neutral description of the legacy vertex formats in lib/vertex.h.
// Memory layout matches the D3D8 FVF order: position, normal, diffuse, texcoords.
// Pre-transformed (XYZRHW) formats are not listed; they belong to the UI path.

#include <cstddef>
#include <cstdint>

namespace rs2 {

enum class VertexFormat : uint8_t { L, LX, LX2, N, NX, NX2 };

// Legacy D3DFVF bit values (only those the game uses).
constexpr uint32_t kFvfXyz = 0x002;
constexpr uint32_t kFvfXyzRhw = 0x004;
constexpr uint32_t kFvfNormal = 0x010;
constexpr uint32_t kFvfDiffuse = 0x040;
constexpr uint32_t kFvfTex1 = 0x100;
constexpr uint32_t kFvfTex2 = 0x200;

struct VertexLayout {
    bool hasNormal;
    uint8_t texCoordSets;
    uint8_t normalOffset;
    uint8_t colorOffset;
    uint8_t texOffset;
    uint8_t stride;
};

constexpr VertexLayout GetLayout(VertexFormat format)
{
    switch (format) {
    case VertexFormat::L: return {false, 0, 0, 12, 0, 16};
    case VertexFormat::LX: return {false, 1, 0, 12, 16, 24};
    case VertexFormat::LX2: return {false, 2, 0, 12, 16, 32};
    case VertexFormat::N: return {true, 0, 12, 24, 0, 28};
    case VertexFormat::NX: return {true, 1, 12, 24, 28, 36};
    case VertexFormat::NX2: return {true, 2, 12, 24, 28, 44};
    }
    return {false, 0, 0, 12, 0, 16};
}

// Maps a legacy FVF code to a format. Returns false for unsupported codes (including XYZRHW).
inline bool FromFvf(uint32_t fvf, VertexFormat* out)
{
    struct Entry {
        uint32_t fvf;
        VertexFormat format;
    };
    static const Entry table[] = {
        {kFvfXyz | kFvfDiffuse, VertexFormat::L},
        {kFvfXyz | kFvfDiffuse | kFvfTex1, VertexFormat::LX},
        {kFvfXyz | kFvfDiffuse | kFvfTex2, VertexFormat::LX2},
        {kFvfXyz | kFvfNormal | kFvfDiffuse, VertexFormat::N},
        {kFvfXyz | kFvfNormal | kFvfDiffuse | kFvfTex1, VertexFormat::NX},
        {kFvfXyz | kFvfNormal | kFvfDiffuse | kFvfTex2, VertexFormat::NX2},
    };
    for (const Entry& e : table) {
        if (e.fvf == fvf) {
            *out = e.format;
            return true;
        }
    }
    return false;
}

// Binary-compatible with the legacy VTX_NX (diffuse is D3DCOLOR 0xAARRGGBB).
struct VtxNX {
    float position[3];
    float normal[3];
    uint32_t diffuse;
    float uv[2];
};
static_assert(sizeof(VtxNX) == 36, "VtxNX must match VTX_NX");

} // namespace rs2
