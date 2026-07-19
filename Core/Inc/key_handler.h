/**
  ******************************************************************************
  * @file           : key_handler.h
  * @brief          : 按键处理模块接口
  ******************************************************************************
  * @attention
  *
  * 按键处理模块，用于按键扫描和处理
  *
  ******************************************************************************
  */

#ifndef __KEY_HANDLER_H
#define __KEY_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "error_tracker.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief 按键码定义
  */
typedef enum {
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_ENTER,
    KEY_BACK,
    KEY_MENU,
    KEY_SELECT
} KeyCode_t;

/**
  * @brief 按键回调函数类型
  */
typedef void (*KeyCallback_t)(KeyCode_t key);

/* Exported constants --------------------------------------------------------*/

/* 按键去抖时间 */
#define KEY_DEBOUNCE_MS     20

/* 按键长按时间 */
#define KEY_LONG_PRESS_MS   1000

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  初始化按键处理模块
  * @retval 错误码
  */
ErrorCode_t KeyHandler_Init(void);

/**
  * @brief  扫描按键
  * @retval 按键码
  */
KeyCode_t Key_Scan(void);

/**
  * @brief  检查按键是否按下
  * @param  key: 按键码
  * @retval true=按下, false=未按下
  */
bool Key_IsPressed(KeyCode_t key);

/**
  * @brief  等待按键按下
  * @param  timeout_ms: 超时时间 (ms)
  * @retval 按键码
  */
KeyCode_t Key_WaitForKey(uint32_t timeout_ms);

/**
  * @brief  注册按键回调函数
  * @param  callback: 回调函数
  * @retval None
  */
void Key_RegisterCallback(KeyCallback_t callback);

/**
  * @brief  检查最后一次按键是否为长按
  * @retval true=长按, false=短按
  */
bool Key_IsLastPressLong(void);

/**
  * @brief  按键处理任务
  * @param  argument: 任务参数
  * @retval None
  */
void Key_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* __KEY_HANDLER_H */
