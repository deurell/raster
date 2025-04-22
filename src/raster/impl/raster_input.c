#include "raster/raster_input.h"
#include "raster/raster_app.h"
#include <GLFW/glfw3.h>
#include <string.h>

// Get the GLFW window handle (defined in raster_app.c)
extern GLFWwindow* _raster_app_get_window(void);

// Input state
typedef struct
{
    bool   keys[512];             // Current key state
    bool   prev_keys[512];        // Previous key state
    bool   mouse_buttons[8];      // Current mouse button state
    bool   prev_mouse_buttons[8]; // Previous mouse button state
    double mouse_x;
    double mouse_y;
} input_state_t;

static input_state_t input_state = {0};

// Internal function to update input state
void _raster_input_update(void)
{
    // Store previous key and mouse states
    memcpy(input_state.prev_keys, input_state.keys, sizeof(input_state.keys));
    memcpy(input_state.prev_mouse_buttons, input_state.mouse_buttons, sizeof(input_state.mouse_buttons));

    // Get mouse position
    GLFWwindow* window = _raster_app_get_window();
    if (window)
    {
        glfwGetCursorPos(window, &input_state.mouse_x, &input_state.mouse_y);
    }
}

// Keyboard input
bool raster_input_key_pressed(raster_key_t key)
{
    return input_state.keys[key] && !input_state.prev_keys[key];
}

bool raster_input_key_down(raster_key_t key)
{
    return input_state.keys[key];
}

bool raster_input_key_released(raster_key_t key)
{
    return !input_state.keys[key] && input_state.prev_keys[key];
}

// Mouse input
void raster_input_mouse_position(float* x, float* y)
{
    if (x)
        *x = (float)input_state.mouse_x;
    if (y)
        *y = (float)input_state.mouse_y;
}

bool raster_input_mouse_button_down(int button)
{
    if (button < 0 || button >= 8)
        return false;
    return input_state.mouse_buttons[button];
}

bool raster_input_mouse_button_pressed(int button)
{
    if (button < 0 || button >= 8)
        return false;
    return input_state.mouse_buttons[button] && !input_state.prev_mouse_buttons[button];
}

bool raster_input_mouse_button_released(int button)
{
    if (button < 0 || button >= 8)
        return false;
    return !input_state.mouse_buttons[button] && input_state.prev_mouse_buttons[button];
}

// GLFW callback functions
void _raster_input_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key >= 0 && key < 512)
    {
        input_state.keys[key] = (action != GLFW_RELEASE);
    }
}

void _raster_input_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button >= 0 && button < 8)
    {
        input_state.mouse_buttons[button] = (action != GLFW_RELEASE);
    }
}