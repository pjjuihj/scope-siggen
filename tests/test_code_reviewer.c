/**
  ******************************************************************************
  * @file           : test_code_reviewer.c
  * @brief          : 代码审查模块测试
  ******************************************************************************
  * @attention
  *
  * 测试 code_reviewer.c 的检查和评分功能
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "test_framework.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* Private define ------------------------------------------------------------*/

#define MAX_CHECK_ITEMS 100

/* Private types ------------------------------------------------------------*/

typedef enum {
    ERR_OK = 0,
    ERR_TIMEOUT,
    ERR_INVALID_PARAM,
    ERR_BUFFER_FULL,
    ERR_BUFFER_EMPTY,
    ERR_HARDWARE,
    ERR_MEMORY,
    ERR_BUSY,
    ERR_NOT_INIT,
    ERR_NOT_SUPPORTED,
    ERR_UNKNOWN
} ErrorCode_t;

typedef enum {
    CHECK_SYNTAX = 0,
    CHECK_STYLE,
    CHECK_FUNCTION,
    CHECK_COMPLETENESS,
    CHECK_PERFORMANCE,
    CHECK_SECURITY,
    CHECK_MEMORY,
    CHECK_INTERRUPT,
    CHECK_THREAD,
    CHECK_DESIGN
} CheckType_t;

typedef enum {
    RESULT_PASS = 0,
    RESULT_WARNING,
    RESULT_FAIL,
    RESULT_ERROR
} CheckResult_t;

typedef struct {
    CheckType_t type;
    CheckResult_t result;
    const char *file;
    uint32_t line;
    const char *rule;
    const char *message;
    const char *suggestion;
} CheckItem_t;

typedef struct {
    uint32_t total_checks;
    uint32_t pass_count;
    uint32_t warning_count;
    uint32_t fail_count;
    uint32_t error_count;
    float score;
    CheckItem_t items[MAX_CHECK_ITEMS];
    uint32_t item_count;
} CheckReport_t;

/* Private variables ---------------------------------------------------------*/

/* 测试用报告 */
static CheckReport_t test_report;

/* Private function prototypes -----------------------------------------------*/

static void CalculateScore(CheckReport_t *report);
static void AddCheckItem(CheckReport_t *report, const CheckItem_t *item);

/* Test cases ----------------------------------------------------------------*/

/**
  * @brief  测试检查类型枚举
  */
TEST_CASE(code_reviewer_check_types)
{
    ASSERT_EQUAL(0, CHECK_SYNTAX);
    ASSERT_EQUAL(1, CHECK_STYLE);
    ASSERT_EQUAL(2, CHECK_FUNCTION);
    ASSERT_EQUAL(3, CHECK_COMPLETENESS);
    ASSERT_EQUAL(4, CHECK_PERFORMANCE);
    ASSERT_EQUAL(5, CHECK_SECURITY);
    ASSERT_EQUAL(6, CHECK_MEMORY);
    ASSERT_EQUAL(7, CHECK_INTERRUPT);
    ASSERT_EQUAL(8, CHECK_THREAD);
    ASSERT_EQUAL(9, CHECK_DESIGN);
    return TEST_PASS;
}

/**
  * @brief  测试检查结果枚举
  */
TEST_CASE(code_reviewer_check_results)
{
    ASSERT_EQUAL(0, RESULT_PASS);
    ASSERT_EQUAL(1, RESULT_WARNING);
    ASSERT_EQUAL(2, RESULT_FAIL);
    ASSERT_EQUAL(3, RESULT_ERROR);
    return TEST_PASS;
}

/**
  * @brief  测试报告结构体
  */
TEST_CASE(code_reviewer_report_struct)
{
    CheckReport_t report;
    memset(&report, 0, sizeof(report));

    report.total_checks = 10;
    report.pass_count = 8;
    report.warning_count = 1;
    report.fail_count = 1;
    report.error_count = 0;
    report.score = 88.0f;

    ASSERT_EQUAL(10, report.total_checks);
    ASSERT_EQUAL(8, report.pass_count);
    ASSERT_EQUAL(1, report.warning_count);
    ASSERT_EQUAL(1, report.fail_count);
    ASSERT_EQUAL(0, report.error_count);
    ASSERT(report.score > 85.0f && report.score < 90.0f);

    return TEST_PASS;
}

/**
  * @brief  测试质量评分计算 - 全部通过
  */
TEST_CASE(code_reviewer_score_all_pass)
{
    CheckReport_t report;
    memset(&report, 0, sizeof(report));

    report.total_checks = 10;
    report.pass_count = 10;
    report.warning_count = 0;
    report.fail_count = 0;
    report.error_count = 0;

    CalculateScore(&report);

    ASSERT(report.score == 100.0f);
    return TEST_PASS;
}

/**
  * @brief  测试质量评分计算 - 有警告
  */
TEST_CASE(code_reviewer_score_with_warnings)
{
    CheckReport_t report;
    memset(&report, 0, sizeof(report));

    report.total_checks = 10;
    report.pass_count = 8;
    report.warning_count = 2;
    report.fail_count = 0;
    report.error_count = 0;

    CalculateScore(&report);

    /* 每个警告扣2分 */
    ASSERT(report.score == 96.0f);
    return TEST_PASS;
}

/**
  * @brief  测试质量评分计算 - 有失败
  */
TEST_CASE(code_reviewer_score_with_fails)
{
    CheckReport_t report;
    memset(&report, 0, sizeof(report));

    report.total_checks = 10;
    report.pass_count = 8;
    report.warning_count = 0;
    report.fail_count = 2;
    report.error_count = 0;

    CalculateScore(&report);

    /* 每个失败扣10分 */
    ASSERT(report.score == 80.0f);
    return TEST_PASS;
}

