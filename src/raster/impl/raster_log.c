#include "raster/raster_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// Current global log level
static rlog_level_t current_log_level = RLOG_LEVEL_INFO;

// Static mapping tables
static const char* level_strings[] = { "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "UNKNOWN" };

// Common formatting constants
#define RLOG_TIME_BUFFER_SIZE 16
#define RLOG_HEADER_BUFFER_SIZE 64
#define RLOG_MESSAGE_BUFFER_SIZE 1024
#define RLOG_FULL_BUFFER_SIZE (RLOG_HEADER_BUFFER_SIZE + RLOG_MESSAGE_BUFFER_SIZE)

// Set global log level
void rlog_set_level(rlog_level_t level)
{
    current_log_level = level;
}

// Get level string safely
static const char* get_level_string(rlog_level_t level)
{
    if (level >= RLOG_LEVEL_TRACE && level <= RLOG_LEVEL_FATAL)
    {
        return level_strings[level];
    }
    return level_strings[6]; // "UNKNOWN"
}

// Format timestamp
static void format_timestamp(char* buffer, size_t buffer_size)
{
    time_t     rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, buffer_size, "%H:%M:%S", timeinfo);
}

// Format log message
static void format_log_message(char*       header_buffer,
                               size_t      header_size,
                               char*       message_buffer,
                               size_t      message_size,
                               char*       full_buffer,
                               size_t      full_size,
                               const char* level_str,
                               const char* fmt,
                               va_list     args)
{
    char time_buffer[RLOG_TIME_BUFFER_SIZE];

    // Format timestamp
    format_timestamp(time_buffer, sizeof(time_buffer));

    // Format header
    snprintf(header_buffer, header_size, "[%s] [%s] ", time_buffer, level_str);

    // Format message with arguments
    vsnprintf(message_buffer, message_size, fmt, args);

    // Combine into full message
    snprintf(full_buffer, full_size, "%s%s", header_buffer, message_buffer);
}

// Platform-specific log output functions
#ifdef __EMSCRIPTEN__
static void platform_log(rlog_level_t level, const char* full_message)
{
    // Map our log levels to Emscripten log levels
    int em_level;
    switch (level)
    {
    case RLOG_LEVEL_TRACE:
    case RLOG_LEVEL_DEBUG:
        em_level = EM_LOG_DEBUG;
        break;
    case RLOG_LEVEL_INFO:
        em_level = EM_LOG_INFO;
        break;
    case RLOG_LEVEL_WARNING:
        em_level = EM_LOG_WARN;
        break;
    case RLOG_LEVEL_ERROR:
    case RLOG_LEVEL_FATAL:
        em_level = EM_LOG_ERROR;
        break;
    default:
        em_level = EM_LOG_INFO;
        break;
    }

    // Send to Emscripten's console
    emscripten_log(em_level, "%s", full_message);
}
#else
static void platform_log(rlog_level_t level, const char* full_message)
{
    // Send to stderr with newline
    fprintf(stderr, "%s\n", full_message);
}
#endif

// Core logging function
static void log_message(rlog_level_t level, const char* fmt, va_list args)
{
    // Early return if below current log level
    if (level < current_log_level)
    {
        return;
    }

    const char* level_str = get_level_string(level);

    // Prepare buffers
    char header_buffer[RLOG_HEADER_BUFFER_SIZE];
    char message_buffer[RLOG_MESSAGE_BUFFER_SIZE];
    char full_message[RLOG_FULL_BUFFER_SIZE];

    // Format the message
    format_log_message(header_buffer,
                       sizeof(header_buffer),
                       message_buffer,
                       sizeof(message_buffer),
                       full_message,
                       sizeof(full_message),
                       level_str,
                       fmt,
                       args);

    // Output using platform-specific implementation
    platform_log(level, full_message);
}

// Public API functions
void rlog(rlog_level_t level, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(level, fmt, args);
    va_end(args);
}

void rlog_trace(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RLOG_LEVEL_TRACE, fmt, args);
    va_end(args);
}

void rlog_debug(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RLOG_LEVEL_DEBUG, fmt, args);
    va_end(args);
}

void rlog_info(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RLOG_LEVEL_INFO, fmt, args);
    va_end(args);
}

void rlog_warning(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RLOG_LEVEL_WARNING, fmt, args);
    va_end(args);
}

void rlog_error(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RLOG_LEVEL_ERROR, fmt, args);
    va_end(args);
}

void rlog_fatal(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(RLOG_LEVEL_FATAL, fmt, args);
    va_end(args);
}