#include "raster/raster_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static rlog_level_t current_log_level = RLOG_LEVEL_INFO;

// Function declaration
static void log_message(rlog_level_t level, const char* level_str, const char* fmt, va_list args);

// Set global log level
void rlog_set_level(rlog_level_t level)
{
    current_log_level = level;
}

// General logging function
void rlog(rlog_level_t level, const char* fmt, ...)
{
    const char* level_str = "";
    
    // Determine level string
    switch (level) {
        case RLOG_LEVEL_TRACE:   level_str = "TRACE"; break;
        case RLOG_LEVEL_DEBUG:   level_str = "DEBUG"; break;
        case RLOG_LEVEL_INFO:    level_str = "INFO"; break;
        case RLOG_LEVEL_WARNING: level_str = "WARNING"; break;
        case RLOG_LEVEL_ERROR:   level_str = "ERROR"; break;
        case RLOG_LEVEL_FATAL:   level_str = "FATAL"; break;
        default:                 level_str = "UNKNOWN"; break;
    }
    
    va_list args;
    va_start(args, fmt);
    log_message(level, level_str, fmt, args);
    va_end(args);
}

// Internal logging function
static void log_message(rlog_level_t level, const char* level_str, const char* fmt, va_list args)
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
void rlog_trace(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RLOG_LEVEL_TRACE, "TRACE", fmt, args);
    va_end(args);
}

void rlog_debug(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RLOG_LEVEL_DEBUG, "DEBUG", fmt, args);
    va_end(args);
}

void rlog_info(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RLOG_LEVEL_INFO, "INFO", fmt, args);
    va_end(args);
}

void rlog_warning(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RLOG_LEVEL_WARNING, "WARNING", fmt, args);
    va_end(args);
}

void rlog_error(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RLOG_LEVEL_ERROR, "ERROR", fmt, args);
    va_end(args);
}

void rlog_fatal(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RLOG_LEVEL_FATAL, "FATAL", fmt, args);
    va_end(args);
}