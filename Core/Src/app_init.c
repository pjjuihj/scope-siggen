/**
  ******************************************************************************
  * @file           : app_init.c
  * @brief          : 应用初始化模块实现
  ******************************************************************************
  * @attention
  *
  * 应用初始化模块，统一管理所有模块的初始化
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "app_init.h"
#include "version.h"
#include "debug.h"
#include "error_tracker.h"
#include "oscilloscope.h"
#include "signal_gen.h"
#include "uart_protocol.h"
#include "display.h"
#include "key_handler.h"
#include "config.h"
#include "cmsis_os.h"

/* 外部变量声明 */
extern DAC_HandleTypeDef hdac;

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* 当前启动阶段 */
static BootStage_t current_boot_stage = BOOT_STAGE_INIT;

/* 就绪标志 */
static bool app_ready = false;

/* 任务句柄 */
osThreadId_t app_init_task_handle;

/* Private function prototypes -----------------------------------------------*/

static ErrorCode_t App_SelfTest(void);
static ErrorCode_t App_LoadConfig(void);
static ErrorCode_t App_StartModules(void);
static void App_KeyCallback(KeyCode_t key);

/* Private variables ---------------------------------------------------------*/

/* 当前缩放级别 */
static uint8_t current_zoom = 4;

/* Private user code ---------------------------------------------------------*/

/* Exported function implementations -----------------------------------------*/

BootStage_t App_Init(void)
{
    LOG_INFO("=== Application Init ===");

    /* 阶段1: 初始化 */
    current_boot_stage = BOOT_STAGE_INIT;
    LOG_INFO("Stage 1: Init");

    /* 阶段2: 自检 */
    current_boot_stage = BOOT_STAGE_SELFTEST;
    LOG_INFO("Stage 2: Self test");

    ErrorCode_t err = App_SelfTest();
    if (err != ERR_OK) {
        LOG_ERROR("Self test failed");
        current_boot_stage = BOOT_STAGE_ERROR;
        return current_boot_stage;
    }

    /* 阶段3: 加载配置 */
    current_boot_stage = BOOT_STAGE_CONFIG;
    LOG_INFO("Stage 3: Load config");

    err = App_LoadConfig();
    if (err != ERR_OK) {
        LOG_WARNING("Failed to load config, using defaults");
        Config_LoadDefaults();
    }

    /* 阶段4: 启动模块 */
    current_boot_stage = BOOT_STAGE_START;
    LOG_INFO("Stage 4: Start modules");

    err = App_StartModules();
    if (err != ERR_OK) {
        LOG_ERROR("Failed to start modules");
        current_boot_stage = BOOT_STAGE_ERROR;
        return current_boot_stage;
    }

    /* 阶段5: 显示启动信息 */
    Display_ShowStatus("Ready");

    /* 阶段6: 就绪 */
    current_boot_stage = BOOT_STAGE_READY;
    app_ready = true;

    LOG_INFO("=== Application Ready ===");

    return current_boot_stage;
}

BootStage_t App_GetBootStage(void)
{
    return current_boot_stage;
}

bool App_IsReady(void)
{
    return app_ready;
}

void App_EnterFallbackMode(void)
{
    LOG_WARNING("Entering fallback mode");

    /* 只启动基本功能 */
    Debug_Init();
    ErrorTracker_Init();

    /* 禁用高级功能 */
    /* TODO: 禁用示波器、信号发生器等 */

    Display_ShowMessage("Fallback Mode");
    Display_ForceUpdate();

    current_boot_stage = BOOT_STAGE_READY;
    app_ready = true;
}

void App_Shutdown(void)
{
    LOG_INFO("=== Shutdown Sequence ===");

    /* 阶段1: 保存配置 */
    LOG_INFO("Stage 1: Saving config...");
    Display_ShowMessage("Shutting Down...");
    Display_ForceUpdate();

    if (Config_Save() != ERR_OK) {
        LOG_WARNING("Failed to save config");
    }

    /* 阶段2: 停止示波器 */
    LOG_INFO("Stage 2: Stopping oscilloscope...");
    Oscilloscope_Stop();

    /* 阶段3: 停止信号发生器 */
    LOG_INFO("Stage 3: Stopping signal generator...");
    SignalGen_Stop();

    /* 阶段4: 设置安全状态 */
    LOG_INFO("Stage 4: Setting safe state...");

    /* DAC 输出 0 */
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);
    HAL_DAC_Stop(&hdac, DAC_CHANNEL_1);

    /* LED 关闭 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);

    /* 阶段5: 显示关机信息 */
    Display_ShowMessage("Safe to Power Off");
    Display_ForceUpdate();

    LOG_INFO("=== Shutdown Complete ===");
    LOG_INFO("Safe to remove power");

    /* 标记为未就绪 */
    app_ready = false;
}

