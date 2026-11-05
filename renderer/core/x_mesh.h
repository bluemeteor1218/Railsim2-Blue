#pragma once

// Text DirectX .x loader (replacement for D3DXLoadMeshFromX on the shipped data).
// Supports Mesh, MeshNormals, MeshTextureCoords, MeshMaterialList and Material
// (also nested in Frame blocks). Frame transforms and animation are ignored.

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include "vertex_format.h"

namespace rs2 {

struct XMaterial {
    float diffuse[4] = {1, 1, 1, 1};
    float power = 0.0f;
    float specular[3] = {0, 0, 0};
    float emissive[3] = {0, 0, 0};
    std::string texture;
};

struct XSubset {
    uint32_t material = 0;
    uint32_t indexStart = 0;
    uint32_t indexCount = 0;
};

struct XMesh {
    std::vector<VtxNX> vertices;
    std::vector<uint32_t> indices; // grouped by subset
    std::vector<XSubset> subsets;
    std::vector<XMaterial> materials;
};

namespace xmesh_detail {

struct Token {
    enum Kind { Word, Number, String, Open, Close, End } kind = End;
    std::string text;
};

class Lexer {
public:
    explicit Lexer(const std::string& s) : s_(s) {}

    Token Next()
    {
        for (;;) {
            while (i_ < s_.size() && (std::isspace(static_cast<unsigned char>(s_[i_])) || s_[i_] == ';' || s_[i_] == ',')) {
                ++i_;
            }
            if (i_ >= s_.size()) {
                return {};
            }
            if (s_[i_] == '#' || (s_[i_] == '/' && i_ + 1 < s_.size() && s_[i_ + 1] == '/')) {
                while (i_ < s_.size() && s_[i_] != '\n') {
                    ++i_;
                }
                continue;
            }
            break;
        }
        const char c = s_[i_];
        Token t;
        if (c == '{') {
            ++i_;
            t.kind = Token::Open;
        } else if (c == '}') {
            ++i_;
            t.kind = Token::Close;
        } else if (c == '"') {
            const size_t end = s_.find('"', i_ + 1);
            const size_t stop = end == std::string::npos ? s_.size() : end;
            t.kind = Token::String;
            t.text = s_.substr(i_ + 1, stop - i_ - 1);
            i_ = stop == s_.size() ? stop : stop + 1;
        } else {
            const size_t start = i_;
            while (i_ < s_.size() && !std::isspace(static_cast<unsigned char>(s_[i_])) && s_[i_] != ';' &&
                   s_[i_] != ',' && s_[i_] != '{' && s_[i_] != '}') {
                ++i_;
            }
            t.text = s_.substr(start, i_ - start);
            const unsigned char f = static_cast<unsigned char>(t.text[0]);
            t.kind = (std::isdigit(f) || f == '-' || f == '+' || f == '.') ? Token::Number : Token::Word;
        }
        return t;
    }

    float Float()
    {
        const Token t = Next();
        return t.kind == Token::Number ? static_cast<float>(std::atof(t.text.c_str())) : 0.0f;
    }
    uint32_t UInt()
    {
        const Token t = Next();
        return t.kind == Token::Number ? static_cast<uint32_t>(std::strtoul(t.text.c_str(), nullptr, 10)) : 0u;
    }

    // Skips to the matching '}' of a block whose '{' was already consumed.
    void SkipBlock()
    {
        int depth = 1;
        while (depth > 0) {
            const Token t = Next();
            if (t.kind == Token::End) {
                return;
            }
            depth += t.kind == Token::Open ? 1 : (t.kind == Token::Close ? -1 : 0);
        }
    }

