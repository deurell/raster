#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include "raster_math.h"  // Include our math library

    // Callback function typedefs
    typedef void (*rapp_update_fn)(float dt);
    typedef void (*rapp_draw_fn)(void);
    typedef void (*rapp_cleanup_fn)(void);

    // Window config
    typedef struct
    {
        const char* title;
        int         width;
        int         height;
    } rapp_window_desc_t;

    // App descriptor
    typedef struct
    {
        rapp_window_desc_t window;
        rapp_update_fn update_fn;
        rapp_draw_fn draw_fn;
        rapp_cleanup_fn cleanup_fn;
    } rapp_desc_t;

    // App lifecycle
    bool rapp_init(const rapp_desc_t* desc);
    void rapp_run(void);
    void rapp_quit(void);
    void rapp_shutdown(void);

    // Time functions
    float rapp_get_time(void);
    float rapp_get_delta_time(void);

#ifdef __cplusplus
}
#endif