#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include "linmath.h"
#include "raster_math.h"

    // Graphics context
    bool rgfx_init(void);
    void rgfx_shutdown(void);

    // Rendering
    void rgfx_clear(float r, float g, float b);
    void rgfx_clear_color(color color);

    // Sprite type forward declaration
    typedef struct rgfx_sprite rgfx_sprite_t;

    // Shader loading
    char*        rgfx_load_shader_source(const char* filepath);
    unsigned int rgfx_create_shader_program(const char* vertexSource, const char* fragmentSource);

    // Texture functions
    unsigned int rgfx_load_texture(const char* filepath);
    void         rgfx_delete_texture(unsigned int textureID);

    // Sprite descriptor
    typedef struct
    {
        vec3        position;
        vec2        size;
        color       color;
        const char* vertex_shader_path;   // Optional: Path to vertex shader file (NULL for default)
        const char* fragment_shader_path; // Optional: Path to fragment shader file (NULL for default)
        const char* texture_path;         // Optional: Path to texture file (NULL for no texture)
    } rgfx_sprite_desc_t;

    // Sprite API
    rgfx_sprite_t* rgfx_sprite_create(const rgfx_sprite_desc_t* desc);
    void           rgfx_sprite_destroy(rgfx_sprite_t* sprite);
    void           rgfx_sprite_draw(rgfx_sprite_t* sprite);

    // Sprite properties
    void rgfx_sprite_set_position(rgfx_sprite_t* sprite, vec3 position);
    void rgfx_sprite_set_size(rgfx_sprite_t* sprite, vec2 size);
    void rgfx_sprite_set_color(rgfx_sprite_t* sprite, color color);
    void rgfx_sprite_set_texture(rgfx_sprite_t* sprite, unsigned int textureID);

    // Getters
    void         rgfx_sprite_get_position(rgfx_sprite_t* sprite, vec3 out_position);
    void         rgfx_sprite_get_size(rgfx_sprite_t* sprite, vec2 out_size);
    color        rgfx_sprite_get_color(rgfx_sprite_t* sprite);
    int          rgfx_sprite_get_z_order(rgfx_sprite_t* sprite);
    unsigned int rgfx_sprite_get_texture_id(rgfx_sprite_t* sprite);

    // Camera type forward declaration
    typedef struct rgfx_camera rgfx_camera_t;

    // Camera descriptor for initialization
    typedef struct {
        vec3 position;
        vec3 target;
        vec3 up;
        float fov;           // Field of view in radians
        float aspect;        // Aspect ratio (width/height)
        float near;         // Near clipping plane
        float far;          // Far clipping plane
    } rgfx_camera_desc_t;

    // Camera functions
    rgfx_camera_t* rgfx_camera_create(const rgfx_camera_desc_t* desc);
    void rgfx_camera_destroy(rgfx_camera_t* camera);
    void rgfx_camera_set_position(rgfx_camera_t* camera, vec3 position);
    void rgfx_camera_set_direction(rgfx_camera_t* camera, vec3 direction);
    void rgfx_camera_look_at(rgfx_camera_t* camera, vec3 target);
    void rgfx_camera_move(rgfx_camera_t* camera, vec3 offset);
    void rgfx_camera_rotate(rgfx_camera_t* camera, float yaw, float pitch);
    void rgfx_camera_get_matrices(const rgfx_camera_t* camera, mat4x4 view, mat4x4 projection);
    void rgfx_set_active_camera(rgfx_camera_t* camera);

#ifdef __cplusplus
}
#endif