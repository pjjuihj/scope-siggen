/**
  ******************************************************************************
  * @file           : debug.h
  * @brief          : 调试工具模块接口
  ******************************************************************************
  * @attention
  *
  * 调试工具模块，提供日志输出、调试命令、性能监测等功能
  *
  ******************************************************************************
  */

#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "error_tracker.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief 日志级别定义
  */
typedef enum {
    LOG_DEBUG = 0,              /*!< 调试信息 */
    LOG_INFO,                   /*!< 一般信息 */
    LOG_WARNING,                /*!< 警告 */
    LOG_ERROR,                  /*!< 错误 */
    LOG_FATAL                   /*!< 致命错误 */
} LogLevel_t;

/**
  * @brief 调试命令类型
  */
typedef enum {
    DBG_CMD_HELP = 0,           /*!< 帮助 */
    DBG_CMD_STATUS,             /*!< 系统状态 */
    DBG_CMD_TASKS,              /*!< 任务列表 */
    DBG_CMD_MEMORY,             /*!< 内存信息 */
    DBG_CMD_VERSION,            /*!< 版本信息 */
    DBG_CMD_ERRORS,             /*!< 错误历史 */
    DBG_CMD_LOG_LEVEL,          /*!< 设置日志级别 */
    DBG_CMD_RESET,              /*!< 系统复位 */
    DBG_CMD_COUNT               /*!< 命令数量 */
} DebugCmdType_t;

/* Exported constants --------------------------------------------------------*/

/* 调试使能 */
#define DEBUG_EN 1

/* 日志缓冲区大小 */
#define LOG_BUFFER_SIZE 256

/* Exported macro ------------------------------------------------------------*/

/**
  * @brief  日志宏定义
  */
#ifdef DEBUG_EN
    #define LOG_DEBUG(fmt, ...)   Debug_Log(LOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...)    Debug_Log(LOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_WARNING(fmt, ...) Debug_Log(LOG_WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...)   Debug_Log(LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_FATAL(fmt, ...)   Debug_Log(LOG_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
    #define LOG_DEBUG(fmt, ...)   ((void)0)
    #define LOG_INFO(fmt, ...)    ((void)0)
    #define LOG_WARNING(fmt, ...) ((void)0)
    #define LOG_ERROR(fmt, ...)   ((void)0)
    #define LOG_FATAL(fmt, ...)   ((void)0)
#endif

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  初始化调试模块
  * @retval 错误码
  */
ErrorCode_t Debug_Init(void);

/**
  * @brief  输出日志
  * @param  level: 日志级别
  * @param  file: 文件名
  * @param  line: 行号
  * @param  fmt: 格式化字符串
  * @param  ...: 可变参数
  * @retval None
  */
void Debug_Log(LogLevel_t level, const char *file, uint32_t line,
               const char *fmt, ...);

/**
  * @brief  设置日志级别
  * @param  level: 日志级别
  * @retval None
  */
void Debug_SetLogLevel(LogLevel_t level);

/**
  * @brief  获取日志级别
  * @retval 日志级别
  */
LogLevel_t Debug_GetLogLevel(void);

/**
  * @brief  处理调试命令
  * @param  cmd: 命令字符串
  * @retval None
  */
void Debug_ProcessCommand(const char *cmd);

/**
  * @brief  打印系统状态
  * @retval None
  */
void Debug_PrintSystemStatus(void);

/**
  * @brief  打印任务状态
  * @retval None
  */
void Debug_PrintTaskStatus(void);

/**
  * @brief  打印内存状态
  * @retval None
  */
void Debug_PrintMemoryStatus(void);

/**
  * @brief  获取CPU使用率
  * @retval CPU使用率 (0-100)
  */
uint32_t Debug_GetCPUUsage(void);

/**
  * @brief  获取空闲堆大小
  * @retval 空闲堆大小 (bytes)
  */
uint32_t Debug_GetFreeHeapSize(void);

/**
  * @brief  获取最小空闲堆大小
  * @retval 最小空闲堆大小 (bytes)
  */
uint32_t Debug_GetMinFreeHeapSize(void);

/**
  * @brief  获取日志级别字符串
  * @param  level: 日志级别
  * @retval 日志级别字符串
  */
const char* Debug_GetLogLevelString(LogLevel_t level);

#ifdef __cplusplus
}
#endif

#endif /* __DEBUG_H */
