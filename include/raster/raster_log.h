#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        RASTER_LOG_LEVEL_TRACE,
        RASTER_LOG_LEVEL_DEBUG,
        RASTER_LOG_LEVEL_INFO,
        RASTER_LOG_LEVEL_WARN,
        RASTER_LOG_LEVEL_ERROR,
        RASTER_LOG_LEVEL_FATAL
    } raster_log_level_t;

    // Set global log level
    void raster_log_set_level(raster_log_level_t level);

    // Core logging functions
    void raster_log_trace(const char* fmt, ...);
    void raster_log_debug(const char* fmt, ...);
    void raster_log_info(const char* fmt, ...);
    void raster_log_warn(const char* fmt, ...);
    void raster_log_error(const char* fmt, ...);
    void raster_log_fatal(const char* fmt, ...);

#ifdef __cplusplus
}
#endif