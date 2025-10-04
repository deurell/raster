#include "raster_gfx_internal.h"

#include <stdlib.h>
#include <string.h>

static void rgfx_sprite_release_resources(rgfx_sprite_t* sprite)
{
    if (!sprite)
    {
        return;
    }

    if (sprite->hasTexture && sprite->textureID)
    {
        glDeleteTextures(1, &sprite->textureID);
        sprite->textureID = 0;
        sprite->hasTexture = false;
    }

    if (sprite->shaderProgram)
    {
        glDeleteProgram(sprite->shaderProgram);
        sprite->shaderProgram = 0;
    }

    if (sprite->VAO)
    {
        glDeleteVertexArrays(1, &sprite->VAO);
        sprite->VAO = 0;
    }

    if (sprite->VBO)
    {
        glDeleteBuffers(1, &sprite->VBO);
        sprite->VBO = 0;
    }

    if (sprite->EBO)
    {
        glDeleteBuffers(1, &sprite->EBO);
        sprite->EBO = 0;
    }

    if (sprite->transform)
    {
        rtransform_destroy(sprite->transform);
        sprite->transform = NULL;
    }
}

static rgfx_sprite_t* rgfx_sprite_from_handle(rgfx_sprite_handle handle)
{
    return rgfx_internal_sprite_resolve(handle);
}

