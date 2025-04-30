#include "raster/raster_gfx.h"
#include "raster/raster_math.h"
#include "raster/raster_app.h"
#include "raster/raster_log.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

#define RGFX_MAX_TEXT_LENGTH 256

// Helper struct for multiline text
typedef struct
{
    char** lines;
    int    count;
} text_lines_t;

// Forward declarations of helper functions for text rendering
static text_lines_t split_text_into_lines(const char* text);
static void         free_text_lines(text_lines_t* lines);

struct rgfx_sprite
{
    unsigned int   VAO;
    unsigned int   VBO;
    unsigned int   EBO;
    unsigned int   shaderProgram;
    unsigned int   textureID;
    bool           hasTexture;
    rtransform_t*  transform;  // New transform component
    vec2           size;
    color          color;
    rgfx_uniform_t uniforms[RGFX_MAX_UNIFORMS];
    int            uniform_count;
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
    unsigned int   VAO;
    unsigned int   VBO;
    unsigned int   EBO;
    unsigned int   shaderProgram;
    unsigned int   textureID;
    rtransform_t*  transform;  // New transform component
    stbtt_fontinfo font_info;
    unsigned char* font_buffer;
    unsigned char* font_bitmap;
    char           text[RGFX_MAX_TEXT_LENGTH];
    float          font_size;
    color          text_color;
    int            bitmap_width;
    int            bitmap_height;
    float          line_spacing;
    int            alignment;
    unsigned int   index_count; // Number of indices for drawing
};

static rgfx_camera_t* active_camera = NULL;

char* rgfx_load_shader_source(const char* filepath)
{
    if (!filepath)
        return NULL;

    FILE* file = fopen(filepath, "rb");
    if (!file)
    {
        rlog_error("Failed to open shader file: %s\n", filepath);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the source (+1 for null terminator)
    char* source = (char*)malloc(size + 1);
    if (!source)
    {
        fclose(file);
        rlog_error("Failed to allocate memory for shader source\n");
        return NULL;
    }

    size_t bytesRead = fread(source, 1, size, file);
    fclose(file);

    if (bytesRead < (size_t)size)
    {
        free(source);
        rlog_error("Failed to read shader file: %s\n", filepath);
        return NULL;
    }

    source[size] = '\0';

    if (strstr(source, "#version") == NULL)
    {
        // Select appropriate GLSL version based on platform
        const char* version_directive;

#if defined(__EMSCRIPTEN__)
        rlog_info("Using WebGL/GLSL ES shader version for file: %s", filepath);
        version_directive = "#version 300 es\nprecision mediump float;\n";
#else
        rlog_info("Using Desktop/GLSL shader version for file: %s", filepath);
        version_directive = "#version 330 core\n";
#endif

        // Version directive not found, allocate new buffer with enough space for directive
        size_t directive_len = strlen(version_directive);
        char*  new_source    = (char*)malloc(size + directive_len + 1);

        if (!new_source)
        {
            free(source);
            rlog_error("Failed to allocate memory for shader source with version directive\n");
            return NULL;
        }

        // Copy version directive followed by original source
        strcpy(new_source, version_directive);
        strcat(new_source, source);

        rlog_info("Added version directive: %s", version_directive);

        // Free original buffer and return the new one
        free(source);
        return new_source;
    }
    else
    {
        // Log that we found an existing version directive
        char        versionLine[64] = { 0 };
        const char* versionStart    = strstr(source, "#version");
        if (versionStart)
        {
            const char* lineEnd = strchr(versionStart, '\n');
            if (lineEnd)
            {
                size_t len = lineEnd - versionStart;
                if (len < sizeof(versionLine) - 1)
                {
                    strncpy(versionLine, versionStart, len);
                    versionLine[len] = '\0';
                    rlog_info("Found existing version directive in %s: %s", filepath, versionLine);
                }
            }
        }
    }

    // Version directive already exists, return original source
    return source;
}

// Helper function to compile shaders
static unsigned int _compile_shader(unsigned int type, const char* source)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check for shader compile errors
    int  success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("ERROR: Shader compilation failed\n%s\n", infoLog);
        return 0;
    }
    return shader;
}

