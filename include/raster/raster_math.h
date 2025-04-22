#pragma once

/*
    raster_math.h - simple math library for 2D/3D graphics
    
    This library provides basic math operations for 2D/3D graphics,
    including vectors, matrices, and color operations.
*/

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <math.h>

// ============================
// Vector types
// ============================

typedef struct rmath_vec2 {
    float x;
    float y;
} rmath_vec2_t;

typedef struct rmath_vec3 {
    float x;
    float y;
    float z;
} rmath_vec3_t;

typedef struct rmath_vec4 {
    float x;
    float y;
    float z;
    float w;
} rmath_vec4_t;

// ============================
// Color types
// ============================

typedef struct rmath_color {
    float r;
    float g;
    float b;
} rmath_color_t;

typedef struct rmath_color_rgba {
    float r;
    float g;
    float b;
    float a;
} rmath_color_rgba_t;

// ============================
// 2D Matrix (3x3 for 2D transformations)
// ============================

typedef struct rmath_mat3 {
    float m[9]; // Row-major: [0,1,2] [3,4,5] [6,7,8]
} rmath_mat3_t;

// ============================
// 3D Matrix (4x4)
// ============================

typedef struct rmath_mat4 {
    float m[16]; // Row-major: [0,1,2,3] [4,5,6,7] [8,9,10,11] [12,13,14,15]
} rmath_mat4_t;

// ============================
// Vec2 operations
// ============================

// Constructors
static inline rmath_vec2_t rmath_vec2(float x, float y) {
    rmath_vec2_t v = { x, y };
    return v;
}

static inline rmath_vec2_t rmath_vec2_zero(void) {
    return rmath_vec2(0.0f, 0.0f);
}

static inline rmath_vec2_t rmath_vec2_one(void) {
    return rmath_vec2(1.0f, 1.0f);
}

// Basic operations
static inline rmath_vec2_t rmath_vec2_add(rmath_vec2_t a, rmath_vec2_t b) {
    return rmath_vec2(a.x + b.x, a.y + b.y);
}

static inline rmath_vec2_t rmath_vec2_sub(rmath_vec2_t a, rmath_vec2_t b) {
    return rmath_vec2(a.x - b.x, a.y - b.y);
}

static inline rmath_vec2_t rmath_vec2_mul(rmath_vec2_t a, float scalar) {
    return rmath_vec2(a.x * scalar, a.y * scalar);
}

static inline rmath_vec2_t rmath_vec2_div(rmath_vec2_t a, float scalar) {
    return rmath_vec2(a.x / scalar, a.y / scalar);
}

static inline float rmath_vec2_dot(rmath_vec2_t a, rmath_vec2_t b) {
    return a.x * b.x + a.y * b.y;
}

static inline float rmath_vec2_cross(rmath_vec2_t a, rmath_vec2_t b) {
    return a.x * b.y - a.y * b.x;
}

static inline float rmath_vec2_length_sq(rmath_vec2_t v) {
    return v.x * v.x + v.y * v.y;
}

static inline float rmath_vec2_length(rmath_vec2_t v) {
    return sqrtf(rmath_vec2_length_sq(v));
}

static inline rmath_vec2_t rmath_vec2_normalize(rmath_vec2_t v) {
    float length = rmath_vec2_length(v);
    if (length < 0.0001f) {
        return rmath_vec2_zero();
    }
    return rmath_vec2_div(v, length);
}

// ============================
// Vec3 operations
// ============================

// Constructors
static inline rmath_vec3_t rmath_vec3(float x, float y, float z) {
    rmath_vec3_t v = { x, y, z };
    return v;
}

static inline rmath_vec3_t rmath_vec3_zero(void) {
    return rmath_vec3(0.0f, 0.0f, 0.0f);
}

static inline rmath_vec3_t rmath_vec3_one(void) {
    return rmath_vec3(1.0f, 1.0f, 1.0f);
}

