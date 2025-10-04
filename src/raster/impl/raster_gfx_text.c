#include "raster_gfx_internal.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

#define RGFX_TEXT_BITMAP_PADDING 10

typedef struct
{
    char** lines;
    int    count;
} rgfx_text_lines_t;

static rgfx_text_lines_t rgfx_split_text_into_lines(const char* text)
{
    rgfx_text_lines_t result = { NULL, 0 };
    if (!text)
    {
        return result;
    }

    int line_count = 1;
    for (const char* p = text; *p; ++p)
    {
        if (*p == '\n')
        {
            line_count++;
        }
    }

    result.lines = (char**)malloc((size_t)line_count * sizeof(char*));
    if (!result.lines)
    {
        return result;
    }

    result.count = line_count;

    const char* start      = text;
    int         line_index = 0;
    for (const char* p = text;; ++p)
    {
        if (*p == '\n' || *p == '\0')
        {
            size_t len = (size_t)(p - start);
            result.lines[line_index] = (char*)malloc(len + 1);
            if (result.lines[line_index])
            {
                strncpy(result.lines[line_index], start, len);
                result.lines[line_index][len] = '\0';
            }
            start = p + 1;
            line_index++;
        }

        if (*p == '\0')
        {
            break;
        }
    }

    return result;
}

static void rgfx_free_text_lines(rgfx_text_lines_t* lines)
{
    if (!lines || !lines->lines)
    {
        return;
    }

    for (int i = 0; i < lines->count; ++i)
    {
        free(lines->lines[i]);
    }
    free(lines->lines);
    lines->lines = NULL;
    lines->count = 0;
}

static rgfx_text_t* rgfx_text_from_handle(rgfx_text_handle handle)
{
    return rgfx_internal_text_resolve(handle);
}

static bool rgfx_text_update_bitmap_ptr(rgfx_text_t* text);

rgfx_text_handle rgfx_text_create(const rgfx_text_desc_t* desc)
{
    if (!desc || !desc->font_path || !desc->text)
    {
        return RGFX_INVALID_TEXT_HANDLE;
    }

    rgfx_text_t* text = (rgfx_text_t*)calloc(1, sizeof(rgfx_text_t));
    if (!text)
    {
        return RGFX_INVALID_TEXT_HANDLE;
    }

    text->type         = RGFX_OBJECT_TYPE_TEXT;
    text->line_spacing = desc->line_spacing > 0.0f ? desc->line_spacing : 1.2f;
    text->font_size    = desc->font_size;
    text->text_color   = desc->text_color;
    text->alignment    = desc->alignment;

    strncpy(text->text, desc->text, RGFX_MAX_TEXT_LENGTH - 1);
    text->text[RGFX_MAX_TEXT_LENGTH - 1] = '\0';

    text->transform = rtransform_create();
    if (!text->transform)
    {
        free(text);
        return RGFX_INVALID_TEXT_HANDLE;
    }

    vec3 position;
    vec3_dup(position, desc->position);
    rtransform_set_position(text->transform, position);

    vec3 scale = { desc->font_size * 0.04f, desc->font_size * -0.04f, 1.0f };
    rtransform_set_scale(text->transform, scale);

    FILE* font_file = fopen(desc->font_path, "rb");
    if (!font_file)
    {
        rtransform_destroy(text->transform);
        free(text);
        return RGFX_INVALID_TEXT_HANDLE;
    }

    fseek(font_file, 0, SEEK_END);
    long font_size_bytes = ftell(font_file);
    fseek(font_file, 0, SEEK_SET);

    text->font_buffer = (unsigned char*)malloc((size_t)font_size_bytes);
    if (!text->font_buffer)
    {
        fclose(font_file);
        rtransform_destroy(text->transform);
        free(text);
        return RGFX_INVALID_TEXT_HANDLE;
    }

    fread(text->font_buffer, 1, (size_t)font_size_bytes, font_file);
    fclose(font_file);

    text->font_info = (stbtt_fontinfo*)calloc(1, sizeof(stbtt_fontinfo));
    if (!text->font_info)
    {
        free(text->font_buffer);
        rtransform_destroy(text->transform);
        free(text);
        return RGFX_INVALID_TEXT_HANDLE;
    }

    if (!stbtt_InitFont(text->font_info, text->font_buffer, 0))
    {
        free(text->font_info);
        free(text->font_buffer);
        rtransform_destroy(text->transform);
        free(text);
        return RGFX_INVALID_TEXT_HANDLE;
    }

    const float quad_vertices[] = {
        -0.5f, -0.5f, 0.0f, 0.0f,
         0.5f, -0.5f, 1.0f, 0.0f,
         0.5f,  0.5f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 1.0f,
    };

    const unsigned int quad_indices[] = { 0, 1, 2, 2, 3, 0 };

    text->shaderProgram = rgfx_internal_acquire_text_shader_program();
    if (!text->shaderProgram)
    {
        free(text->font_info);
        free(text->font_buffer);
        rtransform_destroy(text->transform);
        free(text);
        return RGFX_INVALID_TEXT_HANDLE;
    }

    glGenVertexArrays(1, &text->VAO);
    glGenBuffers(1, &text->VBO);
    glGenBuffers(1, &text->EBO);

    glBindVertexArray(text->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, text->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    if (!rgfx_text_update_bitmap_ptr(text))
    {
        glDeleteVertexArrays(1, &text->VAO);
        glDeleteBuffers(1, &text->VBO);
        glDeleteBuffers(1, &text->EBO);
        rgfx_internal_release_text_shader_program();
        free(text->font_info);
        free(text->font_buffer);
        rtransform_destroy(text->transform);
        free(text);
        return RGFX_INVALID_TEXT_HANDLE;
    }

    rgfx_text handle = rgfx_internal_text_register(text);
    if (handle == RGFX_INVALID_TEXT_HANDLE)
    {
        glDeleteVertexArrays(1, &text->VAO);
        glDeleteBuffers(1, &text->VBO);
        glDeleteBuffers(1, &text->EBO);
        rgfx_internal_release_text_shader_program();
        free(text->font_info);
        free(text->font_buffer);
        rtransform_destroy(text->transform);
        free(text);
        return RGFX_INVALID_TEXT_HANDLE;
    }

    return handle;
}

void rgfx_text_destroy(rgfx_text_handle handle)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    if (!text)
    {
        return;
    }

    rgfx_internal_text_unregister(handle);

    if (text->font_buffer)
    {
        free(text->font_buffer);
    }
    if (text->font_bitmap)
    {
        free(text->font_bitmap);
    }
    if (text->font_info)
    {
        free(text->font_info);
    }
    if (text->textureID)
    {
        glDeleteTextures(1, &text->textureID);
    }
    if (text->transform)
    {
        rtransform_destroy(text->transform);
    }

    glDeleteVertexArrays(1, &text->VAO);
    glDeleteBuffers(1, &text->VBO);
    glDeleteBuffers(1, &text->EBO);
    rgfx_internal_release_text_shader_program();

    free(text);
}

