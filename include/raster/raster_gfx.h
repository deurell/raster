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

    // Shader loading
    char* rgfx_load_shader_source(const char* filepath);
    unsigned int rgfx_create_shader_program(const char* vertexSource, const char* fragmentSource);

    // Texture functions
    unsigned int rgfx_load_texture(const char* filepath);
    void rgfx_delete_texture(unsigned int textureID);

    // Sprite descriptor
    typedef struct
    {
        rmath_vec3_t position;
        rmath_vec2_t size;
        rmath_color_t color;
        const char* vertex_shader_path;   // Optional: Path to vertex shader file (NULL for default)
        const char* fragment_shader_path;  // Optional: Path to fragment shader file (NULL for default)
        const char* texture_path;         // Optional: Path to texture file (NULL for no texture)
    } rgfx_sprite_desc_t;

    // Sprite API
    rgfx_sprite_t* rgfx_sprite_create(const rgfx_sprite_desc_t* desc);
    void rgfx_sprite_destroy(rgfx_sprite_t* sprite);
    void rgfx_sprite_draw(rgfx_sprite_t* sprite);

    // Sprite properties
    void rgfx_sprite_set_position(rgfx_sprite_t* sprite, rmath_vec3_t position);
    void rgfx_sprite_set_size(rgfx_sprite_t* sprite, rmath_vec2_t size);
    void rgfx_sprite_set_color(rgfx_sprite_t* sprite, rmath_color_t color);
    void rgfx_sprite_set_texture(rgfx_sprite_t* sprite, unsigned int textureID);

    // Getters
    rmath_vec3_t rgfx_sprite_get_position_vec3(rgfx_sprite_t* sprite);
    rmath_vec2_t rgfx_sprite_get_size(rgfx_sprite_t* sprite);
    rmath_color_t rgfx_sprite_get_color(rgfx_sprite_t* sprite);
    int rgfx_sprite_get_z_order(rgfx_sprite_t* sprite);
    unsigned int rgfx_sprite_get_texture_id(rgfx_sprite_t* sprite);

#ifdef __cplusplus
}
#endif