#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

    // Graphics context
    bool raster_gfx_init(void);
    void raster_gfx_shutdown(void);

    // Rendering
    void raster_gfx_clear(float r, float g, float b);

    // Sprite type forward declaration
    typedef struct raster_sprite raster_sprite_t;

    // Sprite API
    raster_sprite_t* raster_sprite_create(float x, float y, float width, float height, float r, float g, float b);
    void             raster_sprite_destroy(raster_sprite_t* sprite);
    void             raster_sprite_draw(raster_sprite_t* sprite);

    // Sprite properties
    void raster_sprite_set_position(raster_sprite_t* sprite, float x, float y);
    void raster_sprite_set_size(raster_sprite_t* sprite, float width, float height);
    void raster_sprite_set_color(raster_sprite_t* sprite, float r, float g, float b);

    float raster_sprite_get_x(raster_sprite_t* sprite);
    float raster_sprite_get_y(raster_sprite_t* sprite);
    float raster_sprite_get_width(raster_sprite_t* sprite);
    float raster_sprite_get_height(raster_sprite_t* sprite);
    void  raster_sprite_get_color(raster_sprite_t* sprite, float* r, float* g, float* b);

#ifdef __cplusplus
}
#endif