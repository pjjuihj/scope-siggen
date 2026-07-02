/**
  ******************************************************************************
  * @file           : config.h
  * @brief          : 配置管理模块接口
  ******************************************************************************
  * @attention
  *
  * 配置管理模块，用于配置参数管理
  *
  ******************************************************************************
  */

#ifndef __CONFIG_H
#define __CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "oscilloscope.h"
#include "signal_gen.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief 配置头结构体
  */
typedef struct {
    uint32_t magic;             /* 魔数 0x434F4E46 ("CONF") */
    uint32_t version;           /* 配置版本 */
    uint32_t checksum;          /* 校验和 */
    uint32_t length;            /* 配置长度 */
} ConfigHeader_t;

/**
  * @brief 系统配置结构体
  */
typedef struct {
    uint32_t uart_baudrate;     /* UART波特率 */
    uint8_t display_brightness; /* 显示亮度 */
    uint8_t watchdog_timeout;   /* 看门狗超时 */
    uint8_t log_level;          /* 日志级别 */
} SysConfig_t;

/**
  * @brief 关机配置结构体
  */
typedef struct {
    uint8_t auto_save;          /* 自动保存 */
    uint8_t confirm_shutdown;   /* 确认关机 */
    uint8_t safe_state;         /* 安全状态 */
} ShutdownConfig_t;

/**
  * @brief 应用配置结构体
  */
typedef struct {
    ConfigHeader_t header;      /* 配置头 */
    OscConfig_t osc;            /* 示波器配置 */
    SigGenConfig_t siggen;      /* 信号发生器配置 */
    SysConfig_t sys;            /* 系统配置 */
    ShutdownConfig_t shutdown;  /* 关机配置 */
} AppConfig_t;

/* Exported constants --------------------------------------------------------*/

/* 配置版本 */
#define CONFIG_VERSION          1

/* 配置最大大小 */
#define CONFIG_MAX_SIZE         sizeof(AppConfig_t)

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  初始化配置管理模块
  * @retval 错误码
  */
ErrorCode_t Config_Init(void);

/**
  * @brief  加载配置
  * @retval 错误码
  */
ErrorCode_t Config_Load(void);

/**
  * @brief  保存配置
  * @retval 错误码
  */
ErrorCode_t Config_Save(void);

/**
  * @brief  加载默认配置
  * @retval 错误码
  */
ErrorCode_t Config_LoadDefaults(void);

/**
  * @brief  获取配置
  * @retval 配置结构体指针
  */
AppConfig_t* Config_Get(void);

/**
  * @brief  设置示波器配置
  * @param  config: 配置结构体指针
  * @retval 错误码
  */
ErrorCode_t Config_SetOscConfig(const OscConfig_t *config);

/**
  * @brief  获取示波器配置
  * @param  config: 配置结构体指针
  * @retval 错误码
  */
ErrorCode_t Config_GetOscConfig(OscConfig_t *config);

/**
  * @brief  设置信号发生器配置
  * @param  config: 配置结构体指针
  * @retval 错误码
  */
ErrorCode_t Config_SetSigGenConfig(const SigGenConfig_t *config);

/**
  * @brief  获取信号发生器配置
  * @param  config: 配置结构体指针
  * @retval 错误码
  */
ErrorCode_t Config_GetSigGenConfig(SigGenConfig_t *config);

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H */
