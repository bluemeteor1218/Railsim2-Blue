#pragma once

// D3DX8 math compatibility layer built on DirectXMath.
//
// DirectXMath uses the same conventions as D3DX (row-major matrices,
// row vectors, left-handed coordinates), so legacy call sites can keep the
// D3DX names and semantics while the D3DX8 library is dropped.
//
// Not covered here (renderer-side, not math): mesh/texture/font/sprite
// creation, D3DXGetFVFVertexSize and D3DXCompute* bounding helpers.

#include <DirectXMath.h>

#include <cmath>
#include <cstring>

#pragma warning(push)
#pragma warning(disable : 4201) // anonymous struct in union, same as d3dx8math.h

#ifndef D3DX_PI
#define D3DX_PI (static_cast<float>(3.141592654f))
#endif
#define D3DXToRadian(degree) ((degree) * (D3DX_PI / 180.0f))
#define D3DXToDegree(radian) ((radian) * (180.0f / D3DX_PI))

struct D3DXVECTOR2 {
    float x, y;

    D3DXVECTOR2() = default;
    D3DXVECTOR2(const float* p) : x(p[0]), y(p[1]) {}
    D3DXVECTOR2(float fx, float fy) : x(fx), y(fy) {}

    operator float*() { return &x; }
    operator const float*() const { return &x; }

    D3DXVECTOR2& operator+=(const D3DXVECTOR2& v) { x += v.x; y += v.y; return *this; }
    D3DXVECTOR2& operator-=(const D3DXVECTOR2& v) { x -= v.x; y -= v.y; return *this; }
    D3DXVECTOR2& operator*=(float f) { x *= f; y *= f; return *this; }
    D3DXVECTOR2& operator/=(float f) { x /= f; y /= f; return *this; }

    D3DXVECTOR2 operator+() const { return *this; }
    D3DXVECTOR2 operator-() const { return D3DXVECTOR2(-x, -y); }
    D3DXVECTOR2 operator+(const D3DXVECTOR2& v) const { return D3DXVECTOR2(x + v.x, y + v.y); }
    D3DXVECTOR2 operator-(const D3DXVECTOR2& v) const { return D3DXVECTOR2(x - v.x, y - v.y); }
    D3DXVECTOR2 operator*(float f) const { return D3DXVECTOR2(x * f, y * f); }
    D3DXVECTOR2 operator/(float f) const { return D3DXVECTOR2(x / f, y / f); }
    friend D3DXVECTOR2 operator*(float f, const D3DXVECTOR2& v) { return v * f; }

    bool operator==(const D3DXVECTOR2& v) const { return x == v.x && y == v.y; }
    bool operator!=(const D3DXVECTOR2& v) const { return !(*this == v); }
};

struct D3DXVECTOR3 {
    float x, y, z;

    D3DXVECTOR3() = default;
    D3DXVECTOR3(const float* p) : x(p[0]), y(p[1]), z(p[2]) {}
    D3DXVECTOR3(float fx, float fy, float fz) : x(fx), y(fy), z(fz) {}
#ifdef D3DVECTOR_DEFINED
    D3DXVECTOR3(const D3DVECTOR& v) : x(v.x), y(v.y), z(v.z) {}
    operator D3DVECTOR() const { return D3DVECTOR{x, y, z}; }
#endif

    operator float*() { return &x; }
    operator const float*() const { return &x; }

