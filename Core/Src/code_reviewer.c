/**
  ******************************************************************************
  * @file           : code_reviewer.c
  * @brief          : 代码审查模块实现
  ******************************************************************************
  * @attention
  *
  * 代码审查模块，用于检查代码质量和设计符合性
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "code_reviewer.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* 初始化标志 */
static bool initialized = false;

/* Private function prototypes -----------------------------------------------*/

static ErrorCode_t CodeReviewer_CheckDesign(const CheckReport_t *report);

/* Private user code ---------------------------------------------------------*/

/* Exported function implementations -----------------------------------------*/

ErrorCode_t CodeReviewer_Init(void)
{
    LOG_INFO("Initializing code reviewer module...");

    initialized = true;
    LOG_INFO("Code reviewer module initialized");
    return ERR_OK;
}

ErrorCode_t CodeReviewer_CheckFile(const char *file, CheckReport_t *report)
{
    if (!initialized || file == NULL || report == NULL) {
        return ERR_INVALID_PARAM;
    }

    LOG_INFO("Checking file: %s", file);

    /* TODO: 实现文件检查逻辑 */
    /* 这里需要根据实际的检查规则实现 */

    return ERR_OK;
}

ErrorCode_t CodeReviewer_CheckProject(const char *dir, CheckReport_t *report)
{
    if (!initialized || dir == NULL || report == NULL) {
        return ERR_INVALID_PARAM;
    }

    LOG_INFO("Checking project: %s", dir);

    /* TODO: 实现项目检查逻辑 */
    /* 遍历项目目录，检查所有源文件 */

    return ERR_OK;
}

ErrorCode_t CodeReviewer_RunFullCheck(CheckReport_t *report)
{
    if (!initialized || report == NULL) {
        return ERR_INVALID_PARAM;
    }

    LOG_INFO("Running full code review...");

    /* 清空报告 */
    memset(report, 0, sizeof(CheckReport_t));

    /* 检查设计符合性 */
    CodeReviewer_CheckDesign(report);

    /* 计算质量评分 */
    CodeReviewer_CalculateScore(report);

    LOG_INFO("Code review completed: %lu checks, score: %.1f",
             report->total_checks, report->score);

    return ERR_OK;
}

void CodeReviewer_PrintReport(const CheckReport_t *report)
{
    if (report == NULL) {
        return;
    }

    LOG_INFO("=== Code Review Report ===");
    LOG_INFO("Total checks: %lu", report->total_checks);
    LOG_INFO("Pass: %lu", report->pass_count);
    LOG_INFO("Warnings: %lu", report->warning_count);
    LOG_INFO("Failures: %lu", report->fail_count);
    LOG_INFO("Errors: %lu", report->error_count);
    LOG_INFO("Score: %.1f/100", report->score);

    /* 打印详细信息 */
    for (uint32_t i = 0; i < report->item_count; i++) {
        const CheckItem_t *item = &report->items[i];

        const char *result_str = "UNKNOWN";
        switch (item->result) {
            case RESULT_PASS: result_str = "PASS"; break;
            case RESULT_WARNING: result_str = "WARN"; break;
            case RESULT_FAIL: result_str = "FAIL"; break;
            case RESULT_ERROR: result_str = "ERROR"; break;
        }

        LOG_INFO("[%s] %s at %s:%lu - %s",
                 result_str, item->rule, item->file, item->line, item->message);

        if (item->suggestion != NULL) {
            LOG_INFO("  Suggestion: %s", item->suggestion);
        }
    }

    LOG_INFO("=== End of Report ===");
}

void CodeReviewer_CalculateScore(CheckReport_t *report)
{
    if (report == NULL) {
        return;
    }

    /* 计算基础分 */
    float base_score = 100.0f;

    /* 扣分规则 */
    float warning_penalty = 2.0f;
    float fail_penalty = 10.0f;
    float error_penalty = 20.0f;

    /* 计算扣分 */
    float penalty = 0;
    penalty += report->warning_count * warning_penalty;
    penalty += report->fail_count * fail_penalty;
    penalty += report->error_count * error_penalty;

    /* 计算最终分数 */
    report->score = base_score - penalty;
    if (report->score < 0) {
        report->score = 0;
    }
    if (report->score > 100) {
        report->score = 100;
    }
}

void CodeReviewer_AddCheckItem(CheckReport_t *report, const CheckItem_t *item)
{
    if (report == NULL || item == NULL) {
        return;
    }

    if (report->item_count >= MAX_CHECK_ITEMS) {
        LOG_WARNING("Check items overflow");
        return;
    }

    /* 添加检查项 */
    memcpy(&report->items[report->item_count], item, sizeof(CheckItem_t));
    report->item_count++;
    report->total_checks++;

    /* 更新统计 */
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

/* Private function implementations ------------------------------------------*/

/**
  * @brief  检查设计符合性
  * @param  report: 检查报告指针
  * @retval 错误码
  */
static ErrorCode_t CodeReviewer_CheckDesign(const CheckReport_t *report)
{
    /* TODO: 实现设计符合性检查 */
    /* 检查是否符合设计文档中的规范 */

    return ERR_OK;
}
