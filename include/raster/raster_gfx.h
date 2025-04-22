#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include "raster_math.h"

    // Graphics context
    bool rgfx_init(void);
    void rgfx_shutdown(void);

    // Rendering
    void rgfx_clear(float r, float g, float b);
    void rgfx_clear_color(rmath_color_t color);

    // Sprite type forward declaration
    typedef struct rgfx_sprite rgfx_sprite_t;

    // Sprite descriptor
    typedef struct
    {
        rmath_vec2_t position;
        rmath_vec2_t size;
        rmath_color_t color;
        int z_order;  // Optional depth ordering
    } rgfx_sprite_desc_t;

    // Sprite API
    rgfx_sprite_t* rgfx_sprite_create(const rgfx_sprite_desc_t* desc);
    void rgfx_sprite_destroy(rgfx_sprite_t* sprite);
    void rgfx_sprite_draw(rgfx_sprite_t* sprite);

    // Sprite properties
    void rgfx_sprite_set_position(rgfx_sprite_t* sprite, float x, float y);
    void rgfx_sprite_set_position_vec2(rgfx_sprite_t* sprite, rmath_vec2_t position);
    void rgfx_sprite_set_size(rgfx_sprite_t* sprite, float width, float height);
    void rgfx_sprite_set_size_vec2(rgfx_sprite_t* sprite, rmath_vec2_t size);
    void rgfx_sprite_set_color(rgfx_sprite_t* sprite, float r, float g, float b);
    void rgfx_sprite_set_color_struct(rgfx_sprite_t* sprite, rmath_color_t color);
    void rgfx_sprite_set_z_order(rgfx_sprite_t* sprite, int z_order);

    // Getters
    rmath_vec2_t rgfx_sprite_get_position(rgfx_sprite_t* sprite);
    rmath_vec2_t rgfx_sprite_get_size(rgfx_sprite_t* sprite);
    rmath_color_t rgfx_sprite_get_color(rgfx_sprite_t* sprite);
    int rgfx_sprite_get_z_order(rgfx_sprite_t* sprite);

#ifdef __cplusplus
}
#endif