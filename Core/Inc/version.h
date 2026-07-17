/**
  ******************************************************************************
  * @file           : version.h
  * @brief          : 版本管理模块接口
  ******************************************************************************
  * @attention
  *
  * 版本管理模块，用于管理固件版本信息
  *
  ******************************************************************************
  */

#ifndef __VERSION_H
#define __VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief 版本类型定义
  */
typedef enum {
    VERSION_TYPE_RELEASE = 0,   /*!< 正式版 */
    VERSION_TYPE_RC,            /*!< Release Candidate */
    VERSION_TYPE_BETA,          /*!< Beta版 */
    VERSION_TYPE_ALPHA,         /*!< Alpha版 */
    VERSION_TYPE_DEV            /*!< 开发版 */
} VersionType_t;

/**
  * @brief 版本信息结构体
  */
typedef struct {
    /* 版本号 */
    uint8_t major;              /*!< 主版本号 */
    uint8_t minor;              /*!< 次版本号 */
    uint8_t patch;              /*!< 补丁号 */
    uint16_t build;             /*!< 构建号 */

    /* 版本类型 */
    VersionType_t type;         /*!< 版本类型 */

    /* 字符串 */
    const char *version_string; /*!< 版本字符串，如 "1.0.0" */
    const char *build_date;     /*!< 编译日期 */
    const char *build_time;     /*!< 编译时间 */

    /* Git信息 */
    const char *git_commit;     /*!< Git提交哈希 */
    const char *git_branch;     /*!< Git分支名 */
    uint8_t git_dirty;          /*!< 是否有未提交的更改 (1=dirty, 0=clean) */

    /* 编译信息 */
    const char *compiler;       /*!< 编译器信息 */
    const char *target;         /*!< 目标芯片 */
} VersionInfo_t;

/* Exported constants --------------------------------------------------------*/

/* 版本号定义 */
#define VERSION_MAJOR      1
#define VERSION_MINOR      4
#define VERSION_PATCH      0
#define VERSION_BUILD      0

/* 版本字符串 */
#define VERSION_STRING     "1.4.1"

/* Git信息（由构建脚本自动填充） */
#define GIT_COMMIT         "20fa254"
#define GIT_BRANCH         "main"
#define GIT_DIRTY          0

/* 编译信息 */
#define BUILD_DATE         __DATE__
#define BUILD_TIME         __TIME__
#define COMPILER_INFO      "Keil MDK-ARM"
#define TARGET_INFO        "STM32F407VETx"

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  获取版本信息
  * @retval 指向版本信息结构体的指针
  */
const VersionInfo_t* Version_GetInfo(void);

/**
  * @brief  打印版本信息到串口
  * @retval None
  */
void Version_Print(void);

/**
  * @brief  检查是否有未提交的更改
  * @retval true=有未提交更改, false=无未提交更改
  */
bool Version_IsDirty(void);

/**
  * @brief  获取版本字符串
  * @retval 版本字符串指针
  */
const char* Version_GetString(void);

/**
  * @brief  获取编译时间戳
  * @retval 编译时间戳字符串指针
  */
const char* Version_GetBuildTimestamp(void);

/**
  * @brief  比较两个版本
  * @param  v1: 版本1
  * @param  v2: 版本2
  * @retval <0: v1<v2, 0: v1=v2, >0: v1>v2
  */
int Version_Compare(const VersionInfo_t *v1, const VersionInfo_t *v2);

/**
  * @brief  检查是否是新版本
  * @param  current: 当前版本
  * @param  other: 其他版本
  * @retval true: current比other新, false: current不比other新
  */
bool Version_IsNewer(const VersionInfo_t *current, const VersionInfo_t *other);

#ifdef __cplusplus
}
#endif

#endif /* __VERSION_H */
