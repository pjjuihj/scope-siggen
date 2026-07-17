/**
 * @brief Stub main.h for host-side unit testing
 *        Provides minimal types without STM32 HAL dependency
 */
#ifndef __MAIN_H
#define __MAIN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Stub HAL types used by the project */
typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;

#endif /* __MAIN_H */
