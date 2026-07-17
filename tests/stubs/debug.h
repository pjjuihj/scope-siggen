/**
 * @brief Stub debug.h for host-side unit testing
 */
#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdint.h>
#include <stdarg.h>

/* Stub log levels */
typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG
} LogLevel_t;

/* Stub functions */
static inline void Debug_Log(LogLevel_t level, const char *file, uint32_t line, ...) {
    (void)level; (void)file; (void)line;
}

static inline void Debug_SetLogLevel(LogLevel_t level) { (void)level; }
static inline LogLevel_t Debug_GetLogLevel(void) { return LOG_LEVEL_NONE; }

/* Stub LOG macros */
#define LOG_DEBUG(fmt, ...)   ((void)0)
#define LOG_INFO(fmt, ...)    ((void)0)
#define LOG_WARNING(fmt, ...) ((void)0)
#define LOG_ERROR(fmt, ...)   ((void)0)
#define LOG_FATAL(fmt, ...)   ((void)0)

#endif /* __DEBUG_H */