const char* App_GetBootStageString(BootStage_t stage)
{
    switch (stage) {
        case BOOT_STAGE_INIT:       return "Init";
        case BOOT_STAGE_SELFTEST:   return "Self Test";
        case BOOT_STAGE_CONFIG:     return "Load Config";
        case BOOT_STAGE_START:      return "Start Modules";
        case BOOT_STAGE_READY:      return "Ready";
        case BOOT_STAGE_ERROR:      return "Error";
        default:                    return "Unknown";
    }
}

void App_Init_Task(void *argument)
{
    LOG_INFO("App init task started");

    /* 执行初始化 */
    BootStage_t stage = App_Init();

    if (stage == BOOT_STAGE_ERROR) {
        LOG_ERROR("Init failed, entering fallback mode");
        App_EnterFallbackMode();
    }

    /* 任务完成，删除自己 */
    osThreadTerminate(NULL);
}

/* Private function implementations ------------------------------------------*/

/**
  * @brief  自检
  * @retval 错误码
  */
static ErrorCode_t App_SelfTest(void)
{
    LOG_INFO("Running self test...");

    /* 检查版本模块 */
    const VersionInfo_t *ver = Version_GetInfo();
    if (ver == NULL) {
        LOG_ERROR("Version module failed");
        return ERR_NOT_INIT;
    }
    LOG_INFO("Version: %s", ver->version_string);

    /* 检查错误追踪模块 */
    ErrorCode_t err = ErrorTracker_Init();
    if (err != ERR_OK) {
        LOG_ERROR("Error tracker init failed");
        return err;
    }

    /* 检查调试模块 */
    err = Debug_Init();
    if (err != ERR_OK) {
        LOG_ERROR("Debug init failed");
        return err;
    }

    /* 检查配置模块 */
    err = Config_Init();
    if (err != ERR_OK) {
        LOG_ERROR("Config init failed");
        return err;
    }

    LOG_INFO("Self test passed");
    return ERR_OK;
}

/**
  * @brief  加载配置
  * @retval 错误码
  */
static ErrorCode_t App_LoadConfig(void)
{
    LOG_INFO("Loading config...");

    ErrorCode_t err = Config_Load();
    if (err != ERR_OK) {
        LOG_WARNING("Failed to load config from flash");
        return err;
    }

    LOG_INFO("Config loaded");
    return ERR_OK;
}

/**
  * @brief  启动模块
  * @retval 错误码
  */
static ErrorCode_t App_StartModules(void)
{
    LOG_INFO("Starting modules...");

    /* 启动示波器模块 */
    ErrorCode_t err = Oscilloscope_Init();
    if (err != ERR_OK) {
        LOG_ERROR("Oscilloscope init failed");
        return err;
    }

    /* 启动信号发生器模块 */
    err = SignalGen_Init();
    if (err != ERR_OK) {
        LOG_ERROR("Signal generator init failed");
        return err;
    }

    /* 启动串口协议模块 */
    err = UART_Protocol_Init();
    if (err != ERR_OK) {
        LOG_ERROR("UART protocol init failed");
        return err;
    }

    /* 启动显示模块 */
    err = Display_Init();
    if (err != ERR_OK) {
        LOG_ERROR("Display init failed");
        return err;
    }

    /* 启动按键处理模块 */
    err = KeyHandler_Init();
    if (err != ERR_OK) {
        LOG_ERROR("Key handler init failed");
        return err;
    }

    /* 注册按键回调 */
    Key_RegisterCallback(App_KeyCallback);

    LOG_INFO("All modules started");
    return ERR_OK;
}

/**
  * @brief  按键回调函数
  * @param  key: 按键码
  * @retval None
  */
static void App_KeyCallback(KeyCode_t key)
{
    LOG_INFO("Key callback: %d", key);

    /* KEY_ENTER (PA0) */
    if (key == KEY_ENTER) {
        if (Display_GetCurrentPage() == PAGE_MENU) {
            if (Key_IsLastPressLong()) {
                /* 菜单页面长按：确认选择 */
                Display_SelectMenuItem();
                Display_LogOperation("Menu: SELECT");
            } else {
                /* 菜单页面短按：切换页面 */
                Display_NextPage();
                Display_LogOperation("Page switch");
            }
        } else {
            if (Key_IsLastPressLong()) {
                /* 长按：保存配置到 Flash */
                Config_Save();
                Display_ShowMessage("Config Saved");
                Display_LogOperation("Config saved");
                LOG_INFO("Config saved by key");
            } else {
                /* 短按：切换页面 */
                Display_NextPage();
                Display_LogOperation("Page switch");
                LOG_INFO("Page switched by key");
            }
        }
        return;
    }

    /* KEY_SELECT (PA1) = 菜单中移动选择 */
    if (key == KEY_SELECT) {
        if (Display_GetCurrentPage() == PAGE_MENU) {
            Display_UpdateSelection(Display_GetMenuSelect() + 1);
            Display_LogOperation("Menu: NEXT");
        }
        return;
    }
}