    D3DXVECTOR3& operator+=(const D3DXVECTOR3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    D3DXVECTOR3& operator-=(const D3DXVECTOR3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    D3DXVECTOR3& operator*=(float f) { x *= f; y *= f; z *= f; return *this; }
    D3DXVECTOR3& operator/=(float f) { x /= f; y /= f; z /= f; return *this; }

    D3DXVECTOR3 operator+() const { return *this; }
    D3DXVECTOR3 operator-() const { return D3DXVECTOR3(-x, -y, -z); }
    D3DXVECTOR3 operator+(const D3DXVECTOR3& v) const { return D3DXVECTOR3(x + v.x, y + v.y, z + v.z); }
    D3DXVECTOR3 operator-(const D3DXVECTOR3& v) const { return D3DXVECTOR3(x - v.x, y - v.y, z - v.z); }
    D3DXVECTOR3 operator*(float f) const { return D3DXVECTOR3(x * f, y * f, z * f); }
    D3DXVECTOR3 operator/(float f) const { return D3DXVECTOR3(x / f, y / f, z / f); }
    friend D3DXVECTOR3 operator*(float f, const D3DXVECTOR3& v) { return v * f; }

    bool operator==(const D3DXVECTOR3& v) const { return x == v.x && y == v.y && z == v.z; }
    bool operator!=(const D3DXVECTOR3& v) const { return !(*this == v); }
};

struct D3DXVECTOR4 {
    float x, y, z, w;

    D3DXVECTOR4() = default;
    D3DXVECTOR4(const float* p) : x(p[0]), y(p[1]), z(p[2]), w(p[3]) {}
    D3DXVECTOR4(float fx, float fy, float fz, float fw) : x(fx), y(fy), z(fz), w(fw) {}

    operator float*() { return &x; }
    operator const float*() const { return &x; }

    D3DXVECTOR4& operator+=(const D3DXVECTOR4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    D3DXVECTOR4& operator-=(const D3DXVECTOR4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    D3DXVECTOR4& operator*=(float f) { x *= f; y *= f; z *= f; w *= f; return *this; }
    D3DXVECTOR4& operator/=(float f) { x /= f; y /= f; z /= f; w /= f; return *this; }

    D3DXVECTOR4 operator+() const { return *this; }
    D3DXVECTOR4 operator-() const { return D3DXVECTOR4(-x, -y, -z, -w); }
    D3DXVECTOR4 operator+(const D3DXVECTOR4& v) const { return D3DXVECTOR4(x + v.x, y + v.y, z + v.z, w + v.w); }
    D3DXVECTOR4 operator-(const D3DXVECTOR4& v) const { return D3DXVECTOR4(x - v.x, y - v.y, z - v.z, w - v.w); }
    D3DXVECTOR4 operator*(float f) const { return D3DXVECTOR4(x * f, y * f, z * f, w * f); }
    D3DXVECTOR4 operator/(float f) const { return D3DXVECTOR4(x / f, y / f, z / f, w / f); }
    friend D3DXVECTOR4 operator*(float f, const D3DXVECTOR4& v) { return v * f; }

    bool operator==(const D3DXVECTOR4& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    bool operator!=(const D3DXVECTOR4& v) const { return !(*this == v); }
};

struct D3DXQUATERNION {
    float x, y, z, w;

    D3DXQUATERNION() = default;
    D3DXQUATERNION(const float* p) : x(p[0]), y(p[1]), z(p[2]), w(p[3]) {}
    D3DXQUATERNION(float fx, float fy, float fz, float fw) : x(fx), y(fy), z(fz), w(fw) {}

    operator float*() { return &x; }
    operator const float*() const { return &x; }

    bool operator==(const D3DXQUATERNION& q) const { return x == q.x && y == q.y && z == q.z && w == q.w; }
    bool operator!=(const D3DXQUATERNION& q) const { return !(*this == q); }
};

struct D3DXPLANE {
    float a, b, c, d;

    D3DXPLANE() = default;
    D3DXPLANE(const float* p) : a(p[0]), b(p[1]), c(p[2]), d(p[3]) {}
    D3DXPLANE(float fa, float fb, float fc, float fd) : a(fa), b(fb), c(fc), d(fd) {}

    operator float*() { return &a; }
    operator const float*() const { return &a; }

    bool operator==(const D3DXPLANE& p) const { return a == p.a && b == p.b && c == p.c && d == p.d; }
    bool operator!=(const D3DXPLANE& p) const { return !(*this == p); }
};

struct D3DXMATRIX {
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };

    D3DXMATRIX() = default;
    D3DXMATRIX(const float* p) { std::memcpy(m, p, sizeof(m)); }
    D3DXMATRIX(float f11, float f12, float f13, float f14,
               float f21, float f22, float f23, float f24,
               float f31, float f32, float f33, float f34,
               float f41, float f42, float f43, float f44)
        : _11(f11), _12(f12), _13(f13), _14(f14),
          _21(f21), _22(f22), _23(f23), _24(f24),
          _31(f31), _32(f32), _33(f33), _34(f34),
          _41(f41), _42(f42), _43(f43), _44(f44) {}

    float& operator()(unsigned row, unsigned col) { return m[row][col]; }
    float operator()(unsigned row, unsigned col) const { return m[row][col]; }

    operator float*() { return &_11; }
    operator const float*() const { return &_11; }

    D3DXMATRIX& operator*=(const D3DXMATRIX& rhs);
    D3DXMATRIX& operator+=(const D3DXMATRIX& rhs) { return Combine(rhs, 1.0f); }
    D3DXMATRIX& operator-=(const D3DXMATRIX& rhs) { return Combine(rhs, -1.0f); }
    D3DXMATRIX& operator*=(float f) { return Scale(f); }
    D3DXMATRIX& operator/=(float f) { return Scale(1.0f / f); }

    D3DXMATRIX operator+() const { return *this; }
    D3DXMATRIX operator-() const { D3DXMATRIX r = *this; return r.Scale(-1.0f); }
    D3DXMATRIX operator*(const D3DXMATRIX& rhs) const { D3DXMATRIX r = *this; return r *= rhs; }
    D3DXMATRIX operator+(const D3DXMATRIX& rhs) const { D3DXMATRIX r = *this; return r += rhs; }
    D3DXMATRIX operator-(const D3DXMATRIX& rhs) const { D3DXMATRIX r = *this; return r -= rhs; }
    D3DXMATRIX operator*(float f) const { D3DXMATRIX r = *this; return r.Scale(f); }
    D3DXMATRIX operator/(float f) const { D3DXMATRIX r = *this; return r.Scale(1.0f / f); }
    friend D3DXMATRIX operator*(float f, const D3DXMATRIX& mtx) { return mtx * f; }

    bool operator==(const D3DXMATRIX& rhs) const { return std::memcmp(this, &rhs, sizeof(D3DXMATRIX)) == 0; }
    bool operator!=(const D3DXMATRIX& rhs) const { return !(*this == rhs); }

private:
    D3DXMATRIX& Combine(const D3DXMATRIX& rhs, float sign)
    {
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                m[r][c] += sign * rhs.m[r][c];
            }
        }
        return *this;
    }
    D3DXMATRIX& Scale(float f)
    {
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                m[r][c] *= f;
            }
        }
        return *this;
    }
};

