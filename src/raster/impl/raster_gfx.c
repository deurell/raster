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

struct rgfx_sprite
{
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
    unsigned int shaderProgram;
    unsigned int textureID;
    bool         hasTexture;
    vec3         position;
    vec2         size;
    color        color;
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
        rlog_info("Using WebGL/GLSL ES shader version");
        version_directive = "#version 300 es\nprecision mediump float;\n";
#else
        rlog_info("Using Desktop/GLSL shader version");
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

        // Free original buffer and return the new one
        free(source);
        return new_source;
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