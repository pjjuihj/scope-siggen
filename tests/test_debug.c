/**
  ******************************************************************************
  * @file           : test_debug.c
  * @brief          : 调试模块测试
  ******************************************************************************
  */

#include "test_framework.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Mock 日志级别 */
typedef enum {
    MOCK_LOG_DEBUG = 0,
    MOCK_LOG_INFO,
    MOCK_LOG_WARNING,
    MOCK_LOG_ERROR,
    MOCK_LOG_FATAL
} MockLogLevel_t;

/* Mock 变量 */
static MockLogLevel_t mock_current_level = MOCK_LOG_INFO;
static bool mock_initialized = false;
static char mock_log_buffer[256];
static int mock_log_count = 0;

/* Mock 函数 */
static void Mock_Debug_SetLogLevel(MockLogLevel_t level)
{
    mock_current_level = level;
}

static MockLogLevel_t Mock_Debug_GetLogLevel(void)
{
    return mock_current_level;
}

static void Mock_Debug_Init(void)
{
    mock_current_level = MOCK_LOG_INFO;
    mock_initialized = true;
    mock_log_count = 0;
}

static void Mock_Debug_Log(MockLogLevel_t level, const char *msg)
{
    if (level < mock_current_level) {
        return;  /* 日志级别不够，不输出 */
    }
    mock_log_count++;
    strncpy(mock_log_buffer, msg, sizeof(mock_log_buffer) - 1);
}

/* Test Cases */

/**
  * @brief  测试日志级别设置
  */
TEST_CASE(debug_set_log_level)
{
    Mock_Debug_Init();

    Mock_Debug_SetLogLevel(MOCK_LOG_DEBUG);
    ASSERT_EQUAL(MOCK_LOG_DEBUG, Mock_Debug_GetLogLevel());

    Mock_Debug_SetLogLevel(MOCK_LOG_WARNING);
    ASSERT_EQUAL(MOCK_LOG_WARNING, Mock_Debug_GetLogLevel());

    Mock_Debug_SetLogLevel(MOCK_LOG_FATAL);
    ASSERT_EQUAL(MOCK_LOG_FATAL, Mock_Debug_GetLogLevel());

    return TEST_PASS;
}

/**
  * @brief  测试日志输出（级别匹配）
  */
TEST_CASE(debug_log_level_match)
{
    Mock_Debug_Init();
    Mock_Debug_SetLogLevel(MOCK_LOG_INFO);

    Mock_Debug_Log(MOCK_LOG_INFO, "Test message");
    ASSERT_EQUAL(1, mock_log_count);
    ASSERT_STRING_EQUAL("Test message", mock_log_buffer);

    return TEST_PASS;
}

/**
  * @brief  测试日志输出（级别不够）
  */
TEST_CASE(debug_log_level_too_low)
{
    Mock_Debug_Init();
    Mock_Debug_SetLogLevel(MOCK_LOG_WARNING);

    Mock_Debug_Log(MOCK_LOG_INFO, "Should not appear");
    ASSERT_EQUAL(0, mock_log_count);

    return TEST_PASS;
}

/**
  * @brief  测试日志输出（ERROR 级别）
  */
TEST_CASE(debug_log_error)
{
    Mock_Debug_Init();
    Mock_Debug_SetLogLevel(MOCK_LOG_INFO);

    Mock_Debug_Log(MOCK_LOG_ERROR, "Error message");
    ASSERT_EQUAL(1, mock_log_count);
    ASSERT_STRING_EQUAL("Error message", mock_log_buffer);

    return TEST_PASS;
}

/**
  * @brief  测试日志初始化
  */
TEST_CASE(debug_init)
{
    mock_initialized = false;
    mock_current_level = MOCK_LOG_FATAL;

    Mock_Debug_Init();

    ASSERT_EQUAL(1, mock_initialized);
    ASSERT_EQUAL(MOCK_LOG_INFO, mock_current_level);
    ASSERT_EQUAL(0, mock_log_count);

    return TEST_PASS;
}
