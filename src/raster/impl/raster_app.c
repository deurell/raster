#include "raster/raster_app.h"
#include "raster/raster_gfx.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// Forward declarations for input callbacks
void _rinput_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void _rinput_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void _rinput_update(void);

// Global application state
static struct
{
    GLFWwindow* window;
    float       currentTime;
    float       lastTime;
    float       deltaTime;

    // Callbacks
    rapp_update_fn  update_callback;
    rapp_draw_fn    draw_callback;
    rapp_cleanup_fn cleanup_callback;

    // Quit flag
    bool should_quit;
} app_state = { 0 };

// Emscripten main loop function
#ifdef __EMSCRIPTEN__
void emscripten_main_loop(void)
{
    // Update time values
    app_state.lastTime    = app_state.currentTime;
    app_state.currentTime = glfwGetTime();
    app_state.deltaTime   = app_state.currentTime - app_state.lastTime;

    _rinput_update();
    glfwPollEvents();

    if (app_state.update_callback)
    {
        app_state.update_callback(app_state.deltaTime);
    }

    if (app_state.draw_callback)
    {
        app_state.draw_callback();
    }

    if (app_state.window)
    {
        glfwSwapBuffers(app_state.window);
    }

    // Check if we should quit
    if (app_state.should_quit || glfwWindowShouldClose(app_state.window))
    {
        emscripten_cancel_main_loop();

        if (app_state.cleanup_callback)
        {
            app_state.cleanup_callback();
        }

        rgfx_shutdown();
    }
}
#endif

// Expose window handle to input system
GLFWwindow* _rapp_get_window(void)
{
    return app_state.window;
}

bool rapp_init(const rapp_desc_t* desc)
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

#ifdef __EMSCRIPTEN__
    // For WebGL, we need to use the ES profile
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#else
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    app_state.window = glfwCreateWindow(desc->window.width, desc->window.height, desc->window.title, NULL, NULL);

    if (!app_state.window)
    {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    // Make OpenGL context current
    glfwMakeContextCurrent(app_state.window);

    // Initialize graphics subsystem
    if (!rgfx_init())
    {
        printf("Failed to initialize graphics system\n");
        glfwDestroyWindow(app_state.window);
        glfwTerminate();
        return false;
    }

    // Set input callbacks
    glfwSetKeyCallback(app_state.window, _rinput_key_callback);
    glfwSetMouseButtonCallback(app_state.window, _rinput_mouse_button_callback);

    // Initialize time
    app_state.currentTime = glfwGetTime();
    app_state.lastTime    = app_state.currentTime;
    app_state.deltaTime   = 0.0f;

    // Store callbacks
    app_state.update_callback  = desc->update_fn;
    app_state.draw_callback    = desc->draw_fn;
    app_state.cleanup_callback = desc->cleanup_fn;

    // Initialize quit flag
    app_state.should_quit = false;

    return true;
}

void rapp_run(void)
{
#ifdef __EMSCRIPTEN__
    // Emscripten-specific main loop
    emscripten_set_main_loop(emscripten_main_loop, 0, 1);
    // Note: cleanup is handled in the main loop function when the loop is cancelled
#else
    // Standard main loop for desktop platforms
    while (!glfwWindowShouldClose(app_state.window) && !app_state.should_quit)
    {
        // Update time values
        app_state.lastTime    = app_state.currentTime;
        app_state.currentTime = glfwGetTime();
        app_state.deltaTime   = app_state.currentTime - app_state.lastTime;

        _rinput_update();
        glfwPollEvents();

        if (app_state.update_callback)
        {
            app_state.update_callback(app_state.deltaTime);
        }

        if (app_state.draw_callback)
        {
            app_state.draw_callback();
        }

        if (app_state.window)
        {
            glfwSwapBuffers(app_state.window);
        }
    }

    if (app_state.cleanup_callback)
    {
        app_state.cleanup_callback();
    }

    rapp_shutdown();
#endif
}

void rapp_quit(void)
{
    if (app_state.window)
    {
        glfwSetWindowShouldClose(app_state.window, GLFW_TRUE);
    }
    app_state.should_quit = true;
}

void rapp_shutdown(void)
{
    // Shutdown graphics subsystem
    rgfx_shutdown();

    // Destroy window and terminate GLFW
    if (app_state.window)
    {
        glfwDestroyWindow(app_state.window);
        app_state.window = NULL;
    }
#ifndef __EMSCRIPTEN__
    glfwTerminate();
#endif
}

float rapp_get_time(void)
{
    return app_state.currentTime;
}

float rapp_get_delta_time(void)
{
    return app_state.deltaTime;
}