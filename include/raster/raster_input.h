#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

    // Key codes (matching GLFW for simplicity)
    typedef enum
    {
        RASTER_KEY_UNKNOWN    = -1,
        RASTER_KEY_SPACE      = 32,
        RASTER_KEY_APOSTROPHE = 39,
        RASTER_KEY_COMMA      = 44,
        RASTER_KEY_MINUS      = 45,
        RASTER_KEY_PERIOD     = 46,
        RASTER_KEY_SLASH      = 47,
        RASTER_KEY_0          = 48,
        RASTER_KEY_1          = 49,
        RASTER_KEY_2          = 50,
        RASTER_KEY_3          = 51,
        RASTER_KEY_4          = 52,
        RASTER_KEY_5          = 53,
        RASTER_KEY_6          = 54,
        RASTER_KEY_7          = 55,
        RASTER_KEY_8          = 56,
        RASTER_KEY_9          = 57,
        RASTER_KEY_SEMICOLON  = 59,
        RASTER_KEY_EQUAL      = 61,
        RASTER_KEY_A          = 65,
        RASTER_KEY_B          = 66,
        RASTER_KEY_C          = 67,
        RASTER_KEY_D          = 68,
        RASTER_KEY_E          = 69,
        RASTER_KEY_F          = 70,
        RASTER_KEY_G          = 71,
        RASTER_KEY_H          = 72,
        RASTER_KEY_I          = 73,
        RASTER_KEY_J          = 74,
        RASTER_KEY_K          = 75,
        RASTER_KEY_L          = 76,
        RASTER_KEY_M          = 77,
        RASTER_KEY_N          = 78,
        RASTER_KEY_O          = 79,
        RASTER_KEY_P          = 80,
        RASTER_KEY_Q          = 81,
        RASTER_KEY_R          = 82,
        RASTER_KEY_S          = 83,
        RASTER_KEY_T          = 84,
        RASTER_KEY_U          = 85,
        RASTER_KEY_V          = 86,
        RASTER_KEY_W          = 87,
        RASTER_KEY_X          = 88,
        RASTER_KEY_Y          = 89,
        RASTER_KEY_Z          = 90,
        RASTER_KEY_ESCAPE     = 256,
        RASTER_KEY_ENTER      = 257,
        RASTER_KEY_TAB        = 258,
        RASTER_KEY_BACKSPACE  = 259,
        RASTER_KEY_RIGHT      = 262,
        RASTER_KEY_LEFT       = 263,
        RASTER_KEY_DOWN       = 264,
        RASTER_KEY_UP         = 265,
    } raster_key_t;

    // Keyboard input
    bool raster_input_key_pressed(raster_key_t key);
    bool raster_input_key_down(raster_key_t key);
    bool raster_input_key_released(raster_key_t key);

    // Mouse input
    void raster_input_mouse_position(float* x, float* y);
    bool raster_input_mouse_button_down(int button);
    bool raster_input_mouse_button_pressed(int button);
    bool raster_input_mouse_button_released(int button);

#ifdef __cplusplus
}
#endif