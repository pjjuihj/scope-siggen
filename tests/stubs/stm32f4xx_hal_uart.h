/**
 * @brief Stub stm32f4xx_hal_uart.h for host-side unit testing
 */
#ifndef __STM32F4XX_HAL_UART_H
#define __STM32F4XX_HAL_UART_H

#include <stdint.h>
#include <stdbool.h>

/* HAL types */
typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;

/* UART handle structure (simplified) */
typedef struct {
    uint32_t Instance;
    uint32_t RxState;
    uint32_t gState;
} UART_HandleTypeDef;

/* UART states */
#define HAL_UART_STATE_READY  0x00
#define HAL_UART_STATE_BUSY   0x01

/* USART1 instance */
#define USART1  0x40011000

/* Mock transmit buffer */
#define MOCK_TX_BUF_SIZE 1024
static uint8_t mock_tx_buf[MOCK_TX_BUF_SIZE];
static uint16_t mock_tx_len = 0;

/* Stub HAL functions */
static inline HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart,
                                                   uint8_t *pData,
                                                   uint16_t Size,
                                                   uint32_t Timeout)
{
    (void)huart;
    (void)Timeout;
    /* Store transmitted data for verification */
    uint16_t copy_len = (Size < MOCK_TX_BUF_SIZE - mock_tx_len) ? Size : (MOCK_TX_BUF_SIZE - mock_tx_len);
    if (copy_len > 0) {
        memcpy(&mock_tx_buf[mock_tx_len], pData, copy_len);
        mock_tx_len += copy_len;
    }
    return HAL_OK;
}

static inline HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart,
                                                     uint8_t *pData,
                                                     uint16_t Size)
{
    (void)huart;
    (void)pData;
    (void)Size;
    return HAL_OK;
}

static inline void mock_tx_reset(void)
{
    mock_tx_len = 0;
    memset(mock_tx_buf, 0, MOCK_TX_BUF_SIZE);
}

#endif /* __STM32F4XX_HAL_UART_H */
