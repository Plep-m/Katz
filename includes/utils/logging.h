#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <stdarg.h>

typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;

void log_set_level(LogLevel level);
void log_debug(const char *format, ...);
void log_info(const char *format, ...);
void log_warn(const char *format, ...);
void log_error(const char *format, ...);

#endif // LOGGING_H