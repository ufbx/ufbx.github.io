#pragma once

typedef struct Vector2 {
    float x, y;
} Vector2;

typedef struct Vector3 {
    float x, y, z;
} Vector3;

typedef struct Vector4 {
    float x, y, z, w;
} Vector4;

typedef struct Matrix4 {
    float m00, m10, m20, m30;
    float m01, m11, m21, m31;
    float m02, m12, m22, m32;
    float m03, m13, m23, m33;
} Matrix4;

#include "ufbx.h"

static Vector2 Vector2_new(float x, float y)
{
    Vector2 r = { x, y };
    return r;
}

static Vector3 Vector3_new(float x, float y, float z)
{
    Vector3 r = { x, y, z };
    return r;
}

static Vector4 Vector4_new(float x, float y, float z, float w)
{
    Vector4 r = { x, y, z, w };
    return r;
}

static Vector2 Vector2_add(Vector2 a, Vector2 b) {
    Vector2 r = { a.x + b.x, a.y + b.y };
    return r;
};

static Vector3 Vector3_add(Vector3 a, Vector3 b) {
    Vector3 r = { a.x + b.x, a.y + b.y, a.z + b.z };
    return r;
};

static Vector4 Vector4_add(Vector4 a, Vector4 b) {
    Vector4 r = { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    return r;
};

static Vector2 Vector2_sub(Vector2 a, Vector2 b) {
    Vector2 r = { a.x - b.x, a.y - b.y };
    return r;
};

static Vector3 Vector3_sub(Vector3 a, Vector3 b) {
    Vector3 r = { a.x - b.x, a.y - b.y, a.z - b.z };
    return r;
};

static Vector4 Vector4_sub(Vector4 a, Vector4 b) {
    Vector4 r = { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
    return r;
};

static Vector2 Vector2_mul(Vector2 a, float b) {
    Vector2 r = { a.x * b, a.y * b };
    return r;
};

static Vector3 Vector3_mul(Vector3 a, float b) {
    Vector3 r = { a.x * b, a.y * b, a.z * b };
    return r;
};

static Vector4 Vector4_mul(Vector4 a, float b) {
    Vector4 r = { a.x * b, a.y * b, a.z * b, a.w * b };
    return r;
};

static Matrix4 Matrix4_mul(Matrix4 a, Matrix4 b)
{
    Matrix4 r = {
        a.m00*b.m00 + a.m01*b.m10 + a.m02*b.m20 + a.m03*b.m30,
        a.m10*b.m00 + a.m11*b.m10 + a.m12*b.m20 + a.m13*b.m30,
        a.m20*b.m00 + a.m21*b.m10 + a.m22*b.m20 + a.m23*b.m30,
        a.m30*b.m00 + a.m31*b.m10 + a.m32*b.m20 + a.m33*b.m30,

        a.m00*b.m01 + a.m01*b.m11 + a.m02*b.m21 + a.m03*b.m31,
        a.m10*b.m01 + a.m11*b.m11 + a.m12*b.m21 + a.m13*b.m31,
        a.m20*b.m01 + a.m21*b.m11 + a.m22*b.m21 + a.m23*b.m31,
        a.m30*b.m01 + a.m31*b.m11 + a.m32*b.m21 + a.m33*b.m31,

        a.m00*b.m02 + a.m01*b.m12 + a.m02*b.m22 + a.m03*b.m32,
        a.m10*b.m02 + a.m11*b.m12 + a.m12*b.m22 + a.m13*b.m32,
        a.m20*b.m02 + a.m21*b.m12 + a.m22*b.m22 + a.m23*b.m32,
        a.m30*b.m02 + a.m31*b.m12 + a.m32*b.m22 + a.m33*b.m32,

        a.m00*b.m03 + a.m01*b.m13 + a.m02*b.m23 + a.m03*b.m33,
        a.m10*b.m03 + a.m11*b.m13 + a.m12*b.m23 + a.m13*b.m33,
        a.m20*b.m03 + a.m21*b.m13 + a.m22*b.m23 + a.m23*b.m33,
        a.m30*b.m03 + a.m31*b.m13 + a.m32*b.m23 + a.m33*b.m33,
    };
    return r;
}

static Vector3 Matrix4_transform_point(Matrix4 a, Vector3 b)
{
    Vector3 r = {
        a.m00*b.x + a.m01*b.y + a.m02*b.z + a.m03,
        a.m10*b.x + a.m11*b.y + a.m12*b.z + a.m13,
        a.m20*b.x + a.m21*b.y + a.m22*b.z + a.m23,
    };
    return r;
}

// -- ufbx conversions

static Vector2 Vector2_ufbx(ufbx_vec2 v) {
    Vector2 r = { (float)v.x, (float)v.y };
    return r;
};

static Vector3 Vector3_ufbx(ufbx_vec3 v) {
    Vector3 r = { (float)v.x, (float)v.y, (float)v.z };
    return r;
};

static Vector4 Vector4_ufbx(ufbx_vec4 v) {
    Vector4 r = { (float)v.x, (float)v.y, (float)v.z, (float)v.w };
    return r;
};

static Matrix4 Matrix4_ufbx(ufbx_matrix v) {
    Matrix4 r = {
        (float)v.m00, (float)v.m10, (float)v.m20, 0.0f,
        (float)v.m01, (float)v.m11, (float)v.m21, 0.0f,
        (float)v.m02, (float)v.m12, (float)v.m22, 0.0f,
        (float)v.m03, (float)v.m13, (float)v.m23, 1.0f,
    };
    return r;
}

#ifdef __cplusplus

// Operators for C++
static Vector2 operator+(Vector2 a, Vector2 b) { return Vector2_add(a, b); }
static Vector3 operator+(Vector3 a, Vector3 b) { return Vector3_add(a, b); }
static Vector4 operator+(Vector4 a, Vector4 b) { return Vector4_add(a, b); }
static Vector2 operator-(Vector2 a, Vector2 b) { return Vector2_sub(a, b); }
static Vector3 operator-(Vector3 a, Vector3 b) { return Vector3_sub(a, b); }
static Vector4 operator-(Vector4 a, Vector4 b) { return Vector4_sub(a, b); }
static Vector2 operator*(Vector2 a, float b) { return Vector2_mul(a, b); }
static Vector3 operator*(Vector3 a, float b) { return Vector3_mul(a, b); }
static Vector4 operator*(Vector4 a, float b) { return Vector4_mul(a, b); }
static Matrix4 operator*(Matrix4 a, Matrix4 b) { return Matrix4_mul(a, b); }
static Vector2 &operator+=(Vector2 &a, Vector2 b) { return a = Vector2_add(a, b); }
static Vector3 &operator+=(Vector3 &a, Vector3 b) { return a = Vector3_add(a, b); }
static Vector4 &operator+=(Vector4 &a, Vector4 b) { return a = Vector4_add(a, b); }
static Vector2 &operator-=(Vector2 &a, Vector2 b) { return a = Vector2_sub(a, b); }
static Vector3 &operator-=(Vector3 &a, Vector3 b) { return a = Vector3_sub(a, b); }
static Vector4 &operator-=(Vector4 &a, Vector4 b) { return a = Vector4_sub(a, b); }
static Vector2 &operator*=(Vector2 &a, float b) { return a = Vector2_mul(a, b); }
static Vector3 &operator*=(Vector3 &a, float b) { return a = Vector3_mul(a, b); }
static Vector4 &operator*=(Vector4 &a, float b) { return a = Vector4_mul(a, b); }
static Matrix4 &operator*=(Matrix4 &a, Matrix4 b) { return a = Matrix4_mul(a, b); }

// Implicit conversions for C++
template <> struct ufbx_converter<Vector2> {
    static Vector2 from(const ufbx_vec2 &v) { return Vector2_ufbx(v); }
};
template <> struct ufbx_converter<Vector3> {
    static Vector3 from(const ufbx_vec3 &v) { return Vector3_ufbx(v); }
};
template <> struct ufbx_converter<Vector4> {
    static Vector4 from(const ufbx_vec4 &v) { return Vector4_ufbx(v); }
};
template <> struct ufbx_converter<Matrix4> {
    static Matrix4 from(const ufbx_matrix &v) { return Matrix4_ufbx(v); }
};

#endif
