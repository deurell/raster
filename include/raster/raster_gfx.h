#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include "linmath.h"
#include "raster_math.h"
#include "raster_transform.h"

    // Graphics context
    bool rgfx_init(void);
    void rgfx_shutdown(void);

    // Rendering
    void rgfx_clear(float r, float g, float b);
    void rgfx_clear_color(color color);

    // Sprite type forward declaration
    typedef struct rgfx_sprite rgfx_sprite_t;

    // Text type forward declaration
    typedef struct rgfx_text rgfx_text_t;

    // Shader loading
    char*        rgfx_load_shader_source(const char* filepath);
    unsigned int rgfx_create_shader_program(const char* vertexSource, const char* fragmentSource);

    // Texture functions
    unsigned int rgfx_load_texture(const char* filepath);
    void         rgfx_delete_texture(unsigned int textureID);

    // Uniform system
    typedef enum
    {
        RGFX_UNIFORM_FLOAT,
        RGFX_UNIFORM_INT,
        RGFX_UNIFORM_VEC2,
        RGFX_UNIFORM_VEC3,
        RGFX_UNIFORM_VEC4
    } rgfx_uniform_type_t;

    typedef struct
    {
        const char*         name;
        rgfx_uniform_type_t type;
        union
        {
            float float_val;
            int   int_val;
            vec2  vec2_val;
            vec3  vec3_val;
            vec4  vec4_val;
        };
    } rgfx_uniform_t;

#define RGFX_MAX_UNIFORMS 16

    // Sprite descriptor
    typedef struct
    {
        vec3        position;
        vec2        size;
        color       color;
        const char* vertex_shader_path;   // Optional: Path to vertex shader file (NULL for default)
        const char* fragment_shader_path; // Optional: Path to fragment shader file (NULL for default)
        const char* texture_path;         // Optional: Path to texture file (NULL for no texture)
        // Uniform system
        rgfx_uniform_t uniforms[RGFX_MAX_UNIFORMS];
        int            uniform_count; // Optional: Will be auto-calculated if set to 0
    } rgfx_sprite_desc_t;

    // Text descriptor
    typedef struct
    {
        const char* font_path;     // Path to font file (.ttf)
        float       font_size;     // Font size in pixels
        const char* text;          // Text to render (can include newlines)
        vec3        position;      // Position in world space
        color       text_color;    // Text color
        float       line_spacing;  // Spacing between lines (multiplier of font size), default 1.2f
        int         alignment;     // Text alignment: 0=left, 1=center, 2=right
    } rgfx_text_desc_t;

    // Text alignment enum
    typedef enum
    {
        RGFX_TEXT_ALIGN_LEFT = 0,
        RGFX_TEXT_ALIGN_CENTER = 1,
        RGFX_TEXT_ALIGN_RIGHT = 2
    } rgfx_text_alignment_t;

    // Sprite API
    rgfx_sprite_t* rgfx_sprite_create(const rgfx_sprite_desc_t* desc);
    void           rgfx_sprite_destroy(rgfx_sprite_t* sprite);
    void           rgfx_sprite_draw(rgfx_sprite_t* sprite);

    // Text API
    rgfx_text_t* rgfx_text_create(const rgfx_text_desc_t* desc);
    void         rgfx_text_destroy(rgfx_text_t* text);
    void         rgfx_text_draw(rgfx_text_t* text);
    bool         rgfx_text_update_bitmap(rgfx_text_t* text);

    // Sprite properties
    void rgfx_sprite_set_position(rgfx_sprite_t* sprite, vec3 position);
    void rgfx_sprite_set_size(rgfx_sprite_t* sprite, vec2 size);
    void rgfx_sprite_set_color(rgfx_sprite_t* sprite, color color);
    void rgfx_sprite_set_texture(rgfx_sprite_t* sprite, unsigned int textureID);

    // Text properties
    void         rgfx_text_set_position(rgfx_text_t* text, vec3 position);
    void         rgfx_text_set_color(rgfx_text_t* text, color color);
    void         rgfx_text_set_text(rgfx_text_t* text, const char* new_text);
    void         rgfx_text_set_font_size(rgfx_text_t* text, float size);
    void         rgfx_text_set_alignment(rgfx_text_t* text, int alignment);
    void         rgfx_text_set_line_spacing(rgfx_text_t* text, float spacing);

    // Getters
    void         rgfx_sprite_get_position(rgfx_sprite_t* sprite, vec3 out_position);
    void         rgfx_sprite_get_size(rgfx_sprite_t* sprite, vec2 out_size);
    color        rgfx_sprite_get_color(rgfx_sprite_t* sprite);
    unsigned int rgfx_sprite_get_texture_id(rgfx_sprite_t* sprite);

    // Text getters
    void         rgfx_text_get_position(rgfx_text_t* text, vec3 out_position);
    color        rgfx_text_get_color(rgfx_text_t* text);
    float        rgfx_text_get_font_size(rgfx_text_t* text);
    const char*  rgfx_text_get_text(rgfx_text_t* text);
    int          rgfx_text_get_alignment(rgfx_text_t* text);
    float        rgfx_text_get_line_spacing(rgfx_text_t* text);

    // Uniform API
    void rgfx_sprite_set_uniform_float(rgfx_sprite_t* sprite, const char* name, float value);
    void rgfx_sprite_set_uniform_int(rgfx_sprite_t* sprite, const char* name, int value);
    void rgfx_sprite_set_uniform_vec2(rgfx_sprite_t* sprite, const char* name, vec2 value);
    void rgfx_sprite_set_uniform_vec3(rgfx_sprite_t* sprite, const char* name, vec3 value);
    void rgfx_sprite_set_uniform_vec4(rgfx_sprite_t* sprite, const char* name, vec4 value);

    // Transform related functions - using generic parent system
    void rgfx_set_parent(void* child, void* parent);
    rtransform_t* rgfx_get_transform(void* object);
    
    // Keep specific rotation/position setters for type safety and convenience
    void rgfx_sprite_set_rotation(rgfx_sprite_t* sprite, float rotation);  // Simple Z-axis rotation helper
    void rgfx_sprite_get_world_position(rgfx_sprite_t* sprite, vec3 out_position);

    void rgfx_text_set_rotation(rgfx_text_t* text, float rotation);
    void rgfx_text_get_world_position(rgfx_text_t* text, vec3 out_position);

    // Camera type forward declaration
    typedef struct rgfx_camera rgfx_camera_t;

    // Camera descriptor for initialization
    typedef struct
    {
        vec3  position;
        vec3  target;
        vec3  up;
        float fov;    // Field of view in radians
        float aspect; // Aspect ratio (width/height)
        float near;   // Near clipping plane
        float far;    // Far clipping plane
    } rgfx_camera_desc_t;

    // Camera functions
    rgfx_camera_t* rgfx_camera_create(const rgfx_camera_desc_t* desc);
    void           rgfx_camera_destroy(rgfx_camera_t* camera);
    void           rgfx_camera_set_position(rgfx_camera_t* camera, vec3 position);
    void           rgfx_camera_set_direction(rgfx_camera_t* camera, vec3 direction);
    void           rgfx_camera_look_at(rgfx_camera_t* camera, vec3 target);
    void           rgfx_camera_move(rgfx_camera_t* camera, vec3 offset);
    void           rgfx_camera_rotate(rgfx_camera_t* camera, float yaw, float pitch);
    void           rgfx_camera_get_matrices(const rgfx_camera_t* camera, mat4x4 view, mat4x4 projection);
    void           rgfx_set_active_camera(rgfx_camera_t* camera);

#ifdef __cplusplus
}
#endif