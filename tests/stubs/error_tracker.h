/**
 * @brief Stub error_tracker.h for host-side unit testing
 *        Provides ErrorCode_t enum without HAL dependency
 */
#ifndef __ERROR_TRACKER_H
#define __ERROR_TRACKER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ERR_OK = 0,
    ERR_TIMEOUT,
    ERR_INVALID_PARAM,
    ERR_BUFFER_FULL,
    ERR_BUFFER_EMPTY,
    ERR_HARDWARE,
    ERR_MEMORY,
    ERR_BUSY,
    ERR_NOT_INIT,
    ERR_NOT_SUPPORTED,
    ERR_FILE_OPEN,
    ERR_FILE_READ,
    ERR_FILE_WRITE,
    ERR_CANCELLED,
    ERR_UNKNOWN
} ErrorCode_t;

typedef enum {
    ERROR_SEVERITY_INFO = 0,
    ERROR_SEVERITY_WARNING,
    ERROR_SEVERITY_ERROR,
    ERROR_SEVERITY_FATAL
} ErrorSeverity_t;

typedef struct {
    ErrorCode_t code;
    ErrorSeverity_t severity;
    const char *file;
    uint32_t line;
    uint32_t timestamp;
    char message[64];
} ErrorRecord_t;

#define ERROR_MAX_RECORDS 16

/* Stub functions */
static inline void ErrorTracker_Record(ErrorCode_t code, ErrorSeverity_t severity,
                                       const char *file, uint32_t line, const char *message) {
    (void)code; (void)severity; (void)file; (void)line; (void)message;
}

#endif /* __ERROR_TRACKER_H */
