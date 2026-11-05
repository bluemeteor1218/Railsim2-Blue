#include "../sprite_batch.h"
#include "../text_layout.h"
#include "../viewport_layout.h"

#include <cmath>
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

bool Near(float a, float b) { return std::fabs(a - b) < 1e-3f; }

void TestFitLayout()
{
    // 640x480 UI in a 1920x1080 window: scale 2.25, pillarboxed.
    rs2::ViewportLayout layout(640, 480, 1920, 1080);
    CHECK(Near(layout.Scale(), 2.25f));
    CHECK(Near(layout.Viewport().w, 1440.0f) && Near(layout.Viewport().h, 1080.0f));
    CHECK(Near(layout.Viewport().x, 240.0f) && Near(layout.Viewport().y, 0.0f));

    float vx = 0, vy = 0;
    layout.ToVirtual(240.0f + 1440.0f * 0.5f, 540.0f, &vx, &vy);
    CHECK(Near(vx, 320.0f) && Near(vy, 240.0f));
    float px = 0, py = 0;
    layout.ToWindow(vx, vy, &px, &py);
    CHECK(Near(px, 960.0f) && Near(py, 540.0f));
    CHECK(!layout.ContainsWindowPoint(10.0f, 10.0f));
    CHECK(layout.ContainsWindowPoint(960.0f, 540.0f));
}

void TestFixedHeightLayout()
{
    rs2::ViewportLayout layout(640, 480, 1920, 1080, rs2::LayoutMode::FixedHeight);
    CHECK(Near(layout.Scale(), 2.25f));
    CHECK(Near(layout.VirtualHeight(), 480.0f));
    CHECK(Near(layout.VirtualWidth(), 1920.0f / 2.25f));
    CHECK(Near(layout.Viewport().w, 1920.0f));
}

void TestDegenerateSize()
{
    rs2::ViewportLayout layout(640, 480, 0, 0);
    CHECK(layout.Scale() > 0.0f);
}

void TestDpiScale()
{
    CHECK(Near(rs2::DpiScale(96), 1.0f));
    CHECK(Near(rs2::DpiScale(144), 1.5f));
    CHECK(Near(rs2::DpiScale(120), 1.25f));
    CHECK(Near(rs2::DpiScale(72), 1.0f));
    CHECK(Near(rs2::DpiScale(110), 1.25f)); // 1.146 -> snapped to nearest 0.25
}

void TestSpriteBatch()
{
    rs2::SpriteBatch batch;
    batch.AddSolid({10, 20, 30, 40}, {1, 2, 3, 4});
    batch.AddSolid({0, 0, 1, 1}, {});
    CHECK(batch.Vertices().size() == 12);
    CHECK(batch.Ranges().size() == 1); // same texture 0 -> merged
    CHECK(batch.Ranges()[0].vertexCount == 12);

    const auto& tl = batch.Vertices()[0];
    const auto& br = batch.Vertices()[4];
    CHECK(Near(tl.x, 10) && Near(tl.y, 20));
    CHECK(Near(br.x, 40) && Near(br.y, 60));
    CHECK(tl.color[0] == 1 && tl.color[3] == 4);

    batch.SetTexture(2);
    batch.AddQuad({0, 0, 8, 8}, {0.25f, 0.5f, 0.25f, 0.25f}, {});
    CHECK(batch.Ranges().size() == 2);
    CHECK(batch.Ranges()[1].texture == 2 && batch.Ranges()[1].firstVertex == 12);
    CHECK(Near(batch.Vertices()[16].u, 0.5f) && Near(batch.Vertices()[16].v, 0.75f));

    batch.Clear();
    CHECK(batch.Vertices().empty() && batch.Ranges().empty());
}

void TestFrame()
{
    rs2::SpriteBatch batch;
    batch.AddFrame({0, 0, 100, 50}, 2, {});
    CHECK(batch.Vertices().size() == 24);
}

