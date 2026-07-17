/**
  * @brief  板载单元测试接口
  */
#ifndef __UNIT_TEST_H
#define __UNIT_TEST_H

#include "stm32f4xx_hal.h"

void UnitTest_RunAll(UART_HandleTypeDef *huart);

#endif
