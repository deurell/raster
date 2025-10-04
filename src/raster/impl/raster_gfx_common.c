#include "raster_gfx_internal.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

static rgfx_camera_t* g_active_camera         = NULL;
static unsigned int   g_text_shader_program   = 0;
static int            g_text_shader_refcount  = 0;

#if defined(__EMSCRIPTEN__)
static const char* const RGFX_DEFAULT_SPRITE_VERTEX_SHADER =
    "#version 300 es\n"
    "precision mediump float;\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "out vec2 TexCoord;\n"
    "uniform mat4 uModel;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uProjection;\n"
    "uniform float uTime;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = uProjection * uView * uModel * vec4(aPos, 0.0, 1.0);\n"
    "    TexCoord = aTexCoord;\n"
    "}\n";

static const char* const RGFX_DEFAULT_SPRITE_FRAGMENT_SHADER =
    "#version 300 es\n"
    "precision mediump float;\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;\n"
    "uniform sampler2D uTexture;\n"
    "uniform vec3 uColor;\n"
    "uniform bool uUseTexture;\n"
    "uniform float uTime;\n"
    "void main()\n"
    "{\n"
    "    vec4 finalColor;\n"
    "    if (uUseTexture) {\n"
    "        vec4 texColor = texture(uTexture, TexCoord);\n"
    "        if (texColor.a < 0.01) {\n"
    "            discard;\n"
    "        }\n"
    "        finalColor = vec4(texColor.rgb * uColor, texColor.a);\n"
    "    } else {\n"
    "        finalColor = vec4(uColor, 1.0);\n"
    "    }\n"
    "    FragColor = finalColor;\n"
    "}\n";

static const char* const RGFX_DEFAULT_TEXT_VERTEX_SHADER =
    "#version 300 es\n"
    "precision mediump float;\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "uniform mat4 uModel;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uProjection;\n"
    "out vec2 TexCoord;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = uProjection * uView * uModel * vec4(aPos, 0.0, 1.0);\n"
    "    TexCoord = aTexCoord;\n"
    "}\n";

static const char* const RGFX_DEFAULT_TEXT_FRAGMENT_SHADER =
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec2 TexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTexture;\n"
    "uniform vec3 uColor;\n"
    "void main()\n"
    "{\n"
    "    float textAlpha = texture(uTexture, TexCoord).r;\n"
    "    if (textAlpha < 0.01) {\n"
    "        discard;\n"
    "    }\n"
    "    FragColor = vec4(uColor, textAlpha);\n"
    "}\n";
#else
static const char* const RGFX_DEFAULT_SPRITE_VERTEX_SHADER =
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "out vec2 TexCoord;\n"
    "uniform mat4 uModel;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uProjection;\n"
    "uniform float uTime;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = uProjection * uView * uModel * vec4(aPos, 0.0, 1.0);\n"
    "    TexCoord = aTexCoord;\n"
    "}\n";

static const char* const RGFX_DEFAULT_SPRITE_FRAGMENT_SHADER =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;\n"
    "uniform sampler2D uTexture;\n"
    "uniform vec3 uColor;\n"
    "uniform bool uUseTexture;\n"
    "uniform float uTime;\n"
    "void main()\n"
    "{\n"
    "    vec4 finalColor;\n"
    "    if (uUseTexture) {\n"
    "        vec4 texColor = texture(uTexture, TexCoord);\n"
    "        if (texColor.a < 0.01) {\n"
    "            discard;\n"
    "        }\n"
    "        finalColor = vec4(texColor.rgb * uColor, texColor.a);\n"
    "    } else {\n"
    "        finalColor = vec4(uColor, 1.0);\n"
    "    }\n"
    "    FragColor = finalColor;\n"
    "}\n";

static const char* const RGFX_DEFAULT_TEXT_VERTEX_SHADER =
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "uniform mat4 uModel;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uProjection;\n"
    "out vec2 TexCoord;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = uProjection * uView * uModel * vec4(aPos, 0.0, 1.0);\n"
    "    TexCoord = aTexCoord;\n"
    "}\n";

static const char* const RGFX_DEFAULT_TEXT_FRAGMENT_SHADER =
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTexture;\n"
    "uniform vec3 uColor;\n"
    "void main()\n"
    "{\n"
    "    float textAlpha = texture(uTexture, TexCoord).r;\n"
    "    if (textAlpha < 0.01) {\n"
    "        discard;\n"
    "    }\n"
    "    FragColor = vec4(uColor, textAlpha);\n"
    "}\n";
