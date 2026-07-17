/**
  * @brief  板载单元测试 — 最小版本
  */

#include "unit_test.h"

void UnitTest_RunAll(UART_HandleTypeDef *huart)
{
    (void)huart;
    const char *msg = "TEST:stub\r\n";
    HAL_UART_Transmit(huart, (uint8_t*)msg, 11, 100);
}