// Basic operations
static inline rmath_vec3_t rmath_vec3_add(rmath_vec3_t a, rmath_vec3_t b) {
    return rmath_vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline rmath_vec3_t rmath_vec3_sub(rmath_vec3_t a, rmath_vec3_t b) {
    return rmath_vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline rmath_vec3_t rmath_vec3_mul(rmath_vec3_t a, float scalar) {
    return rmath_vec3(a.x * scalar, a.y * scalar, a.z * scalar);
}

static inline rmath_vec3_t rmath_vec3_div(rmath_vec3_t a, float scalar) {
    return rmath_vec3(a.x / scalar, a.y / scalar, a.z / scalar);
}

static inline float rmath_vec3_dot(rmath_vec3_t a, rmath_vec3_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline rmath_vec3_t rmath_vec3_cross(rmath_vec3_t a, rmath_vec3_t b) {
    return rmath_vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

static inline float rmath_vec3_length_sq(rmath_vec3_t v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

static inline float rmath_vec3_length(rmath_vec3_t v) {
    return sqrtf(rmath_vec3_length_sq(v));
}

static inline rmath_vec3_t rmath_vec3_normalize(rmath_vec3_t v) {
    float length = rmath_vec3_length(v);
    if (length < 0.0001f) {
        return rmath_vec3_zero();
    }
    return rmath_vec3_div(v, length);
}

// ============================
// Color operations
// ============================

// Constructors
static inline rmath_color_t rmath_color(float r, float g, float b) {
    rmath_color_t c = { r, g, b };
    return c;
}

static inline rmath_color_rgba_t rmath_color_rgba(float r, float g, float b, float a) {
    rmath_color_rgba_t c = { r, g, b, a };
    return c;
}

// Predefined colors
static inline rmath_color_t rmath_color_white(void) { return rmath_color(1.0f, 1.0f, 1.0f); }
static inline rmath_color_t rmath_color_black(void) { return rmath_color(0.0f, 0.0f, 0.0f); }
static inline rmath_color_t rmath_color_red(void) { return rmath_color(1.0f, 0.0f, 0.0f); }
static inline rmath_color_t rmath_color_green(void) { return rmath_color(0.0f, 1.0f, 0.0f); }
static inline rmath_color_t rmath_color_blue(void) { return rmath_color(0.0f, 0.0f, 1.0f); }
static inline rmath_color_t rmath_color_yellow(void) { return rmath_color(1.0f, 1.0f, 0.0f); }
static inline rmath_color_t rmath_color_cyan(void) { return rmath_color(0.0f, 1.0f, 1.0f); }
static inline rmath_color_t rmath_color_magenta(void) { return rmath_color(1.0f, 0.0f, 1.0f); }

// Color blending
static inline rmath_color_t rmath_color_lerp(rmath_color_t a, rmath_color_t b, float t) {
    return rmath_color(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t
    );
}

static inline rmath_color_rgba_t rmath_color_rgba_lerp(rmath_color_rgba_t a, rmath_color_rgba_t b, float t) {
    return rmath_color_rgba(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    );
}

// ============================
// Matrix 3x3 operations (for 2D)
// ============================

// Identity matrix
static inline rmath_mat3_t rmath_mat3_identity(void) {
    rmath_mat3_t result = {
        .m = {
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f
        }
    };
    return result;
}

// Translation matrix
static inline rmath_mat3_t rmath_mat3_translation(float x, float y) {
    rmath_mat3_t result = {
        .m = {
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            x,    y,    1.0f
        }
    };
    return result;
}

// Rotation matrix (angle in radians)
static inline rmath_mat3_t rmath_mat3_rotation(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    rmath_mat3_t result = {
        .m = {
            c,   -s,    0.0f,
            s,    c,    0.0f,
            0.0f, 0.0f, 1.0f
        }
    };
    return result;
}

// Scale matrix
static inline rmath_mat3_t rmath_mat3_scale(float x, float y) {
    rmath_mat3_t result = {
        .m = {
            x,    0.0f, 0.0f,
            0.0f, y,    0.0f,
            0.0f, 0.0f, 1.0f
        }
    };
    return result;
}

// Matrix multiplication
static inline rmath_mat3_t rmath_mat3_mul(rmath_mat3_t a, rmath_mat3_t b) {
    rmath_mat3_t result;
    
    // Row 0
    result.m[0] = a.m[0] * b.m[0] + a.m[1] * b.m[3] + a.m[2] * b.m[6];
    result.m[1] = a.m[0] * b.m[1] + a.m[1] * b.m[4] + a.m[2] * b.m[7];
    result.m[2] = a.m[0] * b.m[2] + a.m[1] * b.m[5] + a.m[2] * b.m[8];
    
    // Row 1
    result.m[3] = a.m[3] * b.m[0] + a.m[4] * b.m[3] + a.m[5] * b.m[6];
    result.m[4] = a.m[3] * b.m[1] + a.m[4] * b.m[4] + a.m[5] * b.m[7];
    result.m[5] = a.m[3] * b.m[2] + a.m[4] * b.m[5] + a.m[5] * b.m[8];
    
    // Row 2
    result.m[6] = a.m[6] * b.m[0] + a.m[7] * b.m[3] + a.m[8] * b.m[6];
    result.m[7] = a.m[6] * b.m[1] + a.m[7] * b.m[4] + a.m[8] * b.m[7];
    result.m[8] = a.m[6] * b.m[2] + a.m[7] * b.m[5] + a.m[8] * b.m[8];
    
    return result;
}

// Transform vector
static inline rmath_vec2_t rmath_mat3_transform(rmath_mat3_t m, rmath_vec2_t v) {
    return rmath_vec2(
        v.x * m.m[0] + v.y * m.m[3] + m.m[6],
        v.x * m.m[1] + v.y * m.m[4] + m.m[7]
    );
}

// ============================
// Matrix 4x4 operations (for 3D)
// ============================

// Identity matrix
static inline rmath_mat4_t rmath_mat4_identity(void) {
    rmath_mat4_t result = {
        .m = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        }
    };
    return result;
}

// Translation matrix
static inline rmath_mat4_t rmath_mat4_translation(float x, float y, float z) {
    rmath_mat4_t result = rmath_mat4_identity();
    result.m[12] = x;
    result.m[13] = y;
    result.m[14] = z;
    return result;
}

// Rotation matrix for X axis (angle in radians)
static inline rmath_mat4_t rmath_mat4_rotation_x(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    rmath_mat4_t result = rmath_mat4_identity();
    result.m[5] = c;
    result.m[6] = -s;
    result.m[9] = s;
    result.m[10] = c;
    return result;
}

// Rotation matrix for Y axis (angle in radians)
static inline rmath_mat4_t rmath_mat4_rotation_y(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    rmath_mat4_t result = rmath_mat4_identity();
    result.m[0] = c;
    result.m[2] = s;
    result.m[8] = -s;
    result.m[10] = c;
    return result;
}

// Rotation matrix for Z axis (angle in radians)
static inline rmath_mat4_t rmath_mat4_rotation_z(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    rmath_mat4_t result = rmath_mat4_identity();
    result.m[0] = c;
    result.m[1] = -s;
    result.m[4] = s;
    result.m[5] = c;
    return result;
}

// Scale matrix
static inline rmath_mat4_t rmath_mat4_scale(float x, float y, float z) {
    rmath_mat4_t result = rmath_mat4_identity();
    result.m[0] = x;
    result.m[5] = y;
    result.m[10] = z;
    return result;
}

// Perspective projection matrix
static inline rmath_mat4_t rmath_mat4_perspective(float fov_y, float aspect, float near_z, float far_z) {
    float tan_half_fov = tanf(fov_y / 2.0f);
    
    rmath_mat4_t result = {0};
    result.m[0] = 1.0f / (aspect * tan_half_fov);
    result.m[5] = 1.0f / tan_half_fov;
    result.m[10] = -(far_z + near_z) / (far_z - near_z);
    result.m[11] = -1.0f;
    result.m[14] = -(2.0f * far_z * near_z) / (far_z - near_z);
    
    return result;
}

// Orthographic projection matrix
static inline rmath_mat4_t rmath_mat4_orthographic(float left, float right, float bottom, float top, float near_z, float far_z) {
    rmath_mat4_t result = rmath_mat4_identity();
    
    result.m[0] = 2.0f / (right - left);
    result.m[5] = 2.0f / (top - bottom);
    result.m[10] = -2.0f / (far_z - near_z);
    
    result.m[12] = -(right + left) / (right - left);
    result.m[13] = -(top + bottom) / (top - bottom);
    result.m[14] = -(far_z + near_z) / (far_z - near_z);
    
    return result;
}

// Matrix multiplication
static inline rmath_mat4_t rmath_mat4_mul(rmath_mat4_t a, rmath_mat4_t b) {
    rmath_mat4_t result = {0};
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.m[i * 4 + j] += a.m[i * 4 + k] * b.m[k * 4 + j];
            }
        }
    }
    
    return result;
}

// Transform vector
static inline rmath_vec3_t rmath_mat4_transform_vec3(rmath_mat4_t m, rmath_vec3_t v) {
    return rmath_vec3(
        v.x * m.m[0] + v.y * m.m[4] + v.z * m.m[8] + m.m[12],
        v.x * m.m[1] + v.y * m.m[5] + v.z * m.m[9] + m.m[13],
        v.x * m.m[2] + v.y * m.m[6] + v.z * m.m[10] + m.m[14]
    );
}

// Transform vector (with w component)
static inline rmath_vec4_t rmath_mat4_transform_vec4(rmath_mat4_t m, rmath_vec4_t v) {
    return (rmath_vec4_t){
        v.x * m.m[0] + v.y * m.m[4] + v.z * m.m[8] + v.w * m.m[12],
        v.x * m.m[1] + v.y * m.m[5] + v.z * m.m[9] + v.w * m.m[13],
        v.x * m.m[2] + v.y * m.m[6] + v.z * m.m[10] + v.w * m.m[14],
        v.x * m.m[3] + v.y * m.m[7] + v.z * m.m[11] + v.w * m.m[15]
    };
}

// ============================
// Utility functions
// ============================

// Convert degrees to radians
static inline float rmath_deg_to_rad(float degrees) {
    return degrees * (3.14159265359f / 180.0f);
}

// Convert radians to degrees
static inline float rmath_rad_to_deg(float radians) {
    return radians * (180.0f / 3.14159265359f);
}

// Linear interpolation
static inline float rmath_lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// Clamp value between min and max
static inline float rmath_clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

#ifdef __cplusplus
}
#endif