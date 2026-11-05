#include "../render_state.h"

#include <cmath>
#include <cstdio>
#include <unordered_set>

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

bool Near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

void TestDefaults()
{
    rs2::RenderState s;
    CHECK(s.Pipeline().cull == rs2::CullMode::Back);
    CHECK(s.GetZRead() && s.GetZWrite());
    CHECK(!s.GetLighting());
    CHECK(!s.Pipeline().blend.enable);
}

void TestBlendPresets()
{
    rs2::RenderState s;
    s.BlendAlpha();
    CHECK(s.Pipeline().blend.enable);
    CHECK(s.Pipeline().blend.src == rs2::BlendFactor::SrcAlpha);
    CHECK(s.Pipeline().blend.dst == rs2::BlendFactor::InvSrcAlpha);
    s.BlendMod();
    CHECK(s.Pipeline().blend.src == rs2::BlendFactor::DestColor);
    CHECK(s.Pipeline().blend.dst == rs2::BlendFactor::Zero);

    // Disabled blends with different leftover factors must be equivalent.
    rs2::RenderState a, b;
    a.SetBlend(false, rs2::BlendFactor::One, rs2::BlendFactor::One);
    b.SetBlend(false, rs2::BlendFactor::SrcAlpha, rs2::BlendFactor::Zero);
    CHECK(a.Pipeline() == b.Pipeline());
    CHECK(a.Pipeline().Hash() == b.Pipeline().Hash());
}

void TestColorAndAlpha()
{
    rs2::RenderState s;
    s.SetAmbient(0x80FF8000u);
    CHECK(Near(s.Constants().ambient[0], 1.0f));
    CHECK(Near(s.Constants().ambient[1], 128.0f / 255.0f));
    CHECK(Near(s.Constants().ambient[2], 0.0f));
    CHECK(Near(s.Constants().ambient[3], 128.0f / 255.0f));

    s.SetAlphaTest(true, 128);
    CHECK(s.Pipeline().alphaTest);
    CHECK(Near(s.Constants().alphaRef, 128.0f / 255.0f));
    s.SetAlphaTest(false, 0);
    CHECK(!s.Pipeline().alphaTest);
}

void TestLightsAndFog()
{
    rs2::RenderState s;
    s.SetLight(0, true);
    s.SetLight(3, true);
    CHECK(s.Constants().lightMask == 0x9u);
    s.SetLight(0, false);
    CHECK(s.Constants().lightMask == 0x8u);
    s.SetLight(99, true); // ignored
    CHECK(s.Constants().lightMask == 0x8u);

    s.SetFog(true, 0xFF0000FFu, 10.0f, 200.0f);
    CHECK(s.Pipeline().fog);
    CHECK(Near(s.Constants().fogColor[2], 1.0f));
    CHECK(Near(s.Constants().fogStartEnd[0], 10.0f) && Near(s.Constants().fogStartEnd[1], 200.0f));
}

void TestHashDistinguishesStates()
{
    std::unordered_set<rs2::PipelineKey, rs2::PipelineKeyHasher> keys;
    rs2::RenderState s;
    keys.insert(s.Pipeline());
    s.SetCulling(false);
    keys.insert(s.Pipeline());
    s.SetZWrite(false);
    keys.insert(s.Pipeline());
    s.BlendAdd();
    keys.insert(s.Pipeline());
    s.SetFog(true, 0, 0, 1);
    keys.insert(s.Pipeline());
    CHECK(keys.size() == 5);
    keys.insert(s.Pipeline());
    CHECK(keys.size() == 5);
}

} // namespace

int main()
{
    TestDefaults();
    TestBlendPresets();
    TestColorAndAlpha();
    TestLightsAndFog();
    TestHashDistinguishesStates();
    if (g_failures == 0) {
        std::printf("all render_state tests passed\n");
    }
    return g_failures == 0 ? 0 : 1;
}
