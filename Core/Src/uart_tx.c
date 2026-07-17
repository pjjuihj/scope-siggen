/**
  * @file    uart_tx.c
  * @brief   UART TX - printf 重定向（安全版本）
  */

#include "main.h"
#include <stdio.h>

extern UART_HandleTypeDef huart1;

/* 初始化标志：MX_USART1_UART_Init() 完成后设为 true */
static volatile uint32_t uart_ready = 0;

/**
  * @brief  标记 UART 已就绪
  * @note   在 MX_USART1_UART_Init() 之后调用
  */
void UART_TX_MarkReady(void)
{
    uart_ready = 1;
}

/**
  * @brief  重定向 _write
  */
int _write(int fd, const char *buf, int len)
{
    if (fd != 1 && fd != 2) {
        return -1;
    }

    /* UART 未就绪时丢弃数据 */
    if (!uart_ready) {
        return len;
    }

    HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, 100);
    return len;
}
