/**
  ******************************************************************************
  * @file           : oscilloscope.c
  * @brief          : 示波器模块实现
  ******************************************************************************
  * @attention
  *
  * 示波器模块，用于ADC数据采集、波形处理和测量
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "oscilloscope.h"
#include "signal_gen.h"
#include "config.h"
#include "display.h"
#include "debug.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* 互斥锁宏封装，简化代码 */
#define OSC_LOCK()   do { if (osc_mutex != NULL) osMutexAcquire(osc_mutex, osWaitForever); } while(0)
#define OSC_UNLOCK() do { if (osc_mutex != NULL) osMutexRelease(osc_mutex); } while(0)

/* Private variables ---------------------------------------------------------*/

/* 外部变量声明 */
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim8;
extern TIM_HandleTypeDef htim5;
extern UART_HandleTypeDef huart1;
extern DAC_HandleTypeDef hdac;

/* 示波器配置 */
static OscConfig_t osc_config = {
    .sample_rate = OSC_DEFAULT_SAMPLE_RATE,
    .buffer_size = OSC_DEFAULT_BUFFER_SIZE,
    .trigger_level = OSC_DEFAULT_TRIGGER_LEVEL,
    .trigger_edge = OSC_DEFAULT_TRIGGER_EDGE,
    .enabled = 1
};

/* 供 stm32f4xx_it.c 回调使用的 buffer_size 副本 */
volatile uint32_t osc_config_buffer_size = OSC_DEFAULT_BUFFER_SIZE;

/* 示波器状态 */
static OscStatus_t osc_status = {
    .voltage_mv = 0,
    .frequency_hz = 0,
    .sample_count = 0,
    .running = 0,
    .last_error = ERR_OK
};

/* ADC 双缓冲区（乒乓模式）
 * DMA 循环写入整个缓冲区，半传输/全传输回调通知任务处理
 * 前半段: adc_buffer[0 .. 511]
 * 后半段: adc_buffer[512 .. 1023]
 * 回调时 DMA 正在写另一半，可安全读取已完成的一半
 */
uint16_t adc_buffer[OSC_DEFAULT_BUFFER_SIZE];  /* 非static，供 stm32f4xx_it.c 回调访问 */
volatile uint16_t *process_ptr = NULL;  /* 非static，供 stm32f4xx_it.c 回调访问 */
volatile uint16_t  process_len = 0;     /* 非static，供 stm32f4xx_it.c 回调访问 */

/* 任务句柄 */
osThreadId_t oscilloscope_task_handle;

/* 初始化标志 */
static bool initialized = false;

/* osc_status 互斥锁 — 保护 Oscilloscope_Task 与 UART_Task 之间的共享状态 */
static osMutexId_t osc_mutex = NULL;

/* 波形流输出开关 */
volatile bool osc_stream_enabled = false;

/* 触发对齐缓冲区（存放对齐后的波形数据） */
static uint16_t aligned_buffer[OSC_DEFAULT_BUFFER_SIZE / 2];

/* Private function prototypes -----------------------------------------------*/

static uint32_t Osc_ConvertToVoltage(uint16_t adc_value);
static uint32_t Osc_CalculateFrequency(const uint16_t *buffer, uint32_t size);
static void Osc_ProcessData(const uint16_t *buf, uint16_t len);
static void Osc_UpdateMeasurements(const uint16_t *buf, uint16_t len);
static void Osc_HandleAdcData(void);
static uint32_t Osc_GetTimerClock(void);
static void Osc_ApplyConfig(void);
static uint16_t Osc_FindTriggerPoint(const uint16_t *buf, uint16_t len);

/* Private user code ---------------------------------------------------------*/

/* Exported function implementations -----------------------------------------*/

ErrorCode_t Oscilloscope_Init(void)
{
    LOG_INFO("Initializing oscilloscope module...");

    /* 从 Config 读取示波器配置 */
    OscConfig_t cfg;
    if (Config_GetOscConfig(&cfg) == ERR_OK) {
        memcpy(&osc_config, &cfg, sizeof(OscConfig_t));
        osc_config_buffer_size = osc_config.buffer_size;
        LOG_INFO("Config loaded: rate=%lu, size=%lu",
                 cfg.sample_rate, cfg.buffer_size);
    }

    /* 创建互斥锁（需在调度器启动后才能使用，此处先记录，任务中再创建） */
    osc_mutex = NULL;

    /* 初始化状态 */
    osc_status.voltage_mv = 0;
    osc_status.frequency_hz = 0;
    osc_status.sample_count = 0;
    osc_status.running = 0;
    osc_status.last_error = ERR_OK;

    initialized = true;
    LOG_INFO("Oscilloscope module initialized");
    return ERR_OK;
}

