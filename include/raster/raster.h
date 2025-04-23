// filepath: /Users/mikael/source/raster/include/raster/raster.h
#pragma once

/*
    raster.h - single-header include for the Raster engine

    This file includes all of the Raster engine's functionality in a single header,
    making it easier to use the engine in your projects.

    To use this header, simply include it in your project:
    #include "raster/raster.h"
*/

#ifdef __cplusplus
extern "C"
{
#endif

#include "raster_math.h"
#include "raster_app.h"
#include "raster_gfx.h"
#include "raster_input.h"
#include "raster_log.h"

    typedef rmath_vec2_t  vec2;
    typedef rmath_vec3_t  vec3;
    typedef rmath_vec4_t  vec4;
    typedef rmath_color_t color;

#ifdef __cplusplus
}
#endif