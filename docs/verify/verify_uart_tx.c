/**
  ******************************************************************************
  * @file           : verify_uart_tx.c
  * @brief          : UART TX DMA 验证代码
  ******************************************************************************
  * @attention
  *
  * 烧录后在 main.c 的 while(1) 循环里调用这些函数
  * 观察串口输出是否正常
  *
  ******************************************************************************
  */

#include "uart_tx.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>

/**
  * @brief  验证 1: 基础 printf 输出
  * @note   在 main.c 的 while(1) 里调用
  * @retval None
  */
void Verify_UART_TX_Basic(void)
{
    printf("=== UART TX Basic Test ===\r\n");
    printf("Hello from printf!\r\n");
    printf("Counter: %lu\r\n", HAL_GetTick());
    printf("Test string with numbers: %d, %f, %s\r\n", 42, 3.14, "ok");
    printf("==========================\r\n");
}

/**
  * @brief  验证 2: LOG_* 宏输出
  * @note   在 main.c 的 while(1) 里调用
  * @retval None
  */
void Verify_UART_TX_LOG_Macro(void)
{
    LOG_INFO("LOG_INFO test: tick=%lu", HAL_GetTick());
    LOG_WARNING("LOG_WARNING test: value=%d", 123);
    LOG_ERROR("LOG_ERROR test: this is an error");
}

/**
  * @brief  验证 3: 高频输出（压力测试）
  * @note   在 main.c 的 while(1) 里调用，观察是否丢数据
  * @retval None
  */
void Verify_UART_TX_Stress(void)
{
    for (int i = 0; i < 100; i++) {
        printf("Stress test %d/100\r\n", i);
    }
    printf("Stress test complete, drop count=%lu\r\n",
           UART_TX_GetDropCount());
}

/**
  * @brief  验证 4: 心跳检查
  * @note   在 main.c 的 while(1) 里调用
  * @retval None
  */
void Verify_UART_TX_Heartbeat(void)
{
    static uint32_t last_heartbeat = 0;
    uint32_t current = UART_TX_GetHeartbeat();

    if (current != last_heartbeat) {
        printf("Heartbeat: %lu (alive)\r\n", current);
        last_heartbeat = current;
    }
}

/**
  * @brief  验证 5: 多任务同时 printf
  * @note   创建一个额外任务来测试并发
  * @retval None
  */
void Verify_UART_TX_Concurrent(void)
{
    // 主任务
    printf("Task A: %lu\r\n", HAL_GetTick());
}

// 在另一个 FreeRTOS 任务里调用
void Verify_UART_TX_Concurrent_TaskB(void)
{
    printf("Task B: %lu\r\n", HAL_GetTick());
}
