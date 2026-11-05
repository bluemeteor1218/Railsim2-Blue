#include "../x_mesh.h"

#include <cstdio>
#include <fstream>
#include <filesystem>
#include <iterator>

namespace {

int g_failures = 0;

void Check(bool ok, const char* expr, int line)
{
    if (!ok) {
        std::printf("FAIL line %d: %s\n", line, expr);
        ++g_failures;
    }
}

#define CHECK(expr) Check((expr), #expr, __LINE__)

const char kSample[] =
    "xof 0302txt 0064\n"
    "template Header { <3D82AB43-62DA-11cf-AB39-0020AF71E433> WORD major; }\n"
    "Material Red {\n 1.0;0.0;0.0;1.0;; 8.0; 0.5;0.5;0.5;; 0.1;0.1;0.1;;\n}\n"
    "Frame Root {\n"
    " FrameTransformMatrix { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1;; }\n"
    " Mesh Quad {\n"
    "  4;\n  0.0;0.0;0.0;,\n  0.0;1.0;0.0;,\n  1.0;1.0;0.0;,\n  1.0;0.0;0.0;;\n"
    "  2;\n  3;0,1,2;,\n  3;0,2,3;;\n"
    "  MeshTextureCoords { 4; 0.0;1.0;, 0.0;0.0;, 1.0;0.0;, 1.0;1.0;; }\n"
    "  MeshMaterialList {\n   2; 2; 0, 1;\n"
    "   { Red }\n"
    "   Material { 0.0;1.0;0.0;1.0;; 0.0; 0.0;0.0;0.0;; 0.0;0.0;0.0;;\n"
    "    TextureFilename { \"wall.png\"; }\n   }\n"
    "  }\n"
    " }\n"
    "}\n";

void TestParse()
{
    rs2::XMesh mesh;
    CHECK(rs2::ParseXMesh(kSample, &mesh));
    CHECK(mesh.vertices.size() == 6);
    CHECK(mesh.indices.size() == 6);
    CHECK(mesh.materials.size() == 2);
    CHECK(mesh.subsets.size() == 2);
    CHECK(mesh.materials[0].diffuse[0] == 1.0f && mesh.materials[0].power == 8.0f);
    CHECK(mesh.materials[1].texture == "wall.png");
    CHECK(mesh.subsets[0].material == 0 && mesh.subsets[1].material == 1);
    CHECK(mesh.subsets[1].indexStart == 3);
    // The quad lies in z = 0, so generated normals must be +-Z.
    CHECK(std::fabs(mesh.vertices[0].normal[2]) > 0.99f);
    CHECK(mesh.vertices[2].uv[0] == 1.0f && mesh.vertices[2].uv[1] == 0.0f);
}

void TestRejectsGarbage()
{
    rs2::XMesh mesh;
    CHECK(!rs2::ParseXMesh("not an x file at all", &mesh));
    CHECK(!rs2::ParseXMesh("xof 0302bin 0032", &mesh));
}

void TestShippedFile()
{
#ifdef RS2_TEST_X_FILE
    std::ifstream file(RS2_TEST_X_FILE, std::ios::binary);
    if (!file) {
        std::printf("note: %s not found, skipping\n", RS2_TEST_X_FILE);
        return;
    }
    const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    rs2::XMesh mesh;
    CHECK(rs2::ParseXMesh(text, &mesh));
    // Base.x: 20 quads (64 corners listed in the file, 4 per face).
    CHECK(mesh.vertices.size() == 80);
    CHECK(mesh.indices.size() == 120);
    for (uint32_t i : mesh.indices) {
        CHECK(i < mesh.vertices.size());
    }
#endif
}

void TestAllShippedFiles()
{
#ifdef RS2_TEST_DATA_DIR
    namespace fs = std::filesystem;
    int parsed = 0;
    for (const auto& entry : fs::recursive_directory_iterator(RS2_TEST_DATA_DIR)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".x") {
            continue;
        }
        std::ifstream file(entry.path(), std::ios::binary);
        const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        rs2::XMesh mesh;
        const bool ok = rs2::ParseXMesh(text, &mesh);
        if (!ok) {
            std::printf("failed to parse %s\n", entry.path().string().c_str());
        }
        CHECK(ok);
        for (uint32_t i : mesh.indices) {
            if (i >= mesh.vertices.size()) {
                CHECK(false);
                break;
            }
        }
        ++parsed;
    }
    std::printf("parsed %d shipped .x files\n", parsed);
    CHECK(parsed > 100);
#endif
}

} // namespace

int main()
{
    TestParse();
    TestRejectsGarbage();
    TestShippedFile();
    TestAllShippedFiles();
    if (g_failures == 0) {
        std::printf("x_mesh tests passed\n");
    }
    return g_failures == 0 ? 0 : 1;
}
