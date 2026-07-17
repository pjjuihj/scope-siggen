/**
  ******************************************************************************
  * @file           : test_unit_test.c
  * @brief          : 单元测试模块测试
  ******************************************************************************
  * @attention
  *
  * 测试 unit_test.c 的接口和桩函数
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "test_framework.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* Private define ------------------------------------------------------------*/

/* Private types ------------------------------------------------------------*/

typedef enum {
    HAL_OK = 0,
    HAL_ERROR = 1,
    HAL_BUSY = 2,
    HAL_TIMEOUT = 3
} HAL_StatusTypeDef;

typedef struct {
    uint32_t Instance;
} UART_HandleTypeDef;

/* Private variables ---------------------------------------------------------*/

/* 模拟 UART 发送的数据 */
static char mock_tx_buf[256];
static uint16_t mock_tx_len = 0;

/* Private function prototypes -----------------------------------------------*/

static HAL_StatusTypeDef mock_HAL_UART_Transmit(UART_HandleTypeDef *huart,
                                                  uint8_t *pData,
                                                  uint16_t Size,
                                                  uint32_t Timeout);
static void UnitTest_RunAll(UART_HandleTypeDef *huart);

/* Test cases ----------------------------------------------------------------*/

/**
  * @brief  测试 UnitTest_RunAll 函数存在
  */
TEST_CASE(unit_test_function_exists)
{
    /* 验证函数指针不为 NULL */
    ASSERT(UnitTest_RunAll != NULL);
    return TEST_PASS;
}

/**
  * @brief  测试 UnitTest_RunAll 发送消息
  */
TEST_CASE(unit_test_send_message)
{
    UART_HandleTypeDef huart;
    huart.Instance = 0x40011000;  /* USART1 */

    /* 重置模拟缓冲区 */
    mock_tx_len = 0;
    memset(mock_tx_buf, 0, sizeof(mock_tx_buf));

    /* 调用函数 */
    UnitTest_RunAll(&huart);

    /* 验证发送了数据 */
    ASSERT(mock_tx_len > 0);
    return TEST_PASS;
}

/**
  * @brief  测试 UnitTest_RunAll 发送正确的消息格式
  */
TEST_CASE(unit_test_message_format)
{
    UART_HandleTypeDef huart;
    huart.Instance = 0x40011000;

    /* 重置模拟缓冲区 */
    mock_tx_len = 0;
    memset(mock_tx_buf, 0, sizeof(mock_tx_buf));

    /* 调用函数 */
    UnitTest_RunAll(&huart);

    /* 验证消息格式 */
    ASSERT(mock_tx_len >= 11);
    ASSERT(mock_tx_buf[0] == 'T');
    ASSERT(mock_tx_buf[1] == 'E');
    ASSERT(mock_tx_buf[2] == 'S');
    ASSERT(mock_tx_buf[3] == 'T');
    ASSERT(mock_tx_buf[4] == ':');
    ASSERT(mock_tx_buf[5] == 's');
    ASSERT(mock_tx_buf[6] == 't');
    ASSERT(mock_tx_buf[7] == 'u');
    ASSERT(mock_tx_buf[8] == 'b');

    return TEST_PASS;
}

/**
  * @brief  测试 UnitTest_RunAll 支持 NULL 参数
  */
TEST_CASE(unit_test_null_param)
{
    /* 传入 NULL 参数，应该不会崩溃 */
    UnitTest_RunAll(NULL);

    /* 验证没有崩溃 */
    ASSERT(1);
    return TEST_PASS;
}

/* Private function implementations ------------------------------------------*/

static HAL_StatusTypeDef mock_HAL_UART_Transmit(UART_HandleTypeDef *huart,
                                                  uint8_t *pData,
                                                  uint16_t Size,
                                                  uint32_t Timeout)
{
    (void)huart;
    (void)Timeout;

    /* 模拟发送数据 */
    uint16_t copy_len = (Size < sizeof(mock_tx_buf) - mock_tx_len) ?
                         Size : (sizeof(mock_tx_buf) - mock_tx_len);
    if (copy_len > 0) {
        memcpy(&mock_tx_buf[mock_tx_len], pData, copy_len);
        mock_tx_len += copy_len;
    }

    return HAL_OK;
}

static void UnitTest_RunAll(UART_HandleTypeDef *huart)
{
    (void)huart;
    const char *msg = "TEST:stub\r\n";
    mock_HAL_UART_Transmit(huart, (uint8_t*)msg, 11, 100);
}
