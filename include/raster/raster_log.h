#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdarg.h> // Include for va_list, va_start, va_end

    // Log levels
    typedef enum {
        RLOG_LEVEL_TRACE,
        RLOG_LEVEL_DEBUG,
        RLOG_LEVEL_INFO,
        RLOG_LEVEL_WARNING,
        RLOG_LEVEL_ERROR,
        RLOG_LEVEL_FATAL
    } rlog_level_t;

    // Set global log level
    void rlog_set_level(rlog_level_t level);

    // Log functions
    void rlog(rlog_level_t level, const char* fmt, ...);
    void rlog_trace(const char* fmt, ...);
    void rlog_debug(const char* fmt, ...);
    void rlog_info(const char* fmt, ...);
    void rlog_warning(const char* fmt, ...);
    void rlog_error(const char* fmt, ...);
    void rlog_fatal(const char* fmt, ...);

#ifdef __cplusplus
}
#endif