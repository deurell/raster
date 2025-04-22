#include "raster/raster_gfx.h"
#include "raster/raster_math.h"
#include "raster/raster_log.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h> // Include without implementation

// Internal sprite structure
struct rgfx_sprite
{
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
    unsigned int shaderProgram;
    unsigned int textureID;
    bool hasTexture;
    rmath_vec3_t position;
    rmath_vec2_t size;
    rmath_color_t color;
};

// Default shader source code
static const char* s_vertex_shader_src = "#version 330 core\n"
                                         "layout (location = 0) in vec2 aPos;\n"
                                         "uniform vec2 uPosition;\n"
                                         "uniform vec2 uSize;\n"
                                         "void main()\n"
                                         "{\n"
                                         "    vec2 pos = aPos * uSize + uPosition;\n"
                                         "    gl_Position = vec4(pos, 0.0, 1.0);\n"
                                         "}\n";

static const char* s_fragment_shader_src = "#version 330 core\n"
                                           "out vec4 FragColor;\n"
                                           "uniform vec3 uColor;\n"
                                           "void main()\n"
                                           "{\n"
                                           "    FragColor = vec4(uColor, 1.0);\n"
                                           "}\n";

// Load shader source from file
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

    // Read file content
    size_t bytesRead = fread(source, 1, size, file);
    fclose(file);

    if (bytesRead < (size_t)size)
    {
        free(source);
        rlog_error("Failed to read shader file: %s\n", filepath);
        return NULL;
    }

    // Null terminate the string
    source[size] = '\0';
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

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

void rgfx_clear(float r, float g, float b)
{
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void rgfx_clear_color(rmath_color_t color)
{
    rgfx_clear(color.r, color.g, color.b);
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

    rgfx_sprite_t* sprite = (rgfx_sprite_t*)malloc(sizeof(rgfx_sprite_t));
    if (!sprite)
    {
        return NULL;
    }

    // Initialize sprite data from descriptor
    sprite->position = desc->position;
    sprite->size     = desc->size;
    sprite->color    = desc->color;
    sprite->textureID = 0;  // Initialize with no texture
    sprite->hasTexture = false;

    // Create shader program - either from files or use defaults
    const char* vertexSource = s_vertex_shader_src;
    const char* fragmentSource = s_fragment_shader_src;
    char* customVertexSource = NULL;
    char* customFragmentSource = NULL;
    
    // Load custom shaders if specified
    if (desc->vertex_shader_path)
    {
        customVertexSource = rgfx_load_shader_source(desc->vertex_shader_path);
        if (customVertexSource)
            vertexSource = customVertexSource;
        else
            rlog_warning("Failed to load vertex shader from %s, using default", desc->fragment_shader_path);
    }
    
    if (desc->fragment_shader_path)
    {
        customFragmentSource = rgfx_load_shader_source(desc->fragment_shader_path);
        if (customFragmentSource)
            fragmentSource = customFragmentSource;
        else
            rlog_warning("Failed to load fragment shader from %s, using default", desc->fragment_shader_path);
    }
    
    sprite->shaderProgram = _create_shader_program(vertexSource, fragmentSource);
    
    // Free custom shader sources if allocated
    if (customVertexSource)
        free(customVertexSource);
    if (customFragmentSource)
        free(customFragmentSource);
        
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
            sprite->textureID = texture;
            sprite->hasTexture = true;
        }
        else
        {
            printf("WARNING: Failed to load texture from %s\n", desc->texture_path);
        }
    }

    // Define vertices for a quad (square)
    // For custom shaders with texture support, we need to include texture coordinates
    float vertices[16];
    bool useTextureCoords = false;
    
    // Check if the shader has texture coordinate attribute
    if (glGetAttribLocation(sprite->shaderProgram, "aTexCoord") != -1)
    {
        useTextureCoords = true;
        
        // Vertices with position and texture coordinates
        float verts[] = {
            // positions    // texture coords
            -0.5f, -0.5f,  0.0f, 0.0f, // bottom left
             0.5f, -0.5f,  1.0f, 0.0f, // bottom right
             0.5f,  0.5f,  1.0f, 1.0f, // top right
            -0.5f,  0.5f,  0.0f, 1.0f  // top left
        };
        memcpy(vertices, verts, sizeof(verts));
    }
    else
    {
        // Just positions for default shader
        float verts[] = {
            -0.5f, -0.5f, // bottom left
             0.5f, -0.5f, // bottom right
             0.5f,  0.5f, // top right
            -0.5f,  0.5f  // top left
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
    glBufferData(GL_ARRAY_BUFFER, useTextureCoords ? sizeof(float) * 16 : sizeof(float) * 8, 
                vertices, GL_STATIC_DRAW);

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
    glUniform2f(glGetUniformLocation(sprite->shaderProgram, "uPosition"), sprite->position.x, sprite->position.y);
    glUniform2f(glGetUniformLocation(sprite->shaderProgram, "uSize"), sprite->size.x, sprite->size.y);
    glUniform3f(glGetUniformLocation(sprite->shaderProgram, "uColor"), sprite->color.r, sprite->color.g, sprite->color.b);

    // Check if the uniform exists before setting it
    // This handles the uUseTexture uniform which is only in the custom shader
    int useTextureLocation = glGetUniformLocation(sprite->shaderProgram, "uUseTexture");
    if (useTextureLocation != -1) {
        glUniform1i(useTextureLocation, sprite->hasTexture ? 1 : 0);
    }

    // Handle texture if it exists
    if (sprite->hasTexture && sprite->textureID > 0) {
        // Active texture unit 0
        glActiveTexture(GL_TEXTURE0);
        // Bind the texture
        glBindTexture(GL_TEXTURE_2D, sprite->textureID);
        
        // Set the texture sampler uniform (only if it exists in the shader)
        int textureLoc = glGetUniformLocation(sprite->shaderProgram, "uTexture");
        if (textureLoc != -1) {
            glUniform1i(textureLoc, 0); // Use texture unit 0
        }
    }

    // Bind the vertex array
    glBindVertexArray(sprite->VAO);

    // Draw the sprite
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Unbind the vertex array
    glBindVertexArray(0);
    
    // Unbind texture if we had one
    if (sprite->hasTexture && sprite->textureID > 0) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

// Sprite properties setters and getters
void rgfx_sprite_set_position(rgfx_sprite_t* sprite, rmath_vec3_t position)
{
    if (sprite)
    {
        sprite->position = position;
    }
}

void rgfx_sprite_set_size(rgfx_sprite_t* sprite, rmath_vec2_t size)
{
    if (sprite)
    {
        sprite->size = size;
    }
}

void rgfx_sprite_set_color(rgfx_sprite_t* sprite, rmath_color_t color)
{
    if (sprite)
    {
        sprite->color = color;
    }
}

rmath_vec3_t rgfx_sprite_get_position_vec3(rgfx_sprite_t* sprite)
{
    rmath_vec3_t result = {0};
    if (sprite)
    {
        result = sprite->position;
    }
    return result;
}

rmath_vec2_t rgfx_sprite_get_size(rgfx_sprite_t* sprite)
{
    rmath_vec2_t result = {0};
    if (sprite)
    {
        result = sprite->size;
    }
    return result;
}

rmath_color_t rgfx_sprite_get_color(rgfx_sprite_t* sprite)
{
    rmath_color_t result = {0};
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
        
    sprite->textureID = textureID;
    sprite->hasTexture = (textureID > 0);
}

// Get texture ID from sprite
unsigned int rgfx_sprite_get_texture_id(rgfx_sprite_t* sprite)
{
    return sprite ? sprite->textureID : 0;
}