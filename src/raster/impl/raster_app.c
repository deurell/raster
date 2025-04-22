#include "raster/raster_app.h"
#include "raster/raster_gfx.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

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
    rapp_update_fn update_callback;
    rapp_draw_fn draw_callback;
    rapp_cleanup_fn cleanup_callback;
    
    // Quit flag
    bool should_quit;
} app_state = {0};

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
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Create window
    app_state.window = glfwCreateWindow(
        desc->window.width, 
        desc->window.height, 
        desc->window.title, 
        NULL, NULL
    );

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
    app_state.update_callback = desc->update_fn;
    app_state.draw_callback = desc->draw_fn;
    app_state.cleanup_callback = desc->cleanup_fn;
    
    // Initialize quit flag
    app_state.should_quit = false;

    return true;
}

void rapp_run(void)
{
    // Main game loop
    while (!glfwWindowShouldClose(app_state.window) && !app_state.should_quit)
    {
        // Update time values
        app_state.lastTime    = app_state.currentTime;
        app_state.currentTime = glfwGetTime();
        app_state.deltaTime   = app_state.currentTime - app_state.lastTime;

        // First update input state (copy current to previous)
        _rinput_update();
        
        // Then poll for new events (updates current state)
        glfwPollEvents();
        
        // Now call update callback
        if (app_state.update_callback)
        {
            app_state.update_callback(app_state.deltaTime);
        }
        
        // Call draw callback if registered
        if (app_state.draw_callback)
        {
            app_state.draw_callback();
        }
        
        // Swap buffers
        if (app_state.window)
        {
            glfwSwapBuffers(app_state.window);
        }
    }
    
    // Call cleanup callback if registered
    if (app_state.cleanup_callback)
    {
        app_state.cleanup_callback();
    }
    
    // Shutdown the app
    rapp_shutdown();
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
    glfwTerminate();
}

float rapp_get_time(void)
{
    return app_state.currentTime;
}

float rapp_get_delta_time(void)
{
    return app_state.deltaTime;
}