ErrorCode_t Oscilloscope_Start(void)
{
    if (!initialized) {
        LOG_ERROR("Oscilloscope not initialized");
        return ERR_NOT_INIT;
    }

    if (osc_mutex != NULL) osMutexAcquire(osc_mutex, osWaitForever);
    if (osc_status.running) {
        if (osc_mutex != NULL) osMutexRelease(osc_mutex);
        LOG_WARNING("Oscilloscope already running");
        return ERR_OK;
    }

    LOG_INFO("Starting oscilloscope...");

    /* 应用配置到 TIM8 */
    Osc_ApplyConfig();

    /* 启动ADC DMA（必须先于TIM8，参考RM0090 Section 11.3.15） */
    HAL_StatusTypeDef status = HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, osc_config.buffer_size);
    if (status != HAL_OK) {
        LOG_ERROR("Failed to start ADC DMA: %d", status);
        osc_status.last_error = ERR_HARDWARE;
        if (osc_mutex != NULL) osMutexRelease(osc_mutex);
        return ERR_HARDWARE;
    }

    /* 启动TIM8（ADC触发源，必须在ADC DMA之后） */
    status = HAL_TIM_Base_Start(&htim8);
    if (status != HAL_OK) {
        LOG_ERROR("Failed to start TIM8: %d", status);
        osc_status.last_error = ERR_HARDWARE;
        if (osc_mutex != NULL) osMutexRelease(osc_mutex);
        return ERR_HARDWARE;
    }

    osc_status.running = 1;
    osc_status.sample_count = 0;
    if (osc_mutex != NULL) osMutexRelease(osc_mutex);

    LOG_INFO("Oscilloscope started");
    return ERR_OK;
}

ErrorCode_t Oscilloscope_Stop(void)
{
    if (!initialized) {
        return ERR_NOT_INIT;
    }

    if (osc_mutex != NULL) osMutexAcquire(osc_mutex, osWaitForever);
    if (!osc_status.running) {
        if (osc_mutex != NULL) osMutexRelease(osc_mutex);
        return ERR_OK;
    }

    LOG_INFO("Stopping oscilloscope...");

    /* 停止ADC DMA */
    HAL_StatusTypeDef hal_err = HAL_ADC_Stop_DMA(&hadc1);
    if (hal_err != HAL_OK) {
        LOG_WARNING("HAL_ADC_Stop_DMA failed: %d", hal_err);
    }

    /* 停止TIM8触发源 */
    hal_err = HAL_TIM_Base_Stop(&htim8);
    if (hal_err != HAL_OK) {
        LOG_WARNING("HAL_TIM_Base_Stop failed: %d", hal_err);
    }

    osc_status.running = 0;
    if (osc_mutex != NULL) osMutexRelease(osc_mutex);

    LOG_INFO("Oscilloscope stopped");
    return ERR_OK;
}

ErrorCode_t Oscilloscope_GetStatus(OscStatus_t *status)
{
    if (status == NULL) {
        return ERR_INVALID_PARAM;
    }

    if (osc_mutex != NULL) osMutexAcquire(osc_mutex, osWaitForever);
    memcpy(status, &osc_status, sizeof(OscStatus_t));
    if (osc_mutex != NULL) osMutexRelease(osc_mutex);
    return ERR_OK;
}

bool Oscilloscope_IsRunning(void)
{
    if (osc_mutex != NULL) osMutexAcquire(osc_mutex, osWaitForever);
    bool running = osc_status.running;
    if (osc_mutex != NULL) osMutexRelease(osc_mutex);
    return running;
}

