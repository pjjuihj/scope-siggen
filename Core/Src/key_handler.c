/**
  ******************************************************************************
  * @file           : key_handler.c
  * @brief          : 按键处理模块实现
  ******************************************************************************
  * @attention
  *
  * 按键处理模块，用于按键扫描和处理
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "key_handler.h"
#include "debug.h"
#include "cmsis_os.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* 按键回调函数 */
static KeyCallback_t key_callback = NULL;

/* 按键状态 */
static KeyCode_t last_key = KEY_NONE;
static uint32_t key_press_time = 0;
static bool key_long_press = false;

/* 任务句柄 */
osThreadId_t key_task_handle;

/* 初始化标志 */
static bool initialized = false;

/* Private function prototypes -----------------------------------------------*/

static KeyCode_t Key_ReadHardware(void);

/* Private user code ---------------------------------------------------------*/

/* Exported function implementations -----------------------------------------*/

ErrorCode_t KeyHandler_Init(void)
{
    LOG_INFO("Initializing key handler module...");

    /* 配置 PA0 为按键输入（上拉） */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = GPIO_PIN_0;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_PULLDOWN;  /* 按下为高电平 */
    HAL_GPIO_Init(GPIOA, &gpio_init);

    /* 初始化状态 */
    last_key = KEY_NONE;
    key_press_time = 0;
    key_long_press = false;

    initialized = true;
    LOG_INFO("Key handler module initialized (PA0 input)");
    return ERR_OK;
}

KeyCode_t Key_Scan(void)
{
    if (!initialized) {
        return KEY_NONE;
    }

    KeyCode_t current_key = Key_ReadHardware();

    /* 去抖处理 */
    if (current_key != last_key) {
        osDelay(KEY_DEBOUNCE_MS);
        current_key = Key_ReadHardware();
    }

    /* 检测按键按下 */
    if (current_key != KEY_NONE && last_key == KEY_NONE) {
        key_press_time = HAL_GetTick();
        key_long_press = false;
    }

    /* 检测长按 */
    if (current_key != KEY_NONE && !key_long_press) {
        if (HAL_GetTick() - key_press_time >= KEY_LONG_PRESS_MS) {
            key_long_press = true;
            /* 长按处理 */
            if (key_callback != NULL) {
                key_callback(current_key);
            }
        }
    }

    /* 检测按键释放 */
    if (current_key == KEY_NONE && last_key != KEY_NONE) {
        if (!key_long_press) {
            /* 短按处理 */
            if (key_callback != NULL) {
                key_callback(last_key);
            }
        }
    }

    last_key = current_key;
    return current_key;
}

bool Key_IsPressed(KeyCode_t key)
{
    if (!initialized) {
        return false;
    }

    return (Key_ReadHardware() == key);
}

KeyCode_t Key_WaitForKey(uint32_t timeout_ms)
{
    if (!initialized) {
        return KEY_NONE;
    }

    uint32_t start_time = HAL_GetTick();

    while (HAL_GetTick() - start_time < timeout_ms) {
        KeyCode_t key = Key_Scan();
        if (key != KEY_NONE) {
            return key;
        }
        osDelay(10);
    }

    return KEY_NONE;
}

void Key_RegisterCallback(KeyCallback_t callback)
{
    key_callback = callback;
}

bool Key_IsLastPressLong(void)
{
    return key_long_press;
}

void Key_Task(void *argument)
{
    LOG_INFO("Key task started");

    /* 初始化 */
    KeyHandler_Init();

    for (;;) {
        /* 扫描按键 */
        Key_Scan();

        osDelay(10);  /* 100Hz扫描频率 */
    }
}

/* Private function implementations ------------------------------------------*/

/**
  * @brief  读取硬件按键状态
  * @retval 按键码
  */
static KeyCode_t Key_ReadHardware(void)
{
    /* 读取GPIO状态 */
    /* 注意：这里需要根据实际的硬件连接进行调整 */

    /* PA0 = 按键输入 */
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
        return KEY_ENTER;  /* 假设PA0是确认键 */
    }

    /* TODO: 添加其他按键的读取 */
    /* PB3 = KEY_UP */
    /* PB4 = KEY_DOWN */
    /* PB5 = KEY_LEFT */
    /* PB6 = KEY_RIGHT */

    return KEY_NONE;
}
