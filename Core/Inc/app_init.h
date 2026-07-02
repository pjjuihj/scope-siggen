/**
  ******************************************************************************
  * @file           : app_init.h
  * @brief          : 应用初始化模块接口
  ******************************************************************************
  * @attention
  *
  * 应用初始化模块，统一管理所有模块的初始化
  *
  ******************************************************************************
  */

#ifndef __APP_INIT_H
#define __APP_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief 启动阶段定义
  */
typedef enum {
    BOOT_STAGE_INIT = 0,        /* 初始化阶段 */
    BOOT_STAGE_SELFTEST,        /* 自检阶段 */
    BOOT_STAGE_CONFIG,          /* 配置加载阶段 */
    BOOT_STAGE_START,           /* 启动阶段 */
    BOOT_STAGE_READY,           /* 就绪阶段 */
    BOOT_STAGE_ERROR            /* 错误阶段 */
} BootStage_t;

/* Exported constants --------------------------------------------------------*/

/* 最大重试次数 */
#define BOOT_MAX_RETRY          3

/* 重试延迟 */
#define BOOT_RETRY_DELAY        100

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  应用初始化入口
  * @retval 启动阶段
  */
BootStage_t App_Init(void);

/**
  * @brief  获取当前启动阶段
  * @retval 启动阶段
  */
BootStage_t App_GetBootStage(void);

/**
  * @brief  检查是否就绪
  * @retval true=就绪, false=未就绪
  */
bool App_IsReady(void);

/**
  * @brief  进入降级模式
  * @retval None
  */
void App_EnterFallbackMode(void);

/**
  * @brief  安全关机
  * @retval None
  */
void App_Shutdown(void);

/**
  * @brief  获取启动状态字符串
  * @param  stage: 启动阶段
  * @retval 状态字符串
  */
const char* App_GetBootStageString(BootStage_t stage);

/**
  * @brief  应用初始化任务
  * @param  argument: 任务参数
  * @retval None
  */
void App_Init_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* __APP_INIT_H */
