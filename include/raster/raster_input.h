#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include "raster_math.h" // Include our math library

    // Key codes (matching GLFW for simplicity)
    typedef enum
    {
        RINPUT_KEY_UNKNOWN    = -1,
        RINPUT_KEY_SPACE      = 32,
        RINPUT_KEY_APOSTROPHE = 39,
        RINPUT_KEY_COMMA      = 44,
        RINPUT_KEY_MINUS      = 45,
        RINPUT_KEY_PERIOD     = 46,
        RINPUT_KEY_SLASH      = 47,
        RINPUT_KEY_0          = 48,
        RINPUT_KEY_1          = 49,
        RINPUT_KEY_2          = 50,
        RINPUT_KEY_3          = 51,
        RINPUT_KEY_4          = 52,
        RINPUT_KEY_5          = 53,
        RINPUT_KEY_6          = 54,
        RINPUT_KEY_7          = 55,
        RINPUT_KEY_8          = 56,
        RINPUT_KEY_9          = 57,
        RINPUT_KEY_SEMICOLON  = 59,
        RINPUT_KEY_EQUAL      = 61,
        RINPUT_KEY_A          = 65,
        RINPUT_KEY_B          = 66,
        RINPUT_KEY_C          = 67,
        RINPUT_KEY_D          = 68,
        RINPUT_KEY_E          = 69,
        RINPUT_KEY_F          = 70,
        RINPUT_KEY_G          = 71,
        RINPUT_KEY_H          = 72,
        RINPUT_KEY_I          = 73,
        RINPUT_KEY_J          = 74,
        RINPUT_KEY_K          = 75,
        RINPUT_KEY_L          = 76,
        RINPUT_KEY_M          = 77,
        RINPUT_KEY_N          = 78,
        RINPUT_KEY_O          = 79,
        RINPUT_KEY_P          = 80,
        RINPUT_KEY_Q          = 81,
        RINPUT_KEY_R          = 82,
        RINPUT_KEY_S          = 83,
        RINPUT_KEY_T          = 84,
        RINPUT_KEY_U          = 85,
        RINPUT_KEY_V          = 86,
        RINPUT_KEY_W          = 87,
        RINPUT_KEY_X          = 88,
        RINPUT_KEY_Y          = 89,
        RINPUT_KEY_Z          = 90,
        RINPUT_KEY_ESCAPE     = 256,
        RINPUT_KEY_ENTER      = 257,
        RINPUT_KEY_TAB        = 258,
        RINPUT_KEY_BACKSPACE  = 259,
        RINPUT_KEY_RIGHT      = 262,
        RINPUT_KEY_LEFT       = 263,
        RINPUT_KEY_DOWN       = 264,
        RINPUT_KEY_UP         = 265,
    } rinput_key_t;

    // Mouse buttons
    typedef enum
    {
        RINPUT_MOUSE_BUTTON_LEFT   = 0,
        RINPUT_MOUSE_BUTTON_RIGHT  = 1,
        RINPUT_MOUSE_BUTTON_MIDDLE = 2
    } rinput_mouse_button_t;

    // Keyboard input
    bool rinput_key_pressed(rinput_key_t key);
    bool rinput_key_down(rinput_key_t key);
    bool rinput_key_released(rinput_key_t key);

    // Mouse input
    void rinput_mouse_position(vec2 out_position);
    bool rinput_mouse_button_down(rinput_mouse_button_t button);
    bool rinput_mouse_button_pressed(rinput_mouse_button_t button);
    bool rinput_mouse_button_released(rinput_mouse_button_t button);
    void rinput_debug_print_pressed_keys(void);

#ifdef __cplusplus
}
#endif