void rgfx_text_draw(rgfx_text_handle handle)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    if (!text)
    {
        return;
    }

    glUseProgram(text->shaderProgram);

    rtransform_update(text->transform);

    mat4x4 bitmap_scale;
    mat4x4_identity(bitmap_scale);

    float aspect_ratio = text->bitmap_height > 0 ? (float)text->bitmap_width / (float)text->bitmap_height : 1.0f;
    bitmap_scale[0][0] = aspect_ratio;

    mat4x4 final_transform;
    mat4x4_mul(final_transform, text->transform->world, bitmap_scale);

    glUniformMatrix4fv(glGetUniformLocation(text->shaderProgram, "uModel"), 1, GL_FALSE, (float*)final_transform);

    glUniform3f(glGetUniformLocation(text->shaderProgram, "uColor"),
                text->text_color.r,
                text->text_color.g,
                text->text_color.b);

    rgfx_camera_t* camera = rgfx_internal_get_active_camera();
    if (camera)
    {
        mat4x4 view;
        mat4x4 projection;
        rgfx_camera_get_matrices(camera, view, projection);
        glUniformMatrix4fv(glGetUniformLocation(text->shaderProgram, "uView"), 1, GL_FALSE, (float*)view);
        glUniformMatrix4fv(glGetUniformLocation(text->shaderProgram, "uProjection"), 1, GL_FALSE, (float*)projection);
    }
    else
    {
        mat4x4 identity;
        mat4x4_identity(identity);
        glUniformMatrix4fv(glGetUniformLocation(text->shaderProgram, "uView"), 1, GL_FALSE, (float*)identity);
        glUniformMatrix4fv(glGetUniformLocation(text->shaderProgram, "uProjection"), 1, GL_FALSE, (float*)identity);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, text->textureID);
    glUniform1i(glGetUniformLocation(text->shaderProgram, "uTexture"), 0);

    glBindVertexArray(text->VAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)text->index_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

