/**
  ******************************************************************************
  * @file           : config.c
  * @brief          : 配置管理模块实现
  ******************************************************************************
  * @attention
  *
  * 配置管理模块，用于配置参数管理
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "error_tracker.h"
#include "debug.h"
#include "stm32f4xx_hal_flash.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Flash 配置存储区域（使用 Sector 11，128KB） */
#define CONFIG_FLASH_SECTOR      FLASH_SECTOR_11
#define CONFIG_FLASH_ADDRESS     0x080E0000U
#define CONFIG_FLASH_MAGIC       0x434F4E46U  /* "CONF" */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* 应用配置 */
static AppConfig_t app_config;

/* 初始化标志 */
static bool initialized = false;

/* Private function prototypes -----------------------------------------------*/

static uint32_t Config_CalculateChecksum(const AppConfig_t *config);
static bool Config_VerifyChecksum(const AppConfig_t *config);
static HAL_StatusTypeDef Config_FlashErase(void);
static HAL_StatusTypeDef Config_FlashWrite(const uint32_t *data, uint32_t len);
static void Config_FlashRead(uint32_t *data, uint32_t len);

/* Private user code ---------------------------------------------------------*/

/* Exported function implementations -----------------------------------------*/

ErrorCode_t Config_Init(void)
{
    LOG_INFO("Initializing config module...");

    /* 加载默认配置 */
    Config_LoadDefaults();

    /* 尝试从Flash加载配置 */
    ErrorCode_t err = Config_Load();
    if (err != ERR_OK) {
        LOG_WARNING("Failed to load config from flash, using defaults");
        Config_LoadDefaults();
    }

    initialized = true;
    LOG_INFO("Config module initialized");
    return ERR_OK;
}

/**
  * @brief  擦除配置 Flash 扇区
  * @retval HAL 状态
  */
static HAL_StatusTypeDef Config_FlashErase(void)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase_init;
    uint32_t sector_error = 0;

    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.Sector = CONFIG_FLASH_SECTOR;
    erase_init.NbSectors = 1;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;  /* 2.7V - 3.6V */

    HAL_FLASH_Unlock();
    status = HAL_FLASHEx_Erase(&erase_init, &sector_error);
    HAL_FLASH_Lock();

    if (status != HAL_OK) {
        LOG_ERROR("Flash erase failed: %lu", sector_error);
    }

    return status;
}

/**
  * @brief  写入数据到 Flash
  * @param  data: 数据指针（32位对齐）
  * @param  len: 数据长度（字节数）
  * @retval HAL 状态
  */
static HAL_StatusTypeDef Config_FlashWrite(const uint32_t *data, uint32_t len)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint32_t address = CONFIG_FLASH_ADDRESS;
    uint32_t words = (len + 3) / 4;  /* 向上取整到 4 字节 */

    HAL_FLASH_Unlock();

    for (uint32_t i = 0; i < words; i++) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data[i]);
        if (status != HAL_OK) {
            LOG_ERROR("Flash write failed at 0x%08lX", address);
            break;
        }
        address += 4;
    }

    HAL_FLASH_Lock();

    return status;
}

/**
  * @brief  从 Flash 读取数据
  * @param  data: 数据缓冲区指针
  * @param  len: 数据长度（字节数）
  * @retval None
  */
static void Config_FlashRead(uint32_t *data, uint32_t len)
{
    const uint32_t *src = (const uint32_t *)CONFIG_FLASH_ADDRESS;
    uint32_t words = (len + 3) / 4;

    for (uint32_t i = 0; i < words; i++) {
        data[i] = src[i];
    }
}

ErrorCode_t Config_Load(void)
{
    if (!initialized) {
        return ERR_NOT_INIT;
    }

    /* 从 Flash 读取配置 */
    AppConfig_t flash_config;
    Config_FlashRead((uint32_t *)&flash_config, sizeof(AppConfig_t));

    /* 验证魔数 */
    if (flash_config.header.magic != CONFIG_FLASH_MAGIC) {
        LOG_WARNING("Config magic mismatch (flash empty or corrupt)");
        return ERR_NOT_SUPPORTED;
    }

    /* 验证版本 */
    if (flash_config.header.version != CONFIG_VERSION) {
        LOG_WARNING("Config version mismatch: %lu -> %d",
                    flash_config.header.version, CONFIG_VERSION);
        return ERR_NOT_SUPPORTED;
    }

    /* 验证校验和 */
    if (!Config_VerifyChecksum(&flash_config)) {
        LOG_WARNING("Config checksum mismatch");
        return ERR_NOT_SUPPORTED;
    }

    /* 配置有效，加载 */
    memcpy(&app_config, &flash_config, sizeof(AppConfig_t));
    LOG_INFO("Config loaded from Flash");
    return ERR_OK;
}

