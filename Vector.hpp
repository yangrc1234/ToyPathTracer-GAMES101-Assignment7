//
// Created by LEI XU on 5/13/19.
//
#pragma once

#include <iostream>
#include <cmath>
#include <algorithm>

class alignas(16) Vector3f;
inline double DotProduct(const Vector3f& a, const Vector3f& b);

class alignas(16) Vector3f {
public:
    float x, y, z;
    Vector3f() : x(0), y(0), z(0) {}
    Vector3f(float xx) : x(xx), y(xx), z(xx) {}
    Vector3f(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {
#if _DEBUG
        if (isnan(xx) || isnan(yy) || isnan(zz))
            __debugbreak();
#endif
    }
    Vector3f operator * (const float &r) const { return Vector3f(x * r, y * r, z * r); }
    Vector3f operator / (const float &r) const { return Vector3f(x / r, y / r, z / r); }

    inline float SqrMagnitude() { return x * x + y * y + z * z; }

    inline float Magnitude() {return std::sqrt(SqrMagnitude());}

    inline Vector3f Normalized() {
        float n = std::sqrt(x * x + y * y + z * z);
        return Vector3f(x / n, y / n, z / n);
    }

    inline Vector3f NormlizeAndGetLengthSqr(float* lengthSqr) {
        *lengthSqr = DotProduct(*this, *this);
        return *this/ std::sqrt(*lengthSqr);
    }

    Vector3f operator * (const Vector3f& v) const { return Vector3f(x * v.x, y * v.y, z * v.z); }
    Vector3f operator / (const Vector3f& v) const { return Vector3f(x / v.x, y / v.y, z / v.z); }
    Vector3f operator - (const Vector3f &v) const { return Vector3f(x - v.x, y - v.y, z - v.z); }
    Vector3f operator + (const Vector3f &v) const { return Vector3f(x + v.x, y + v.y, z + v.z); }
    Vector3f operator - () const { return Vector3f(-x, -y, -z); }
    Vector3f& operator += (const Vector3f &v) { x += v.x, y += v.y, z += v.z; return *this; }
    friend Vector3f operator * (const float &r, const Vector3f &v)
    { return Vector3f(v.x * r, v.y * r, v.z * r); }
    friend std::ostream & operator << (std::ostream &os, const Vector3f &v)
    { return os << v.x << ", " << v.y << ", " << v.z; }
    float       operator[](int index) const;
    float&      operator[](int index);

    static inline Vector3f Lerp(Vector3f a, Vector3f b, float t) {
        return Vector3f(
            (1-t) * a.x + t * b.x,
            (1-t) * a.y + t * b.y,
            (1-t) * a.z + t * b.z
        );
    }

    static inline Vector3f One(){
        return Vector3f(1.0f, 1.0f, 1.0f);
    }

    static Vector3f Min(const Vector3f &p1, const Vector3f &p2) {
        return Vector3f(std::min(p1.x, p2.x), std::min(p1.y, p2.y),
                       std::min(p1.z, p2.z));
    }

    static Vector3f Max(const Vector3f &p1, const Vector3f &p2) {
        return Vector3f(std::max(p1.x, p2.x), std::max(p1.y, p2.y),
                       std::max(p1.z, p2.z));
    }

    Vector3f Cross(const Vector3f& v) const {
        auto result = Vector3f();
        result.x = y * v.z - z * v.y;
        result.y = z * v.x - x * v.z;
        result.z = x * v.y - y * v.x;
        return result;
    }
};
inline float Vector3f::operator[](int index) const {
    return (&x)[index];
}


class Vector2f
{
public:
    Vector2f() : x(0), y(0) {}
    Vector2f(float xx) : x(xx), y(xx) {}
    Vector2f(float xx, float yy) : x(xx), y(yy) {}
    Vector2f operator * (const float &r) const { return Vector2f(x * r, y * r); }
    Vector2f operator + (const Vector2f &v) const { return Vector2f(x + v.x, y + v.y); }
    float x, y;
};

inline Vector3f Lerp(const Vector3f &a, const Vector3f& b, const float &t)
{ return a * (1 - t) + b * t; }

inline double DotProduct(const Vector3f &a, const Vector3f &b)
{ return (double)a.x * b.x + (double)a.y * b.y + (double)a.z * b.z; }

inline Vector3f CrossProduct(const Vector3f &a, const Vector3f &b)
{
    return Vector3f(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
    );
}