#pragma warning(pop)

namespace d3dx_compat_detail {

inline DirectX::XMVECTOR Load(const D3DXVECTOR3& v) { return DirectX::XMVectorSet(v.x, v.y, v.z, 0.0f); }
inline DirectX::XMVECTOR Load(const D3DXVECTOR4& v) { return DirectX::XMVectorSet(v.x, v.y, v.z, v.w); }
inline DirectX::XMVECTOR Load(const D3DXQUATERNION& q) { return DirectX::XMVectorSet(q.x, q.y, q.z, q.w); }
inline DirectX::XMVECTOR Load(const D3DXPLANE& p) { return DirectX::XMVectorSet(p.a, p.b, p.c, p.d); }

inline DirectX::XMMATRIX Load(const D3DXMATRIX& m)
{
    DirectX::XMFLOAT4X4 tmp(&m._11);
    return DirectX::XMLoadFloat4x4(&tmp);
}

inline void Store(D3DXMATRIX* out, DirectX::FXMMATRIX m)
{
    DirectX::XMFLOAT4X4 tmp;
    DirectX::XMStoreFloat4x4(&tmp, m);
    std::memcpy(out->m, tmp.m, sizeof(out->m));
}

} // namespace d3dx_compat_detail

inline D3DXMATRIX& D3DXMATRIX::operator*=(const D3DXMATRIX& rhs)
{
    using namespace d3dx_compat_detail;
    Store(this, DirectX::XMMatrixMultiply(Load(*this), Load(rhs)));
    return *this;
}

