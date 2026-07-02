/**
  ******************************************************************************
  * @file           : error_tracker.c
  * @brief          : 错误追踪模块实现
  ******************************************************************************
  * @attention
  *
  * 错误追踪模块，用于记录和查询错误信息
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "error_tracker.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/**
  * @brief 错误记录数组
  */
static ErrorRecord_t error_records[ERROR_MAX_RECORDS];

/**
  * @brief 错误计数
  */
static uint8_t error_count = 0;

/**
  * @brief 当前写入索引
  */
static uint8_t error_index = 0;

/**
  * @brief 初始化标志
  */
static bool initialized = false;

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Exported function implementations -----------------------------------------*/

ErrorCode_t ErrorTracker_Init(void)
{
    /* 清除错误记录 */
    memset(error_records, 0, sizeof(error_records));
    error_count = 0;
    error_index = 0;
    initialized = true;

    LOG_INFO("Error tracker initialized");
    return ERR_OK;
}

void ErrorTracker_Record(ErrorCode_t code, ErrorSeverity_t severity,
                         const char *file, uint32_t line, const char *message)
{
    if (!initialized) {
        return;
    }

    /* 获取当前时间戳 */
    uint32_t timestamp = HAL_GetTick();

    /* 写入错误记录 */
    ErrorRecord_t *record = &error_records[error_index];
    record->code = code;
    record->severity = severity;
    record->file = file;
    record->line = line;
    record->timestamp = timestamp;

    /* 复制消息（安全复制） */
    if (message != NULL) {
        strncpy(record->message, message, sizeof(record->message) - 1);
        record->message[sizeof(record->message) - 1] = '\0';
    } else {
        record->message[0] = '\0';
    }

    /* 更新索引和计数 */
    error_index = (error_index + 1) % ERROR_MAX_RECORDS;
    if (error_count < ERROR_MAX_RECORDS) {
        error_count++;
    }

    /* 输出到日志 */
    const char *severity_str = ErrorTracker_GetSeverityString(severity);
    const char *code_str = ErrorTracker_GetCodeString(code);

    LOG_ERROR("[%s] %s at %s:%lu - %s",
              severity_str, code_str, file, line, message ? message : "");
}

ErrorCode_t ErrorTracker_GetLastError(ErrorRecord_t *record)
{
    if (!initialized || record == NULL) {
        return ERR_INVALID_PARAM;
    }

    if (error_count == 0) {
        return ERR_BUFFER_EMPTY;
    }

    /* 获取最后一条记录 */
    uint8_t last_index = (error_index + ERROR_MAX_RECORDS - 1) % ERROR_MAX_RECORDS;
    memcpy(record, &error_records[last_index], sizeof(ErrorRecord_t));

    return ERR_OK;
}

ErrorCode_t ErrorTracker_GetHistory(ErrorRecord_t *records,
                                     uint8_t max_count, uint8_t *actual_count)
{
    if (!initialized || records == NULL || actual_count == NULL) {
        return ERR_INVALID_PARAM;
    }

    /* 计算实际要复制的记录数 */
    uint8_t count = (error_count < max_count) ? error_count : max_count;
    *actual_count = count;

    /* 复制记录（从最旧到最新） */
    uint8_t start_index = (error_index + ERROR_MAX_RECORDS - count) % ERROR_MAX_RECORDS;
    for (uint8_t i = 0; i < count; i++) {
        uint8_t idx = (start_index + i) % ERROR_MAX_RECORDS;
        memcpy(&records[i], &error_records[idx], sizeof(ErrorRecord_t));
    }

    return ERR_OK;
}

void ErrorTracker_PrintHistory(void)
{
    if (!initialized) {
        LOG_ERROR("Error tracker not initialized");
        return;
    }

    LOG_INFO("=== Error History (%d/%d) ===", error_count, ERROR_MAX_RECORDS);

    if (error_count == 0) {
        LOG_INFO("No errors recorded");
        return;
    }

    /* 打印所有记录（从最旧到最新） */
    uint8_t start_index = (error_index + ERROR_MAX_RECORDS - error_count) % ERROR_MAX_RECORDS;
    for (uint8_t i = 0; i < error_count; i++) {
        uint8_t idx = (start_index + i) % ERROR_MAX_RECORDS;
        ErrorRecord_t *record = &error_records[idx];

        const char *severity_str = ErrorTracker_GetSeverityString(record->severity);
        const char *code_str = ErrorTracker_GetCodeString(record->code);

        LOG_INFO("[%2d] %lu.%03lu [%s] %s at %s:%lu",
                 i + 1,
                 record->timestamp / 1000,
                 record->timestamp % 1000,
                 severity_str,
                 code_str,
                 record->file ? record->file : "unknown",
                 record->line);

        if (record->message[0] != '\0') {
            LOG_INFO("     %s", record->message);
        }
    }

    LOG_INFO("=== End of Error History ===");
}

uint8_t ErrorTracker_GetCount(void)
{
    return error_count;
}

void ErrorTracker_Clear(void)
{
    if (!initialized) {
        return;
    }

    memset(error_records, 0, sizeof(error_records));
    error_count = 0;
    error_index = 0;

    LOG_INFO("Error history cleared");
}

const char* ErrorTracker_GetCodeString(ErrorCode_t code)
{
    switch (code) {
        case ERR_OK:            return "OK";
        case ERR_TIMEOUT:       return "TIMEOUT";
        case ERR_INVALID_PARAM: return "INVALID_PARAM";
        case ERR_BUFFER_FULL:   return "BUFFER_FULL";
        case ERR_BUFFER_EMPTY:  return "BUFFER_EMPTY";
        case ERR_HARDWARE:      return "HARDWARE";
        case ERR_MEMORY:        return "MEMORY";
        case ERR_BUSY:          return "BUSY";
        case ERR_NOT_INIT:      return "NOT_INIT";
        case ERR_NOT_SUPPORTED: return "NOT_SUPPORTED";
        case ERR_FILE_OPEN:     return "FILE_OPEN";
        case ERR_FILE_READ:     return "FILE_READ";
        case ERR_FILE_WRITE:    return "FILE_WRITE";
        case ERR_CANCELLED:     return "CANCELLED";
        case ERR_UNKNOWN:       return "UNKNOWN";
        default:                return "UNKNOWN";
    }
}

const char* ErrorTracker_GetSeverityString(ErrorSeverity_t severity)
{
    switch (severity) {
        case ERROR_SEVERITY_INFO:    return "INFO";
        case ERROR_SEVERITY_WARNING: return "WARN";
        case ERROR_SEVERITY_ERROR:   return "ERROR";
        case ERROR_SEVERITY_FATAL:   return "FATAL";
        default:                     return "UNKNOWN";
    }
}

/* Private function implementations ------------------------------------------*/

