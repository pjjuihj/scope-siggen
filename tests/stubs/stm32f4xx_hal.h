/**
 * @brief Stub stm32f4xx_hal.h for host-side unit testing
 *        Provides minimal HAL types without real HAL dependency
 */
#ifndef __STM32F4XX_HAL_H
#define __STM32F4XX_HAL_H

#include <stdint.h>
#include <stdbool.h>

/* Minimal HAL types */
typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;

#endif /* __STM32F4XX_HAL_H */
