#pragma once

#include <math.h>

#define array_count(arr) ((sizeof(arr))/sizeof(*(arr)))

#define F_PI (3.14159265359f)
#define F_DEG_TO_RAD (F_PI / 180.0f)
#define F_RAD_TO_DEG (180.0f / F_PI)

typedef struct Vector2 {
    float x, y;
} Vector2;

typedef struct Vector3 {
    float x, y, z;
} Vector3;

typedef struct Vector4 {
    float x, y, z, w;
} Vector4;

typedef struct Quaternion {
    float x, y, z, w;
} Quaternion;

typedef struct Matrix4 {
    float m00, m10, m20, m30;
    float m01, m11, m21, m31;
    float m02, m12, m22, m32;
    float m03, m13, m23, m33;
} Matrix4;

#if !defined(UFBX_VERSION)
    #include "ufbx.h"
#endif

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

static float Vector2_dot(Vector2 a, Vector2 b) {
    return a.x*b.x + a.y*b.y;
}

static float Vector3_dot(Vector3 a, Vector3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static float Vector4_dot(Vector4 a, Vector4 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

static float Vector2_length(Vector2 a) {
    return sqrtf(Vector2_dot(a, a));
}

static float Vector3_length(Vector3 a) {
    return sqrtf(Vector3_dot(a, a));
}

static float Vector4_length(Vector4 a) {
    return sqrtf(Vector4_dot(a, a));
}

static Vector2 Vector2_normalize(Vector2 a) {
    return Vector2_mul(a, 1.0f / Vector2_length(a));
}

static Vector3 Vector3_normalize(Vector3 a) {
    return Vector3_mul(a, 1.0f / Vector3_length(a));
}

static Vector4 Vector4_normalize(Vector4 a) {
    return Vector4_mul(a, 1.0f / Vector4_length(a));
}

static Vector3 Vector3_cross(Vector3 a, Vector3 b) {
    return Vector3_new(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}

static Quaternion Quaternion_new(float x, float y, float z, float w)
{
    Quaternion r = { x, y, z, w };
    return r;
}

static Quaternion Quaternion_axis_angle(Vector3 axis, float radians)
{
	axis = Vector3_normalize(axis);
	float c = cosf(radians * 0.5f), s = sinf(radians * 0.5f);
	return Quaternion_new(axis.x * s, axis.y * s, axis.z * s, c);
}

static Quaternion Quaternion_mul(Quaternion a, Quaternion b)
{
	return Quaternion_new(
		a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
		a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
		a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
		a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z);
}

static Vector3 Quaternion_rotate(Quaternion a, Vector3 b)
{
	float xy = a.x*b.y - a.y*b.x;
	float xz = a.x*b.z - a.z*b.x;
	float yz = a.y*b.z - a.z*b.y;
	return Vector3_new(
		2.0f * (+ a.w*yz + a.y*xy + a.z*xz) + b.x,
		2.0f * (- a.x*xy - a.w*xz + a.z*yz) + b.y,
		2.0f * (- a.x*xz - a.y*yz + a.w*xy) + b.z);
}

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

static Matrix4 Matrix4_transpose(Matrix4 a)
{
    Matrix4 r = {
        a.m00, a.m01, a.m02, a.m03,
        a.m10, a.m11, a.m12, a.m13,
        a.m20, a.m21, a.m22, a.m23,
        a.m30, a.m31, a.m32, a.m33,
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


static Vector4 Vector4_xyz(Vector3 p)
{
    Vector4 r = { p.x, p.y, p.z, 0.0f }; 
    return r;
}

static Matrix4 Matrix4_inverse_basis(Vector3 x, Vector3 y, Vector3 z, Vector3 origin)
{
	Matrix4 r = {
		x.x, x.y, x.z, -Vector3_dot(origin, x),
		y.x, y.y, y.z, -Vector3_dot(origin, y),
		z.x, z.y, z.z, -Vector3_dot(origin, z),
		0, 0, 0, 1,
    };
    return Matrix4_transpose(r);
}

static Matrix4 Matrix4_perspective_gl(float fov_degrees, float aspect, float near_plane, float far_plane)
{
    float fov = fov_degrees * (F_PI / 180.0f);
	float tan_fov = 1.0f / tanf(fov / 2.0f);
	float n = near_plane, f = far_plane;
	Matrix4 r = {
		tan_fov / aspect, 0, 0, 0,
		0, tan_fov, 0, 0,
		0, 0, (f+n) / (f-n), -2.0f * (f*n)/(f-n),
		0, 0, 1, 0,
    };
    return Matrix4_transpose(r);
}

static Matrix4 Matrix4_look(Vector3 position, Vector3 direction, Vector3 up_hint)
{
	Vector3 right = Vector3_normalize(Vector3_cross(direction, up_hint));
	Vector3 up = Vector3_normalize(Vector3_cross(right, direction));
	return Matrix4_inverse_basis(right, up, direction, position);
}

static float float_min(float a, float b) {
    return a < b ? a : b;
}
static float float_max(float a, float b) {
    return a < b ? b : a;
}
static float float_clamp(float a, float min_v, float max_v) {
    return float_min(float_max(a, min_v), max_v);
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