    // After a block keyword: consumes an optional name and the '{'. Returns the name.
    bool OpenBlock(std::string* name)
    {
        name->clear();
        Token t = Next();
        if (t.kind == Token::Word) {
            *name = t.text;
            t = Next();
        }
        return t.kind == Token::Open;
    }

private:
    const std::string& s_;
    size_t i_ = 0;
};

struct RawMesh {
    std::vector<float> positions; // xyz
    std::vector<std::vector<uint32_t>> faces;
    std::vector<float> normals;
    std::vector<std::vector<uint32_t>> normalFaces;
    std::vector<float> uvs;
    std::vector<uint32_t> faceMaterial;
    std::vector<XMaterial> materials;
};

inline void ReadFaces(Lexer& lx, std::vector<std::vector<uint32_t>>* faces)
{
    const uint32_t count = lx.UInt();
    faces->resize(count);
    for (auto& face : *faces) {
        const uint32_t n = lx.UInt();
        face.resize(n);
        for (uint32_t& index : face) {
            index = lx.UInt();
        }
    }
}

inline XMaterial ReadMaterial(Lexer& lx)
{
    XMaterial m;
    for (float& v : m.diffuse) {
        v = lx.Float();
    }
    m.power = lx.Float();
    for (float& v : m.specular) {
        v = lx.Float();
    }
    for (float& v : m.emissive) {
        v = lx.Float();
    }
    for (;;) {
        const Token t = lx.Next();
        if (t.kind == Token::Close || t.kind == Token::End) {
            break;
        }
        std::string name;
        if (t.kind == Token::Word && t.text == "TextureFilename" && lx.OpenBlock(&name)) {
            const Token file = lx.Next();
            m.texture = file.text;
            lx.SkipBlock();
        } else if (t.kind == Token::Word) {
            if (lx.OpenBlock(&name)) {
                lx.SkipBlock();
            }
        }
    }
    return m;
}

inline void ReadMaterialList(Lexer& lx, RawMesh* raw, std::map<std::string, XMaterial>* named)
{
    const uint32_t materialCount = lx.UInt();
    const uint32_t faceCount = lx.UInt();
    raw->faceMaterial.resize(faceCount);
    for (uint32_t& m : raw->faceMaterial) {
        m = lx.UInt();
    }
    (void)materialCount;
    for (;;) {
        const Token t = lx.Next();
        if (t.kind == Token::Close || t.kind == Token::End) {
            break;
        }
        if (t.kind == Token::Open) { // { name } reference to a top-level Material
            const Token ref = lx.Next();
            lx.Next();
            const auto it = named->find(ref.text);
            raw->materials.push_back(it != named->end() ? it->second : XMaterial());
        } else if (t.kind == Token::Word && t.text == "Material") {
            std::string name;
            if (lx.OpenBlock(&name)) {
                raw->materials.push_back(ReadMaterial(lx));
            }
        }
    }
}

inline void ReadMesh(Lexer& lx, RawMesh* raw, std::map<std::string, XMaterial>* named)
{
    const uint32_t vertexCount = lx.UInt();
    raw->positions.resize(static_cast<size_t>(vertexCount) * 3);
    for (float& v : raw->positions) {
        v = lx.Float();
    }
    ReadFaces(lx, &raw->faces);
    for (;;) {
        const Token t = lx.Next();
        if (t.kind == Token::Close || t.kind == Token::End) {
            break;
        }
        if (t.kind != Token::Word) {
            continue;
        }
        std::string name;
        if (!lx.OpenBlock(&name)) {
            continue;
        }
        if (t.text == "MeshNormals") {
            const uint32_t n = lx.UInt();
            raw->normals.resize(static_cast<size_t>(n) * 3);
            for (float& v : raw->normals) {
                v = lx.Float();
            }
            ReadFaces(lx, &raw->normalFaces);
            lx.SkipBlock();
        } else if (t.text == "MeshTextureCoords") {
            const uint32_t n = lx.UInt();
            raw->uvs.resize(static_cast<size_t>(n) * 2);
            for (float& v : raw->uvs) {
                v = lx.Float();
            }
            lx.SkipBlock();
        } else if (t.text == "MeshMaterialList") {
            ReadMaterialList(lx, raw, named);
        } else {
            lx.SkipBlock();
        }
    }
}

inline void Normalize(float v[3])
{
    const float len = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len > 0.0f) {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}

// Flattens one parsed mesh into an indexed vertex list (one vertex per face corner).
inline void Append(const RawMesh& raw, XMesh* out)
{
    const uint32_t materialBase = static_cast<uint32_t>(out->materials.size());
    if (raw.materials.empty()) {
        out->materials.push_back(XMaterial());
    } else {
        out->materials.insert(out->materials.end(), raw.materials.begin(), raw.materials.end());
    }
    const size_t vertexCount = raw.positions.size() / 3;
    const bool haveNormals = raw.normalFaces.size() == raw.faces.size() && !raw.normals.empty();
    const bool haveUv = raw.uvs.size() / 2 == vertexCount;

    std::map<uint32_t, std::vector<uint32_t>> byMaterial; // local material -> indices (into out->vertices)
    for (size_t f = 0; f < raw.faces.size(); ++f) {
        const auto& face = raw.faces[f];
        if (face.size() < 3) {
            continue;
        }
        float faceNormal[3] = {0, 1, 0};
        if (!haveNormals) {
            // Newell's method; the file's winding is clockwise in a left-handed system.
            float n[3] = {0, 0, 0};
            for (size_t i = 0; i < face.size(); ++i) {
                const float* a = &raw.positions[face[i] * 3];
                const float* b = &raw.positions[face[(i + 1) % face.size()] * 3];
                n[0] += (a[1] - b[1]) * (a[2] + b[2]);
                n[1] += (a[2] - b[2]) * (a[0] + b[0]);
                n[2] += (a[0] - b[0]) * (a[1] + b[1]);
            }
            Normalize(n);
            faceNormal[0] = -n[0];
            faceNormal[1] = -n[1];
            faceNormal[2] = -n[2];
        }
        const uint32_t base = static_cast<uint32_t>(out->vertices.size());
        for (size_t i = 0; i < face.size(); ++i) {
            const uint32_t pi = face[i];
            if (pi >= vertexCount) {
                return;
            }
            VtxNX v{};
            std::copy_n(&raw.positions[pi * 3], 3, v.position);
            if (haveNormals) {
                const uint32_t ni = raw.normalFaces[f].size() > i ? raw.normalFaces[f][i] : 0;
                if (ni * 3 + 2 < raw.normals.size()) {
                    std::copy_n(&raw.normals[ni * 3], 3, v.normal);
                }
            } else {
                std::copy_n(faceNormal, 3, v.normal);
            }
            v.diffuse = 0xFFFFFFFFu;
            if (haveUv) {
                std::copy_n(&raw.uvs[pi * 2], 2, v.uv);
            }
            out->vertices.push_back(v);
        }
        const uint32_t m = f < raw.faceMaterial.size() ? raw.faceMaterial[f] : 0;
        auto& list = byMaterial[m];
        for (uint32_t i = 1; i + 1 < face.size(); ++i) {
            list.insert(list.end(), {base, base + i, base + i + 1});
        }
    }
    for (auto& entry : byMaterial) {
        XSubset subset;
        subset.material = materialBase + std::min<uint32_t>(entry.first, static_cast<uint32_t>(std::max<size_t>(
                                              raw.materials.size(), 1) - 1));
        subset.indexStart = static_cast<uint32_t>(out->indices.size());
        subset.indexCount = static_cast<uint32_t>(entry.second.size());
        out->indices.insert(out->indices.end(), entry.second.begin(), entry.second.end());
        out->subsets.push_back(subset);
    }
}

} // namespace xmesh_detail

// Returns false if the data is not a text .x file or contains no geometry.
inline bool ParseXMesh(const std::string& text, XMesh* out)
{
    using namespace xmesh_detail;
    *out = XMesh();
    if (text.compare(0, 4, "xof ") != 0 || text.size() < 16 || text.compare(8, 4, "txt ") != 0) {
        return false;
    }
    Lexer lx(text);
    std::map<std::string, XMaterial> named;
    for (;;) {
        const Token t = lx.Next();
        if (t.kind == Token::End) {
            break;
        }
        if (t.kind != Token::Word) {
            continue;
        }
        std::string name;
        if (t.text == "template") {
            if (lx.OpenBlock(&name)) {
                lx.SkipBlock();
            }
        } else if (t.text == "Mesh") {
            if (lx.OpenBlock(&name)) {
                RawMesh raw;
                ReadMesh(lx, &raw, &named);
                Append(raw, out);
            }
        } else if (t.text == "Material") {
            if (lx.OpenBlock(&name)) {
                named[name] = ReadMaterial(lx);
            }
        } else if (t.text == "Frame") {
            lx.OpenBlock(&name); // descend into the frame; children are handled by this loop
        } else if (lx.OpenBlock(&name)) {
            lx.SkipBlock();
        }
    }
    return !out->vertices.empty();
}

} // namespace rs2
