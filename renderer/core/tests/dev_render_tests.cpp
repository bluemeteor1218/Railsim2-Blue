#include "../dev_render.h"

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

void TestBlendPresets()
{
    rs2::DevRender dev;
    using namespace rs2::d3d8;
    dev.SetBlend(true, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
    rs2::RenderState alpha;
    alpha.BlendAlpha();
    CHECK(dev.State().Pipeline().blend == alpha.Pipeline().blend);

    dev.SetBlend(true, D3DBLEND_ONE, D3DBLEND_ONE);
    rs2::RenderState add;
    add.BlendAdd();
    CHECK(dev.State().Pipeline().blend == add.Pipeline().blend);

    dev.SetBlend(true, D3DBLEND_DESTCOLOR, D3DBLEND_ZERO);
    rs2::RenderState mod;
    mod.BlendMod();
    CHECK(dev.State().Pipeline().blend == mod.Pipeline().blend);

    dev.SetBlend(false, 0, 0);
    CHECK(!dev.State().Pipeline().blend.enable);
}

void TestStates()
{
    rs2::DevRender dev;
    using namespace rs2::d3d8;
    dev.SetLighting(true);
    CHECK(dev.GetLighting());
    dev.SetCulling(false);
    CHECK(dev.State().Pipeline().cull == rs2::CullMode::None);
    dev.SetCulling(true);
    CHECK(dev.State().Pipeline().cull == rs2::CullMode::Back);
    dev.SetShading(D3DSHADE_FLAT);
    CHECK(dev.State().Pipeline().flatShading);
    dev.SetShading(D3DSHADE_GOURAUD);
    CHECK(!dev.State().Pipeline().flatShading);
    dev.AlphaGreat(128);
    CHECK(dev.State().Pipeline().alphaTest);
    CHECK(std::fabs(dev.State().Constants().alphaRef - 128.0f / 255.0f) < 1e-6f);
    dev.AlphaPass();
    CHECK(!dev.State().Pipeline().alphaTest);
    dev.SetZWrite(false);
    CHECK(!dev.GetZWrite());
    dev.SetEnvMap(true);
    CHECK(dev.State().Pipeline().secondTexture == rs2::SecondTexture::EnvMap);
    dev.SetLightMap(true);
    CHECK(dev.State().Pipeline().secondTexture == rs2::SecondTexture::Modulate);
    dev.SetLightMap(false);
    CHECK(dev.State().Pipeline().secondTexture == rs2::SecondTexture::None);
}

void TestFogShared()
{
    rs2::DevRender a, b;
    a.SetFog(true, 0xFF102030u, 1.0f, 5.0f);
    b.SetPixelFog(true, 0xFF102030u, 1.0f, 5.0f);
    CHECK(a.State().Pipeline() == b.State().Pipeline());
    CHECK(a.State().Constants().fogStartEnd[1] == 5.0f);
}

} // namespace

int main()
{
    TestBlendPresets();
    TestStates();
    TestFogShared();
    if (g_failures == 0) {
        std::printf("dev_render tests passed\n");
    }
    return g_failures == 0 ? 0 : 1;
}