#endif

const char* rgfx_internal_default_sprite_vertex_shader(void)
{
    return RGFX_DEFAULT_SPRITE_VERTEX_SHADER;
}

const char* rgfx_internal_default_sprite_fragment_shader(void)
{
    return RGFX_DEFAULT_SPRITE_FRAGMENT_SHADER;
}

const char* rgfx_internal_default_text_vertex_shader(void)
{
    return RGFX_DEFAULT_TEXT_VERTEX_SHADER;
}

const char* rgfx_internal_default_text_fragment_shader(void)
{
    return RGFX_DEFAULT_TEXT_FRAGMENT_SHADER;
}

static unsigned int rgfx_compile_shader(unsigned int type, const char* source)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        rlog_error("ERROR: Shader compilation failed\n%s\n", infoLog);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

unsigned int rgfx_create_shader_program(const char* vertexSource, const char* fragmentSource)
{
    if (!vertexSource || !fragmentSource)
    {
        return 0;
    }

    unsigned int vertexShader   = rgfx_compile_shader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fragmentShader = vertexShader ? rgfx_compile_shader(GL_FRAGMENT_SHADER, fragmentSource) : 0;

    if (!vertexShader || !fragmentShader)
    {
        if (vertexShader)
        {
            glDeleteShader(vertexShader);
        }
        if (fragmentShader)
        {
            glDeleteShader(fragmentShader);
        }
        return 0;
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success = 0;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        rlog_error("Shader program linking failed\n%s\n", infoLog);
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

char* rgfx_load_shader_source(const char* filepath)
{
    if (!filepath)
    {
        return NULL;
    }

    FILE* file = fopen(filepath, "rb");
    if (!file)
    {
        rlog_error("Failed to open shader file: %s\n", filepath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

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
        const char* version_directive = NULL;
#if defined(__EMSCRIPTEN__)
        rlog_info("Using WebGL/GLSL ES shader version for file: %s", filepath);
        version_directive = "#version 300 es\nprecision mediump float;\n";
#else
        rlog_info("Using Desktop/GLSL shader version for file: %s", filepath);
        version_directive = "#version 330 core\n";
#endif
        size_t directive_len = strlen(version_directive);
        char*  new_source    = (char*)malloc(directive_len + size + 1);
        if (!new_source)
        {
            free(source);
            rlog_error("Failed to allocate memory for shader source with version directive\n");
            return NULL;
        }

        strcpy(new_source, version_directive);
        strcat(new_source, source);
        free(source);
        source = new_source;
    }
    else
    {
        char        versionLine[64] = { 0 };
        const char* versionStart    = strstr(source, "#version");
        if (versionStart)
        {
            const char* lineEnd = strchr(versionStart, '\n');
            if (lineEnd)
            {
                size_t len = (size_t)(lineEnd - versionStart);
                if (len < sizeof(versionLine) - 1)
                {
                    strncpy(versionLine, versionStart, len);
                    versionLine[len] = '\0';
                    rlog_info("Found existing version directive in %s: %s", filepath, versionLine);
                }
            }
        }
    }

    return source;
}

bool rgfx_init(void)
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        return false;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    return true;
}

void rgfx_shutdown(void)
{
    if (g_text_shader_program)
    {
        glDeleteProgram(g_text_shader_program);
        g_text_shader_program  = 0;
        g_text_shader_refcount = 0;
    }

    g_active_camera = NULL;
}

void rgfx_clear(float r, float g, float b)
{
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void rgfx_clear_color(color color)
{
    glClearColor(color.r, color.g, color.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

unsigned int rgfx_load_texture(const char* filepath)
{
    if (!filepath)
    {
        return 0;
    }

    unsigned int textureID = 0;
    glGenTextures(1, &textureID);

    int            width   = 0;
    int            height  = 0;
    int            channels = 0;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(filepath, &width, &height, &channels, 0);
    if (!data)
    {
        rlog_error("Failed to load texture: %s\n", filepath);
        if (textureID)
        {
            glDeleteTextures(1, &textureID);
        }
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 4)
    {
        format = GL_RGBA;
    }
    else if (channels == 1)
    {
        format = GL_RED;
    }

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}

void rgfx_delete_texture(unsigned int textureID)
{
    if (textureID)
    {
        glDeleteTextures(1, &textureID);
    }
}

unsigned int rgfx_internal_acquire_text_shader_program(void)
{
    if (g_text_shader_program == 0)
    {
        g_text_shader_program = rgfx_create_shader_program(
            rgfx_internal_default_text_vertex_shader(),
            rgfx_internal_default_text_fragment_shader());
        if (g_text_shader_program == 0)
        {
            return 0;
        }
    }

    g_text_shader_refcount++;
    return g_text_shader_program;
}

void rgfx_internal_release_text_shader_program(void)
{
    if (g_text_shader_refcount > 0)
    {
        g_text_shader_refcount--;
        if (g_text_shader_refcount == 0 && g_text_shader_program != 0)
        {
            glDeleteProgram(g_text_shader_program);
            g_text_shader_program = 0;
        }
    }
}

rgfx_camera_t* rgfx_internal_get_active_camera(void)
{
    return g_active_camera;
}

void rgfx_internal_set_active_camera(rgfx_camera_t* camera)
{
    g_active_camera = camera;
}

rgfx_camera_t* rgfx_camera_create(const rgfx_camera_desc_t* desc)
{
    if (!desc)
    {
        return NULL;
    }

    rgfx_camera_t* camera = (rgfx_camera_t*)calloc(1, sizeof(rgfx_camera_t));
    if (!camera)
    {
        return NULL;
    }

    vec3_dup(camera->position, desc->position);

    vec3 forward;
    vec3_sub(forward, desc->target, desc->position);
    vec3_norm(camera->forward, forward);

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
        if (g_active_camera == camera)
        {
            g_active_camera = NULL;
        }
        free(camera);
    }
}

void rgfx_camera_set_position(rgfx_camera_t* camera, vec3 position)
{
    if (!camera)
    {
        return;
    }

    vec3_dup(camera->position, position);
}

void rgfx_camera_set_direction(rgfx_camera_t* camera, vec3 direction)
{
    if (!camera)
    {
        return;
    }

    vec3_norm(camera->forward, direction);
}

void rgfx_camera_look_at(rgfx_camera_t* camera, vec3 target)
{
    if (!camera)
    {
        return;
    }

    vec3 direction;
    vec3_sub(direction, target, camera->position);
    vec3_norm(camera->forward, direction);
}

void rgfx_camera_move(rgfx_camera_t* camera, vec3 offset)
{
    if (!camera)
    {
        return;
    }

    camera->position[0] += offset[0];
    camera->position[1] += offset[1];
    camera->position[2] += offset[2];
}

void rgfx_camera_rotate(rgfx_camera_t* camera, float yaw, float pitch)
{
    if (!camera)
    {
        return;
    }

    mat4x4 rotation;
    mat4x4_identity(rotation);

    mat4x4 rotateYaw;
    mat4x4_identity(rotateYaw);
    mat4x4_rotate_Y(rotateYaw, rotateYaw, yaw);

    mat4x4 rotatePitch;
    mat4x4_identity(rotatePitch);
    mat4x4_rotate_X(rotatePitch, rotatePitch, pitch);

    mat4x4_mul(rotation, rotateYaw, rotatePitch);

    vec4 forward4 = { camera->forward[0], camera->forward[1], camera->forward[2], 0.0f };
    vec4 transformed;
    mat4x4_mul_vec4(transformed, rotation, forward4);
    camera->forward[0] = transformed[0];
    camera->forward[1] = transformed[1];
    camera->forward[2] = transformed[2];
    vec3_norm(camera->forward, camera->forward);
}

void rgfx_camera_get_matrices(const rgfx_camera_t* camera, mat4x4 view, mat4x4 projection)
{
    if (!camera)
    {
        return;
    }

    vec3 target = { camera->position[0] + camera->forward[0],
                    camera->position[1] + camera->forward[1],
                    camera->position[2] + camera->forward[2] };

    mat4x4_look_at(view, camera->position, target, camera->up);
    mat4x4_perspective(projection, camera->fov, camera->aspect, camera->near, camera->far);
}

void rgfx_set_active_camera(rgfx_camera_t* camera)
{
    rgfx_internal_set_active_camera(camera);
}
