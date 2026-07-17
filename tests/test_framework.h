/**
  ******************************************************************************
  * @file           : test_framework.h
  * @brief          : 测试框架接口
  ******************************************************************************
  * @attention
  *
  * 轻量级单元测试框架
  *
  ******************************************************************************
  */

#ifndef __TEST_FRAMEWORK_H
#define __TEST_FRAMEWORK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief 测试结果
  */
typedef enum {
    TEST_PASS = 0,
    TEST_FAIL,
    TEST_SKIP
} TestResult_t;

/**
  * @brief 测试用例结构体
  */
typedef struct {
    const char *name;
    TestResult_t (*func)(void);
} TestCase_t;

/**
  * @brief 测试套件结构体
  */
typedef struct {
    const char *name;
    TestCase_t *cases;
    uint32_t count;
    uint32_t passed;
    uint32_t failed;
    uint32_t skipped;
} TestSuite_t;

/* Exported macro ------------------------------------------------------------*/

/**
  * @brief  断言宏
  */
#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("  ASSERT FAILED: %s (line %d)\n", #condition, __LINE__); \
        return TEST_FAIL; \
    } \
} while(0)

#define ASSERT_EQUAL(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf("  ASSERT_EQUAL FAILED: expected %d, got %d (line %d)\n", \
               (int)(expected), (int)(actual), __LINE__); \
        return TEST_FAIL; \
    } \
} while(0)

#define ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        printf("  ASSERT_NOT_NULL FAILED: pointer is NULL (line %d)\n", __LINE__); \
        return TEST_FAIL; \
    } \
} while(0)

#define ASSERT_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        printf("  ASSERT_NULL FAILED: pointer is not NULL (line %d)\n", __LINE__); \
        return TEST_FAIL; \
    } \
} while(0)

#define ASSERT_STRING_EQUAL(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        printf("  ASSERT_STRING_EQUAL FAILED: expected '%s', got '%s' (line %d)\n", \
               (expected), (actual), __LINE__); \
        return TEST_FAIL; \
    } \
} while(0)

/**
  * @brief  测试用例定义宏
  */
#define TEST_CASE(name) TestResult_t test_##name(void)

/**
  * @brief  测试套件定义宏
  */
#define TEST_SUITE_BEGIN(suite_name) \
    static TestCase_t suite_name##_cases[] = {

#define TEST_CASE_ENTRY(name) \
    { #name, test_##name },

#define TEST_SUITE_END(suite_name) \
    }; \
    static TestSuite_t suite_name = { \
        .name = #suite_name, \
        .cases = suite_name##_cases, \
        .count = sizeof(suite_name##_cases) / sizeof(suite_name##_cases[0]), \
        .passed = 0, \
        .failed = 0, \
        .skipped = 0 \
    };

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  运行测试套件
  * @param  suite: 测试套件指针
  * @retval 测试结果
  */
TestResult_t Test_RunSuite(TestSuite_t *suite);

/**
  * @brief  运行所有测试
  * @retval 测试结果
  */
TestResult_t Test_RunAll(void);

/**
  * @brief  打印测试报告
  * @param  suite: 测试套件指针
  * @retval None
  */
void Test_PrintReport(const TestSuite_t *suite);

#ifdef __cplusplus
}
#endif

#endif /* __TEST_FRAMEWORK_H */
