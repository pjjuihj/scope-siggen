/**
  ******************************************************************************
  * @file           : code_reviewer.h
  * @brief          : 代码审查模块接口
  ******************************************************************************
  * @attention
  *
  * 代码审查模块，用于检查代码质量和设计符合性
  *
  ******************************************************************************
  */

#ifndef __CODE_REVIEWER_H
#define __CODE_REVIEWER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "error_tracker.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief 检查项类型
  */
typedef enum {
    CHECK_SYNTAX = 0,       /* 语法检查 */
    CHECK_STYLE,            /* 代码风格 */
    CHECK_FUNCTION,         /* 功能实现 */
    CHECK_COMPLETENESS,     /* 完整性检查 */
    CHECK_PERFORMANCE,      /* 性能检查 */
    CHECK_SECURITY,         /* 安全检查 */
    CHECK_MEMORY,           /* 内存检查 */
    CHECK_INTERRUPT,        /* 中断安全 */
    CHECK_THREAD,           /* 线程安全 */
    CHECK_DESIGN            /* 设计符合性 */
} CheckType_t;

/**
  * @brief 检查结果
  */
typedef enum {
    RESULT_PASS = 0,        /* 通过 */
    RESULT_WARNING,         /* 警告 */
    RESULT_FAIL,            /* 失败 */
    RESULT_ERROR            /* 错误 */
} CheckResult_t;

/**
  * @brief 检查项结构体
  */
typedef struct {
    CheckType_t type;       /* 检查类型 */
    CheckResult_t result;   /* 检查结果 */
    const char *file;       /* 文件名 */
    uint32_t line;          /* 行号 */
    const char *rule;       /* 规则描述 */
    const char *message;    /* 详细信息 */
    const char *suggestion; /* 修改建议 */
} CheckItem_t;

/**
  * @brief 检查报告结构体
  */
typedef struct {
    uint32_t total_checks;  /* 总检查数 */
    uint32_t pass_count;    /* 通过数 */
    uint32_t warning_count; /* 警告数 */
    uint32_t fail_count;    /* 失败数 */
    uint32_t error_count;   /* 错误数 */
    float score;            /* 质量评分 (0-100) */
    CheckItem_t items[100]; /* 检查项列表 */
    uint32_t item_count;    /* 检查项数量 */
} CheckReport_t;

/* Exported constants --------------------------------------------------------*/

/* 最大检查项数量 */
#define MAX_CHECK_ITEMS     100

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  初始化代码审查模块
  * @retval 错误码
  */
ErrorCode_t CodeReviewer_Init(void);

/**
  * @brief  检查文件
  * @param  file: 文件路径
  * @param  report: 检查报告指针
  * @retval 错误码
  */
ErrorCode_t CodeReviewer_CheckFile(const char *file, CheckReport_t *report);

/**
  * @brief  检查项目
  * @param  dir: 项目目录
  * @param  report: 检查报告指针
  * @retval 错误码
  */
ErrorCode_t CodeReviewer_CheckProject(const char *dir, CheckReport_t *report);

/**
  * @brief  运行完整检查
  * @param  report: 检查报告指针
  * @retval 错误码
  */
ErrorCode_t CodeReviewer_RunFullCheck(CheckReport_t *report);

/**
  * @brief  打印检查报告
  * @param  report: 检查报告指针
  * @retval None
  */
void CodeReviewer_PrintReport(const CheckReport_t *report);

/**
  * @brief  计算质量评分
  * @param  report: 检查报告指针
  * @retval None
  */
void CodeReviewer_CalculateScore(CheckReport_t *report);

/**
  * @brief  添加检查项
  * @param  report: 检查报告指针
  * @param  item: 检查项指针
  * @retval None
  */
void CodeReviewer_AddCheckItem(CheckReport_t *report, const CheckItem_t *item);

#ifdef __cplusplus
}
#endif

#endif /* __CODE_REVIEWER_H */