ErrorCode_t Oscilloscope_SetConfig(const OscConfig_t *config)
{
    if (config == NULL) {
        return ERR_INVALID_PARAM;
    }

    /* 检查参数 */
    if (config->sample_rate == 0 || config->sample_rate > 10000000) {
        LOG_ERROR("Invalid sample rate: %lu", config->sample_rate);
        return ERR_INVALID_PARAM;
    }

    if (config->buffer_size == 0 || config->buffer_size > OSC_DEFAULT_BUFFER_SIZE) {
        LOG_ERROR("Invalid buffer size: %lu", config->buffer_size);
        return ERR_INVALID_PARAM;
    }

    /* 更新配置 */
    OSC_LOCK();
    memcpy(&osc_config, config, sizeof(OscConfig_t));
    osc_config_buffer_size = osc_config.buffer_size;
    Osc_ApplyConfig();
    OscConfig_t cfg_copy = osc_config;  /* 锁内拷贝，避免竞态 */
    OSC_UNLOCK();

    /* 同步到 Config */
    Config_SetOscConfig(&cfg_copy);

    LOG_INFO("Oscilloscope config updated: rate=%lu, size=%lu",
             cfg_copy.sample_rate, cfg_copy.buffer_size);

    return ERR_OK;
}

ErrorCode_t Oscilloscope_GetConfig(OscConfig_t *config)
{
    if (config == NULL) {
        return ERR_INVALID_PARAM;
    }

    if (osc_mutex != NULL) osMutexAcquire(osc_mutex, osWaitForever);
    memcpy(config, &osc_config, sizeof(OscConfig_t));
    if (osc_mutex != NULL) osMutexRelease(osc_mutex);
    return ERR_OK;
}

const uint16_t *Oscilloscope_GetAdcBuffer(void)
{
    return (const uint16_t *)adc_buffer;
}

uint32_t Oscilloscope_GetAdcBufferSize(void)
{
    if (osc_mutex != NULL) osMutexAcquire(osc_mutex, osWaitForever);
    uint32_t size = osc_config.buffer_size;
    if (osc_mutex != NULL) osMutexRelease(osc_mutex);
    return size;
}

void Oscilloscope_SetStreamEnabled(bool enabled)
{
    osc_stream_enabled = enabled;
    LOG_INFO("Waveform stream %s", enabled ? "ON" : "OFF");
}

ErrorCode_t Oscilloscope_GetVoltage(uint32_t *voltage_mv)
{
    if (voltage_mv == NULL) {
        return ERR_INVALID_PARAM;
    }

    if (osc_mutex != NULL) osMutexAcquire(osc_mutex, osWaitForever);
    *voltage_mv = osc_status.voltage_mv;
    if (osc_mutex != NULL) osMutexRelease(osc_mutex);
    return ERR_OK;
}

ErrorCode_t Oscilloscope_GetFrequency(uint32_t *frequency_hz)
{
    if (frequency_hz == NULL) {
        return ERR_INVALID_PARAM;
    }

    if (osc_mutex != NULL) osMutexAcquire(osc_mutex, osWaitForever);
    *frequency_hz = osc_status.frequency_hz;
    if (osc_mutex != NULL) osMutexRelease(osc_mutex);
    return ERR_OK;
}

ErrorCode_t Oscilloscope_SelfTest(void)
{
    LOG_INFO("Oscilloscope self test...");

    /* 检查初始化状态 */
    if (!initialized) {
        LOG_ERROR("Not initialized");
        return ERR_NOT_INIT;
    }

    /* 检查配置 */
    if (osc_mutex != NULL) osMutexAcquire(osc_mutex, osWaitForever);
    uint32_t sample_rate = osc_config.sample_rate;
    if (osc_mutex != NULL) osMutexRelease(osc_mutex);

    if (sample_rate == 0) {
        LOG_ERROR("Invalid sample rate");
        return ERR_INVALID_PARAM;
    }

    LOG_INFO("Oscilloscope self test passed");
    return ERR_OK;
}

void Oscilloscope_Task(void *argument)
{
    LOG_INFO("Oscilloscope task started");

    /* 初始化已在 App_StartModules 中完成，此处仅启动 */
    if (!initialized) {
        Oscilloscope_Init();
    }

    /* 在调度器运行后创建互斥锁 */
    if (osc_mutex == NULL) {
        osc_mutex = osMutexNew(NULL);
    }
    configASSERT(osc_mutex != NULL);

    /* 启动示波器 */
    Oscilloscope_Start();

    for (;;) {
        /* 等待ADC数据就绪（任务通知：半传输或全传输） */
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100)) != 0) {
            Osc_HandleAdcData();
        } else {
            Display_ShowStatus("Waiting...");
        }
    }
}

