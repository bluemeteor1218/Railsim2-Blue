#include "../d3dx_math.h"

#include <cmath>
#include <cstdio>

// Test-only: lets the tests write &D3DXVECTOR3(...) for temporaries by
// overloading operator& on derived wrappers.
template <class T>
struct Addressable : T {
    using T::T;
    Addressable() = default;
    Addressable(const T& t) : T(t) {}
    Addressable* operator&() { return this; }
    const Addressable* operator&() const { return this; }
};
using TestVec2 = Addressable<D3DXVECTOR2>;
using TestVec3 = Addressable<D3DXVECTOR3>;
using TestVec4 = Addressable<D3DXVECTOR4>;
#define D3DXVECTOR2 TestVec2
#define D3DXVECTOR3 TestVec3
#define D3DXVECTOR4 TestVec4

namespace {

int g_failures = 0;

bool Near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) <= eps; }

void Check(bool ok, const char* expr, int line)
{
    if (!ok) {
        std::printf("FAIL line %d: %s\n", line, expr);
        ++g_failures;
    }
}

#define CHECK(expr) Check((expr), #expr, __LINE__)
#define CHECK_VEC3(v, ex, ey, ez) CHECK(Near((v).x, ex) && Near((v).y, ey) && Near((v).z, ez))

const float kHalfPi = D3DX_PI / 2.0f;

void TestVectors()
{
    D3DXVECTOR3 a(1, 2, 3), b(4, 5, 6), r;
    CHECK(Near(D3DXVec3Dot(&a, &b), 32.0f));
    CHECK(Near(D3DXVec3Length(&D3DXVECTOR3(3, 4, 0)), 5.0f));
    D3DXVec3Cross(&r, &D3DXVECTOR3(1, 0, 0), &D3DXVECTOR3(0, 1, 0));
    CHECK_VEC3(r, 0, 0, 1);
    D3DXVec3Normalize(&r, &D3DXVECTOR3(0, 0, 0));
    CHECK_VEC3(r, 0, 0, 0);
    D3DXVec3Normalize(&r, &D3DXVECTOR3(0, 3, 4));
    CHECK_VEC3(r, 0, 0.6f, 0.8f);
    D3DXVECTOR2 v2(3, 4);
    CHECK(Near(D3DXVec2Length(&v2), 5.0f));
    CHECK(Near(D3DXVec2Dot(&v2, &D3DXVECTOR2(1, 1)), 7.0f));
}

