#include "raster/raster_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static raster_log_level_t current_log_level = RASTER_LOG_LEVEL_INFO;

// Set global log level
void raster_log_set_level(raster_log_level_t level)
{
    current_log_level = level;
}

// Internal logging function
static void log_message(raster_log_level_t level, const char* level_str, const char* fmt, va_list args)
{
    if (level < current_log_level)
    {
        return;
    }

    // Get current time
    time_t     rawtime;
    struct tm* timeinfo;
    char       time_buffer[9]; // HH:MM:SS\0

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", timeinfo);

    // Print log header
    fprintf(stderr, "[%s] [%s] ", time_buffer, level_str);

    // Print the actual message
    vfprintf(stderr, fmt, args);

    // End with newline
    fprintf(stderr, "\n");
}

// Log functions
void raster_log_trace(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RASTER_LOG_LEVEL_TRACE, "TRACE", fmt, args);
    va_end(args);
}

void raster_log_debug(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RASTER_LOG_LEVEL_DEBUG, "DEBUG", fmt, args);
    va_end(args);
}

void raster_log_info(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RASTER_LOG_LEVEL_INFO, "INFO", fmt, args);
    va_end(args);
}

void raster_log_warn(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RASTER_LOG_LEVEL_WARN, "WARN", fmt, args);
    va_end(args);
}

void raster_log_error(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RASTER_LOG_LEVEL_ERROR, "ERROR", fmt, args);
    va_end(args);
}

void raster_log_fatal(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RASTER_LOG_LEVEL_FATAL, "FATAL", fmt, args);
    va_end(args);
}