#pragma once

/*
    raster_math.h - Math library for raster engine

    This file directly integrates linmath.h with minimal additions
    for color handling and utility functions.
*/

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <math.h>
#include "linmath.h"

    typedef struct
    {
        float r;
        float g;
        float b;
    } color_t;

    typedef struct
    {
        float r;
        float g;
        float b;
        float a;
    } color_rgba_t;

    // Create a color
    static inline color_t color_rgb(float r, float g, float b)
    {
        color_t c = { r, g, b };
        return c;
    }

    // Create a color with alpha
    static inline color_rgba_t color_rgba(float r, float g, float b, float a)
    {
        color_rgba_t c = { r, g, b, a };
        return c;
    }

    // Predefined colors
    static inline color_t color_white(void)
    {
        return color_rgb(1.0f, 1.0f, 1.0f);
    }
    static inline color_t color_black(void)
    {
        return color_rgb(0.0f, 0.0f, 0.0f);
    }
    static inline color_t color_red(void)
    {
        return color_rgb(1.0f, 0.0f, 0.0f);
    }
    static inline color_t color_green(void)
    {
        return color_rgb(0.0f, 1.0f, 0.0f);
    }
    static inline color_t color_blue(void)
    {
        return color_rgb(0.0f, 0.0f, 1.0f);
    }
    static inline color_t color_yellow(void)
    {
        return color_rgb(1.0f, 1.0f, 0.0f);
    }
    static inline color_t color_cyan(void)
    {
        return color_rgb(0.0f, 1.0f, 1.0f);
    }
    static inline color_t color_magenta(void)
    {
        return color_rgb(1.0f, 0.0f, 1.0f);
    }

    // Color blending
    static inline color_t color_lerp(color_t a, color_t b, float t)
    {
        return color_rgb(a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t);
    }

    static inline color_rgba_t color_rgba_lerp(color_rgba_t a, color_rgba_t b, float t)
    {
        return color_rgba(a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t);
    }

    // Convert degrees to radians
    static inline float deg_to_rad(float degrees)
    {
        return degrees * (3.14159265359f / 180.0f);
    }

    // Convert radians to degrees
    static inline float rad_to_deg(float radians)
    {
        return radians * (180.0f / 3.14159265359f);
    }

    // Linear interpolation
    static inline float lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

    // Clamp value between min and max
    static inline float clamp(float value, float min, float max)
    {
        if (value < min)
            return min;
        if (value > max)
            return max;
        return value;
    }

    // For compatibility with engine code
    typedef color_t      color;
    typedef color_rgba_t color_a; // Renamed to avoid conflict with the function

#ifdef __cplusplus
}
#endif