void TestMatrices()
{
    D3DXMATRIX m;
    D3DXVECTOR3 r;

    D3DXMatrixRotationY(&m, kHalfPi);
    D3DXVec3TransformCoord(&r, &D3DXVECTOR3(0, 0, 1), &m);
    CHECK_VEC3(r, 1, 0, 0);
    D3DXMatrixRotationX(&m, kHalfPi);
    D3DXVec3TransformCoord(&r, &D3DXVECTOR3(0, 1, 0), &m);
    CHECK_VEC3(r, 0, 0, 1);
    D3DXMatrixRotationZ(&m, kHalfPi);
    D3DXVec3TransformCoord(&r, &D3DXVECTOR3(1, 0, 0), &m);
    CHECK_VEC3(r, 0, 1, 0);

    D3DXMatrixTranslation(&m, 1, 2, 3);
    D3DXVec3TransformCoord(&r, &D3DXVECTOR3(0, 0, 0), &m);
    CHECK_VEC3(r, 1, 2, 3);
    D3DXVec3TransformNormal(&r, &D3DXVECTOR3(1, 1, 1), &m);
    CHECK_VEC3(r, 1, 1, 1);

    D3DXMATRIX t, s, inv, prod;
    D3DXMatrixTranslation(&t, 1, 2, 3);
    D3DXMatrixScaling(&s, 2, 3, 4);
    m = s * t;
    float det = 0;
    CHECK(D3DXMatrixInverse(&inv, &det, &m) == &inv);
    CHECK(Near(det, 24.0f));
    prod = m * inv;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            CHECK(Near(prod(i, j), i == j ? 1.0f : 0.0f));

    D3DXMATRIX zero(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    CHECK(D3DXMatrixInverse(&inv, nullptr, &zero) == nullptr);

    // In-place use must work.
    D3DXMatrixRotationY(&m, kHalfPi);
    D3DXMatrixInverse(&m, nullptr, &m);
    D3DXVec3TransformCoord(&r, &D3DXVECTOR3(1, 0, 0), &m);
    CHECK_VEC3(r, 0, 0, 1);

    D3DXMatrixLookAtLH(&m, &D3DXVECTOR3(0, 0, -5), &D3DXVECTOR3(0, 0, 0), &D3DXVECTOR3(0, 1, 0));
    D3DXVec3TransformCoord(&r, &D3DXVECTOR3(0, 0, 0), &m);
    CHECK_VEC3(r, 0, 0, 5);

    D3DXMATRIX p1, p2;
    D3DXMatrixPerspectiveFovLH(&p1, kHalfPi, 1.0f, 1.0f, 100.0f);
    D3DXMatrixPerspectiveOffCenterLH(&p2, -1, 1, -1, 1, 1.0f, 100.0f);
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            CHECK(Near(p1(i, j), p2(i, j)));

    D3DXMATRIX axis, ry;
    D3DXMatrixRotationAxis(&axis, &D3DXVECTOR3(0, 5, 0), 0.7f);
    D3DXMatrixRotationY(&ry, 0.7f);
    CHECK(Near(axis._11, ry._11) && Near(axis._13, ry._13) && Near(axis._31, ry._31));
    D3DXMatrixRotationYawPitchRoll(&m, 0.7f, 0, 0);
    CHECK(Near(m._11, ry._11) && Near(m._13, ry._13) && Near(m._31, ry._31));

    D3DXQUATERNION q(0, std::sin(0.35f), 0, std::cos(0.35f));
    D3DXMatrixRotationQuaternion(&m, &q);
    CHECK(Near(m._11, ry._11) && Near(m._13, ry._13));
}

void TestPlanesAndShadow()
{
    D3DXPLANE plane;
    D3DXPlaneFromPoints(&plane, &D3DXVECTOR3(0, 0, 1), &D3DXVECTOR3(1, 0, 1), &D3DXVECTOR3(0, 1, 1));
    CHECK(Near(plane.a, 0) && Near(plane.b, 0) && Near(plane.c, 1) && Near(plane.d, -1));
    CHECK(Near(D3DXPlaneDotCoord(&plane, &D3DXVECTOR3(0, 0, 0)), -1.0f));

    D3DXPLANE ground(0, 1, 0, 0);
    D3DXMATRIX m;
    D3DXVECTOR3 r;
    D3DXMatrixShadow(&m, &D3DXVECTOR4(0, 1, 0, 0), &ground);
    D3DXVec3TransformCoord(&r, &D3DXVECTOR3(2, 5, 3), &m);
    CHECK(Near(r.y, 0.0f));
}

void TestQuaternionAndProbes()
{
    D3DXQUATERNION a(0, 0, 0, 1), b(0, 1, 0, 0), r;
    D3DXQuaternionSlerp(&r, &a, &b, 0.5f);
    CHECK(Near(r.y, std::sqrt(0.5f)) && Near(r.w, std::sqrt(0.5f)));

    CHECK(Near(D3DXToRadian(180.0f), D3DX_PI));
    CHECK(Near(D3DXToDegree(D3DX_PI), 180.0f));

    D3DXVECTOR3 lo(-1, -1, -1), hi(1, 1, 1);
    CHECK(D3DXBoxBoundProbe(&lo, &hi, &D3DXVECTOR3(0, 0, -5), &D3DXVECTOR3(0, 0, 1)) == 1);
    CHECK(D3DXBoxBoundProbe(&lo, &hi, &D3DXVECTOR3(0, 0, -5), &D3DXVECTOR3(0, 0, -1)) == 0);
    CHECK(D3DXBoxBoundProbe(&lo, &hi, &D3DXVECTOR3(5, 0, -5), &D3DXVECTOR3(0, 0, 1)) == 0);
    CHECK(D3DXSphereBoundProbe(&D3DXVECTOR3(0, 0, 0), 1.0f, &D3DXVECTOR3(0, 0, -5), &D3DXVECTOR3(0, 0, 1)) == 1);
    CHECK(D3DXSphereBoundProbe(&D3DXVECTOR3(0, 0, 0), 1.0f, &D3DXVECTOR3(3, 0, -5), &D3DXVECTOR3(0, 0, 1)) == 0);
    CHECK(D3DXSphereBoundProbe(&D3DXVECTOR3(0, 0, 0), 1.0f, &D3DXVECTOR3(0, 0, 0), &D3DXVECTOR3(0, 0, 1)) == 1);
}

} // namespace

int main()
{
    TestVectors();
    TestMatrices();
    TestPlanesAndShadow();
    TestQuaternionAndProbes();
    if (g_failures == 0) {
        std::printf("all d3dx_math tests passed\n");
    }
    return g_failures == 0 ? 0 : 1;
}
