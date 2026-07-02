/**
  ******************************************************************************
  * @file           : version.c
  * @brief          : 版本管理模块实现
  ******************************************************************************
  * @attention
  *
  * 版本管理模块，用于管理固件版本信息
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "version.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/**
  * @brief 版本信息实例
  */
static const VersionInfo_t g_version_info = {
    /* 版本号 */
    .major = VERSION_MAJOR,
    .minor = VERSION_MINOR,
    .patch = VERSION_PATCH,
    .build = VERSION_BUILD,

    /* 版本类型 */
    .type = VERSION_TYPE_DEV,

    /* 字符串 */
    .version_string = VERSION_STRING,
    .build_date = BUILD_DATE,
    .build_time = BUILD_TIME,

    /* Git信息 */
    .git_commit = GIT_COMMIT,
    .git_branch = GIT_BRANCH,
    .git_dirty = GIT_DIRTY,

    /* 编译信息 */
    .compiler = COMPILER_INFO,
    .target = TARGET_INFO
};

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Exported function implementations -----------------------------------------*/

const VersionInfo_t* Version_GetInfo(void)
{
    return &g_version_info;
}

void Version_Print(void)
{
    const VersionInfo_t *info = &g_version_info;

    /* 打印版本信息 */
    LOG_INFO("========================================");
    LOG_INFO("SCOPE-SIGGEN Firmware");
    LOG_INFO("========================================");
    LOG_INFO("Version: %s (build %d)", info->version_string, info->build);
    LOG_INFO("Built: %s %s", info->build_date, info->build_time);
    LOG_INFO("Git: %s@%s%s",
             info->git_branch,
             info->git_commit,
             info->git_dirty ? " (dirty)" : "");
    LOG_INFO("Compiler: %s", info->compiler);
    LOG_INFO("Target: %s", info->target);
    LOG_INFO("========================================");
}

bool Version_IsDirty(void)
{
    return g_version_info.git_dirty;
}

const char* Version_GetString(void)
{
    return g_version_info.version_string;
}

const char* Version_GetBuildTimestamp(void)
{
    static char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "%s %s",
             g_version_info.build_date,
             g_version_info.build_time);
    return timestamp;
}

int Version_Compare(const VersionInfo_t *v1, const VersionInfo_t *v2)
{
    if (v1 == NULL || v2 == NULL) {
        return 0;
    }

    /* 比较主版本号 */
    if (v1->major != v2->major) {
        return (int)v1->major - (int)v2->major;
    }

    /* 比较次版本号 */
    if (v1->minor != v2->minor) {
        return (int)v1->minor - (int)v2->minor;
    }

    /* 比较补丁号 */
    if (v1->patch != v2->patch) {
        return (int)v1->patch - (int)v2->patch;
    }

    /* 比较构建号 */
    return (int)v1->build - (int)v2->build;
}

bool Version_IsNewer(const VersionInfo_t *current, const VersionInfo_t *other)
{
    return Version_Compare(current, other) > 0;
}

/* Private function implementations ------------------------------------------*/