// --- Vectors -------------------------------------------------------------

inline float D3DXVec2Dot(const D3DXVECTOR2* a, const D3DXVECTOR2* b)
{
    return a->x * b->x + a->y * b->y;
}

inline float D3DXVec2Length(const D3DXVECTOR2* v)
{
    return std::sqrt(v->x * v->x + v->y * v->y);
}

inline D3DXVECTOR2* D3DXVec2Normalize(D3DXVECTOR2* out, const D3DXVECTOR2* v)
{
    const float length = D3DXVec2Length(v);
    if (length == 0.0f) {
        *out = D3DXVECTOR2(0.0f, 0.0f);
    } else {
        const float inverse = 1.0f / length;
        *out = D3DXVECTOR2(v->x * inverse, v->y * inverse);
    }
    return out;
}

inline float D3DXVec3Dot(const D3DXVECTOR3* a, const D3DXVECTOR3* b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

inline float D3DXVec3Length(const D3DXVECTOR3* v)
{
    return std::sqrt(D3DXVec3Dot(v, v));
}

inline D3DXVECTOR3* D3DXVec3Cross(D3DXVECTOR3* out, const D3DXVECTOR3* a, const D3DXVECTOR3* b)
{
    const D3DXVECTOR3 result(a->y * b->z - a->z * b->y,
                             a->z * b->x - a->x * b->z,
                             a->x * b->y - a->y * b->x);
    *out = result;
    return out;
}

inline D3DXVECTOR3* D3DXVec3Normalize(D3DXVECTOR3* out, const D3DXVECTOR3* v)
{
    const float length = D3DXVec3Length(v);
    if (length == 0.0f) {
        *out = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    } else {
        const float inverse = 1.0f / length;
        *out = D3DXVECTOR3(v->x * inverse, v->y * inverse, v->z * inverse);
    }
    return out;
}

// Transforms (x, y, z, 1) and projects the result back with w.
inline D3DXVECTOR3* D3DXVec3TransformCoord(D3DXVECTOR3* out, const D3DXVECTOR3* v, const D3DXMATRIX* m)
{
    const float x = v->x * m->_11 + v->y * m->_21 + v->z * m->_31 + m->_41;
    const float y = v->x * m->_12 + v->y * m->_22 + v->z * m->_32 + m->_42;
    const float z = v->x * m->_13 + v->y * m->_23 + v->z * m->_33 + m->_43;
    const float w = v->x * m->_14 + v->y * m->_24 + v->z * m->_34 + m->_44;
    const float inverse = 1.0f / w;
    *out = D3DXVECTOR3(x * inverse, y * inverse, z * inverse);
    return out;
}

// Transforms (x, y, z, 0): rotation and scale only, no translation.
inline D3DXVECTOR3* D3DXVec3TransformNormal(D3DXVECTOR3* out, const D3DXVECTOR3* v, const D3DXMATRIX* m)
{
    *out = D3DXVECTOR3(v->x * m->_11 + v->y * m->_21 + v->z * m->_31,
                       v->x * m->_12 + v->y * m->_22 + v->z * m->_32,
                       v->x * m->_13 + v->y * m->_23 + v->z * m->_33);
    return out;
}

// --- Matrices ------------------------------------------------------------

// Returns nullptr when the matrix is singular, like D3DX.
inline D3DXMATRIX* D3DXMatrixInverse(D3DXMATRIX* out, float* determinant, const D3DXMATRIX* m)
{
    using namespace d3dx_compat_detail;
    DirectX::XMVECTOR det;
    const DirectX::XMMATRIX inverse = DirectX::XMMatrixInverse(&det, Load(*m));
    const float d = DirectX::XMVectorGetX(det);
    if (determinant != nullptr) {
        *determinant = d;
    }
    if (d == 0.0f) {
        return nullptr;
    }
    Store(out, inverse);
    return out;
}

inline D3DXMATRIX* D3DXMatrixTranslation(D3DXMATRIX* out, float x, float y, float z)
{
    d3dx_compat_detail::Store(out, DirectX::XMMatrixTranslation(x, y, z));
    return out;
}

inline D3DXMATRIX* D3DXMatrixScaling(D3DXMATRIX* out, float sx, float sy, float sz)
{
    d3dx_compat_detail::Store(out, DirectX::XMMatrixScaling(sx, sy, sz));
    return out;
}

inline D3DXMATRIX* D3DXMatrixRotationX(D3DXMATRIX* out, float angle)
{
    d3dx_compat_detail::Store(out, DirectX::XMMatrixRotationX(angle));
    return out;
}

inline D3DXMATRIX* D3DXMatrixRotationY(D3DXMATRIX* out, float angle)
{
    d3dx_compat_detail::Store(out, DirectX::XMMatrixRotationY(angle));
    return out;
}

inline D3DXMATRIX* D3DXMatrixRotationZ(D3DXMATRIX* out, float angle)
{
    d3dx_compat_detail::Store(out, DirectX::XMMatrixRotationZ(angle));
    return out;
}

// D3DX normalizes the axis; XMMatrixRotationNormal requires it already normalized.
inline D3DXMATRIX* D3DXMatrixRotationAxis(D3DXMATRIX* out, const D3DXVECTOR3* axis, float angle)
{
    using namespace d3dx_compat_detail;
    Store(out, DirectX::XMMatrixRotationNormal(DirectX::XMVector3Normalize(Load(*axis)), angle));
    return out;
}

inline D3DXMATRIX* D3DXMatrixRotationQuaternion(D3DXMATRIX* out, const D3DXQUATERNION* q)
{
    using namespace d3dx_compat_detail;
    Store(out, DirectX::XMMatrixRotationQuaternion(Load(*q)));
    return out;
}

inline D3DXMATRIX* D3DXMatrixRotationYawPitchRoll(D3DXMATRIX* out, float yaw, float pitch, float roll)
{
    d3dx_compat_detail::Store(out, DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll));
    return out;
}

