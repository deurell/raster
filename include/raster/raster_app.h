#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

    // Window config
    typedef struct
    {
        const char* title;
        int         width;
        int         height;
    } raster_app_config_t;

    // App lifecycle
    bool raster_app_init(const raster_app_config_t* config);
    void raster_app_shutdown(void);
    bool raster_app_should_close(void);
    void raster_app_poll_events(void);
    void raster_app_present(void);

    // Time functions
    float raster_app_get_time(void);
    float raster_app_get_delta_time(void);

#ifdef __cplusplus
}
#endif