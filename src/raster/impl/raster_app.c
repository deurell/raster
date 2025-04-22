#include "raster/raster_app.h"
#include "raster/raster_gfx.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

// Forward declarations for input callbacks
void _raster_input_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void _raster_input_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void _raster_input_update(void);

// Global application state
static struct
{
    GLFWwindow* window;
    float       currentTime;
    float       lastTime;
    float       deltaTime;
} app_state = {0};

// Expose window handle to input system
GLFWwindow* _raster_app_get_window(void)
{
    return app_state.window;
}

bool raster_app_init(const raster_app_config_t* config)
{
    // Initialize GLFW
    if (!glfwInit())
    {
        printf("Failed to initialize GLFW\n");
        return false;
    }

    // Configure OpenGL context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Create window
    app_state.window = glfwCreateWindow(config->width, config->height, config->title, NULL, NULL);

    if (!app_state.window)
    {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    // Make OpenGL context current
    glfwMakeContextCurrent(app_state.window);

    // Initialize graphics subsystem
    if (!raster_gfx_init())
    {
        printf("Failed to initialize graphics system\n");
        glfwDestroyWindow(app_state.window);
        glfwTerminate();
        return false;
    }

    // Set input callbacks
    glfwSetKeyCallback(app_state.window, _raster_input_key_callback);
    glfwSetMouseButtonCallback(app_state.window, _raster_input_mouse_button_callback);

    // Initialize time
    app_state.currentTime = glfwGetTime();
    app_state.lastTime    = app_state.currentTime;
    app_state.deltaTime   = 0.0f;

    return true;
}

void raster_app_shutdown(void)
{
    // Shutdown graphics subsystem
    raster_gfx_shutdown();

    // Destroy window and terminate GLFW
    if (app_state.window)
    {
        glfwDestroyWindow(app_state.window);
        app_state.window = NULL;
    }
    glfwTerminate();
}

bool raster_app_should_close(void)
{
    return app_state.window ? glfwWindowShouldClose(app_state.window) : true;
}

void raster_app_poll_events(void)
{
    // Update time values
    app_state.lastTime    = app_state.currentTime;
    app_state.currentTime = glfwGetTime();
    app_state.deltaTime   = app_state.currentTime - app_state.lastTime;

    // Poll for events
    glfwPollEvents();

    // Update input state
    _raster_input_update();
}

void raster_app_present(void)
{
    if (app_state.window)
    {
        glfwSwapBuffers(app_state.window);
    }
}

float raster_app_get_time(void)
{
    return app_state.currentTime;
}

float raster_app_get_delta_time(void)
{
    return app_state.deltaTime;
}