static bool rgfx_text_update_bitmap_ptr(rgfx_text_t* text)
{
    if (!text || !text->font_info)
    {
        return false;
    }

    if (text->font_bitmap)
    {
        free(text->font_bitmap);
        text->font_bitmap = NULL;
    }

    rgfx_text_lines_t lines = rgfx_split_text_into_lines(text->text);
    if (lines.count == 0)
    {
        return false;
    }

    float scale = stbtt_ScaleForPixelHeight(text->font_info, text->font_size);

    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(text->font_info, &ascent, &descent, &line_gap);

    int rounded_ascent   = (int)(ascent * scale + 0.5f);
    int rounded_descent  = (int)(descent * scale - 0.5f);

    float line_spacing = text->line_spacing > 0 ? text->line_spacing : 1.2f;
    int   line_height  = (int)(((rounded_ascent - rounded_descent) * line_spacing) + 0.5f);

    int total_width = 0;
    int* line_widths = (int*)calloc((size_t)lines.count, sizeof(int));
    if (!line_widths)
    {
        rgfx_free_text_lines(&lines);
        return false;
    }

    for (int i = 0; i < lines.count; ++i)
    {
        int line_width = 0;
        for (const char* p = lines.lines[i]; *p; ++p)
        {
            int advance = 0;
            int lsb     = 0;
            stbtt_GetCodepointHMetrics(text->font_info, (int)*p, &advance, &lsb);

            line_width += (int)(advance * scale + 0.5f);

            if (*(p + 1))
            {
                int kern = stbtt_GetCodepointKernAdvance(text->font_info, (int)*p, (int)*(p + 1));
                line_width += (int)(kern * scale + 0.5f);
            }
        }

        line_widths[i] = line_width;
        if (line_width > total_width)
        {
            total_width = line_width;
        }
    }

    int total_height = lines.count * line_height + RGFX_TEXT_BITMAP_PADDING;

    text->bitmap_width  = total_width + RGFX_TEXT_BITMAP_PADDING;
    text->bitmap_height = total_height;

    text->font_bitmap = (unsigned char*)calloc((size_t)text->bitmap_width * (size_t)text->bitmap_height, sizeof(unsigned char));
    if (!text->font_bitmap)
    {
        free(line_widths);
        rgfx_free_text_lines(&lines);
        return false;
    }

    int baseline = rounded_ascent;
    int y        = baseline;

    for (int line_index = 0; line_index < lines.count; ++line_index)
    {
        const char* line_text = lines.lines[line_index];
        int         line_width = line_widths[line_index];

        int x = RGFX_TEXT_BITMAP_PADDING / 2;
        if (text->alignment == RGFX_TEXT_ALIGN_CENTER)
        {
            x = (text->bitmap_width - line_width) / 2;
        }
        else if (text->alignment == RGFX_TEXT_ALIGN_RIGHT)
        {
            x = text->bitmap_width - line_width - RGFX_TEXT_BITMAP_PADDING / 2;
        }

        for (const char* p = line_text; *p; ++p)
        {
            int advance = 0;
            int lsb     = 0;
            stbtt_GetCodepointHMetrics(text->font_info, (int)*p, &advance, &lsb);

            int c_x1, c_y1, c_x2, c_y2;
            stbtt_GetCodepointBitmapBox(text->font_info, (int)*p, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

            int width  = c_x2 - c_x1;
            int height = c_y2 - c_y1;

            unsigned char* bitmap = (unsigned char*)malloc((size_t)width * (size_t)height);
            if (bitmap)
            {
                stbtt_MakeCodepointBitmap(text->font_info,
                                          bitmap,
                                          width,
                                          height,
                                          width,
                                          scale,
                                          scale,
                                          (int)*p);

                for (int row = 0; row < height; ++row)
                {
                    unsigned char* dst = text->font_bitmap + (size_t)(y + row + c_y1) * (size_t)text->bitmap_width + (size_t)(x + c_x1);
                    unsigned char* src = bitmap + (size_t)row * (size_t)width;
                    memcpy(dst, src, (size_t)width);
                }

                free(bitmap);
            }

            x += (int)(advance * scale + 0.5f);
            if (*(p + 1))
            {
                int kern = stbtt_GetCodepointKernAdvance(text->font_info, (int)*p, (int)*(p + 1));
                x += (int)(kern * scale + 0.5f);
            }
        }

        y += line_height;
    }

    rgfx_free_text_lines(&lines);
    free(line_widths);

    if (!text->textureID)
    {
        glGenTextures(1, &text->textureID);
    }

    glBindTexture(GL_TEXTURE_2D, text->textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
#if defined(__EMSCRIPTEN__)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, text->bitmap_width, text->bitmap_height, 0, GL_RED, GL_UNSIGNED_BYTE, text->font_bitmap);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, text->bitmap_width, text->bitmap_height, 0, GL_RED, GL_UNSIGNED_BYTE, text->font_bitmap);
#endif
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    text->index_count = 6;

    return true;
}

bool rgfx_text_update_bitmap(rgfx_text_handle handle)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    return rgfx_text_update_bitmap_ptr(text);
}

void rgfx_text_set_position(rgfx_text_handle handle, vec3 position)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    if (!text)
    {
        return;
    }

    rtransform_set_position(text->transform, position);
}

