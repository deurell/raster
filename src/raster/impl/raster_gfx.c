#include "raster/raster_gfx.h"
#include "raster/raster_math.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>

// Internal sprite structure
struct rgfx_sprite
{
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
    unsigned int shaderProgram;
    float        x, y;
    float        width, height;
    float        r, g, b;
    int          z_order;
};

// Shader source code
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

// Helper function to compile shaders
static unsigned int compile_shader(unsigned int type, const char* source)
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
static unsigned int create_shader_program(const char* vertexSource, const char* fragmentSource)
{
    unsigned int vertexShader   = compile_shader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fragmentShader = compile_shader(GL_FRAGMENT_SHADER, fragmentSource);

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
        printf("ERROR: Shader program linking failed\n%s\n", infoLog);
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
    sprite->x       = desc->position.x;
    sprite->y       = desc->position.y;
    sprite->width   = desc->size.x;
    sprite->height  = desc->size.y;
    sprite->r       = desc->color.r;
    sprite->g       = desc->color.g;
    sprite->b       = desc->color.b;
    sprite->z_order = desc->z_order;

    // Create shader program
    sprite->shaderProgram = create_shader_program(s_vertex_shader_src, s_fragment_shader_src);
    if (sprite->shaderProgram == 0)
    {
        free(sprite);
        return NULL;
    }

    // Define vertices for a quad (square)
    // Using normalized coordinates (-0.5 to 0.5 for both x and y)
    float vertices[] = {
        -0.5f, -0.5f, // bottom left
        0.5f,  -0.5f, // bottom right
        0.5f,  0.5f,  // top right
        -0.5f, 0.5f   // top left
    };

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
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Set the vertex attribute pointers (position only)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

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

    // Set uniforms for position, size, and color
    glUniform2f(glGetUniformLocation(sprite->shaderProgram, "uPosition"), sprite->x, sprite->y);
    glUniform2f(glGetUniformLocation(sprite->shaderProgram, "uSize"), sprite->width, sprite->height);
    glUniform3f(glGetUniformLocation(sprite->shaderProgram, "uColor"), sprite->r, sprite->g, sprite->b);

    // Bind the vertex array
    glBindVertexArray(sprite->VAO);

    // Draw the sprite
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Unbind the vertex array
    glBindVertexArray(0);
}

// Sprite properties setters and getters
void rgfx_sprite_set_position(rgfx_sprite_t* sprite, float x, float y)
{
    if (sprite)
    {
        sprite->x = x;
        sprite->y = y;
    }
}

void rgfx_sprite_set_size(rgfx_sprite_t* sprite, float width, float height)
{
    if (sprite)
    {
        sprite->width  = width;
        sprite->height = height;
    }
}

void rgfx_sprite_set_color(rgfx_sprite_t* sprite, float r, float g, float b)
{
    if (sprite)
    {
        sprite->r = r;
        sprite->g = g;
        sprite->b = b;
    }
}

void rgfx_sprite_set_position_vec2(rgfx_sprite_t* sprite, rmath_vec2_t position)
{
    if (sprite)
    {
        sprite->x = position.x;
        sprite->y = position.y;
    }
}

void rgfx_sprite_set_size_vec2(rgfx_sprite_t* sprite, rmath_vec2_t size)
{
    if (sprite)
    {
        sprite->width  = size.x;
        sprite->height = size.y;
    }
}

void rgfx_sprite_set_color_struct(rgfx_sprite_t* sprite, rmath_color_t color)
{
    if (sprite)
    {
        sprite->r = color.r;
        sprite->g = color.g;
        sprite->b = color.b;
    }
}

void rgfx_sprite_set_z_order(rgfx_sprite_t* sprite, int z_order)
{
    if (sprite)
    {
        sprite->z_order = z_order;
    }
}

rmath_vec2_t rgfx_sprite_get_position(rgfx_sprite_t* sprite)
{
    rmath_vec2_t result = {0};
    if (sprite)
    {
        result.x = sprite->x;
        result.y = sprite->y;
    }
    return result;
}

rmath_vec2_t rgfx_sprite_get_size(rgfx_sprite_t* sprite)
{
    rmath_vec2_t result = {0};
    if (sprite)
    {
        result.x = sprite->width;
        result.y = sprite->height;
    }
    return result;
}

rmath_color_t rgfx_sprite_get_color(rgfx_sprite_t* sprite)
{
    rmath_color_t result = {0};
    if (sprite)
    {
        result.r = sprite->r;
        result.g = sprite->g;
        result.b = sprite->b;
    }
    return result;
}

int rgfx_sprite_get_z_order(rgfx_sprite_t* sprite)
{
    return sprite ? sprite->z_order : 0;
}