#include "raster/raster_input.h"
#include "raster/raster_math.h"
#include <GLFW/glfw3.h>
#include <string.h>
#include <stdio.h>

// Get the GLFW window handle (defined in raster_app.c)
extern GLFWwindow* _rapp_get_window(void);

// Input state
typedef struct
{
    bool   keys[512];             // Current key state
    bool   prev_keys[512];        // Previous key state
    bool   mouse_buttons[8];      // Current mouse button state
    bool   prev_mouse_buttons[8]; // Previous mouse button state
    double mouse_x;
    double mouse_y;
    // Char input buffer
    unsigned int char_buffer[32]; // Simple ring buffer for codepoints
    int char_buffer_head;
    int char_buffer_tail;
} input_state_t;

static input_state_t input_state = { 0 };

// Internal function to update input state
void _rinput_update(void)
{
    // Store previous key and mouse states
    memcpy(input_state.prev_keys, input_state.keys, sizeof(input_state.keys));
    memcpy(input_state.prev_mouse_buttons, input_state.mouse_buttons, sizeof(input_state.mouse_buttons));

    // Get mouse position
    GLFWwindow* window = _rapp_get_window();
    if (window)
    {
        glfwGetCursorPos(window, &input_state.mouse_x, &input_state.mouse_y);
    }
}

// Keyboard input functions - Sokol-style naming
bool rinput_key_pressed(rinput_key_t key)
{
    return input_state.keys[key] && !input_state.prev_keys[key];
}

bool rinput_key_down(rinput_key_t key)
{
    return input_state.keys[key];
}

bool rinput_key_released(rinput_key_t key)
{
    return !input_state.keys[key] && input_state.prev_keys[key];
}

// Mouse position function with output parameter - simplified direct assignment
void rinput_mouse_position(vec2 out_position)
{
    if (out_position)
    {
        out_position[0] = (float)input_state.mouse_x;
        out_position[1] = (float)input_state.mouse_y;
    }
}

// Mouse button functions
bool rinput_mouse_button_down(rinput_mouse_button_t button)
{
    if ((int)button < 0 || (int)button >= 8)
        return false;
    return input_state.mouse_buttons[(int)button];
}

bool rinput_mouse_button_pressed(rinput_mouse_button_t button)
{
    if ((int)button < 0 || (int)button >= 8)
        return false;
    return input_state.mouse_buttons[(int)button] && !input_state.prev_mouse_buttons[(int)button];
}

bool rinput_mouse_button_released(rinput_mouse_button_t button)
{
    if ((int)button < 0 || (int)button >= 8)
        return false;
    return !input_state.mouse_buttons[(int)button] && input_state.prev_mouse_buttons[(int)button];
}

void rinput_debug_print_pressed_keys(void)
{
    printf("Currently pressed keys: ");
    bool any_key_pressed = false;

    // Loop through all possible keys
    for (int i = 0; i < 512; i++)
    {
        if (input_state.keys[i])
        {
            printf("%d ", i);
            any_key_pressed = true;
        }
    }

    if (!any_key_pressed)
    {
        printf("None");
    }

    printf("\n");
}

// Retrieve and clear the char input buffer
int rinput_get_chars(unsigned int* out_buffer, int max_chars)
{
    int count = 0;
    while (input_state.char_buffer_tail != input_state.char_buffer_head && count < max_chars)
    {
        out_buffer[count++] = input_state.char_buffer[input_state.char_buffer_tail];
        input_state.char_buffer_tail = (input_state.char_buffer_tail + 1) % 32;
    }
    return count;
}

// GLFW callback functions
void _rinput_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key >= 0 && key < 512)
    {
        input_state.keys[key] = (action != GLFW_RELEASE);
    }
}

void _rinput_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button >= 0 && button < 8)
    {
        input_state.mouse_buttons[button] = (action != GLFW_RELEASE);
    }
}

void _rinput_char_callback(GLFWwindow* window, unsigned int codepoint)
{
    int next_head = (input_state.char_buffer_head + 1) % 32;
    if (next_head != input_state.char_buffer_tail) // Only add if buffer not full
    {
        input_state.char_buffer[input_state.char_buffer_head] = codepoint;
        input_state.char_buffer_head = next_head;
    }
}