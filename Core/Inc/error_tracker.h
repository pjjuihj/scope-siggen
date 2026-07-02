/**
  ******************************************************************************
  * @file           : error_tracker.h
  * @brief          : 错误追踪模块接口
  ******************************************************************************
  * @attention
  *
  * 错误追踪模块，用于记录和查询错误信息
  *
  ******************************************************************************
  */

#ifndef __ERROR_TRACKER_H
#define __ERROR_TRACKER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief 错误码定义
  */
typedef enum {
    ERR_OK = 0,                 /*!< 成功 */
    ERR_TIMEOUT,                /*!< 超时 */
    ERR_INVALID_PARAM,          /*!< 无效参数 */
    ERR_BUFFER_FULL,            /*!< 缓冲区满 */
    ERR_BUFFER_EMPTY,           /*!< 缓冲区空 */
    ERR_HARDWARE,               /*!< 硬件错误 */
    ERR_MEMORY,                 /*!< 内存错误 */
    ERR_BUSY,                   /*!< 忙 */
    ERR_NOT_INIT,               /*!< 未初始化 */
    ERR_NOT_SUPPORTED,          /*!< 不支持 */
    ERR_FILE_OPEN,              /*!< 文件打开失败 */
    ERR_FILE_READ,              /*!< 文件读取失败 */
    ERR_FILE_WRITE,             /*!< 文件写入失败 */
    ERR_CANCELLED,              /*!< 已取消 */
    ERR_UNKNOWN                 /*!< 未知错误 */
} ErrorCode_t;

/**
  * @brief 错误严重程度
  */
typedef enum {
    ERROR_SEVERITY_INFO = 0,    /*!< 信息 */
    ERROR_SEVERITY_WARNING,     /*!< 警告 */
    ERROR_SEVERITY_ERROR,       /*!< 错误 */
    ERROR_SEVERITY_FATAL        /*!< 致命错误 */
} ErrorSeverity_t;

/**
  * @brief 错误记录结构体
  */
typedef struct {
    ErrorCode_t code;           /*!< 错误码 */
    ErrorSeverity_t severity;   /*!< 严重程度 */
    const char *file;           /*!< 文件名 */
    uint32_t line;              /*!< 行号 */
    uint32_t timestamp;         /*!< 时间戳 (ms) */
    char message[64];           /*!< 错误消息 */
} ErrorRecord_t;

/* Exported constants --------------------------------------------------------*/

/* 最大错误记录数 */
#define ERROR_MAX_RECORDS 16

/* Exported macro ------------------------------------------------------------*/

/**
  * @brief  记录错误的宏定义（简化使用）
  * @param  code: 错误码
  * @param  msg: 错误消息
  */
#define RECORD_ERROR(code, msg) \
    ErrorTracker_Record(code, ERROR_SEVERITY_ERROR, __FILE__, __LINE__, msg)

/**
  * @brief  记录警告的宏定义
  * @param  code: 错误码
  * @param  msg: 错误消息
  */
#define RECORD_WARNING(code, msg) \
    ErrorTracker_Record(code, ERROR_SEVERITY_WARNING, __FILE__, __LINE__, msg)

/**
  * @brief  记录信息的宏定义
  * @param  code: 错误码
  * @param  msg: 错误消息
  */
#define RECORD_INFO(code, msg) \
    ErrorTracker_Record(code, ERROR_SEVERITY_INFO, __FILE__, __LINE__, msg)

/**
  * @brief  记录致命错误的宏定义
  * @param  code: 错误码
  * @param  msg: 错误消息
  */
#define RECORD_FATAL(code, msg) \
    ErrorTracker_Record(code, ERROR_SEVERITY_FATAL, __FILE__, __LINE__, msg)

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  初始化错误追踪模块
  * @retval 错误码
  */
ErrorCode_t ErrorTracker_Init(void);

/**
  * @brief  记录错误
  * @param  code: 错误码
  * @param  severity: 严重程度
  * @param  file: 文件名
  * @param  line: 行号
  * @param  message: 错误消息
  * @retval None
  */
void ErrorTracker_Record(ErrorCode_t code, ErrorSeverity_t severity,
                         const char *file, uint32_t line, const char *message);

/**
  * @brief  获取最后一条错误记录
  * @param  record: 错误记录指针
  * @retval 错误码
  */
ErrorCode_t ErrorTracker_GetLastError(ErrorRecord_t *record);

/**
  * @brief  获取错误历史
  * @param  records: 错误记录数组
  * @param  max_count: 最大记录数
  * @param  actual_count: 实际记录数指针
  * @retval 错误码
  */
ErrorCode_t ErrorTracker_GetHistory(ErrorRecord_t *records,
                                     uint8_t max_count, uint8_t *actual_count);

/**
  * @brief  打印错误历史
  * @retval None
  */
void ErrorTracker_PrintHistory(void);

/**
  * @brief  获取错误计数
  * @retval 错误计数
  */
uint8_t ErrorTracker_GetCount(void);

/**
  * @brief  清除错误记录
  * @retval None
  */
void ErrorTracker_Clear(void);

/**
  * @brief  获取错误码字符串
  * @param  code: 错误码
  * @retval 错误码字符串
  */
const char* ErrorTracker_GetCodeString(ErrorCode_t code);

/**
  * @brief  获取严重程度字符串
  * @param  severity: 严重程度
  * @retval 严重程度字符串
  */
const char* ErrorTracker_GetSeverityString(ErrorSeverity_t severity);

#ifdef __cplusplus
}
#endif

#endif /* __ERROR_TRACKER_H */