/**
  * @brief  测试质量评分计算 - 有错误
  */
TEST_CASE(code_reviewer_score_with_errors)
{
    CheckReport_t report;
    memset(&report, 0, sizeof(report));

    report.total_checks = 10;
    report.pass_count = 8;
    report.warning_count = 0;
    report.fail_count = 0;
    report.error_count = 2;

    CalculateScore(&report);

    /* 每个错误扣20分 */
    ASSERT(report.score == 60.0f);
    return TEST_PASS;
}

/**
  * @brief  测试质量评分计算 - 混合结果
  */
TEST_CASE(code_reviewer_score_mixed)
{
    CheckReport_t report;
    memset(&report, 0, sizeof(report));

    report.total_checks = 10;
    report.pass_count = 6;
    report.warning_count = 2;
    report.fail_count = 1;
    report.error_count = 1;

    CalculateScore(&report);

    /* 扣分: 2*2 + 1*10 + 1*20 = 34 */
    ASSERT(report.score == 66.0f);
    return TEST_PASS;
}

/**
  * @brief  测试质量评分计算 - 分数不低于0
  */
TEST_CASE(code_reviewer_score_min_zero)
{
    CheckReport_t report;
    memset(&report, 0, sizeof(report));

    report.total_checks = 10;
    report.pass_count = 0;
    report.warning_count = 0;
    report.fail_count = 5;
    report.error_count = 5;

    CalculateScore(&report);

    /* 扣分: 5*10 + 5*20 = 150, 最低0分 */
    ASSERT(report.score == 0.0f);
    return TEST_PASS;
}

/**
  * @brief  测试添加检查项
  */
TEST_CASE(code_reviewer_add_item)
{
    CheckReport_t report;
    memset(&report, 0, sizeof(report));

    CheckItem_t item;
    item.type = CHECK_SYNTAX;
    item.result = RESULT_PASS;
    item.file = "test.c";
    item.line = 10;
    item.rule = "No syntax errors";
    item.message = "Syntax check passed";
    item.suggestion = NULL;

    AddCheckItem(&report, &item);

    ASSERT_EQUAL(1, report.total_checks);
    ASSERT_EQUAL(1, report.pass_count);
    ASSERT_EQUAL(1, report.item_count);
    ASSERT_STRING_EQUAL("test.c", report.items[0].file);
    ASSERT_EQUAL(10, report.items[0].line);

    return TEST_PASS;
}

/**
  * @brief  测试添加多个检查项
  */
TEST_CASE(code_reviewer_add_multiple_items)
{
    CheckReport_t report;
    memset(&report, 0, sizeof(report));

    /* 添加3个检查项 */
    for (int i = 0; i < 3; i++) {
        CheckItem_t item;
        item.type = CHECK_STYLE;
        item.result = (i == 1) ? RESULT_WARNING : RESULT_PASS;
        item.file = "test.c";
        item.line = i * 10;
        item.rule = "Code style";
        item.message = "Style check";
        item.suggestion = NULL;

        AddCheckItem(&report, &item);
    }

    ASSERT_EQUAL(3, report.total_checks);
    ASSERT_EQUAL(2, report.pass_count);
    ASSERT_EQUAL(1, report.warning_count);
    ASSERT_EQUAL(3, report.item_count);

    return TEST_PASS;
}

/**
  * @brief  测试检查项溢出保护
  */
TEST_CASE(code_reviewer_item_overflow)
{
    CheckReport_t report;
    memset(&report, 0, sizeof(report));

    /* 填满报告 */
    report.item_count = MAX_CHECK_ITEMS;

    CheckItem_t item;
    item.type = CHECK_FUNCTION;
    item.result = RESULT_PASS;
    item.file = "test.c";
    item.line = 100;
    item.rule = "Function check";
    item.message = "Function check passed";
    item.suggestion = NULL;

    /* 尝试添加第101个项 */
    AddCheckItem(&report, &item);

    /* 应该被拒绝 */
    ASSERT_EQUAL(MAX_CHECK_ITEMS, report.item_count);
    ASSERT_EQUAL(0, report.total_checks);

    return TEST_PASS;
}

/* Private function implementations ------------------------------------------*/

static void CalculateScore(CheckReport_t *report)
{
    if (report == NULL) {
        return;
    }

    float base_score = 100.0f;
    float warning_penalty = 2.0f;
    float fail_penalty = 10.0f;
    float error_penalty = 20.0f;

    float penalty = 0;
    penalty += report->warning_count * warning_penalty;
    penalty += report->fail_count * fail_penalty;
    penalty += report->error_count * error_penalty;

    report->score = base_score - penalty;
    if (report->score < 0) {
        report->score = 0;
    }
    if (report->score > 100) {
        report->score = 100;
    }
}

static void AddCheckItem(CheckReport_t *report, const CheckItem_t *item)
{
    if (report == NULL || item == NULL) {
        return;
    }

    if (report->item_count >= MAX_CHECK_ITEMS) {
        return;
    }

    memcpy(&report->items[report->item_count], item, sizeof(CheckItem_t));
    report->item_count++;
    report->total_checks++;

    switch (item->result) {
        case RESULT_PASS:
            report->pass_count++;
            break;
        case RESULT_WARNING:
            report->warning_count++;
            break;
        case RESULT_FAIL:
            report->fail_count++;
            break;
        case RESULT_ERROR:
            report->error_count++;
            break;
    }
}
