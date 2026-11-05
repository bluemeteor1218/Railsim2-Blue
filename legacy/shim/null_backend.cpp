#include "d3d8_objects.h"

namespace gfx {
namespace {

class NullBackend : public Backend {
public:
    bool Init(HWND, const D3DPRESENT_PARAMETERS&) override { return true; }
    bool Resize(const D3DPRESENT_PARAMETERS&) override { return true; }
    void BeginScene() override {}
    void EndScene() override {}
    void Clear(const State&, DWORD, D3DCOLOR, float, DWORD) override {}
    void Draw(const State&, D3DPRIMITIVETYPE, const Geometry&) override {}
    void Present() override {}
    void ReadSurface(Surface8*, uint32_t* out) override { (void)out; }
    void OnTextureDestroyed(Tex8*) override {}
    void OnGeometryDestroyed(uint64_t) override {}
};

} // namespace

Backend* CreateNullBackend() { return new NullBackend(); }

} // namespace gfx