inline D3DXMATRIX* D3DXMatrixLookAtLH(D3DXMATRIX* out, const D3DXVECTOR3* eye, const D3DXVECTOR3* at, const D3DXVECTOR3* up)
{
    using namespace d3dx_compat_detail;
    Store(out, DirectX::XMMatrixLookAtLH(Load(*eye), Load(*at), Load(*up)));
    return out;
}

inline D3DXMATRIX* D3DXMatrixPerspectiveFovLH(D3DXMATRIX* out, float fovY, float aspect, float zNear, float zFar)
{
    d3dx_compat_detail::Store(out, DirectX::XMMatrixPerspectiveFovLH(fovY, aspect, zNear, zFar));
    return out;
}

inline D3DXMATRIX* D3DXMatrixPerspectiveOffCenterLH(D3DXMATRIX* out, float left, float right, float bottom, float top, float zNear, float zFar)
{
    d3dx_compat_detail::Store(out, DirectX::XMMatrixPerspectiveOffCenterLH(left, right, bottom, top, zNear, zFar));
    return out;
}

// Projects onto the plane; light.w == 0 is a directional light, 1 a point light.
inline D3DXMATRIX* D3DXMatrixShadow(D3DXMATRIX* out, const D3DXVECTOR4* light, const D3DXPLANE* plane)
{
    using namespace d3dx_compat_detail;
    Store(out, DirectX::XMMatrixShadow(Load(*plane), Load(*light)));
    return out;
}

// --- Quaternions and planes ------------------------------------------------

