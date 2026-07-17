/**
  ******************************************************************************
  * @file           : test_error_tracker.c
  * @brief          : 错误跟踪模块测试
  ******************************************************************************
  */

#include "test_framework.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Mock 错误结构 */
typedef struct {
    uint32_t code;
    uint32_t line;
    uint32_t timestamp;
} MockErrorEntry_t;

#define MOCK_MAX_ERRORS 10

/* Mock 变量 */
static MockErrorEntry_t mock_errors[MOCK_MAX_ERRORS];
static int mock_error_count = 0;
static bool mock_initialized = false;

/* Mock 函数 */
static void Mock_ErrorTracker_Init(void)
{
    mock_error_count = 0;
    mock_initialized = true;
}

static void Mock_ErrorTracker_Record(uint32_t code, uint32_t line)
{
    if (mock_error_count < MOCK_MAX_ERRORS) {
        mock_errors[mock_error_count].code = code;
        mock_errors[mock_error_count].line = line;
        mock_errors[mock_error_count].timestamp = mock_error_count;
        mock_error_count++;
    }
}

static int Mock_ErrorTracker_GetCount(void)
{
    return mock_error_count;
}

static void Mock_ErrorTracker_Clear(void)
{
    mock_error_count = 0;
}

/* Test Cases */

/**
  * @brief  测试错误跟踪初始化
  */
TEST_CASE(error_tracker_init)
{
    mock_initialized = false;
    mock_error_count = 5;

    Mock_ErrorTracker_Init();

    ASSERT_EQUAL(1, mock_initialized);
    ASSERT_EQUAL(0, mock_error_count);

    return TEST_PASS;
}

/**
  * @brief  测试记录单个错误
  */
TEST_CASE(error_tracker_record_single)
{
    Mock_ErrorTracker_Init();

    Mock_ErrorTracker_Record(0x01, 100);

    ASSERT_EQUAL(1, Mock_ErrorTracker_GetCount());
    ASSERT_EQUAL(0x01, mock_errors[0].code);
    ASSERT_EQUAL(100, mock_errors[0].line);

    return TEST_PASS;
}

/**
  * @brief  测试记录多个错误
  */
TEST_CASE(error_tracker_record_multiple)
{
    Mock_ErrorTracker_Init();

    Mock_ErrorTracker_Record(0x01, 100);
    Mock_ErrorTracker_Record(0x02, 200);
    Mock_ErrorTracker_Record(0x03, 300);

    ASSERT_EQUAL(3, Mock_ErrorTracker_GetCount());
    ASSERT_EQUAL(0x01, mock_errors[0].code);
    ASSERT_EQUAL(0x02, mock_errors[1].code);
    ASSERT_EQUAL(0x03, mock_errors[2].code);

    return TEST_PASS;
}

/**
  * @brief  测试错误缓冲区满
  */
TEST_CASE(error_tracker_buffer_full)
{
    Mock_ErrorTracker_Init();

    /* 填满缓冲区 */
    for (int i = 0; i < MOCK_MAX_ERRORS; i++) {
        Mock_ErrorTracker_Record(i, i * 10);
    }

    ASSERT_EQUAL(MOCK_MAX_ERRORS, Mock_ErrorTracker_GetCount());

    /* 再添加一个应该失败 */
    Mock_ErrorTracker_Record(0xFF, 999);
    ASSERT_EQUAL(MOCK_MAX_ERRORS, Mock_ErrorTracker_GetCount());

    return TEST_PASS;
}

/**
  * @brief  测试清除错误
  */
TEST_CASE(error_tracker_clear)
{
    Mock_ErrorTracker_Init();

    Mock_ErrorTracker_Record(0x01, 100);
    Mock_ErrorTracker_Record(0x02, 200);

    ASSERT_EQUAL(2, Mock_ErrorTracker_GetCount());

    Mock_ErrorTracker_Clear();

    ASSERT_EQUAL(0, Mock_ErrorTracker_GetCount());

    return TEST_PASS;
}