void rgfx_text_set_color(rgfx_text_handle handle, color color)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    if (!text)
    {
        return;
    }

    text->text_color = color;
}

void rgfx_text_set_text(rgfx_text_handle handle, const char* new_text)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    if (!text || !new_text)
    {
        return;
    }

    strncpy(text->text, new_text, RGFX_MAX_TEXT_LENGTH - 1);
    text->text[RGFX_MAX_TEXT_LENGTH - 1] = '\0';
    rgfx_text_update_bitmap_ptr(text);
}

void rgfx_text_set_font_size(rgfx_text_handle handle, float size)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    if (!text || size <= 0.0f)
    {
        return;
    }

    text->font_size = size;

    vec3 scale = { size * 0.04f, size * -0.04f, 1.0f };
    rtransform_set_scale(text->transform, scale);
    rgfx_text_update_bitmap_ptr(text);
}

void rgfx_text_set_alignment(rgfx_text_handle handle, int alignment)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    if (!text)
    {
        return;
    }

    if (alignment < RGFX_TEXT_ALIGN_LEFT || alignment > RGFX_TEXT_ALIGN_RIGHT)
    {
        alignment = RGFX_TEXT_ALIGN_LEFT;
    }

    if (text->alignment != alignment)
    {
        text->alignment = alignment;
        rgfx_text_update_bitmap_ptr(text);
    }
}

void rgfx_text_set_line_spacing(rgfx_text_handle handle, float spacing)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    if (!text)
    {
        return;
    }

    if (spacing < 0.5f)
    {
        spacing = 0.5f;
    }
    if (spacing > 3.0f)
    {
        spacing = 3.0f;
    }

    if (text->line_spacing != spacing)
    {
        text->line_spacing = spacing;
        rgfx_text_update_bitmap_ptr(text);
    }
}

void rgfx_text_get_position(rgfx_text_handle handle, vec3 out_position)
{
    if (!out_position)
    {
        return;
    }

    rgfx_text_t* text = rgfx_text_from_handle(handle);
    if (!text)
    {
        out_position[0] = out_position[1] = out_position[2] = 0.0f;
        return;
    }

    rtransform_get_world_position(text->transform, out_position);
}

color rgfx_text_get_color(rgfx_text_handle handle)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    if (!text)
    {
        color c = { 0, 0, 0 };
        return c;
    }
    return text->text_color;
}

float rgfx_text_get_font_size(rgfx_text_handle handle)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    return text ? text->font_size : 0.0f;
}

const char* rgfx_text_get_text(rgfx_text_handle handle)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    return text ? text->text : NULL;
}

int rgfx_text_get_alignment(rgfx_text_handle handle)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    return text ? text->alignment : RGFX_TEXT_ALIGN_LEFT;
}

float rgfx_text_get_line_spacing(rgfx_text_handle handle)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    return text ? text->line_spacing : 1.2f;
}

rtransform_t* rgfx_text_get_transform(rgfx_text_handle handle)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    return text ? text->transform : NULL;
}

void rgfx_text_set_rotation(rgfx_text_handle handle, float rotation)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    if (!text)
    {
        return;
    }

    vec3 axis = { 0.0f, 0.0f, 1.0f };
    rtransform_set_rotation_axis_angle(text->transform, axis, rotation);
}

void rgfx_text_get_world_position(rgfx_text_handle handle, vec3 out_position)
{
    if (!out_position)
    {
        return;
    }

    rgfx_text_t* text = rgfx_text_from_handle(handle);
    if (!text)
    {
        out_position[0] = out_position[1] = out_position[2] = 0.0f;
        return;
    }

    rtransform_get_world_position(text->transform, out_position);
}

void rgfx_text_set_parent(rgfx_text_handle handle, rgfx_sprite_handle parent)
{
    rgfx_text_t* text = rgfx_text_from_handle(handle);
    if (!text)
    {
        return;
    }

    rtransform_t* parent_transform = NULL;
    if (parent != RGFX_INVALID_SPRITE_HANDLE)
    {
        rgfx_sprite_t* parent_ptr = rgfx_internal_sprite_resolve(parent);
        parent_transform = parent_ptr ? parent_ptr->transform : NULL;
    }

    rtransform_set_parent(text->transform, parent_transform);
}
