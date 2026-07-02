/**
  ******************************************************************************
  * @file           : test_framework.c
  * @brief          : 测试框架实现
  ******************************************************************************
  * @attention
  *
  * 轻量级单元测试框架
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "test_framework.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Exported function implementations -----------------------------------------*/

TestResult_t Test_RunSuite(TestSuite_t *suite)
{
    if (suite == NULL) {
        return TEST_FAIL;
    }

    printf("\n=== Running Test Suite: %s ===\n", suite->name);
    printf("Total tests: %lu\n", suite->count);

    suite->passed = 0;
    suite->failed = 0;
    suite->skipped = 0;

    for (uint32_t i = 0; i < suite->count; i++) {
        TestCase_t *tc = &suite->cases[i];

        printf("\n[%lu/%lu] Running: %s\n", i + 1, suite->count, tc->name);

        TestResult_t result = tc->func();

        switch (result) {
            case TEST_PASS:
                printf("  ✓ PASS\n");
                suite->passed++;
                break;
            case TEST_FAIL:
                printf("  ✗ FAIL\n");
                suite->failed++;
                break;
            case TEST_SKIP:
                printf("  - SKIP\n");
                suite->skipped++;
                break;
        }
    }

    Test_PrintReport(suite);

    return (suite->failed == 0) ? TEST_PASS : TEST_FAIL;
}

TestResult_t Test_RunAll(void)
{
    printf("\n=== Running All Tests ===\n");

    /* TODO: 运行所有测试套件 */

    printf("\n=== All Tests Completed ===\n");

    return TEST_PASS;
}

void Test_PrintReport(const TestSuite_t *suite)
{
    if (suite == NULL) {
        return;
    }

    printf("\n=== Test Report: %s ===\n", suite->name);
    printf("Total:  %lu\n", suite->count);
    printf("Passed: %lu\n", suite->passed);
    printf("Failed: %lu\n", suite->failed);
    printf("Skipped: %lu\n", suite->skipped);
    printf("Pass rate: %.1f%%\n",
           (suite->count > 0) ? (100.0f * suite->passed / suite->count) : 0.0f);
    printf("==========================\n");
}

/* Private function implementations ------------------------------------------*/
