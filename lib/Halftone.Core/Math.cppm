module;
#include <cmath>

export module com.citronlabs.Halftone.Core:Math;

import :Types;

// Minimal, dependency-free linear algebra. Column-major Mat4 to match
// Vulkan/GLSL conventions; clip space is Vulkan's (y down, depth 0..1).
// Extend as features need it (quaternions, etc.).
export namespace Halftone::Core
{
    struct Vec2 { f32 x = 0, y = 0; };
    struct Vec3 { f32 x = 0, y = 0, z = 0; };
    struct Vec4 { f32 x = 0, y = 0, z = 0, w = 0; };

    constexpr Vec2 operator+(Vec2 a, Vec2 b) { return { a.x + b.x, a.y + b.y }; }
    constexpr Vec2 operator-(Vec2 a, Vec2 b) { return { a.x - b.x, a.y - b.y }; }
    constexpr Vec2 operator*(Vec2 a, f32 s)  { return { a.x * s,   a.y * s   }; }

    constexpr Vec3 operator+(Vec3 a, Vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
    constexpr Vec3 operator-(Vec3 a, Vec3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
    constexpr Vec3 operator-(Vec3 a)         { return { -a.x, -a.y, -a.z }; }
    constexpr Vec3 operator*(Vec3 a, f32 s)  { return { a.x * s,   a.y * s,   a.z * s   }; }

    constexpr f32  Dot(Vec3 a, Vec3 b)   { return a.x * b.x + a.y * b.y + a.z * b.z; }
    constexpr Vec3 Cross(Vec3 a, Vec3 b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
    inline    f32  Length(Vec3 v)        { return std::sqrt(Dot(v, v)); }
    inline    Vec3 Normalize(Vec3 v)     { f32 l = Length(v); return l > 0 ? v * (1.0f / l) : v; }

    struct Mat4
    {
        f32 m[16] = { 1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1 };  // identity

        constexpr f32&       operator()(u32 col, u32 row)       { return m[col * 4 + row]; }
        constexpr const f32& operator()(u32 col, u32 row) const { return m[col * 4 + row]; }

        static constexpr Mat4 Identity() { return {}; }
    };

    constexpr Mat4 operator*(const Mat4& a, const Mat4& b)
    {
        Mat4 r;
        for (u32 c = 0; c < 4; c++)
            for (u32 rIdx = 0; rIdx < 4; rIdx++)
            {
                f32 sum = 0;
                for (u32 k = 0; k < 4; k++) sum += a(k, rIdx) * b(c, k);
                r(c, rIdx) = sum;
            }
        return r;
    }

    constexpr Vec4 operator*(const Mat4& a, Vec4 v)
    {
        return {
            a(0,0) * v.x + a(1,0) * v.y + a(2,0) * v.z + a(3,0) * v.w,
            a(0,1) * v.x + a(1,1) * v.y + a(2,1) * v.z + a(3,1) * v.w,
            a(0,2) * v.x + a(1,2) * v.y + a(2,2) * v.z + a(3,2) * v.w,
            a(0,3) * v.x + a(1,3) * v.y + a(2,3) * v.z + a(3,3) * v.w,
        };
    }

    constexpr Vec3 TransformPoint(const Mat4& a, Vec3 p)
    {
        Vec4 r = a * Vec4 { p.x, p.y, p.z, 1.0f };
        return { r.x, r.y, r.z };
    }

    constexpr Mat4 Translation(Vec3 t)
    {
        Mat4 r; r(3,0) = t.x; r(3,1) = t.y; r(3,2) = t.z; return r;
    }

    constexpr Mat4 Scaling(Vec3 s)
    {
        Mat4 r; r(0,0) = s.x; r(1,1) = s.y; r(2,2) = s.z; return r;
    }

    // Euler XYZ (applied X, then Y, then Z)
    inline Mat4 Rotation(Vec3 e)
    {
        const f32 cx = std::cos(e.x), sx = std::sin(e.x);
        const f32 cy = std::cos(e.y), sy = std::sin(e.y);
        const f32 cz = std::cos(e.z), sz = std::sin(e.z);
        Mat4 rx, ry, rz;
        rx(1,1) = cx;  rx(2,1) = -sx; rx(1,2) = sx; rx(2,2) = cx;
        ry(0,0) = cy;  ry(2,0) = sy;  ry(0,2) = -sy; ry(2,2) = cy;
        rz(0,0) = cz;  rz(1,0) = -sz; rz(0,1) = sz; rz(1,1) = cz;
        return rz * ry * rx;
    }

    // Right-handed view matrix (camera looks down -Z).
    inline Mat4 LookAt(Vec3 eye, Vec3 target, Vec3 up)
    {
        const Vec3 f = Normalize(target - eye);
        const Vec3 s = Normalize(Cross(f, up));
        const Vec3 u = Cross(s, f);
        Mat4 r;
        r(0,0) = s.x;  r(1,0) = s.y;  r(2,0) = s.z;  r(3,0) = -Dot(s, eye);
        r(0,1) = u.x;  r(1,1) = u.y;  r(2,1) = u.z;  r(3,1) = -Dot(u, eye);
        r(0,2) = -f.x; r(1,2) = -f.y; r(2,2) = -f.z; r(3,2) =  Dot(f, eye);
        return r;
    }

    // Vulkan clip space: y down, depth 0..1.
    inline Mat4 Perspective(f32 fovY, f32 aspect, f32 nearClip, f32 farClip)
    {
        const f32 f = 1.0f / std::tan(fovY * 0.5f);
        Mat4 r;
        r(0,0) = f / aspect;
        r(1,1) = -f;
        r(2,2) = farClip / (nearClip - farClip);
        r(2,3) = -1.0f;
        r(3,2) = (nearClip * farClip) / (nearClip - farClip);
        r(3,3) = 0.0f;
        return r;
    }

    inline Mat4 Orthographic(f32 height, f32 aspect, f32 nearClip, f32 farClip)
    {
        const f32 w = height * aspect;
        Mat4 r;
        r(0,0) = 2.0f / w;
        r(1,1) = -2.0f / height;
        r(2,2) = 1.0f / (nearClip - farClip);
        r(3,2) = nearClip / (nearClip - farClip);
        return r;
    }

    struct Transform
    {
        Vec3 Position;
        Vec3 Rotation;              // Euler, radians (XYZ)
        Vec3 Scale = { 1, 1, 1 };

        Mat4 Matrix() const { return Translation(Position) * Halftone::Core::Rotation(Rotation) * Scaling(Scale); }
    };
}