rs2::GlyphTable MakeTable()
{
    rs2::GlyphTable table;
    table.lineHeight = 10;
    for (char32_t ch = rs2::GlyphTable::kFirst; ch <= rs2::GlyphTable::kLast; ++ch) {
        rs2::Glyph g;
        g.valid = true;
        g.width = ch == ' ' ? 0.0f : 6.0f;
        g.height = 8;
        g.offsetX = 1;
        g.offsetY = 1;
        g.advance = 7;
        g.u0 = 0.0f; g.v0 = 0.0f; g.u1 = 0.1f; g.v1 = 0.1f;
        table.Set(ch, g);
    }
    return table;
}

void TestTextLayout()
{
    const rs2::GlyphTable table = MakeTable();
    rs2::SpriteBatch batch;
    batch.SetTexture(1);
    const rs2::TextMetrics m = rs2::LayoutText(table, "AB C", 100, 50, 2.0f, {}, &batch);
    CHECK(Near(m.width, 4 * 7 * 2.0f));
    CHECK(Near(m.height, 20.0f));
    CHECK(batch.Vertices().size() == 3 * 6); // the space draws nothing
    CHECK(Near(batch.Vertices()[0].x, 100 + 1 * 2.0f));
    CHECK(Near(batch.Vertices()[0].y, 50 + 1 * 2.0f));
    CHECK(Near(batch.Vertices()[6].x, 100 + 14 + 2.0f)); // second glyph one advance later

    // Measuring without a batch gives the same result.
    const rs2::TextMetrics measured = rs2::LayoutText(table, "AB C", 0, 0, 2.0f, {}, nullptr);
    CHECK(Near(measured.width, m.width));
}

void TestMultiLineAndFallback()
{
    const rs2::GlyphTable table = MakeTable();
    const rs2::TextMetrics m = rs2::LayoutText(table, "ab\nabcd", 0, 0, 1.0f, {}, nullptr);
    CHECK(Near(m.width, 28.0f));
    CHECK(Near(m.height, 20.0f));

    // A non-ASCII code point without a glyph falls back to '?' (one quad, not one per UTF-8 byte).
    rs2::SpriteBatch batch;
    rs2::LayoutText(table, "\xE3\x81\x82", 0, 0, 1.0f, {}, &batch);
    CHECK(batch.Vertices().size() == 6);
}

void TestUtf8()
{
    rs2::GlyphTable table = MakeTable();
    rs2::Glyph wide;
    wide.valid = true;
    wide.width = 12;
    wide.height = 8;
    wide.advance = 14;
    table.Set(0x3042, wide); // U+3042 HIRAGANA A
    CHECK(table.Has(0x3042));
    CHECK(!table.Has(0x3044));

    size_t i = 0;
    const std::string text = "A\xE3\x81\x82\xF0\x9F\x98\x80z";
    CHECK(rs2::DecodeUtf8(text, &i) == 'A');
    CHECK(rs2::DecodeUtf8(text, &i) == 0x3042);
    CHECK(rs2::DecodeUtf8(text, &i) == 0x1F600);
    CHECK(rs2::DecodeUtf8(text, &i) == 'z');
    CHECK(i == text.size());

    // A truncated sequence must not read past the end.
    i = 0;
    const std::string cut = "\xE3\x81";
    CHECK(rs2::DecodeUtf8(cut, &i) == 0xFFFD);
    CHECK(i <= cut.size());

    const rs2::TextMetrics m = rs2::LayoutText(table, "A\xE3\x81\x82", 0, 0, 1.0f, {}, nullptr);
    CHECK(Near(m.width, 7 + 14));

    std::set<char32_t> extra;
    rs2::CollectExtraCodepoints("abc\xE3\x81\x82\xE3\x81\x82", &extra);
    CHECK(extra.size() == 1 && *extra.begin() == 0x3042);
}

} // namespace

int main()
{
    TestFitLayout();
    TestFixedHeightLayout();
    TestDegenerateSize();
    TestDpiScale();
    TestSpriteBatch();
    TestFrame();
    TestTextLayout();
    TestMultiLineAndFallback();
    TestUtf8();
    if (g_failures == 0) {
        std::printf("all ui tests passed\n");
    }
    return g_failures == 0 ? 0 : 1;
}
