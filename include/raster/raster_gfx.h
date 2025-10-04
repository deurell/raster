#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "linmath.h"
#include "raster_math.h"
#include "raster_transform.h"

    typedef enum {
        RGFX_OBJECT_TYPE_SPRITE,
        RGFX_OBJECT_TYPE_TEXT
    } rgfx_object_type_t;

    bool rgfx_init(void);
    void rgfx_shutdown(void);

    void rgfx_clear(float r, float g, float b);
    void rgfx_clear_color(color color);

    typedef uint32_t rgfx_sprite_handle;
    typedef uint32_t rgfx_text_handle;

    typedef rgfx_sprite_handle rgfx_sprite; /* legacy alias */
    typedef rgfx_text_handle   rgfx_text;   /* legacy alias */

#define RGFX_INVALID_SPRITE_HANDLE 0u
#define RGFX_INVALID_TEXT_HANDLE   0u

#define RGFX_INVALID_SPRITE RGFX_INVALID_SPRITE_HANDLE
#define RGFX_INVALID_TEXT   RGFX_INVALID_TEXT_HANDLE

    char*        rgfx_load_shader_source(const char* filepath);
    unsigned int rgfx_create_shader_program(const char* vertexSource, const char* fragmentSource);

    unsigned int rgfx_load_texture(const char* filepath);
    void         rgfx_delete_texture(unsigned int textureID);

    typedef enum {
        RGFX_UNIFORM_FLOAT,
        RGFX_UNIFORM_INT,
        RGFX_UNIFORM_VEC2,
        RGFX_UNIFORM_VEC3,
        RGFX_UNIFORM_VEC4
    } rgfx_uniform_type_t;

    typedef struct {
        const char*         name;
        rgfx_uniform_type_t type;
        union {
            float uniform_float;
            int   uniform_int;
            vec2  uniform_vec2;
            vec3  uniform_vec3;
            vec4  uniform_vec4;
        };
    } rgfx_uniform_t;

#define RGFX_MAX_UNIFORMS 16

    typedef struct {
        vec3        position;
        vec3        scale;
        color       color;
        const char* vertex_shader_path;
        const char* fragment_shader_path;
        const char* texture_path;
        rgfx_uniform_t uniforms[RGFX_MAX_UNIFORMS];
        int            uniform_count;
    } rgfx_sprite_desc_t;

    typedef struct {
        const char* font_path;
        float       font_size;
        const char* text;
        vec3        position;
        color       text_color;
        float       line_spacing;
        int         alignment;
    } rgfx_text_desc_t;

    typedef enum {
        RGFX_TEXT_ALIGN_LEFT   = 0,
        RGFX_TEXT_ALIGN_CENTER = 1,
        RGFX_TEXT_ALIGN_RIGHT  = 2
    } rgfx_text_alignment_t;

    rgfx_sprite_handle rgfx_sprite_create(const rgfx_sprite_desc_t* desc);
    void               rgfx_sprite_destroy(rgfx_sprite_handle sprite);
    void               rgfx_sprite_draw(rgfx_sprite_handle sprite);

    rgfx_text_handle rgfx_text_create(const rgfx_text_desc_t* desc);
    void             rgfx_text_destroy(rgfx_text_handle text);
    void             rgfx_text_draw(rgfx_text_handle text);
    bool             rgfx_text_update_bitmap(rgfx_text_handle text);

    void rgfx_sprite_set_position(rgfx_sprite_handle sprite, vec3 position);
    void rgfx_sprite_set_size(rgfx_sprite_handle sprite, vec2 size);
    void rgfx_sprite_set_color(rgfx_sprite_handle sprite, color color);
    void rgfx_sprite_set_texture(rgfx_sprite_handle sprite, unsigned int textureID);

    void rgfx_text_set_position(rgfx_text_handle text, vec3 position);
    void rgfx_text_set_color(rgfx_text_handle text, color color);
    void rgfx_text_set_text(rgfx_text_handle text, const char* new_text);
    void rgfx_text_set_font_size(rgfx_text_handle text, float size);
    void rgfx_text_set_alignment(rgfx_text_handle text, int alignment);
    void rgfx_text_set_line_spacing(rgfx_text_handle text, float spacing);

    void         rgfx_sprite_get_position(rgfx_sprite_handle sprite, vec3 out_position);
    void         rgfx_sprite_get_size(rgfx_sprite_handle sprite, vec2 out_size);
    color        rgfx_sprite_get_color(rgfx_sprite_handle sprite);
    unsigned int rgfx_sprite_get_texture_id(rgfx_sprite_handle sprite);

    void         rgfx_text_get_position(rgfx_text_handle text, vec3 out_position);
    color        rgfx_text_get_color(rgfx_text_handle text);
    float        rgfx_text_get_font_size(rgfx_text_handle text);
    const char*  rgfx_text_get_text(rgfx_text_handle text);
    int          rgfx_text_get_alignment(rgfx_text_handle text);
    float        rgfx_text_get_line_spacing(rgfx_text_handle text);

    void rgfx_sprite_set_uniform_float(rgfx_sprite_handle sprite, const char* name, float value);
    void rgfx_sprite_set_uniform_int(rgfx_sprite_handle sprite, const char* name, int value);
    void rgfx_sprite_set_uniform_vec2(rgfx_sprite_handle sprite, const char* name, vec2 value);
    void rgfx_sprite_set_uniform_vec3(rgfx_sprite_handle sprite, const char* name, vec3 value);
    void rgfx_sprite_set_uniform_vec4(rgfx_sprite_handle sprite, const char* name, vec4 value);

    rtransform_t* rgfx_sprite_get_transform(rgfx_sprite_handle sprite);
    rtransform_t* rgfx_text_get_transform(rgfx_text_handle text);

    void rgfx_sprite_set_rotation(rgfx_sprite_handle sprite, float rotation);
    void rgfx_sprite_get_world_position(rgfx_sprite_handle sprite, vec3 out_position);

    void rgfx_text_set_rotation(rgfx_text_handle text, float rotation);
    void rgfx_text_get_world_position(rgfx_text_handle text, vec3 out_position);

    void rgfx_sprite_set_parent(rgfx_sprite_handle child, rgfx_sprite_handle parent);
    void rgfx_text_set_parent(rgfx_text_handle child, rgfx_sprite_handle parent);

    typedef struct rgfx_camera rgfx_camera_t;

    typedef struct {
        vec3 position;
        vec3 target;
        vec3 up;
        float fov;
        float aspect;
        float near;
        float far;
    } rgfx_camera_desc_t;

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
