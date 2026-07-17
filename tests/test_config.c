/**
  ******************************************************************************
  * @file           : test_config.c
  * @brief          : 配置管理模块测试
  ******************************************************************************
  */

#include "test_framework.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Mock 配置结构 */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t length;
    uint32_t checksum;
} MockConfigHeader_t;

typedef struct {
    MockConfigHeader_t header;
    uint32_t sample_rate;
    uint32_t buffer_size;
    uint32_t frequency;
    uint32_t amplitude;
    uint32_t waveform;
} MockAppConfig_t;

/* Mock 变量 */
static MockAppConfig_t mock_config;
static bool mock_initialized = false;
static bool mock_flash_loaded = false;
static bool mock_flash_saved = false;

/* Mock 函数 */
static void Mock_Config_LoadDefaults(void)
{
    memset(&mock_config, 0, sizeof(MockAppConfig_t));
    mock_config.header.magic = 0x434F4E46;  /* "CONF" */
    mock_config.header.version = 1;
    mock_config.header.length = sizeof(MockAppConfig_t);
    mock_config.sample_rate = 1000000;
    mock_config.buffer_size = 1024;
    mock_config.frequency = 1000;
    mock_config.amplitude = 1650;
    mock_config.waveform = 0;
}

static int Mock_Config_Load(void)
{
    if (!mock_flash_loaded) {
        return -1;  /* Flash 为空 */
    }
    return 0;  /* 加载成功 */
}

static int Mock_Config_Save(void)
{
    if (!mock_initialized) {
        return -1;  /* 未初始化 */
    }
    mock_flash_saved = true;
    return 0;  /* 保存成功 */
}

static void Mock_Config_Init(void)
{
    Mock_Config_LoadDefaults();
    mock_initialized = true;

    if (Mock_Config_Load() != 0) {
        /* Flash 加载失败，保存默认配置 */
        Mock_Config_Save();
    }
}

/* Test Cases */

/**
  * @brief  测试配置加载默认值
  */
TEST_CASE(config_load_defaults)
{
    Mock_Config_LoadDefaults();

    ASSERT_EQUAL(0x434F4E46, mock_config.header.magic);
    ASSERT_EQUAL(1, mock_config.header.version);
    ASSERT_EQUAL(1000000, mock_config.sample_rate);
    ASSERT_EQUAL(1024, mock_config.buffer_size);
    ASSERT_EQUAL(1000, mock_config.frequency);
    ASSERT_EQUAL(1650, mock_config.amplitude);
    ASSERT_EQUAL(0, mock_config.waveform);

    return TEST_PASS;
}

/**
  * @brief  测试配置初始化（Flash 为空）
  */
TEST_CASE(config_init_flash_empty)
{
    mock_flash_loaded = false;
    mock_flash_saved = false;
    mock_initialized = false;

    Mock_Config_Init();

    ASSERT_EQUAL(1, mock_initialized);
    ASSERT_EQUAL(1, mock_flash_saved);  /* 应该保存默认配置 */

    return TEST_PASS;
}

/**
  * @brief  测试配置初始化（Flash 有数据）
  */
TEST_CASE(config_init_flash_loaded)
{
    mock_flash_loaded = true;
    mock_flash_saved = false;
    mock_initialized = false;

    Mock_Config_Init();

    ASSERT_EQUAL(1, mock_initialized);
    ASSERT_EQUAL(0, mock_flash_saved);  /* 不应该保存 */

    return TEST_PASS;
}

/**
  * @brief  测试配置保存
  */
TEST_CASE(config_save)
{
    mock_initialized = true;
    mock_flash_saved = false;

    int result = Mock_Config_Save();

    ASSERT_EQUAL(0, result);
    ASSERT_EQUAL(1, mock_flash_saved);

    return TEST_PASS;
}

/**
  * @brief  测试配置保存（未初始化）
  */
TEST_CASE(config_save_not_initialized)
{
    mock_initialized = false;
    mock_flash_saved = false;

    int result = Mock_Config_Save();

    ASSERT_EQUAL(-1, result);
    ASSERT_EQUAL(0, mock_flash_saved);

    return TEST_PASS;
}
