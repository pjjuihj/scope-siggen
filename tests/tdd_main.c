/**
  ******************************************************************************
  * @file           : tdd_main.c
  * @brief          : TDD 测试主程序
  ******************************************************************************
  */

#include "test_framework.h"
#include <stdio.h>

/* Slice 1 测试声明 */
extern TestResult_t test_s1_init_null_rb(void);
extern TestResult_t test_s1_init_null_buffer(void);
extern TestResult_t test_s1_init_zero_size(void);
extern TestResult_t test_s1_init_valid(void);

int main(void)
{
    printf("\n=== TDD: ring_buffer 逐切片测试 ===\n\n");

    /* Slice 1: Init */
    printf("--- Slice 1: Init ---\n");
    uint32_t pass = 0, fail = 0;

    TestCase_t s1_cases[] = {
        { "s1_init_null_rb",    test_s1_init_null_rb },
        { "s1_init_null_buffer", test_s1_init_null_buffer },
        { "s1_init_zero_size",  test_s1_init_zero_size },
        { "s1_init_valid",      test_s1_init_valid },
    };
    uint32_t s1_count = sizeof(s1_cases) / sizeof(s1_cases[0]);

    for (uint32_t i = 0; i < s1_count; i++) {
        printf("[%u/%u] %s ... ", i + 1, s1_count, s1_cases[i].name);
        TestResult_t r = s1_cases[i].func();
        if (r == TEST_PASS) { printf("PASS\n"); pass++; }
        else                { printf("FAIL\n"); fail++; }
    }

    printf("\nResult: %u passed, %u failed\n", pass, fail);
    return (fail == 0) ? 0 : 1;
}
