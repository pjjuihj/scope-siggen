/**
  ******************************************************************************
  * @file           : test_version.c
  * @brief          : 版本管理模块测试
  ******************************************************************************
  * @attention
  *
  * 版本管理模块单元测试
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "test_framework.h"
#include "version.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Exported function implementations -----------------------------------------*/

TEST_CASE(version_get_info)
{
    const VersionInfo_t *info = Version_GetInfo();
    ASSERT_NOT_NULL(info);

    return TEST_PASS;
}

TEST_CASE(version_string)
{
    const VersionInfo_t *info = Version_GetInfo();
    ASSERT_NOT_NULL(info->version_string);
    ASSERT(strlen(info->version_string) > 0);

    return TEST_PASS;
}

TEST_CASE(version_build_date)
{
    const VersionInfo_t *info = Version_GetInfo();
    ASSERT_NOT_NULL(info->build_date);
    ASSERT(strlen(info->build_date) > 0);

    return TEST_PASS;
}

TEST_CASE(version_build_time)
{
    const VersionInfo_t *info = Version_GetInfo();
    ASSERT_NOT_NULL(info->build_time);
    ASSERT(strlen(info->build_time) > 0);

    return TEST_PASS;
}

TEST_CASE(version_compare_equal)
{
    VersionInfo_t v1 = {1, 0, 0, 100};
    VersionInfo_t v2 = {1, 0, 0, 100};

    int result = Version_Compare(&v1, &v2);
    ASSERT_EQUAL(0, result);

    return TEST_PASS;
}

TEST_CASE(version_compare_newer)
{
    VersionInfo_t v1 = {1, 1, 0, 100};
    VersionInfo_t v2 = {1, 0, 0, 100};

    int result = Version_Compare(&v1, &v2);
    ASSERT(result > 0);

    return TEST_PASS;
}

TEST_CASE(version_compare_older)
{
    VersionInfo_t v1 = {1, 0, 0, 100};
    VersionInfo_t v2 = {1, 1, 0, 100};

    int result = Version_Compare(&v1, &v2);
    ASSERT(result < 0);

    return TEST_PASS;
}

TEST_CASE(version_is_newer)
{
    VersionInfo_t v1 = {1, 1, 0, 100};
    VersionInfo_t v2 = {1, 0, 0, 100};

    bool result = Version_IsNewer(&v1, &v2);
    ASSERT(result);

    result = Version_IsNewer(&v2, &v1);
    ASSERT(!result);

    return TEST_PASS;
}

TEST_CASE(version_get_string)
{
    const char *str = Version_GetString();
    ASSERT_NOT_NULL(str);
    ASSERT(strlen(str) > 0);

    return TEST_PASS;
}

TEST_CASE(version_get_timestamp)
{
    const char *ts = Version_GetBuildTimestamp();
    ASSERT_NOT_NULL(ts);
    ASSERT(strlen(ts) > 0);

    return TEST_PASS;
}

/* Private function implementations ------------------------------------------*/