ErrorCode_t Config_Save(void)
{
    if (!initialized) {
        return ERR_NOT_INIT;
    }

    /* 更新校验和 */
    app_config.header.magic = CONFIG_FLASH_MAGIC;
    app_config.header.version = CONFIG_VERSION;
    app_config.header.length = sizeof(AppConfig_t);
    app_config.header.checksum = Config_CalculateChecksum(&app_config);

    /* 擦除 Flash 扇区 */
    if (Config_FlashErase() != HAL_OK) {
        LOG_ERROR("Failed to erase Flash");
        return ERR_HARDWARE;
    }

    /* 写入配置到 Flash */
    if (Config_FlashWrite((const uint32_t *)&app_config, sizeof(AppConfig_t)) != HAL_OK) {
        LOG_ERROR("Failed to write config to Flash");
        return ERR_HARDWARE;
    }

    LOG_INFO("Config saved to Flash");
    return ERR_OK;
}

ErrorCode_t Config_LoadDefaults(void)
{
    /* 清空配置 */
    memset(&app_config, 0, sizeof(AppConfig_t));

    /* 配置头 */
    app_config.header.magic = CONFIG_FLASH_MAGIC;
    app_config.header.version = CONFIG_VERSION;
    app_config.header.length = sizeof(AppConfig_t);

    /* 示波器默认配置 */
    app_config.osc.sample_rate = OSC_DEFAULT_SAMPLE_RATE;
    app_config.osc.buffer_size = OSC_DEFAULT_BUFFER_SIZE;
    app_config.osc.trigger_level = OSC_DEFAULT_TRIGGER_LEVEL;
    app_config.osc.trigger_edge = OSC_DEFAULT_TRIGGER_EDGE;
    app_config.osc.enabled = 1;

    /* 信号发生器默认配置 */
    app_config.siggen.frequency = SIGGEN_DEFAULT_FREQUENCY;
    app_config.siggen.amplitude = SIGGEN_DEFAULT_AMPLITUDE;
    app_config.siggen.waveform = SIGGEN_DEFAULT_WAVEFORM;
    app_config.siggen.duty_cycle = SIGGEN_DEFAULT_DUTY_CYCLE;
    app_config.siggen.enabled = 1;

    /* 系统默认配置 */
    app_config.sys.uart_baudrate = 115200;
    app_config.sys.display_brightness = 128;
    app_config.sys.watchdog_timeout = 30;
    app_config.sys.log_level = LOG_INFO;

    /* 关机默认配置 */
    app_config.shutdown.auto_save = 1;
    app_config.shutdown.confirm_shutdown = 0;
    app_config.shutdown.safe_state = 1;

    /* 计算校验和 */
    app_config.header.checksum = Config_CalculateChecksum(&app_config);

    LOG_INFO("Default config loaded");
    return ERR_OK;
}

AppConfig_t* Config_Get(void)
{
    return &app_config;
}

ErrorCode_t Config_SetOscConfig(const OscConfig_t *config)
{
    if (config == NULL) {
        return ERR_INVALID_PARAM;
    }

    memcpy(&app_config.osc, config, sizeof(OscConfig_t));
    LOG_INFO("Osc config updated");
    return ERR_OK;
}

ErrorCode_t Config_GetOscConfig(OscConfig_t *config)
{
    if (config == NULL) {
        return ERR_INVALID_PARAM;
    }

    memcpy(config, &app_config.osc, sizeof(OscConfig_t));
    return ERR_OK;
}

ErrorCode_t Config_SetSigGenConfig(const SigGenConfig_t *config)
{
    if (config == NULL) {
        return ERR_INVALID_PARAM;
    }

    memcpy(&app_config.siggen, config, sizeof(SigGenConfig_t));
    LOG_INFO("SigGen config updated");
    return ERR_OK;
}

ErrorCode_t Config_GetSigGenConfig(SigGenConfig_t *config)
{
    if (config == NULL) {
        return ERR_INVALID_PARAM;
    }

    memcpy(config, &app_config.siggen, sizeof(SigGenConfig_t));
    return ERR_OK;
}

/* Private function implementations ------------------------------------------*/

/**
  * @brief  计算配置校验和
  * @param  config: 配置结构体指针
  * @retval 校验和
  */
static uint32_t Config_CalculateChecksum(const AppConfig_t *config)
{
    if (config == NULL) {
        return 0;
    }

    uint32_t checksum = 0;
    const uint8_t *data = (const uint8_t *)config;

    /* 跳过校验和字段 */
    for (uint32_t i = 0; i < sizeof(AppConfig_t); i++) {
        if (i < offsetof(AppConfig_t, header.checksum) ||
            i >= offsetof(AppConfig_t, header.checksum) + sizeof(uint32_t)) {
            checksum += data[i];
        }
    }

    return checksum;
}

/**
  * @brief  验证配置校验和
  * @param  config: 配置结构体指针
  * @retval true=有效, false=无效
  */
static bool Config_VerifyChecksum(const AppConfig_t *config)
{
    if (config == NULL) {
        return false;
    }

    uint32_t calculated = Config_CalculateChecksum(config);
    return (calculated == config->header.checksum);
}
