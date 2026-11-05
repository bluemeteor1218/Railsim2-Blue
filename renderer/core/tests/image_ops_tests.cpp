#include "../image_ops.h"

#include <cstdio>

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

rs2::Image Solid(uint32_t w, uint32_t h, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    rs2::Image image;
    image.width = w;
    image.height = h;
    for (uint32_t i = 0; i < w * h; ++i) {
        image.rgba.insert(image.rgba.end(), {r, g, b, a});
    }
    return image;
}

void TestRadialGradient()
{
    const rs2::Image g = rs2::MakeRadialGradient(64);
    CHECK(g.width == 64 && g.rgba.size() == 64u * 64u * 4u);
    const auto at = [&](uint32_t x, uint32_t y) { return g.rgba[(static_cast<size_t>(y) * 64 + x) * 4]; };
    CHECK(at(32, 32) > 240);   // center is bright
    CHECK(at(0, 0) == 0);      // corner is black
    CHECK(at(32, 32) > at(48, 32) && at(48, 32) > at(62, 32)); // monotonic falloff
}

void TestChainSizes()
{
    auto chain = rs2::GenerateMipChain(Solid(8, 4, 1, 2, 3, 4));
    CHECK(chain.size() == 4);
    CHECK(chain[1].width == 4 && chain[1].height == 2);
    CHECK(chain[2].width == 2 && chain[2].height == 1);
    CHECK(chain[3].width == 1 && chain[3].height == 1);
    CHECK(chain[3].rgba[0] == 1 && chain[3].rgba[1] == 2 && chain[3].rgba[2] == 3 && chain[3].rgba[3] == 4);

    // Odd sizes shrink like floor(n/2) and still end at 1x1.
    chain = rs2::GenerateMipChain(Solid(5, 3, 9, 9, 9, 9));
    CHECK(chain.size() == 3);
    CHECK(chain[1].width == 2 && chain[1].height == 1);
    CHECK(chain.back().width == 1 && chain.back().height == 1);
}

void TestAveraging()
{
    // 2x2: black, white / white, black -> mid grey.
    rs2::Image image;
    image.width = image.height = 2;
    image.rgba = {0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 255};
    const auto chain = rs2::GenerateMipChain(image);
    CHECK(chain.size() == 2);
    CHECK(chain[1].rgba[0] == 128 && chain[1].rgba[1] == 128 && chain[1].rgba[2] == 128);
    CHECK(chain[1].rgba[3] == 255);
}

void TestInvalidInput()
{
    rs2::Image bad;
    bad.width = 4;
    bad.height = 4;
    bad.rgba.resize(10);
    CHECK(rs2::GenerateMipChain(bad).empty());
    CHECK(rs2::GenerateMipChain(rs2::Image{}).empty());
}

void TestCheckerboard()
{
    const rs2::Image image = rs2::MakeCheckerboard(16, 8);
    CHECK(image.rgba.size() == 16u * 16u * 4u);
    CHECK(image.rgba[0] == 60);
    CHECK(image.rgba[8 * 4] == 230);
}

} // namespace

int main()
{
    TestRadialGradient();
    TestChainSizes();
    TestAveraging();
    TestInvalidInput();
    TestCheckerboard();
    if (g_failures == 0) {
        std::printf("all image_ops tests passed\n");
    }
    return g_failures == 0 ? 0 : 1;
}
