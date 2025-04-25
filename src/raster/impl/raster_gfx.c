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
    vec3           position;
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
    stbtt_fontinfo font_info;
    unsigned char* font_buffer;
    unsigned char* font_bitmap;
    char           text[RGFX_MAX_TEXT_LENGTH];
    float          font_size;
    vec3           position;
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

    // Use linmath.h's vec*_dup functions
    vec3_dup(sprite->position, desc->position);
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
    if (!sprite)
    {
        return;
    }

    // Use shader program
    glUseProgram(sprite->shaderProgram);

    // Set common uniforms for position, size, and color
    glUniform3f(glGetUniformLocation(sprite->shaderProgram, "uPosition"),
                sprite->position[0],
                sprite->position[1],
                sprite->position[2]);
    glUniform2f(glGetUniformLocation(sprite->shaderProgram, "uSize"), sprite->size[0], sprite->size[1]);
    glUniform3f(
        glGetUniformLocation(sprite->shaderProgram, "uColor"), sprite->color.r, sprite->color.g, sprite->color.b);

    // Add uniform for time - pass current time to all shaders
    float currentTime = rapp_get_time();
    glUniform1f(glGetUniformLocation(sprite->shaderProgram, "uTime"), currentTime);

    // Apply custom uniforms
    for (int i = 0; i < sprite->uniform_count; i++)
    {
        const rgfx_uniform_t* uniform  = &sprite->uniforms[i];
        int                   location = glGetUniformLocation(sprite->shaderProgram, uniform->name);
        if (location != -1)
        {
            switch (uniform->type)
            {
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

    // Check if the uniform exists before setting it
    // This handles the uUseTexture uniform which is only in the custom shader
    int useTextureLocation = glGetUniformLocation(sprite->shaderProgram, "uUseTexture");
    if (useTextureLocation != -1)
    {
        glUniform1i(useTextureLocation, sprite->hasTexture ? 1 : 0);
    }

    // Get and set camera matrices if there's an active camera
    if (active_camera)
    {
        mat4x4 view, projection;
        rgfx_camera_get_matrices(active_camera, view, projection);

        // Set view and projection matrices in shader
        glUniformMatrix4fv(glGetUniformLocation(sprite->shaderProgram, "uView"), 1, GL_FALSE, (float*)view);
        glUniformMatrix4fv(glGetUniformLocation(sprite->shaderProgram, "uProjection"), 1, GL_FALSE, (float*)projection);
    }
    else
    {
        // Use identity matrices if no camera is active
        mat4x4 identity;
        mat4x4_identity(identity);
        glUniformMatrix4fv(glGetUniformLocation(sprite->shaderProgram, "uView"), 1, GL_FALSE, (float*)identity);
        glUniformMatrix4fv(glGetUniformLocation(sprite->shaderProgram, "uProjection"), 1, GL_FALSE, (float*)identity);
    }

    // Handle texture if it exists
    if (sprite->hasTexture && sprite->textureID > 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sprite->textureID);

        int textureLoc = glGetUniformLocation(sprite->shaderProgram, "uTexture");
        if (textureLoc != -1)
        {
            glUniform1i(textureLoc, 0);
        }

        int useTextureLoc = glGetUniformLocation(sprite->shaderProgram, "uUseTexture");
        if (useTextureLoc != -1)
        {
            glUniform1i(useTextureLoc, 1);
        }
    }
    else if (glGetUniformLocation(sprite->shaderProgram, "uUseTexture") != -1)
    {
        glUniform1i(glGetUniformLocation(sprite->shaderProgram, "uUseTexture"), 0);
    }

    glBindVertexArray(sprite->VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    if (sprite->hasTexture && sprite->textureID > 0)
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

// Sprite properties setters and getters
// Improved setters using linmath's vec*_dup functions
void rgfx_sprite_set_position(rgfx_sprite_t* sprite, vec3 position)
{
    if (sprite)
    {
        // Use the built-in vec3_dup function instead of element-by-element assignment
        vec3_dup(sprite->position, position);
    }
}

void rgfx_sprite_set_size(rgfx_sprite_t* sprite, vec2 size)
{
    if (sprite)
    {
        // Use the built-in vec2_dup function instead of element-by-element assignment
        vec2_dup(sprite->size, size);
    }
}

void rgfx_sprite_set_color(rgfx_sprite_t* sprite, color color)
{
    if (sprite)
    {
        sprite->color = color;
    }
}

// Getter functions can also use vec*_dup
void rgfx_sprite_get_position(rgfx_sprite_t* sprite, vec3 out_position)
{
    if (sprite && out_position)
    {
        // Use vec3_dup instead of manual assignment
        vec3_dup(out_position, sprite->position);
    }
    else if (out_position)
    {
        // Zero-initialize
        out_position[0] = 0.0f;
        out_position[1] = 0.0f;
        out_position[2] = 0.0f;
    }
}

void rgfx_sprite_get_size(rgfx_sprite_t* sprite, vec2 out_size)
{
    if (sprite && out_size)
    {
        // Use vec2_dup instead of manual assignment
        vec2_dup(out_size, sprite->size);
    }
    else if (out_size)
    {
        // Zero-initialize
        out_size[0] = 0.0f;
        out_size[1] = 0.0f;
    }
}

color rgfx_sprite_get_color(rgfx_sprite_t* sprite)
{
    color result = { 0 };
    if (sprite)
    {
        result = sprite->color;
    }
    return result;
}

// Load texture from file
unsigned int rgfx_load_texture(const char* filepath)
{
    if (!filepath)
        return 0;

    // Generate texture
    unsigned int textureID;
    glGenTextures(1, &textureID);

    // Load image data using stb_image
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true); // Flip y-axis during loading
    unsigned char* data = stbi_load(filepath, &width, &height, &channels, 0);

    if (data)
    {
        GLenum format;
        if (channels == 1)
            format = GL_RED;
        else if (channels == 3)
            format = GL_RGB;
        else if (channels == 4)
            format = GL_RGBA;
        else
        {
            printf("ERROR: Unsupported texture format with %d channels\n", channels);
            stbi_image_free(data);
            glDeleteTextures(1, &textureID);
            return 0;
        }

        // Bind and set texture parameters
        glBindTexture(GL_TEXTURE_2D, textureID);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload texture data
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Free image data
        stbi_image_free(data);

        return textureID;
    }
    else
    {
        rlog_error("Failed to load texture: %s\n", filepath);
        glDeleteTextures(1, &textureID);
        return 0;
    }
}

void rgfx_delete_texture(unsigned int textureID)
{
    if (textureID > 0)
    {
        glDeleteTextures(1, &textureID);
    }
}

// Set texture for a sprite
void rgfx_sprite_set_texture(rgfx_sprite_t* sprite, unsigned int textureID)
{
    if (!sprite)
        return;

    sprite->textureID  = textureID;
    sprite->hasTexture = (textureID > 0);
}

// Get texture ID from sprite
unsigned int rgfx_sprite_get_texture_id(rgfx_sprite_t* sprite)
{
    return sprite ? sprite->textureID : 0;
}

rgfx_camera_t* rgfx_camera_create(const rgfx_camera_desc_t* desc)
{
    if (!desc)
        return NULL;

    rgfx_camera_t* camera = (rgfx_camera_t*)malloc(sizeof(rgfx_camera_t));
    if (!camera)
        return NULL;

    vec3_dup(camera->position, desc->position);

    // Calculate forward direction from position to target
    vec3 direction;
    vec3_sub(direction, desc->target, desc->position);
    vec3_norm(camera->forward, direction); // Store normalized direction

    vec3_dup(camera->up, desc->up);
    camera->fov    = desc->fov;
    camera->aspect = desc->aspect;
    camera->near   = desc->near;
    camera->far    = desc->far;

    return camera;
}

void rgfx_camera_destroy(rgfx_camera_t* camera)
{
    if (camera)
    {
        if (active_camera == camera)
        {
            active_camera = NULL;
        }
        free(camera);
    }
}

void rgfx_camera_set_position(rgfx_camera_t* camera, vec3 position)
{
    if (camera)
    {
        vec3_dup(camera->position, position);
    }
}

void rgfx_camera_set_target(rgfx_camera_t* camera, vec3 target)
{
    if (camera)
    {
        // Calculate direction vector from camera position to target
        vec3 direction;
        direction[0] = target[0] - camera->position[0];
        direction[1] = target[1] - camera->position[1];
        direction[2] = target[2] - camera->position[2];

        // Normalize the direction vector
        vec3_norm(direction, direction);

        // Set the camera's forward direction
        vec3_dup(camera->forward, direction);
    }
}

void rgfx_camera_get_matrices(const rgfx_camera_t* camera, mat4x4 view, mat4x4 projection)
{
    if (!camera)
    {
        mat4x4_identity(view);
        mat4x4_identity(projection);
        return;
    }

    // Calculate target point by adding forward direction to position
    vec3 target;
    vec3_add(target, camera->position, camera->forward);

    // Calculate view matrix using position and target point
    mat4x4_look_at(view, camera->position, target, camera->up);

    // Calculate projection matrix
    mat4x4_perspective(projection, camera->fov, camera->aspect, camera->near, camera->far);
}

void rgfx_set_active_camera(rgfx_camera_t* camera)
{
    active_camera = camera;
}

void rgfx_camera_set_direction(rgfx_camera_t* camera, vec3 direction)
{
    if (camera)
    {
        vec3_norm(camera->forward, direction); // Normalize the direction vector
    }
}

void rgfx_camera_move(rgfx_camera_t* camera, vec3 offset)
{
    if (camera)
    {
        vec3_add(camera->position, camera->position, offset);
    }
}

void rgfx_camera_rotate(rgfx_camera_t* camera, float yaw, float pitch)
{
    if (!camera)
        return;

    // Convert angles to radians
    float yaw_rad   = yaw * (3.14159f / 180.0f);
    float pitch_rad = pitch * (3.14159f / 180.0f);

    // Calculate new forward direction
    camera->forward[0] = cosf(yaw_rad) * cosf(pitch_rad);
    camera->forward[1] = sinf(pitch_rad);
    camera->forward[2] = sinf(yaw_rad) * cosf(pitch_rad);

    vec3_norm(camera->forward, camera->forward);
}

void rgfx_camera_look_at(rgfx_camera_t* camera, vec3 target)
{
    if (!camera)
        return;

    // Calculate direction vector from camera position to target
    vec3 direction;
    direction[0] = target[0] - camera->position[0];
    direction[1] = target[1] - camera->position[1];
    direction[2] = target[2] - camera->position[2];

    // Normalize the direction vector
    vec3_norm(direction, direction);

    // Set the camera's forward direction
    vec3_dup(camera->forward, direction);
}

// Uniform API implementations
void rgfx_sprite_set_uniform_float(rgfx_sprite_t* sprite, const char* name, float value)
{
    if (!sprite || !name)
        return;

    // First check if the uniform already exists
    for (int i = 0; i < sprite->uniform_count; i++)
    {
        if (strcmp(sprite->uniforms[i].name, name) == 0)
        {
            sprite->uniforms[i].type      = RGFX_UNIFORM_FLOAT;
            sprite->uniforms[i].float_val = value;
            return;
        }
    }

    // If not found and we have room, add a new uniform
    if (sprite->uniform_count < RGFX_MAX_UNIFORMS)
    {
        sprite->uniforms[sprite->uniform_count].name      = name;
        sprite->uniforms[sprite->uniform_count].type      = RGFX_UNIFORM_FLOAT;
        sprite->uniforms[sprite->uniform_count].float_val = value;
        sprite->uniform_count++;
    }
}

void rgfx_sprite_set_uniform_int(rgfx_sprite_t* sprite, const char* name, int value)
{
    if (!sprite || !name)
        return;

    // First check if the uniform already exists
    for (int i = 0; i < sprite->uniform_count; i++)
    {
        if (strcmp(sprite->uniforms[i].name, name) == 0)
        {
            sprite->uniforms[i].type    = RGFX_UNIFORM_INT;
            sprite->uniforms[i].int_val = value;
            return;
        }
    }

    // If not found and we have room, add a new uniform
    if (sprite->uniform_count < RGFX_MAX_UNIFORMS)
    {
        sprite->uniforms[sprite->uniform_count].name    = name;
        sprite->uniforms[sprite->uniform_count].type    = RGFX_UNIFORM_INT;
        sprite->uniforms[sprite->uniform_count].int_val = value;
        sprite->uniform_count++;
    }
}

void rgfx_sprite_set_uniform_vec2(rgfx_sprite_t* sprite, const char* name, vec2 value)
{
    if (!sprite || !name)
        return;

    // First check if the uniform already exists
    for (int i = 0; i < sprite->uniform_count; i++)
    {
        if (strcmp(sprite->uniforms[i].name, name) == 0)
        {
            sprite->uniforms[i].type = RGFX_UNIFORM_VEC2;
            vec2_dup(sprite->uniforms[i].vec2_val, value);
            return;
        }
    }

    // If not found and we have room, add a new uniform
    if (sprite->uniform_count < RGFX_MAX_UNIFORMS)
    {
        sprite->uniforms[sprite->uniform_count].name = name;
        sprite->uniforms[sprite->uniform_count].type = RGFX_UNIFORM_VEC2;
        vec2_dup(sprite->uniforms[sprite->uniform_count].vec2_val, value);
        sprite->uniform_count++;
    }
}

void rgfx_sprite_set_uniform_vec3(rgfx_sprite_t* sprite, const char* name, vec3 value)
{
    if (!sprite || !name)
        return;

    // First check if the uniform already exists
    for (int i = 0; i < sprite->uniform_count; i++)
    {
        if (strcmp(sprite->uniforms[i].name, name) == 0)
        {
            sprite->uniforms[i].type = RGFX_UNIFORM_VEC3;
            vec3_dup(sprite->uniforms[i].vec3_val, value);
            return;
        }
    }

    // If not found and we have room, add a new uniform
    if (sprite->uniform_count < RGFX_MAX_UNIFORMS)
    {
        sprite->uniforms[sprite->uniform_count].name = name;
        sprite->uniforms[sprite->uniform_count].type = RGFX_UNIFORM_VEC3;
        vec3_dup(sprite->uniforms[sprite->uniform_count].vec3_val, value);
        sprite->uniform_count++;
    }
}

void rgfx_sprite_set_uniform_vec4(rgfx_sprite_t* sprite, const char* name, vec4 value)
{
    if (!sprite || !name)
        return;

    // First check if the uniform already exists
    for (int i = 0; i < sprite->uniform_count; i++)
    {
        if (strcmp(sprite->uniforms[i].name, name) == 0)
        {
            sprite->uniforms[i].type = RGFX_UNIFORM_VEC4;
            vec4_dup(sprite->uniforms[i].vec4_val, value);
            return;
        }
    }

    // If not found and we have room, add a new uniform
    if (sprite->uniform_count < RGFX_MAX_UNIFORMS)
    {
        sprite->uniforms[sprite->uniform_count].name = name;
        sprite->uniforms[sprite->uniform_count].type = RGFX_UNIFORM_VEC4;
        vec4_dup(sprite->uniforms[sprite->uniform_count].vec4_val, value);
        sprite->uniform_count++;
    }
}

// Text implementation with stb_truetype
rgfx_text_t* rgfx_text_create(const rgfx_text_desc_t* desc)
{
    if (!desc || !desc->font_path || !desc->text)
        return NULL;

    rgfx_text_t* text  = (rgfx_text_t*)calloc(1, sizeof(rgfx_text_t));
    text->line_spacing = 1.2f;
    // Initialize alignment from descriptor
    text->alignment = desc->alignment;
    if (!text)
        return NULL;

    // Initialize basic properties
    vec3_dup(text->position, desc->position);
    text->text_color = desc->text_color;
    text->font_size  = desc->font_size;

    // Copy text with length limit
    strncpy(text->text, desc->text, RGFX_MAX_TEXT_LENGTH - 1);
    text->text[RGFX_MAX_TEXT_LENGTH - 1] = '\0';

    // Load font file
    FILE* font_file = fopen(desc->font_path, "rb");
    if (!font_file)
    {
        rlog_error("Failed to open font file: %s", desc->font_path);
        free(text);
        return NULL;
    }

    // Get file size
    fseek(font_file, 0, SEEK_END);
    long font_size = ftell(font_file);
    fseek(font_file, 0, SEEK_SET);

    // Load entire TTF file into memory
    text->font_buffer = (unsigned char*)malloc(font_size);
    if (!text->font_buffer)
    {
        rlog_error("Failed to allocate memory for font");
        fclose(font_file);
        free(text);
        return NULL;
    }

    size_t bytes_read = fread(text->font_buffer, 1, font_size, font_file);
    fclose(font_file);

    if (bytes_read != (size_t)font_size)
    {
        rlog_error("Failed to read font file");
        free(text->font_buffer);
        free(text);
        return NULL;
    }

    // Initialize font
    if (!stbtt_InitFont(&text->font_info, text->font_buffer, 0))
    {
        rlog_error("Failed to initialize font");
        free(text->font_buffer);
        free(text);
        return NULL;
    }

    // Create bitmap for the text
    text->bitmap_width  = 0;
    text->bitmap_height = 0;
    text->font_bitmap   = NULL;

    // Generate the bitmap for the text
    if (!rgfx_text_update_bitmap(text))
    {
        free(text->font_buffer);
        free(text);
        return NULL;
    }

    // Load shader files instead of using embedded shaders
    char* vertexSource = rgfx_load_shader_source("assets/shaders/text.vert");
    if (!vertexSource)
    {
        rlog_error("Failed to load text vertex shader");
        free(text->font_buffer);
        free(text->font_bitmap);
        free(text);
        return NULL;
    }

    char* fragmentSource = rgfx_load_shader_source("assets/shaders/text.frag");
    if (!fragmentSource)
    {
        rlog_error("Failed to load text fragment shader");
        free(vertexSource);
        free(text->font_buffer);
        free(text->font_bitmap);
        free(text);
        return NULL;
    }

    // Create shader program
    text->shaderProgram = _create_shader_program(vertexSource, fragmentSource);
    if (text->shaderProgram == 0)
    {
        free(text->font_buffer);
        free(text->font_bitmap);
        free(text);
        return NULL;
    }

    // Setup VAO, VBO, EBO for rendering
    float vertices[] = {
        // positions        // texture coords
        0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // top left
        1.0f, 1.0f, 0.0f, 1.0f, 0.0f, // top right
        1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // bottom right
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f  // bottom left
    };

    unsigned int indices[] = {
        0, 1, 2, // first triangle
        2, 3, 0  // second triangle
    };

    glGenVertexArrays(1, &text->VAO);
    glGenBuffers(1, &text->VBO);
    glGenBuffers(1, &text->EBO);

    glBindVertexArray(text->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, text->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Generate texture for text
    glGenTextures(1, &text->textureID);
    glBindTexture(GL_TEXTURE_2D, text->textureID);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_R8,
                 text->bitmap_width,
                 text->bitmap_height,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 text->font_bitmap);
    glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    return text;
}

bool rgfx_text_update_bitmap(rgfx_text_t* text)
{
    if (!text)
        return false;

    // Free existing bitmap if it exists
    if (text->font_bitmap)
    {
        free(text->font_bitmap);
        text->font_bitmap = NULL;
    }

    // Split text into lines
    text_lines_t lines = split_text_into_lines(text->text);
    if (lines.count == 0)
    {
        return false;
    }

    // Calculate scale for font size
    float scale = stbtt_ScaleForPixelHeight(&text->font_info, text->font_size);

    // Calculate text dimensions
    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&text->font_info, &ascent, &descent, &line_gap);

    ascent   = (int)(ascent * scale);
    descent  = (int)(descent * scale);
    line_gap = (int)(line_gap * scale);

    // Calculate line height including spacing
    float line_spacing = text->line_spacing > 0 ? text->line_spacing : 1.2f;
    int   line_height  = (int)((ascent - descent) * line_spacing);

    // First pass: calculate total width and height
    int total_width = 0;
    int line_widths[lines.count];

    for (int i = 0; i < lines.count; i++)
    {
        int line_width = 0;
        for (const char* p = lines.lines[i]; *p; p++)
        {
            int advance, lsb;
            stbtt_GetCodepointHMetrics(&text->font_info, *p, &advance, &lsb);
            line_width += (int)(advance * scale);

            // Add kerning with next character
            if (*(p + 1))
            {
                line_width += (int)(stbtt_GetCodepointKernAdvance(&text->font_info, *p, *(p + 1)) * scale);
            }
        }

        line_widths[i] = line_width;
        if (line_width > total_width)
        {
            total_width = line_width;
        }
    }

    // Calculate total height (with padding)
    int total_height = lines.count * line_height + 10; // Add some padding

    // Create bitmap with dimensions
    text->bitmap_width  = total_width + 8; // Add padding
    text->bitmap_height = total_height;

    text->font_bitmap = (unsigned char*)calloc(text->bitmap_width * text->bitmap_height, 1);
    if (!text->font_bitmap)
    {
        rlog_error("Failed to allocate memory for text bitmap");
        free_text_lines(&lines);
        return false;
    }

    // Second pass: render each line into the bitmap
    int y = ascent + 4; // Start position (baseline) with some padding

    for (int i = 0; i < lines.count; i++)
    {
        // Calculate starting x position based on alignment
        int x = 4; // Default left alignment with padding

        if (text->alignment == RGFX_TEXT_ALIGN_CENTER)
        {
            x = (text->bitmap_width - line_widths[i]) / 2;
        }
        else if (text->alignment == RGFX_TEXT_ALIGN_RIGHT)
        {
            x = text->bitmap_width - line_widths[i] - 4; // Padding
        }

        // Render each character in the line
        for (const char* p = lines.lines[i]; *p; p++)
        {
            // Get character bitmap
            int c_x1, c_y1, c_x2, c_y2;
            stbtt_GetCodepointBitmapBox(&text->font_info, *p, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

            // Render character
            int            w, h, xoff, yoff;
            unsigned char* bitmap = stbtt_GetCodepointBitmap(&text->font_info, scale, scale, *p, &w, &h, &xoff, &yoff);

            // Copy character bitmap to our text bitmap
            for (int row = 0; row < h; row++)
            {
                for (int col = 0; col < w; col++)
                {
                    if (x + col + xoff >= 0 && x + col + xoff < text->bitmap_width && y + row + yoff >= 0 &&
                        y + row + yoff < text->bitmap_height)
                    {
                        text->font_bitmap[(y + row + yoff) * text->bitmap_width + (x + col + xoff)] =
                            bitmap[row * w + col];
                    }
                }
            }

            // Move to next character position
            int advance, lsb;
            stbtt_GetCodepointHMetrics(&text->font_info, *p, &advance, &lsb);
            x += (int)(advance * scale);

            // Add kerning with next character
            if (*(p + 1))
            {
                x += (int)(stbtt_GetCodepointKernAdvance(&text->font_info, *p, *(p + 1)) * scale);
            }

            // Free character bitmap
            stbtt_FreeBitmap(bitmap, NULL);
        }

        // Move to next line position
        y += line_height;
    }

    // Free the lines
    free_text_lines(&lines);
    return true;
}

void rgfx_text_set_text(rgfx_text_t* text, const char* new_text)
{
    if (!text || !new_text)
        return;

    // Copy new text with length limit
    strncpy(text->text, new_text, RGFX_MAX_TEXT_LENGTH - 1);
    text->text[RGFX_MAX_TEXT_LENGTH - 1] = '\0';

    // Update the bitmap
    if (rgfx_text_update_bitmap(text))
    {
        // Update texture with new bitmap
        glBindTexture(GL_TEXTURE_2D, text->textureID);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RED,
                     text->bitmap_width,
                     text->bitmap_height,
                     0,
                     GL_RED,
                     GL_UNSIGNED_BYTE,
                     text->font_bitmap);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void rgfx_text_draw(rgfx_text_t* text)
{
    if (!text)
        return;

    // Use shader program
    glUseProgram(text->shaderProgram);

    // Use a larger scale factor for bigger text in world space
    float scale_factor = 0.015f; // Increased 3x for better visibility

    // Calculate dimensions
    float width  = text->bitmap_width * scale_factor;
    float height = text->bitmap_height * scale_factor;

    // Center text horizontally at its position
    glUniform3f(glGetUniformLocation(text->shaderProgram, "uPosition"),
                text->position[0] - (width / 2),
                text->position[1],
                text->position[2]);

    // Set size uniform
    glUniform2f(glGetUniformLocation(text->shaderProgram, "uSize"), width, height);

    // Set color uniform
    glUniform3f(glGetUniformLocation(text->shaderProgram, "uColor"),
                text->text_color.r,
                text->text_color.g,
                text->text_color.b);

    // Handle camera matrices
    if (active_camera)
    {
        mat4x4 view, projection;
        rgfx_camera_get_matrices(active_camera, view, projection);

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

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, text->textureID);
    glUniform1i(glGetUniformLocation(text->shaderProgram, "uTexture"), 0);

    // Draw text quad
    glBindVertexArray(text->VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void rgfx_text_destroy(rgfx_text_t* text)
{
    if (!text)
        return;

    // Delete OpenGL resources
    glDeleteVertexArrays(1, &text->VAO);
    glDeleteBuffers(1, &text->VBO);
    glDeleteBuffers(1, &text->EBO);
    glDeleteTextures(1, &text->textureID);
    glDeleteProgram(text->shaderProgram);

    // Free memory
    if (text->font_buffer)
    {
        free(text->font_buffer);
    }

    if (text->font_bitmap)
    {
        free(text->font_bitmap);
    }

    free(text);
}

// Setter and getter functions
void rgfx_text_set_position(rgfx_text_t* text, vec3 position)
{
    if (text)
    {
        vec3_dup(text->position, position);
    }
}

void rgfx_text_set_color(rgfx_text_t* text, color color)
{
    if (text)
    {
        text->text_color = color;
    }
}

void rgfx_text_set_font_size(rgfx_text_t* text, float size)
{
    if (!text || size <= 0)
        return;

    text->font_size = size;

    // Update the bitmap with new font size
    if (rgfx_text_update_bitmap(text))
    {
        // Update texture
        glBindTexture(GL_TEXTURE_2D, text->textureID);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RED,
                     text->bitmap_width,
                     text->bitmap_height,
                     0,
                     GL_RED,
                     GL_UNSIGNED_BYTE,
                     text->font_bitmap);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void rgfx_text_get_position(rgfx_text_t* text, vec3 out_position)
{
    if (text && out_position)
    {
        vec3_dup(out_position, text->position);
    }
    else if (out_position)
    {
        out_position[0] = 0.0f;
        out_position[1] = 0.0f;
        out_position[2] = 0.0f;
    }
}

color rgfx_text_get_color(rgfx_text_t* text)
{
    color result = { 0 };
    if (text)
    {
        result = text->text_color;
    }
    return result;
}

float rgfx_text_get_font_size(rgfx_text_t* text)
{
    return text ? text->font_size : 0.0f;
}

const char* rgfx_text_get_text(rgfx_text_t* text)
{
    return text ? text->text : NULL;
}

void rgfx_text_set_alignment(rgfx_text_t* text, int alignment)
{
    if (!text)
        return;

    // Validate alignment value (0=left, 1=center, 2=right)
    if (alignment < RGFX_TEXT_ALIGN_LEFT || alignment > RGFX_TEXT_ALIGN_RIGHT)
    {
        alignment = RGFX_TEXT_ALIGN_LEFT; // Default to left if invalid
    }

    if (text->alignment != alignment)
    {
        text->alignment = alignment;

        // Update the bitmap with new alignment
        if (rgfx_text_update_bitmap(text))
        {
            // Update texture
            glBindTexture(GL_TEXTURE_2D, text->textureID);
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RED,
                         text->bitmap_width,
                         text->bitmap_height,
                         0,
                         GL_RED,
                         GL_UNSIGNED_BYTE,
                         text->font_bitmap);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
}

void rgfx_text_set_line_spacing(rgfx_text_t* text, float spacing)
{
    if (!text)
        return;

    // Validate spacing (reasonable limits)
    if (spacing < 0.5f)
        spacing = 0.5f;
    if (spacing > 3.0f)
        spacing = 3.0f;

    if (text->line_spacing != spacing)
    {
        text->line_spacing = spacing;

        // Update the bitmap with new line spacing
        if (rgfx_text_update_bitmap(text))
        {
            // Update texture
            glBindTexture(GL_TEXTURE_2D, text->textureID);
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RED,
                         text->bitmap_width,
                         text->bitmap_height,
                         0,
                         GL_RED,
                         GL_UNSIGNED_BYTE,
                         text->font_bitmap);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
}

int rgfx_text_get_alignment(rgfx_text_t* text)
{
    return text ? text->alignment : RGFX_TEXT_ALIGN_LEFT;
}

float rgfx_text_get_line_spacing(rgfx_text_t* text)
{
    return text ? text->line_spacing : 1.2f;
}

// Helper function to split text into lines
static text_lines_t split_text_into_lines(const char* text)
{
    text_lines_t result = { NULL, 0 };
    if (!text)
        return result;

    // Count number of lines (number of newlines + 1)
    int line_count = 1;
    for (const char* p = text; *p; p++)
    {
        if (*p == '\n')
        {
            line_count++;
        }
    }

    // Allocate memory for line pointers
    result.lines = (char**)malloc(line_count * sizeof(char*));
    if (!result.lines)
        return result;

    result.count = line_count;

    // Copy each line
    const char* start      = text;
    int         line_index = 0;

    for (const char* p = text;; p++)
    {
        if (*p == '\n' || *p == '\0')
        {
            // Calculate line length
            size_t len = p - start;

            // Allocate and copy the line
            result.lines[line_index] = (char*)malloc(len + 1);
            if (result.lines[line_index])
            {
                strncpy(result.lines[line_index], start, len);
                result.lines[line_index][len] = '\0';
            }

            line_index++;
            start = p + 1;
        }

        if (*p == '\0')
            break;
    }

    return result;
}

// Helper function to free text_lines_t
static void free_text_lines(text_lines_t* lines)
{
    if (!lines || !lines->lines)
        return;

    for (int i = 0; i < lines->count; i++)
    {
        if (lines->lines[i])
        {
            free(lines->lines[i]);
        }
    }

    free(lines->lines);
    lines->lines = NULL;
    lines->count = 0;
}