inline D3DXQUATERNION* D3DXQuaternionSlerp(D3DXQUATERNION* out, const D3DXQUATERNION* a, const D3DXQUATERNION* b, float t)
{
    using namespace d3dx_compat_detail;
    const DirectX::XMVECTOR q = DirectX::XMQuaternionSlerp(Load(*a), Load(*b), t);
    *out = D3DXQUATERNION(DirectX::XMVectorGetX(q), DirectX::XMVectorGetY(q),
                          DirectX::XMVectorGetZ(q), DirectX::XMVectorGetW(q));
    return out;
}

inline D3DXPLANE* D3DXPlaneFromPointNormal(D3DXPLANE* out, const D3DXVECTOR3* point, const D3DXVECTOR3* normal)
{
    *out = D3DXPLANE(normal->x, normal->y, normal->z, -D3DXVec3Dot(point, normal));
    return out;
}

// The normal follows (p2 - p1) x (p3 - p1), normalized.
inline D3DXPLANE* D3DXPlaneFromPoints(D3DXPLANE* out, const D3DXVECTOR3* p1, const D3DXVECTOR3* p2, const D3DXVECTOR3* p3)
{
    const D3DXVECTOR3 edge1 = *p2 - *p1;
    const D3DXVECTOR3 edge2 = *p3 - *p1;
    D3DXVECTOR3 normal;
    D3DXVec3Cross(&normal, &edge1, &edge2);
    D3DXVec3Normalize(&normal, &normal);
    return D3DXPlaneFromPointNormal(out, p1, &normal);
}

inline float D3DXPlaneDotCoord(const D3DXPLANE* plane, const D3DXVECTOR3* v)
{
    return plane->a * v->x + plane->b * v->y + plane->c * v->z + plane->d;
}

// --- Bounding probes -----------------------------------------------------

// Ray vs. axis-aligned box. Returns 1 when the ray (not the full line) hits it.
inline int D3DXBoxBoundProbe(const D3DXVECTOR3* boxMin, const D3DXVECTOR3* boxMax, const D3DXVECTOR3* rayPosition, const D3DXVECTOR3* rayDirection)
{
    const float lo[3] = {boxMin->x, boxMin->y, boxMin->z};
    const float hi[3] = {boxMax->x, boxMax->y, boxMax->z};
    const float origin[3] = {rayPosition->x, rayPosition->y, rayPosition->z};
    const float direction[3] = {rayDirection->x, rayDirection->y, rayDirection->z};

    float tNear = -INFINITY;
    float tFar = INFINITY;
    for (int axis = 0; axis < 3; ++axis) {
        if (direction[axis] == 0.0f) {
            if (origin[axis] < lo[axis] || origin[axis] > hi[axis]) {
                return 0;
            }
            continue;
        }
        float t1 = (lo[axis] - origin[axis]) / direction[axis];
        float t2 = (hi[axis] - origin[axis]) / direction[axis];
        if (t1 > t2) {
            const float swap = t1;
            t1 = t2;
            t2 = swap;
        }
        tNear = t1 > tNear ? t1 : tNear;
        tFar = t2 < tFar ? t2 : tFar;
        if (tNear > tFar) {
            return 0;
        }
    }
    return tFar >= 0.0f ? 1 : 0;
}

// Ray vs. sphere. A ray starting inside the sphere counts as a hit.
inline int D3DXSphereBoundProbe(const D3DXVECTOR3* center, float radius, const D3DXVECTOR3* rayPosition, const D3DXVECTOR3* rayDirection)
{
    const D3DXVECTOR3 offset = *rayPosition - *center;
    const float a = D3DXVec3Dot(rayDirection, rayDirection);
    const float b = 2.0f * D3DXVec3Dot(&offset, rayDirection);
    const float c = D3DXVec3Dot(&offset, &offset) - radius * radius;
    if (a == 0.0f) {
        return c <= 0.0f ? 1 : 0;
    }
    const float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) {
        return 0;
    }
    const float farT = (-b + std::sqrt(discriminant)) / (2.0f * a);
    return farT >= 0.0f ? 1 : 0;
}