/**
  * @brief  查找触发点（上升沿过零点）
  * @param  buf: 数据缓冲区
  * @param  len: 缓冲区长度
  * @retval 触发点索引，未找到返回 0
  */
static uint16_t Osc_FindTriggerPoint(const uint16_t *buf, uint16_t len)
{
    if (buf == NULL || len < 10) return 0;

    /* 计算信号中点作为触发电平 */
    uint16_t min_val = buf[0], max_val = buf[0];
    for (uint16_t i = 1; i < len; i++) {
        if (buf[i] < min_val) min_val = buf[i];
        if (buf[i] > max_val) max_val = buf[i];
    }

    uint16_t range = max_val - min_val;
    if (range < 100) return 0;  /* 信号幅度太小，不触发 */

    uint16_t threshold = (min_val + max_val) / 2;

    /* 查找第一个上升沿过零点（从 1/4 处开始，避免对齐后数据不足） */
    uint16_t start = len / 4;
    for (uint16_t i = start; i < len - 1; i++) {
        if (buf[i] < threshold && buf[i + 1] >= threshold) {
            return i;
        }
    }

    return 0;  /* 未找到触发点 */
}

/**
  * @brief  处理ADC数据并更新显示
  * @retval None
  */
static void Osc_HandleAdcData(void)
{
    /* 获取可处理的数据指针（回调已设置） */
    volatile uint16_t *buf = process_ptr;
    uint16_t len = process_len;

    if (buf == NULL || len == 0) {
        return;
    }

    /* 处理数据 */
    Osc_ProcessData((const uint16_t *)buf, len);

    /* 更新测量结果 */
    Osc_UpdateMeasurements((const uint16_t *)buf, len);

    /* 原子更新显示状态（避免 Display_Task 读到新旧混合值） */
    OSC_LOCK();
    uint32_t v = osc_status.voltage_mv;
    uint32_t f = osc_status.frequency_hz;
    OSC_UNLOCK();

    /* 触发对齐：找到过零点，从该点开始，输出固定长度 */
    uint16_t trig_idx = Osc_FindTriggerPoint((const uint16_t *)buf, len);

    /* 固定输出长度 = 缓冲区长度 */
    uint16_t out_len = len;
    uint16_t buf_cap = sizeof(aligned_buffer) / sizeof(aligned_buffer[0]);
    if (out_len > buf_cap) out_len = buf_cap;

    /* 从触发点开始拷贝，不足部分从缓冲区头部循环填充 */
    for (uint16_t i = 0; i < out_len; i++) {
        aligned_buffer[i] = buf[(trig_idx + i) % len];
    }

    /* 传入对齐后的数据 */
    Display_UpdateScope(aligned_buffer, out_len, v, f);

    /* 波形流输出（2Hz） */
    if (osc_stream_enabled) {
        static uint32_t stream_tick = 0;
        uint32_t now = HAL_GetTick();
        if (now - stream_tick >= 2000) {
            stream_tick = now;

            for (uint32_t i = 0; i < len; i++) {
                char line[8];
                int l = snprintf(line, sizeof(line), "%u\r\n", buf[i]);
                HAL_UART_Transmit(&huart1, (uint8_t *)line, l, 10);
            }
            HAL_UART_Transmit(&huart1, (uint8_t *)"---\r\n", 5, 10);
        }
    }
}

/* Private function implementations ------------------------------------------*/

/**
  * @brief  将ADC值转换为电压 (mV)
  * @param  adc_value: ADC值
  * @retval 电压值 (mV)
  */
static uint32_t Osc_ConvertToVoltage(uint16_t adc_value)
{
    /* 电压 = ADC值 * 参考电压 / 分辨率 */
    return (uint32_t)((uint32_t)adc_value * ADC_REF_VOLTAGE / ADC_RESOLUTION);
}

/**
  * @brief  计算频率
  * @param  buffer: 数据缓冲区
  * @param  size: 缓冲区大小
  * @retval 频率值 (Hz)
  */