rgfx_sprite_handle rgfx_sprite_create(const rgfx_sprite_desc_t* desc)
{
    if (!desc)
    {
        return RGFX_INVALID_SPRITE_HANDLE;
    }

    rgfx_sprite_t* sprite = (rgfx_sprite_t*)calloc(1, sizeof(rgfx_sprite_t));
    if (!sprite)
    {
        return RGFX_INVALID_SPRITE_HANDLE;
    }

    sprite->type = RGFX_OBJECT_TYPE_SPRITE;
    sprite->transform = rtransform_create();
    if (!sprite->transform)
    {
        free(sprite);
        return RGFX_INVALID_SPRITE_HANDLE;
    }

    vec3 position;
    vec3_dup(position, desc->position);
    rtransform_set_position(sprite->transform, position);

    vec3 scale = { desc->scale[0], desc->scale[1], desc->scale[2] };
    rtransform_set_scale(sprite->transform, scale);

    vec3_dup(sprite->size, desc->scale);
    sprite->color = desc->color;

    if (desc->uniform_count > 0)
    {
        sprite->uniform_count = desc->uniform_count > RGFX_MAX_UNIFORMS ? RGFX_MAX_UNIFORMS : desc->uniform_count;
        for (int i = 0; i < sprite->uniform_count; ++i)
        {
            sprite->uniforms[i] = desc->uniforms[i];
        }
    }
    else
    {
        for (int i = 0; i < RGFX_MAX_UNIFORMS; ++i)
        {
            if (desc->uniforms[i].name)
            {
                sprite->uniforms[sprite->uniform_count++] = desc->uniforms[i];
            }
        }
    }

    char* vertex_source   = NULL;
    char* fragment_source = NULL;

    if (desc->vertex_shader_path)
    {
        vertex_source = rgfx_load_shader_source(desc->vertex_shader_path);
        if (!vertex_source)
        {
            rlog_error("Failed to load vertex shader from %s", desc->vertex_shader_path);
            goto fail;
        }
    }

    if (desc->fragment_shader_path)
    {
        fragment_source = rgfx_load_shader_source(desc->fragment_shader_path);
        if (!fragment_source)
        {
            rlog_error("Failed to load fragment shader from %s", desc->fragment_shader_path);
            goto fail;
        }
    }

    const char* vertex_ptr   = vertex_source ? vertex_source : rgfx_internal_default_sprite_vertex_shader();
    const char* fragment_ptr = fragment_source ? fragment_source : rgfx_internal_default_sprite_fragment_shader();

    sprite->shaderProgram = rgfx_create_shader_program(vertex_ptr, fragment_ptr);

    free(vertex_source);
    free(fragment_source);

    if (sprite->shaderProgram == 0)
    {
        goto fail;
    }

    if (desc->texture_path)
    {
        unsigned int texture = rgfx_load_texture(desc->texture_path);
        if (texture > 0)
        {
            sprite->textureID  = texture;
            sprite->hasTexture = true;
        }
        else
        {
            rlog_warning("Failed to load texture from %s", desc->texture_path);
            sprite->textureID  = 0;
            sprite->hasTexture = false;
        }
    }

    float vertices[16];
    bool  use_texcoords = false;

    if (glGetAttribLocation(sprite->shaderProgram, "aTexCoord") != -1)
    {
        use_texcoords = true;
        const float textured[] = {
            -0.5f, -0.5f, 0.0f, 0.0f,
             0.5f, -0.5f, 1.0f, 0.0f,
             0.5f,  0.5f, 1.0f, 1.0f,
            -0.5f,  0.5f, 0.0f, 1.0f,
        };
        memcpy(vertices, textured, sizeof(textured));
    }
    else
    {
        const float positions[] = {
            -0.5f, -0.5f,
             0.5f, -0.5f,
             0.5f,  0.5f,
            -0.5f,  0.5f,
        };
        memcpy(vertices, positions, sizeof(positions));
    }

    const unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    glGenVertexArrays(1, &sprite->VAO);
    glGenBuffers(1, &sprite->VBO);
    glGenBuffers(1, &sprite->EBO);

    glBindVertexArray(sprite->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, sprite->VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 use_texcoords ? sizeof(float) * 16 : sizeof(float) * 8,
                 vertices,
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    if (use_texcoords)
    {
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
    else
    {
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    rgfx_sprite handle = rgfx_internal_sprite_register(sprite);
    if (handle == RGFX_INVALID_SPRITE_HANDLE)
    {
        goto fail;
    }

    return handle;

fail:
    free(vertex_source);
    free(fragment_source);
    rgfx_sprite_release_resources(sprite);
    free(sprite);
    return RGFX_INVALID_SPRITE_HANDLE;
}

void rgfx_sprite_destroy(rgfx_sprite_handle sprite)
{
    rgfx_sprite_t* sprite_ptr = rgfx_sprite_from_handle(sprite);
    if (!sprite_ptr)
    {
        return;
    }

    rgfx_internal_sprite_unregister(sprite);
    rgfx_sprite_release_resources(sprite_ptr);
    free(sprite_ptr);
}

void rgfx_sprite_draw(rgfx_sprite_handle sprite)
{
    rgfx_sprite_t* sprite_ptr = rgfx_sprite_from_handle(sprite);
    if (!sprite_ptr)
    {
        return;
    }

    glUseProgram(sprite_ptr->shaderProgram);

    rtransform_update(sprite_ptr->transform);
    glUniformMatrix4fv(glGetUniformLocation(sprite_ptr->shaderProgram, "uModel"), 1, GL_FALSE, (float*)sprite_ptr->transform->world);

    glUniform2f(glGetUniformLocation(sprite_ptr->shaderProgram, "uSize"), sprite_ptr->size[0], sprite_ptr->size[1]);
    glUniform3f(glGetUniformLocation(sprite_ptr->shaderProgram, "uColor"), sprite_ptr->color.r, sprite_ptr->color.g, sprite_ptr->color.b);

    float currentTime = rapp_get_time();
    glUniform1f(glGetUniformLocation(sprite_ptr->shaderProgram, "uTime"), currentTime);

    glUniform1i(glGetUniformLocation(sprite_ptr->shaderProgram, "uUseTexture"), sprite_ptr->hasTexture ? 1 : 0);

    for (int i = 0; i < sprite_ptr->uniform_count; ++i)
    {
        const rgfx_uniform_t* uniform = &sprite_ptr->uniforms[i];
        int location = glGetUniformLocation(sprite_ptr->shaderProgram, uniform->name);
        if (location == -1)
        {
            continue;
        }

        switch (uniform->type)
        {
        case RGFX_UNIFORM_FLOAT:
            glUniform1f(location, uniform->uniform_float);
            break;
        case RGFX_UNIFORM_INT:
            glUniform1i(location, uniform->uniform_int);
            break;
        case RGFX_UNIFORM_VEC2:
            glUniform2fv(location, 1, uniform->uniform_vec2);
            break;
        case RGFX_UNIFORM_VEC3:
            glUniform3fv(location, 1, uniform->uniform_vec3);
            break;
        case RGFX_UNIFORM_VEC4:
            glUniform4fv(location, 1, uniform->uniform_vec4);
            break;
        }
    }

    rgfx_camera_t* camera = rgfx_internal_get_active_camera();
    if (camera)
    {
        mat4x4 view;
        mat4x4 projection;
        rgfx_camera_get_matrices(camera, view, projection);
        glUniformMatrix4fv(glGetUniformLocation(sprite_ptr->shaderProgram, "uView"), 1, GL_FALSE, (float*)view);
        glUniformMatrix4fv(glGetUniformLocation(sprite_ptr->shaderProgram, "uProjection"), 1, GL_FALSE, (float*)projection);
    }
    else
    {
        mat4x4 identity;
        mat4x4_identity(identity);
        glUniformMatrix4fv(glGetUniformLocation(sprite_ptr->shaderProgram, "uView"), 1, GL_FALSE, (float*)identity);
        glUniformMatrix4fv(glGetUniformLocation(sprite_ptr->shaderProgram, "uProjection"), 1, GL_FALSE, (float*)identity);
    }

    if (sprite_ptr->hasTexture)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sprite_ptr->textureID);
        glUniform1i(glGetUniformLocation(sprite_ptr->shaderProgram, "uTexture"), 0);
    }

    glBindVertexArray(sprite_ptr->VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void rgfx_sprite_set_position(rgfx_sprite_handle sprite, vec3 position)
{
    rgfx_sprite_t* sprite_ptr = rgfx_sprite_from_handle(sprite);
    if (!sprite_ptr)
    {
        return;
    }

    rtransform_set_position(sprite_ptr->transform, position);
}

void rgfx_sprite_set_size(rgfx_sprite_handle sprite, vec2 size)
{
    rgfx_sprite_t* sprite_ptr = rgfx_sprite_from_handle(sprite);
    if (!sprite_ptr)
    {
        return;
    }

    vec3 new_scale = { size[0], size[1], sprite_ptr->size[2] };
    rtransform_set_scale(sprite_ptr->transform, new_scale);
    sprite_ptr->size[0] = size[0];
    sprite_ptr->size[1] = size[1];
}

void rgfx_sprite_set_color(rgfx_sprite_handle sprite, color color)
{
    rgfx_sprite_t* sprite_ptr = rgfx_sprite_from_handle(sprite);
    if (!sprite_ptr)
    {
        return;
    }

    sprite_ptr->color = color;
}

void rgfx_sprite_set_texture(rgfx_sprite_handle sprite, unsigned int textureID)
{
    rgfx_sprite_t* sprite_ptr = rgfx_sprite_from_handle(sprite);
    if (!sprite_ptr)
    {
        return;
    }

    sprite_ptr->textureID  = textureID;
    sprite_ptr->hasTexture = textureID != 0;
}

void rgfx_sprite_get_position(rgfx_sprite_handle sprite, vec3 out_position)
{
    if (!out_position)
    {
        return;
    }

    rgfx_sprite_t* sprite_ptr = rgfx_sprite_from_handle(sprite);
    if (!sprite_ptr)
    {
        out_position[0] = out_position[1] = out_position[2] = 0.0f;
        return;
    }

    rtransform_get_world_position(sprite_ptr->transform, out_position);
}

void rgfx_sprite_get_size(rgfx_sprite_handle sprite, vec2 out_size)
{
    rgfx_sprite_t* sprite_ptr = rgfx_sprite_from_handle(sprite);
    if (!sprite_ptr || !out_size)
    {
        return;
    }

    out_size[0] = sprite_ptr->size[0];
    out_size[1] = sprite_ptr->size[1];
}

color rgfx_sprite_get_color(rgfx_sprite_handle sprite)
{
    rgfx_sprite_t* sprite_ptr = rgfx_sprite_from_handle(sprite);
    if (!sprite_ptr)
    {
        color c = { 0, 0, 0 };
        return c;
    }
    return sprite_ptr->color;
}

unsigned int rgfx_sprite_get_texture_id(rgfx_sprite_handle sprite)
{
    rgfx_sprite_t* sprite_ptr = rgfx_sprite_from_handle(sprite);
    return sprite_ptr ? sprite_ptr->textureID : 0;
}

static void rgfx_sprite_set_uniform(rgfx_sprite_handle sprite, const char* name, const rgfx_uniform_t* new_uniform)
{
    if (!name)
    {
        return;
    }

    rgfx_sprite_t* sprite_ptr = rgfx_sprite_from_handle(sprite);
    if (!sprite_ptr)
    {
        return;
    }

    for (int i = 0; i < sprite_ptr->uniform_count; ++i)
    {
        if (strcmp(sprite_ptr->uniforms[i].name, name) == 0 && sprite_ptr->uniforms[i].type == new_uniform->type)
        {
            sprite_ptr->uniforms[i] = *new_uniform;
            sprite_ptr->uniforms[i].name = name;
            return;
        }
    }

    if (sprite_ptr->uniform_count >= RGFX_MAX_UNIFORMS)
    {
        return;
    }

    sprite_ptr->uniforms[sprite_ptr->uniform_count] = *new_uniform;
    sprite_ptr->uniforms[sprite_ptr->uniform_count].name = name;
    sprite_ptr->uniform_count++;
}

void rgfx_sprite_set_uniform_float(rgfx_sprite_handle sprite, const char* name, float value)
{
    rgfx_uniform_t uniform = {
        .name          = name,
        .type          = RGFX_UNIFORM_FLOAT,
        .uniform_float = value,
    };
    rgfx_sprite_set_uniform(sprite, name, &uniform);
}

void rgfx_sprite_set_uniform_int(rgfx_sprite_handle sprite, const char* name, int value)
{
    rgfx_uniform_t uniform = {
        .name        = name,
        .type        = RGFX_UNIFORM_INT,
        .uniform_int = value,
    };
    rgfx_sprite_set_uniform(sprite, name, &uniform);
}

void rgfx_sprite_set_uniform_vec2(rgfx_sprite_handle sprite, const char* name, vec2 value)
{
    rgfx_uniform_t uniform = {
        .name = name,
        .type = RGFX_UNIFORM_VEC2,
    };
    vec2_dup(uniform.uniform_vec2, value);
    rgfx_sprite_set_uniform(sprite, name, &uniform);
}

void rgfx_sprite_set_uniform_vec3(rgfx_sprite_handle sprite, const char* name, vec3 value)
{
    rgfx_uniform_t uniform = {
        .name = name,
        .type = RGFX_UNIFORM_VEC3,
    };
    vec3_dup(uniform.uniform_vec3, value);
    rgfx_sprite_set_uniform(sprite, name, &uniform);
}

void rgfx_sprite_set_uniform_vec4(rgfx_sprite_handle sprite, const char* name, vec4 value)
{
    rgfx_uniform_t uniform = {
        .name = name,
        .type = RGFX_UNIFORM_VEC4,
    };
    vec4_dup(uniform.uniform_vec4, value);
    rgfx_sprite_set_uniform(sprite, name, &uniform);
}

rtransform_t* rgfx_sprite_get_transform(rgfx_sprite_handle sprite)
{
    rgfx_sprite_t* sprite_ptr = rgfx_sprite_from_handle(sprite);
    return sprite_ptr ? sprite_ptr->transform : NULL;
}

void rgfx_sprite_set_parent(rgfx_sprite_handle child, rgfx_sprite_handle parent)
{
    rgfx_sprite_t* child_ptr = rgfx_sprite_from_handle(child);
    if (!child_ptr)
    {
        return;
    }

    rtransform_t* parent_transform = NULL;
    if (parent != RGFX_INVALID_SPRITE_HANDLE)
    {
        rgfx_sprite_t* parent_ptr = rgfx_sprite_from_handle(parent);
        parent_transform = parent_ptr ? parent_ptr->transform : NULL;
    }

    rtransform_set_parent(child_ptr->transform, parent_transform);
}

void rgfx_sprite_set_rotation(rgfx_sprite_handle sprite, float rotation)
{
    rgfx_sprite_t* sprite_ptr = rgfx_sprite_from_handle(sprite);
    if (!sprite_ptr)
    {
        return;
    }

    vec3 axis = { 0.0f, 0.0f, 1.0f };
    rtransform_set_rotation_axis_angle(sprite_ptr->transform, axis, rotation);
}

void rgfx_sprite_get_world_position(rgfx_sprite_handle sprite, vec3 out_position)
{
    if (!out_position)
    {
        return;
    }

    rgfx_sprite_t* sprite_ptr = rgfx_sprite_from_handle(sprite);
    if (!sprite_ptr)
    {
        out_position[0] = out_position[1] = out_position[2] = 0.0f;
        return;
    }

    rtransform_get_world_position(sprite_ptr->transform, out_position);
}
