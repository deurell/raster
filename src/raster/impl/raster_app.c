#include "raster/raster_app.h"
#include "raster/raster_gfx.h"
#include "raster/raster_log.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// Forward declarations for input callbacks
void _rinput_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void _rinput_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void _rinput_char_callback(GLFWwindow* window, unsigned int codepoint);
void _rinput_update(void);

// Forward declaration for framebuffer size callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

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

    // Camera
    rgfx_camera_t* main_camera;

    // Quit flag
    bool should_quit;
 } engine_state = { 0 };

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
    return engine_state.window;
}

bool rapp_init(const rapp_desc_t* desc)
{
    // Initialize GLFW
    if (!glfwInit())
    {
        rlog_error("Failed to initialize GLFW\n");
        return false;
    }

    // Configure OpenGL context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

#ifdef __EMSCRIPTEN__
    // For WebGL, we need to use the ES profile
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);fds
#else
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    engine_state.window = glfwCreateWindow(desc->window.width, desc->window.height, desc->window.title, NULL, NULL);

    if (!engine_state.window)
    {
        rlog_fatal("Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    // Make OpenGL context current
    glfwMakeContextCurrent(engine_state.window);

    // Initialize graphics subsystem
    if (!rgfx_init())
    {
        rlog_fatal("Failed to initialize graphics system\n");
        glfwDestroyWindow(engine_state.window);
        glfwTerminate();
        return false;
    }

    // Create main camera
    engine_state.main_camera = rgfx_camera_create(&desc->camera);
    if (!engine_state.main_camera)
    {
        rlog_fatal("Failed to create main camera\n");
        rgfx_shutdown();
        glfwTerminate();
        return false;
    }
    rgfx_set_active_camera(engine_state.main_camera);

    // Set input callbacks
    glfwSetKeyCallback(engine_state.window, _rinput_key_callback);
    glfwSetMouseButtonCallback(engine_state.window, _rinput_mouse_button_callback);
    glfwSetCharCallback(engine_state.window, _rinput_char_callback); // Register char callback

    // Register framebuffer size callback
    glfwSetFramebufferSizeCallback(engine_state.window, framebuffer_size_callback);

    // Manually set the viewport initially
    int fb_width, fb_height;
    glfwGetFramebufferSize(engine_state.window, &fb_width, &fb_height);
    glViewport(0, 0, fb_width, fb_height);

    // Initialize time
    engine_state.currentTime = glfwGetTime();
    engine_state.lastTime    = engine_state.currentTime;
    engine_state.deltaTime   = 0.0f;

    // Store callbacks
    engine_state.update_callback  = desc->update_fn;
    engine_state.draw_callback    = desc->draw_fn;
    engine_state.cleanup_callback = desc->cleanup_fn;

    // Initialize quit flag
    engine_state.should_quit = false;

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
    while (!glfwWindowShouldClose(engine_state.window) && !engine_state.should_quit)
    {
        // Update time values
        engine_state.lastTime    = engine_state.currentTime;
        engine_state.currentTime = glfwGetTime();
        engine_state.deltaTime   = engine_state.currentTime - engine_state.lastTime;

        _rinput_update();
        glfwPollEvents();

        if (engine_state.update_callback)
        {
            engine_state.update_callback(engine_state.deltaTime);
        }

        if (engine_state.draw_callback)
        {
            engine_state.draw_callback();
        }

        if (engine_state.window)
        {
            glfwSwapBuffers(engine_state.window);
        }
    }

    if (engine_state.cleanup_callback)
    {
        engine_state.cleanup_callback();
    }

    rapp_shutdown();
#endif
}

void rapp_quit(void)
{
    if (engine_state.window)
    {
        glfwSetWindowShouldClose(engine_state.window, GLFW_TRUE);
    }
    engine_state.should_quit = true;
}

void rapp_shutdown(void)
{
    // Cleanup camera
    if (engine_state.main_camera)
    {
        rgfx_camera_destroy(engine_state.main_camera);
        engine_state.main_camera = NULL;
    }

    // Shutdown graphics subsystem
    rgfx_shutdown();

    // Destroy window and terminate GLFW
    if (engine_state.window)
    {
        glfwDestroyWindow(engine_state.window);
        engine_state.window = NULL;
    }
#ifndef __EMSCRIPTEN__
    glfwTerminate();
#endif
}

float rapp_get_time(void)
{
    return engine_state.currentTime;
}

float rapp_get_delta_time(void)
{
    return engine_state.deltaTime;
}

rgfx_camera_t* rapp_get_main_camera(void)
{
    return engine_state.main_camera;
}

void rapp_get_window_size(int* width, int* height)
{
    if (engine_state.window)
    {
        glfwGetWindowSize(engine_state.window, width, height);
    }
    else if (width && height)
    {
        *width  = 0;
        *height = 0;
    }
}

// Framebuffer size callback function
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    rlog_info("Framebuffer resized: %dx%d\n", width, height);
}