// Helper function to create shader program
static unsigned int _create_shader_program(const char* vertexSource, const char* fragmentSource)
{
    unsigned int vertexShader   = _compile_shader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fragmentShader = _compile_shader(GL_FRAGMENT_SHADER, fragmentSource);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    int  success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        rlog_error("Shader program linking failed\n%s\n", infoLog);
        return 0;
    }

    // Delete shaders after linking
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

bool rgfx_init(void)
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        return false;
    }

    // Enable blending for transparency with proper blend factors
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable depth testing for proper z-ordering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL); // Use LEQUAL to allow same-depth sprites to render

    return true;
}

void rgfx_clear(float r, float g, float b)
{
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear both color and depth buffers
}

void rgfx_clear_color(color color)
{
    glClearColor(color.r, color.g, color.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear both color and depth buffers
}

void rgfx_shutdown(void)
{
    // Nothing to do for now
}

// Public function to create a shader program
unsigned int rgfx_create_shader_program(const char* vertexSource, const char* fragmentSource)
{
    return _create_shader_program(vertexSource, fragmentSource);
}

// Descriptor-based sprite creation
rgfx_sprite_t* rgfx_sprite_create(const rgfx_sprite_desc_t* desc)
{
    if (!desc)
        return NULL;

    // Require vertex and fragment shader paths
    if (!desc->vertex_shader_path || !desc->fragment_shader_path)
    {
        rlog_error("Vertex and fragment shader paths are required");
        return NULL;
    }

    rgfx_sprite_t* sprite = (rgfx_sprite_t*)malloc(sizeof(rgfx_sprite_t));
    if (!sprite)
    {
        return NULL;
    }

    // Create transform component
    sprite->transform = rtransform_create(sprite);
    if (!sprite->transform)
    {
        free(sprite);
        return NULL;
    }

    // Initialize transform with descriptor position
    vec3 pos;
    memcpy(pos, desc->position, sizeof(vec3));
    rtransform_set_position(sprite->transform, pos);
    
    // Set initial scale based on size
    vec3 scale = {desc->size[0], desc->size[1], 1.0f};
    rtransform_set_scale(sprite->transform, scale);
    
    // Store size and color
    vec2_dup(sprite->size, desc->size);
    sprite->color = desc->color;

    // Initialize uniform system
    sprite->uniform_count = 0;
    if (desc->uniform_count > 0)
    {
        // Copy uniforms from descriptor using the provided count
        sprite->uniform_count = desc->uniform_count > RGFX_MAX_UNIFORMS ? RGFX_MAX_UNIFORMS : desc->uniform_count;
        for (int i = 0; i < sprite->uniform_count; i++)
        {
            sprite->uniforms[i] = desc->uniforms[i];
        }
    }
    else
    {
        // Auto-calculate the uniform count by finding the last non-empty uniform
        for (int i = 0; i < RGFX_MAX_UNIFORMS; i++)
        {
            if (desc->uniforms[i].name != NULL)
            {
                sprite->uniforms[sprite->uniform_count] = desc->uniforms[i];
                sprite->uniform_count++;
            }
        }
    }

    // Load shader files - no fallback to defaults
    char* vertexSource = rgfx_load_shader_source(desc->vertex_shader_path);
    if (!vertexSource)
    {
        rlog_error("Failed to load vertex shader from %s", desc->vertex_shader_path);
        free(sprite);
        return NULL;
    }

    char* fragmentSource = rgfx_load_shader_source(desc->fragment_shader_path);
    if (!fragmentSource)
    {
        rlog_error("Failed to load fragment shader from %s", desc->fragment_shader_path);
        free(vertexSource);
        free(sprite);
        return NULL;
    }

    // Create the shader program
    sprite->shaderProgram = _create_shader_program(vertexSource, fragmentSource);

    // Free shader sources after creating program
    free(vertexSource);
    free(fragmentSource);

    if (sprite->shaderProgram == 0)
    {
        free(sprite);
        return NULL;
    }

    // Load texture if specified
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
        }
    }

    // Define vertices for a quad (square)
    // For custom shaders with texture support, we need to include texture coordinates
    float vertices[16];
    bool  useTextureCoords = false;

    // Check if the shader has texture coordinate attribute
    if (glGetAttribLocation(sprite->shaderProgram, "aTexCoord") != -1)
    {
        useTextureCoords = true;

        // Vertices with position and texture coordinates
        float verts[] = {
            // positions    // texture coords
            -0.5f, -0.5f, 0.0f, 0.0f, // bottom left
            0.5f,  -0.5f, 1.0f, 0.0f, // bottom right
            0.5f,  0.5f,  1.0f, 1.0f, // top right
            -0.5f, 0.5f,  0.0f, 1.0f  // top left
        };
        memcpy(vertices, verts, sizeof(verts));
    }
    else
    {
        // Just positions for default shader
        float verts[] = {
            -0.5f, -0.5f, // bottom left
            0.5f,  -0.5f, // bottom right
            0.5f,  0.5f,  // top right
            -0.5f, 0.5f   // top left
        };
        memcpy(vertices, verts, sizeof(verts));
    }

    // Define indices for the quad (two triangles)
    unsigned int indices[] = {
        0, 1, 2, // first triangle
        2, 3, 0  // second triangle
    };

    // Create VAO, VBO, and EBO
    glGenVertexArrays(1, &sprite->VAO);
    glGenBuffers(1, &sprite->VBO);
    glGenBuffers(1, &sprite->EBO);

    // Bind the Vertex Array Object first
    glBindVertexArray(sprite->VAO);

    // Bind and set VBO and EBO
    glBindBuffer(GL_ARRAY_BUFFER, sprite->VBO);
    glBufferData(GL_ARRAY_BUFFER, useTextureCoords ? sizeof(float) * 16 : sizeof(float) * 8, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    if (useTextureCoords)
    {
        // Set position attribute
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Set texture coordinate attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
    else
    {
        // Set the vertex attribute pointers (position only)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    // Unbind the VBO and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return sprite;
}

void rgfx_sprite_destroy(rgfx_sprite_t* sprite)
{
    if (!sprite)
    {
        return;
    }

    // Delete resources
    glDeleteVertexArrays(1, &sprite->VAO);
    glDeleteBuffers(1, &sprite->VBO);
    glDeleteBuffers(1, &sprite->EBO);
    glDeleteProgram(sprite->shaderProgram);

    // Free memory
    free(sprite);
}

void rgfx_sprite_draw(rgfx_sprite_t* sprite)
{
    if (!sprite) return;

    // Use shader program
    glUseProgram(sprite->shaderProgram);

    // Update transform
    rtransform_update(sprite->transform);

    // Set model matrix from transform's world matrix
    glUniformMatrix4fv(glGetUniformLocation(sprite->shaderProgram, "uModel"), 1, GL_FALSE, (float*)sprite->transform->world);

    // Set size and color
    glUniform2f(glGetUniformLocation(sprite->shaderProgram, "uSize"), sprite->size[0], sprite->size[1]);
    glUniform3f(glGetUniformLocation(sprite->shaderProgram, "uColor"), sprite->color.r, sprite->color.g, sprite->color.b);

    // Set time uniform
    float currentTime = rapp_get_time();
    glUniform1f(glGetUniformLocation(sprite->shaderProgram, "uTime"), currentTime);

    // Set texture usage flag
    glUniform1i(glGetUniformLocation(sprite->shaderProgram, "uUseTexture"), sprite->hasTexture ? 1 : 0);

    // Apply custom uniforms
    for (int i = 0; i < sprite->uniform_count; i++) {
        const rgfx_uniform_t* uniform = &sprite->uniforms[i];
        int location = glGetUniformLocation(sprite->shaderProgram, uniform->name);
        if (location != -1) {
            switch (uniform->type) {
                case RGFX_UNIFORM_FLOAT:
                    glUniform1f(location, uniform->float_val);
                    break;
                case RGFX_UNIFORM_INT:
                    glUniform1i(location, uniform->int_val);
                    break;
                case RGFX_UNIFORM_VEC2:
                    glUniform2fv(location, 1, uniform->vec2_val);
                    break;
                case RGFX_UNIFORM_VEC3:
                    glUniform3fv(location, 1, uniform->vec3_val);
                    break;
                case RGFX_UNIFORM_VEC4:
                    glUniform4fv(location, 1, uniform->vec4_val);
                    break;
            }
        }
    }

    // Get and set camera matrices if there's an active camera
    if (active_camera) {
        mat4x4 view, projection;
        rgfx_camera_get_matrices(active_camera, view, projection);
        glUniformMatrix4fv(glGetUniformLocation(sprite->shaderProgram, "uView"), 1, GL_FALSE, (float*)view);
        glUniformMatrix4fv(glGetUniformLocation(sprite->shaderProgram, "uProjection"), 1, GL_FALSE, (float*)projection);
    } else {
        mat4x4 identity;
        mat4x4_identity(identity);
        glUniformMatrix4fv(glGetUniformLocation(sprite->shaderProgram, "uView"), 1, GL_FALSE, (float*)identity);
        glUniformMatrix4fv(glGetUniformLocation(sprite->shaderProgram, "uProjection"), 1, GL_FALSE, (float*)identity);
    }

    // Bind texture if available
    if (sprite->hasTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sprite->textureID);
        glUniform1i(glGetUniformLocation(sprite->shaderProgram, "uTexture"), 0);
    }

    // Bind VAO and draw the sprite
    glBindVertexArray(sprite->VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

rgfx_text_t* rgfx_text_create(const rgfx_text_desc_t* desc) {
    if (!desc || !desc->font_path || !desc->text) return NULL;

    rgfx_text_t* text = (rgfx_text_t*)calloc(1, sizeof(rgfx_text_t));
    if (!text) return NULL;

    text->line_spacing = desc->line_spacing > 0.0f ? desc->line_spacing : 1.2f;
    
    // Create transform component
    text->transform = rtransform_create(text);
    if (!text->transform) {
        free(text);
        return NULL;
    }

    // Initialize transform with descriptor position
    vec3 pos;
    vec3_dup(pos, desc->position);
    rtransform_set_position(text->transform, pos);

    // Initialize text properties
    text->text_color = desc->text_color;
    text->font_size = desc->font_size;
    text->alignment = desc->alignment;
    
    // Copy text with length limit
    strncpy(text->text, desc->text, RGFX_MAX_TEXT_LENGTH - 1);
    text->text[RGFX_MAX_TEXT_LENGTH - 1] = '\0';

    // Load font file
    FILE* fontFile = fopen(desc->font_path, "rb");
    if (!fontFile) {
        free(text->transform);
        free(text);
        return NULL;
    }

    fseek(fontFile, 0, SEEK_END);
    long fontFileSize = ftell(fontFile);
    fseek(fontFile, 0, SEEK_SET);

    text->font_buffer = (unsigned char*)malloc(fontFileSize);
    if (!text->font_buffer) {
        fclose(fontFile);
        free(text->transform);
        free(text);
        return NULL;
    }

    fread(text->font_buffer, 1, fontFileSize, fontFile);
    fclose(fontFile);

    // Initialize font
    if (!stbtt_InitFont(&text->font_info, text->font_buffer, 0)) {
        free(text->font_buffer);
        free(text->transform);
        free(text);
        return NULL;
    }

    // Create vertex buffer for text quad
    float vertices[] = {
        // positions    // texture coords
        -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.0f, 1.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    // Load shaders
    char* vertexSource = rgfx_load_shader_source("assets/shaders/text.vert");
    char* fragmentSource = rgfx_load_shader_source("assets/shaders/text.frag");

    if (!vertexSource || !fragmentSource) {
        if (vertexSource) free(vertexSource);
        if (fragmentSource) free(fragmentSource);
        free(text->font_buffer);
        free(text->transform);
        free(text);
        return NULL;
    }

    text->shaderProgram = _create_shader_program(vertexSource, fragmentSource);
    free(vertexSource);
    free(fragmentSource);

    if (!text->shaderProgram) {
        free(text->font_buffer);
        free(text->transform);
        free(text);
        return NULL;
    }

    // Create VAO and buffers
    glGenVertexArrays(1, &text->VAO);
    glGenBuffers(1, &text->VBO);
    glGenBuffers(1, &text->EBO);

    glBindVertexArray(text->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, text->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Generate bitmap texture for text
    if (!rgfx_text_update_bitmap(text)) {
        glDeleteVertexArrays(1, &text->VAO);
        glDeleteBuffers(1, &text->VBO);
        glDeleteBuffers(1, &text->EBO);
        glDeleteProgram(text->shaderProgram);
        free(text->font_buffer);
        free(text->transform);
        free(text);
        return NULL;
    }

    return text;
}

void rgfx_text_draw(rgfx_text_t* text) {
    if (!text) return;

    glUseProgram(text->shaderProgram);

    // Update transform
    rtransform_update(text->transform);

    // Set model matrix from transform's world matrix
    glUniformMatrix4fv(glGetUniformLocation(text->shaderProgram, "uModel"), 1, GL_FALSE, (float*)text->transform->world);

    // Set color uniform
    glUniform3f(glGetUniformLocation(text->shaderProgram, "uColor"),
                text->text_color.r,
                text->text_color.g,
                text->text_color.b);

    // Handle camera matrices
    if (active_camera) {
        mat4x4 view, projection;
        rgfx_camera_get_matrices(active_camera, view, projection);
        glUniformMatrix4fv(glGetUniformLocation(text->shaderProgram, "uView"), 1, GL_FALSE, (float*)view);
        glUniformMatrix4fv(glGetUniformLocation(text->shaderProgram, "uProjection"), 1, GL_FALSE, (float*)projection);
    } else {
        mat4x4 identity;
        mat4x4_identity(identity);
        glUniformMatrix4fv(glGetUniformLocation(text->shaderProgram, "uView"), 1, GL_FALSE, (float*)identity);
        glUniformMatrix4fv(glGetUniformLocation(text->shaderProgram, "uProjection"), 1, GL_FALSE, (float*)identity);
    }

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, text->textureID);
    glUniform1i(glGetUniformLocation(text->shaderProgram, "uTexture"), 0);

    // Draw text quad
    glBindVertexArray(text->VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// Update position setters to use transform
void rgfx_sprite_set_position(rgfx_sprite_t* sprite, vec3 position) {
    if (sprite && sprite->transform) {
        vec3 pos;
        memcpy(pos, position, sizeof(vec3));
        rtransform_set_position(sprite->transform, pos);
    }
}

void rgfx_text_set_position(rgfx_text_t* text, vec3 position) {
    if (text && text->transform) {
        vec3 pos;
        memcpy(pos, position, sizeof(vec3));
        rtransform_set_position(text->transform, pos);
    }
}

// Add rotation setters
void rgfx_sprite_set_rotation(rgfx_sprite_t* sprite, float rotation) {
    if (sprite && sprite->transform) {
        rtransform_set_rotation(sprite->transform, rotation);
    }
}

void rgfx_text_set_rotation(rgfx_text_t* text, float rotation) {
    if (text && text->transform) {
        rtransform_set_rotation(text->transform, rotation);
    }
}

// Get world position for both sprite and text
void rgfx_sprite_get_world_position(rgfx_sprite_t* sprite, vec3 out_position) {
    if (sprite && sprite->transform) {
        rtransform_get_world_position(sprite->transform, out_position);
    } else if (out_position) {
        out_position[0] = out_position[1] = out_position[2] = 0.0f;
    }
}

void rgfx_text_get_world_position(rgfx_text_t* text, vec3 out_position) {
    if (text && text->transform) {
        rtransform_get_world_position(text->transform, out_position);
    } else if (out_position) {
        out_position[0] = out_position[1] = out_position[2] = 0.0f;
    }
}

// Transform getters
rtransform_t* rgfx_sprite_get_transform(rgfx_sprite_t* sprite) {
    return sprite ? sprite->transform : NULL;
}

rtransform_t* rgfx_text_get_transform(rgfx_text_t* text) {
    return text ? text->transform : NULL;
}

// Parent-child relationship functions
void rgfx_sprite_set_parent(rgfx_sprite_t* sprite, rgfx_sprite_t* parent) {
    if (sprite && sprite->transform) {
        rtransform_set_parent(sprite->transform, parent ? parent->transform : NULL);
    }
}

void rgfx_text_set_parent(rgfx_text_t* text, rgfx_sprite_t* parent) {
    if (text && text->transform) {
        rtransform_set_parent(text->transform, parent ? parent->transform : NULL);
    }
}

rgfx_camera_t* rgfx_camera_create(const rgfx_camera_desc_t* desc) {
    if (!desc) return NULL;

    rgfx_camera_t* camera = (rgfx_camera_t*)malloc(sizeof(rgfx_camera_t));
    if (!camera) return NULL;

    // Initialize camera properties
    vec3_dup(camera->position, desc->position);
    
    // Calculate forward vector from position and target
    vec3 forward;
    vec3_sub(forward, desc->target, desc->position);
    vec3_norm(camera->forward, forward);
    
    // Store up vector
    vec3_dup(camera->up, desc->up);
    
    // Store projection parameters
    camera->fov = desc->fov;
    camera->aspect = desc->aspect;
    camera->near = desc->near;
    camera->far = desc->far;

    return camera;
}

void rgfx_camera_destroy(rgfx_camera_t* camera) {
    if (camera) {
        if (camera == active_camera) {
            active_camera = NULL;
        }
        free(camera);
    }
}

void rgfx_camera_set_position(rgfx_camera_t* camera, vec3 position) {
    if (camera) {
        vec3_dup(camera->position, position);
    }
}

void rgfx_camera_get_matrices(const rgfx_camera_t* camera, mat4x4 view, mat4x4 projection) {
    if (!camera) return;

    // Calculate view matrix using look_at
    mat4x4_look_at(view, 
                   camera->position,
                   (vec3){camera->position[0] + camera->forward[0],
                         camera->position[1] + camera->forward[1],
                         camera->position[2] + camera->forward[2]},
                   camera->up);

    // Calculate projection matrix
    mat4x4_perspective(projection,
                      camera->fov,
                      camera->aspect,
                      camera->near,
                      camera->far);
}

void rgfx_set_active_camera(rgfx_camera_t* camera) {
    active_camera = camera;
}

// Texture loading implementation
unsigned int rgfx_load_texture(const char* filepath) {
    if (!filepath) return 0;

    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, channels;
    unsigned char* data = stbi_load(filepath, &width, &height, &channels, 0);
    if (!data) {
        rlog_error("Failed to load texture: %s\n", filepath);
        return 0;
    }

    GLenum format = channels == 4 ? GL_RGBA : GL_RGB;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}

// Text bitmap update implementation
bool rgfx_text_update_bitmap(rgfx_text_t* text) {
    if (!text || !text->text[0]) return false;

    // Calculate bitmap dimensions
    float scale = stbtt_ScaleForPixelHeight(&text->font_info, text->font_size);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&text->font_info, &ascent, &descent, &lineGap);

    // Calculate text dimensions
    int text_width = 0;
    int text_height = (int)((ascent - descent) * scale);
    
    // Calculate total width
    const char* p = text->text;
    while (*p) {
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&text->font_info, *p, &advance, &lsb);
        text_width += (int)(advance * scale);
        p++;
    }

    // Create bitmap with some padding
    text->bitmap_width = text_width + 4;
    text->bitmap_height = text_height + 4;
    
    // Allocate bitmap memory
    text->font_bitmap = (unsigned char*)realloc(text->font_bitmap, 
                                              text->bitmap_width * text->bitmap_height);
    if (!text->font_bitmap) return false;

    // Clear bitmap
    memset(text->font_bitmap, 0, text->bitmap_width * text->bitmap_height);

    // Render text into bitmap
    int x = 2;
    int y = 2 + (int)(ascent * scale);
    
    p = text->text;
    while (*p) {
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&text->font_info, *p, &advance, &lsb);
        
        // Render glyph
        int c_x1, c_y1, c_x2, c_y2;
        stbtt_GetCodepointBitmapBox(&text->font_info, *p, scale, scale,
                                   &c_x1, &c_y1, &c_x2, &c_y2);
        
        stbtt_MakeCodepointBitmap(&text->font_info, 
                                 text->font_bitmap + x + (y + c_y1) * text->bitmap_width,
                                 c_x2 - c_x1, c_y2 - c_y1,
                                 text->bitmap_width,
                                 scale, scale,
                                 *p);
        
        x += (int)(advance * scale);
        
        // Handle kerning
        if (*(p + 1)) {
            int kern = stbtt_GetCodepointKernAdvance(&text->font_info, *p, *(p + 1));
            x += (int)(kern * scale);
        }
        p++;
    }

    // Generate or update texture
    if (!text->textureID) {
        glGenTextures(1, &text->textureID);
    }
    
    glBindTexture(GL_TEXTURE_2D, text->textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                 text->bitmap_width, text->bitmap_height,
                 0, GL_RED, GL_UNSIGNED_BYTE, text->font_bitmap);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return true;
}

void rgfx_text_destroy(rgfx_text_t* text) {
    if (!text) return;

    if (text->font_buffer) {
        free(text->font_buffer);
    }
    if (text->font_bitmap) {
        free(text->font_bitmap);
    }
    if (text->textureID) {
        glDeleteTextures(1, &text->textureID);
    }
    if (text->transform) {
        free(text->transform);
    }

    glDeleteVertexArrays(1, &text->VAO);
    glDeleteBuffers(1, &text->VBO);
    glDeleteBuffers(1, &text->EBO);
    glDeleteProgram(text->shaderProgram);

    free(text);
}

// Uniform setters
void rgfx_sprite_set_uniform_float(rgfx_sprite_t* sprite, const char* name, float value) {
    if (!sprite || !name || sprite->uniform_count >= RGFX_MAX_UNIFORMS) return;

    // Look for existing uniform
    for (int i = 0; i < sprite->uniform_count; i++) {
        if (strcmp(sprite->uniforms[i].name, name) == 0) {
            sprite->uniforms[i].type = RGFX_UNIFORM_FLOAT;
            sprite->uniforms[i].float_val = value;
            return;
        }
    }

    // Add new uniform
    sprite->uniforms[sprite->uniform_count].name = name;
    sprite->uniforms[sprite->uniform_count].type = RGFX_UNIFORM_FLOAT;
    sprite->uniforms[sprite->uniform_count].float_val = value;
    sprite->uniform_count++;
}

void rgfx_sprite_set_uniform_int(rgfx_sprite_t* sprite, const char* name, int value) {
    if (!sprite || !name || sprite->uniform_count >= RGFX_MAX_UNIFORMS) return;

    // Look for existing uniform
    for (int i = 0; i < sprite->uniform_count; i++) {
        if (strcmp(sprite->uniforms[i].name, name) == 0) {
            sprite->uniforms[i].type = RGFX_UNIFORM_INT;
            sprite->uniforms[i].int_val = value;
            return;
        }
    }

    // Add new uniform
    sprite->uniforms[sprite->uniform_count].name = name;
    sprite->uniforms[sprite->uniform_count].type = RGFX_UNIFORM_INT;
    sprite->uniforms[sprite->uniform_count].int_val = value;
    sprite->uniform_count++;
}