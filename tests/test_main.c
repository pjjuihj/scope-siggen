/**
  ******************************************************************************
  * @file           : test_main.c
  * @brief          : 测试主程序
  ******************************************************************************
  * @attention
  *
  * 测试主程序，运行所有测试用例
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "test_framework.h"
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* 外部测试用例声明 */
extern TEST_CASE(ring_buffer_init);
extern TEST_CASE(ring_buffer_put_get);
extern TEST_CASE(ring_buffer_full);
extern TEST_CASE(ring_buffer_empty);
extern TEST_CASE(ring_buffer_peek);
extern TEST_CASE(ring_buffer_wrap);
extern TEST_CASE(ring_buffer_block);
extern TEST_CASE(ring_buffer_flush);

extern TEST_CASE(version_get_info);
extern TEST_CASE(version_string);
extern TEST_CASE(version_build_date);
extern TEST_CASE(version_build_time);
extern TEST_CASE(version_compare_equal);
extern TEST_CASE(version_compare_newer);
extern TEST_CASE(version_compare_older);
extern TEST_CASE(version_is_newer);
extern TEST_CASE(version_get_string);
extern TEST_CASE(version_get_timestamp);

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Exported function implementations -----------------------------------------*/

/* 环形缓冲测试套件 */
TEST_SUITE_BEGIN(ring_buffer_suite)
    TEST_CASE_ENTRY(ring_buffer_init)
    TEST_CASE_ENTRY(ring_buffer_put_get)
    TEST_CASE_ENTRY(ring_buffer_full)
    TEST_CASE_ENTRY(ring_buffer_empty)
    TEST_CASE_ENTRY(ring_buffer_peek)
    TEST_CASE_ENTRY(ring_buffer_wrap)
    TEST_CASE_ENTRY(ring_buffer_block)
    TEST_CASE_ENTRY(ring_buffer_flush)
TEST_SUITE_END(ring_buffer_suite)

/* 版本管理测试套件 */
TEST_SUITE_BEGIN(version_suite)
    TEST_CASE_ENTRY(version_get_info)
    TEST_CASE_ENTRY(version_string)
    TEST_CASE_ENTRY(version_build_date)
    TEST_CASE_ENTRY(version_build_time)
    TEST_CASE_ENTRY(version_compare_equal)
    TEST_CASE_ENTRY(version_compare_newer)
    TEST_CASE_ENTRY(version_compare_older)
    TEST_CASE_ENTRY(version_is_newer)
    TEST_CASE_ENTRY(version_get_string)
    TEST_CASE_ENTRY(version_get_timestamp)
TEST_SUITE_END(version_suite)

/**
  * @brief  测试主函数
  * @retval 测试结果
  */
int main(void)
{
    printf("\n=== SCOPE-SIGGEN Test Suite ===\n");
    printf("Version: %s\n", VERSION_STRING);
    printf("Build: %s %s\n", __DATE__, __TIME__);
    printf("==============================\n");

    TestResult_t result = TEST_PASS;

    /* 运行环形缓冲测试 */
    result |= Test_RunSuite(&ring_buffer_suite);

    /* 运行版本管理测试 */
    result |= Test_RunSuite(&version_suite);

    /* 打印总报告 */
    printf("\n=== Overall Test Report ===\n");
    printf("Ring Buffer: %s\n",
           (ring_buffer_suite.failed == 0) ? "PASS" : "FAIL");
    printf("Version: %s\n",
           (version_suite.failed == 0) ? "PASS" : "FAIL");
    printf("===========================\n");

    if (result == TEST_PASS) {
        printf("\n✓ All tests passed!\n");
    } else {
        printf("\n✗ Some tests failed!\n");
    }

    return (result == TEST_PASS) ? 0 : 1;
}

/* Private function implementations ------------------------------------------*/
