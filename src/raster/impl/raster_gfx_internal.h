#pragma once

#include "raster/raster_gfx.h"
#include "raster/raster_math.h"
#include "raster/raster_app.h"
#include "raster/raster_log.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdbool.h>
#include <stddef.h>

typedef struct stbtt_fontinfo stbtt_fontinfo;

#define RGFX_MAX_TEXT_LENGTH 256

struct rgfx_sprite
{
    rgfx_object_type_t type;
    rtransform_t*      transform;
    unsigned int       VAO;
    unsigned int       VBO;
    unsigned int       EBO;
    unsigned int       shaderProgram;
    unsigned int       textureID;
    bool               hasTexture;
    vec3               size;
    color              color;
    rgfx_uniform_t     uniforms[RGFX_MAX_UNIFORMS];
    int                uniform_count;
};

struct rgfx_camera
{
    vec3  position;
    vec3  forward;
    vec3  up;
    float fov;
    float aspect;
    float near;
    float far;
};

struct rgfx_text
{
    rgfx_object_type_t type;
    rtransform_t*      transform;
    unsigned int       VAO;
    unsigned int       VBO;
    unsigned int       EBO;
    unsigned int       shaderProgram;
    unsigned int       textureID;
    stbtt_fontinfo*    font_info;
    unsigned char*     font_buffer;
    unsigned char*     font_bitmap;
    char               text[RGFX_MAX_TEXT_LENGTH];
    float              font_size;
    color              text_color;
    int                bitmap_width;
    int                bitmap_height;
    float              line_spacing;
    int                alignment;
    unsigned int       index_count;
};

const char* rgfx_internal_default_sprite_vertex_shader(void);
const char* rgfx_internal_default_sprite_fragment_shader(void);
const char* rgfx_internal_default_text_vertex_shader(void);
const char* rgfx_internal_default_text_fragment_shader(void);

rgfx_camera_t* rgfx_internal_get_active_camera(void);
void           rgfx_internal_set_active_camera(rgfx_camera_t* camera);

unsigned int rgfx_internal_acquire_text_shader_program(void);
void         rgfx_internal_release_text_shader_program(void);