static uint32_t Osc_CalculateFrequency(const uint16_t *buffer, uint32_t size)
{
    if (buffer == NULL || size < 2) {
        return 0;
    }

    /* 自适应阈值：使用信号的实际中点 */
    uint16_t min_val = buffer[0], max_val = buffer[0];
    for (uint32_t i = 1; i < size; i++) {
        if (buffer[i] < min_val) min_val = buffer[i];
        if (buffer[i] > max_val) max_val = buffer[i];
    }

    /* 信号幅度太小，无法检测频率 */
    uint16_t range = max_val - min_val;
    if (range < 100) {
        return 0;
    }

    /* 使用信号中点作为阈值 */
    uint16_t threshold = (min_val + max_val) / 2;

    /* 过零检测 */
    uint32_t zero_count = 0;
    for (uint32_t i = 1; i < size; i++) {
        if ((buffer[i-1] < threshold && buffer[i] >= threshold) ||
            (buffer[i-1] >= threshold && buffer[i] < threshold)) {
            zero_count++;
        }
    }

    /* 频率 = 过零次数 / 2 / 时间 */
    /* 时间 = 采样点数 / 采样率 */
    if (zero_count > 0) {
        uint32_t time_us = size * 1000000 / osc_config.sample_rate;
        return (zero_count * 1000000) / (2 * time_us);
    }

    return 0;
}

/**
  * @brief  处理ADC数据
  * @param  buf: 数据缓冲区指针
  * @param  len: 数据长度
  * @retval None
  */
static void Osc_ProcessData(const uint16_t *buf, uint16_t len)
{
    /* 计算平均电压 */
    uint32_t sum = 0;
    for (uint32_t i = 0; i < len; i++) {
        sum += buf[i];
    }

    uint16_t avg_adc = sum / len;
    if (osc_mutex != NULL) osMutexAcquire(osc_mutex, osWaitForever);
    osc_status.voltage_mv = Osc_ConvertToVoltage(avg_adc);
    osc_status.sample_count += len;
    if (osc_mutex != NULL) osMutexRelease(osc_mutex);
}

/**
  * @brief  更新测量结果
  * @param  buf: 数据缓冲区指针
  * @param  len: 数据长度
  * @retval None
  */
static void Osc_UpdateMeasurements(const uint16_t *buf, uint16_t len)
{
    /* 频率测量：基于过零检测 */
    uint32_t freq = Osc_CalculateFrequency(buf, len);
    if (osc_mutex != NULL) osMutexAcquire(osc_mutex, osWaitForever);
    osc_status.frequency_hz = freq;
    if (osc_mutex != NULL) osMutexRelease(osc_mutex);
}

/**
  * @brief  获取 TIM8 时钟频率（APB2 定时器时钟）
  * @retval 时钟频率 (Hz)
  */
static uint32_t Osc_GetTimerClock(void)
{
    uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();
    uint32_t prescaler = RCC->CFGR & RCC_CFGR_PPRE2;
    /* APB2 prescaler > 1 时，定时器时钟 = PCLK2 × 2 */
    if (prescaler != RCC_CFGR_PPRE2_DIV1) {
        return pclk2 * 2;
    }
    return pclk2;
}

/**
  * @brief  应用示波器配置到 TIM8 硬件
  *         调用前必须持有 osc_mutex
  * @retval None
  */
static void Osc_ApplyConfig(void)
{
    if (osc_config.sample_rate == 0) return;

    uint32_t timer_clk = Osc_GetTimerClock();
    uint32_t target_rate = osc_config.sample_rate;

    /* 计算 PSC 和 ARR 使 TRGO 频率 = target_rate
     * TRGO 频率 = timer_clk / (PSC+1) / (ARR+1)
     * 策略: 优先用 PSC=0, ARR = timer_clk/target_rate - 1
     * 如果 ARR > 65535, 则用 PSC 分频
     */
    uint32_t psc = 0;
    uint32_t arr;

    if (target_rate >= timer_clk) {
        arr = 0;
    } else {
        arr = (timer_clk / target_rate) - 1;
    }

    /* 如果 ARR 超过 16 位, 需要预分频 */
    if (arr > 65535) {
        psc = (arr / 65536) + 1;
        arr = (timer_clk / (psc * target_rate)) - 1;
    }
    if (arr > 65535) arr = 65535;
    if (psc > 65535) psc = 65535;

    __HAL_TIM_SET_PRESCALER(&htim8, psc);
    __HAL_TIM_SET_AUTORELOAD(&htim8, arr);
    htim8.Instance->EGR = TIM_EGR_UG;

    LOG_INFO("TIM8: PSC=%lu, ARR=%lu, rate=%lu Hz", psc, arr, target_rate);
}
