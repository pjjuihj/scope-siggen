/**
  ******************************************************************************
  * @file           : signal_gen.c
  * @brief          : 信号发生器模块实现
  ******************************************************************************
  * @attention
  *
  * 信号发生器模块，用于DAC波形输出
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
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
#define SIGGEN_LOCK()   do { if (siggen_mutex != NULL) osMutexAcquire(siggen_mutex, osWaitForever); } while(0)
#define SIGGEN_UNLOCK() do { if (siggen_mutex != NULL) osMutexRelease(siggen_mutex); } while(0)

/* Private variables ---------------------------------------------------------*/

/* 外部变量声明 */
extern DAC_HandleTypeDef hdac;
extern TIM_HandleTypeDef htim5;
extern UART_HandleTypeDef huart1;

/* 信号发生器配置 */
static SigGenConfig_t siggen_config = {
    .frequency = SIGGEN_DEFAULT_FREQUENCY,
    .amplitude = SIGGEN_DEFAULT_AMPLITUDE,
    .waveform = SIGGEN_DEFAULT_WAVEFORM,
    .duty_cycle = SIGGEN_DEFAULT_DUTY_CYCLE,
    .enabled = 1
};

/* 信号发生器状态 */
static SigGenStatus_t siggen_status = {
    .current_frequency = 0,
    .current_amplitude = 0,
    .current_waveform = WAVE_SINE,
    .running = 0,
    .last_error = ERR_OK
};

/* 波形缓冲区 */
static uint16_t waveform_buffer[DAC_BUFFER_SIZE];

/* 任务句柄 */
osThreadId_t signalgen_task_handle;

/* 初始化标志 */
static bool initialized = false;

/* siggen_config/siggen_status 互斥锁 — 保护 UART_Task 与 SignalGen_Task 之间的共享状态 */
static osMutexId_t siggen_mutex = NULL;

/* Private function prototypes -----------------------------------------------*/

static void SigGen_GenerateWaveform(void);
static void SigGen_ApplyConfig(void);
static void SigGen_GenerateSine(uint16_t *buffer, uint32_t size, uint32_t amplitude);
static void SigGen_GenerateSquare(uint16_t *buffer, uint32_t size, uint32_t amplitude, uint16_t duty_cycle);
static void SigGen_GenerateTriangle(uint16_t *buffer, uint32_t size, uint32_t amplitude);
static void SigGen_GenerateSawtooth(uint16_t *buffer, uint32_t size, uint32_t amplitude);
static void SigGen_GenerateDC(uint16_t *buffer, uint32_t size, uint32_t amplitude);
static uint16_t SigGen_VoltageToDac(uint32_t voltage_mv);

/* Private user code ---------------------------------------------------------*/

/* Exported function implementations -----------------------------------------*/

ErrorCode_t SignalGen_Init(void)
{
    LOG_INFO("Initializing signal generator module...");

    /* 从 Config 读取信号发生器配置 */
    SigGenConfig_t cfg;
    if (Config_GetSigGenConfig(&cfg) == ERR_OK) {
        memcpy(&siggen_config, &cfg, sizeof(SigGenConfig_t));
        LOG_INFO("Config loaded: freq=%lu, amp=%lu, wave=%d",
                 cfg.frequency, cfg.amplitude, cfg.waveform);
    }

    /* 初始化状态 */
    siggen_status.current_frequency = 0;
    siggen_status.current_amplitude = 0;
    siggen_status.current_waveform = WAVE_SINE;
    siggen_status.running = 0;
    siggen_status.last_error = ERR_OK;

    /* 生成初始波形 */
    SigGen_GenerateWaveform();

    initialized = true;
    LOG_INFO("Signal generator module initialized");
    return ERR_OK;
}

ErrorCode_t SignalGen_Start(void)
{
    if (!initialized) {
        LOG_ERROR("Signal generator not initialized");
        return ERR_NOT_INIT;
    }

    if (siggen_mutex != NULL) osMutexAcquire(siggen_mutex, osWaitForever);
    if (siggen_status.running) {
        if (siggen_mutex != NULL) osMutexRelease(siggen_mutex);
        LOG_WARNING("Signal generator already running");
        return ERR_OK;
    }

    LOG_INFO("Starting signal generator...");

    /* 生成波形 */
    SigGen_GenerateWaveform();

    /* 应用配置到 TIM5 */
    SigGen_ApplyConfig();

    if (siggen_mutex != NULL) osMutexRelease(siggen_mutex);

    /* 启动TIM5（DAC触发源） */
    HAL_StatusTypeDef status = HAL_TIM_Base_Start(&htim5);
    if (status != HAL_OK) {
        LOG_ERROR("Failed to start TIM5: %d", status);
        if (siggen_mutex != NULL) osMutexAcquire(siggen_mutex, osWaitForever);
        siggen_status.last_error = ERR_HARDWARE;
        if (siggen_mutex != NULL) osMutexRelease(siggen_mutex);
        return ERR_HARDWARE;
    }

    /* 启动DAC DMA */
    status = HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)waveform_buffer, DAC_BUFFER_SIZE, DAC_ALIGN_12B_R);
    if (status != HAL_OK) {
        LOG_ERROR("Failed to start DAC DMA: %d", status);
        if (siggen_mutex != NULL) osMutexAcquire(siggen_mutex, osWaitForever);
        siggen_status.last_error = ERR_HARDWARE;
        if (siggen_mutex != NULL) osMutexRelease(siggen_mutex);
        return ERR_HARDWARE;
    }

    if (siggen_mutex != NULL) osMutexAcquire(siggen_mutex, osWaitForever);
    siggen_status.running = 1;
    siggen_status.current_frequency = siggen_config.frequency;
    siggen_status.current_amplitude = siggen_config.amplitude;
    siggen_status.current_waveform = siggen_config.waveform;
    uint32_t freq = siggen_config.frequency;
    uint32_t amp = siggen_config.amplitude;
    uint8_t wave = (uint8_t)siggen_config.waveform;
    uint16_t duty = siggen_config.duty_cycle;
    if (siggen_mutex != NULL) osMutexRelease(siggen_mutex);

    /* 更新显示 */
    Display_UpdateSigGen(freq, amp, wave, duty, true);

    LOG_INFO("Signal generator started: freq=%lu, amp=%lu, wave=%d",
             freq, amp, wave);

    return ERR_OK;
}

ErrorCode_t SignalGen_Stop(void)
{
    if (!initialized) {
        return ERR_NOT_INIT;
    }

    if (siggen_mutex != NULL) osMutexAcquire(siggen_mutex, osWaitForever);
    if (!siggen_status.running) {
        if (siggen_mutex != NULL) osMutexRelease(siggen_mutex);
        return ERR_OK;
    }

    LOG_INFO("Stopping signal generator...");

    /* 停止DAC DMA */
    HAL_StatusTypeDef hal_err = HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
    if (hal_err != HAL_OK) {
        LOG_WARNING("HAL_DAC_Stop_DMA failed: %d", hal_err);
    }

    /* 停止TIM5（DAC触发源） */
    hal_err = HAL_TIM_Base_Stop(&htim5);
    if (hal_err != HAL_OK) {
        LOG_WARNING("HAL_TIM_Base_Stop failed: %d", hal_err);
    }

    siggen_status.running = 0;
    uint32_t freq = siggen_config.frequency;
    uint32_t amp = siggen_config.amplitude;
    uint8_t wave = (uint8_t)siggen_config.waveform;
    uint16_t duty = siggen_config.duty_cycle;
    if (siggen_mutex != NULL) osMutexRelease(siggen_mutex);

    /* 更新显示 */
    Display_UpdateSigGen(freq, amp, wave, duty, false);

    LOG_INFO("Signal generator stopped");
    return ERR_OK;
}

ErrorCode_t SignalGen_GetStatus(SigGenStatus_t *status)
{
    if (status == NULL) {
        return ERR_INVALID_PARAM;
    }

    if (siggen_mutex != NULL) osMutexAcquire(siggen_mutex, osWaitForever);
    memcpy(status, &siggen_status, sizeof(SigGenStatus_t));
    if (siggen_mutex != NULL) osMutexRelease(siggen_mutex);
    return ERR_OK;
}

bool SignalGen_IsRunning(void)
{
    if (siggen_mutex != NULL) osMutexAcquire(siggen_mutex, osWaitForever);
    bool running = siggen_status.running;
    if (siggen_mutex != NULL) osMutexRelease(siggen_mutex);
    return running;
}

ErrorCode_t SignalGen_SetConfig(const SigGenConfig_t *config)
{
    if (config == NULL) {
        return ERR_INVALID_PARAM;
    }

    /* 检查参数 */
    if (config->frequency == 0 || config->frequency > 1000000) {
        LOG_ERROR("Invalid frequency: %lu", config->frequency);
        return ERR_INVALID_PARAM;
    }

    if (config->amplitude > DAC_REF_VOLTAGE) {
        LOG_ERROR("Invalid amplitude: %lu", config->amplitude);
        return ERR_INVALID_PARAM;
    }

    if (config->duty_cycle > 1000) {
        LOG_ERROR("Invalid duty cycle: %d", config->duty_cycle);
        return ERR_INVALID_PARAM;
    }

    /* 锁内：更新配置 + 拷贝 */
    SIGGEN_LOCK();
    memcpy(&siggen_config, config, sizeof(SigGenConfig_t));
    SigGenConfig_t cfg_copy = siggen_config;
    bool is_running = siggen_status.running;
    SIGGEN_UNLOCK();

    /* 锁外：生成波形（耗时操作，~768 次乘法） */
    SigGen_GenerateWaveform();

    /* 锁内：应用硬件配置（PSC/ARR） */
    SIGGEN_LOCK();
    SigGen_ApplyConfig();
    SIGGEN_UNLOCK();

    /* 同步到 Config */
    Config_SetSigGenConfig(&cfg_copy);

    /* 更新显示 */
    Display_UpdateSigGen(cfg_copy.frequency, cfg_copy.amplitude,
                         (uint8_t)cfg_copy.waveform, cfg_copy.duty_cycle,
                         is_running);

    LOG_INFO("Signal generator config updated: freq=%lu, amp=%lu, wave=%d",
             cfg_copy.frequency, cfg_copy.amplitude, cfg_copy.waveform);

    return ERR_OK;
}

ErrorCode_t SignalGen_GetConfig(SigGenConfig_t *config)
{
    if (config == NULL) {
        return ERR_INVALID_PARAM;
    }

    if (siggen_mutex != NULL) osMutexAcquire(siggen_mutex, osWaitForever);
    memcpy(config, &siggen_config, sizeof(SigGenConfig_t));
    if (siggen_mutex != NULL) osMutexRelease(siggen_mutex);
    return ERR_OK;
}

ErrorCode_t SignalGen_SetWaveform(WaveformType_t type)
{
    if (type > WAVE_DC) {
        LOG_ERROR("Invalid waveform type: %d", type);
        return ERR_INVALID_PARAM;
    }

    /* 锁内：更新配置 + 拷贝 */
    SIGGEN_LOCK();
    siggen_config.waveform = type;
    SigGenConfig_t cfg_copy = siggen_config;
    SIGGEN_UNLOCK();

    /* 锁外：生成波形 */
    SigGen_GenerateWaveform();

    /* 同步到 Config */
    Config_SetSigGenConfig(&cfg_copy);

    LOG_INFO("Waveform set to: %d", type);
    return ERR_OK;
}

ErrorCode_t SignalGen_SetFrequency(uint32_t freq_hz)
{
    if (freq_hz == 0 || freq_hz > 1000000) {
        LOG_ERROR("Invalid frequency: %lu", freq_hz);
        return ERR_INVALID_PARAM;
    }

    /* 锁内：更新配置 + 拷贝 */
    SIGGEN_LOCK();
    siggen_config.frequency = freq_hz;
    SigGenConfig_t cfg_copy = siggen_config;
    SIGGEN_UNLOCK();

    /* 锁外：生成波形 */
    SigGen_GenerateWaveform();

    /* 锁内：应用硬件配置 */
    SIGGEN_LOCK();
    SigGen_ApplyConfig();
    SIGGEN_UNLOCK();

    /* 同步到 Config */
    Config_SetSigGenConfig(&cfg_copy);

    LOG_INFO("Frequency set to: %lu Hz", freq_hz);
    return ERR_OK;
}

ErrorCode_t SignalGen_SetAmplitude(uint32_t amplitude_mv)
{
    if (amplitude_mv > DAC_REF_VOLTAGE) {
        LOG_ERROR("Invalid amplitude: %lu", amplitude_mv);
        return ERR_INVALID_PARAM;
    }

    /* 锁内：更新配置 + 拷贝 */
    SIGGEN_LOCK();
    siggen_config.amplitude = amplitude_mv;
    SigGenConfig_t cfg_copy = siggen_config;
    SIGGEN_UNLOCK();

    /* 锁外：生成波形 */
    SigGen_GenerateWaveform();

    /* 同步到 Config */
    Config_SetSigGenConfig(&cfg_copy);

    LOG_INFO("Amplitude set to: %lu mV", amplitude_mv);
    return ERR_OK;
}

ErrorCode_t SignalGen_SetDutyCycle(uint16_t duty_permille)
{
    if (duty_permille > 1000) {
        LOG_ERROR("Invalid duty cycle: %d", duty_permille);
        return ERR_INVALID_PARAM;
    }

    /* 锁内：更新配置 + 拷贝 */
    SIGGEN_LOCK();
    siggen_config.duty_cycle = duty_permille;
    SigGenConfig_t cfg_copy = siggen_config;
    SIGGEN_UNLOCK();

    /* 锁外：生成波形 */
    SigGen_GenerateWaveform();

    /* 同步到 Config */
    Config_SetSigGenConfig(&cfg_copy);

    LOG_INFO("Duty cycle set to: %d permille", duty_permille);
    return ERR_OK;
}

ErrorCode_t SignalGen_SelfTest(void)
{
    LOG_INFO("Signal generator self test...");

    /* 检查初始化状态 */
    if (!initialized) {
        LOG_ERROR("Not initialized");
        return ERR_NOT_INIT;
    }

    /* 检查配置 */
    if (siggen_config.frequency == 0) {
        LOG_ERROR("Invalid frequency");
        return ERR_INVALID_PARAM;
    }

    if (siggen_config.amplitude > DAC_REF_VOLTAGE) {
        LOG_ERROR("Invalid amplitude");
        return ERR_INVALID_PARAM;
    }

    LOG_INFO("Signal generator self test passed");
    return ERR_OK;
}

void SignalGen_Task(void *argument)
{
    LOG_INFO("Signal generator task started");

    /* 初始化已在 App_StartModules 中完成，此处仅启动 */
    if (!initialized) {
        SignalGen_Init();
    }

    /* 在调度器运行后创建互斥锁 */
    if (siggen_mutex == NULL) {
        siggen_mutex = osMutexNew(NULL);
    }
    configASSERT(siggen_mutex != NULL);

    /* 启动信号发生器 */
    SignalGen_Start();

    for (;;) {
        /* 命令接收已由 UART_Task 处理（直接调用 SignalGen_Set* 函数） */
        /* 本任务保留用于未来动态波形更新或状态监控 */

        osDelay(1000);  /* 1Hz 状态检查 */
    }
}

/* Private function implementations ------------------------------------------*/

/**
  * @brief  生成波形
  * @retval None
  */
static void SigGen_GenerateWaveform(void)
{
    uint16_t dac_amplitude = SigGen_VoltageToDac(siggen_config.amplitude);

    switch (siggen_config.waveform) {
        case WAVE_SINE:
            SigGen_GenerateSine(waveform_buffer, DAC_BUFFER_SIZE, dac_amplitude);
            break;
        case WAVE_SQUARE:
            SigGen_GenerateSquare(waveform_buffer, DAC_BUFFER_SIZE, dac_amplitude, siggen_config.duty_cycle);
            break;
        case WAVE_TRIANGLE:
            SigGen_GenerateTriangle(waveform_buffer, DAC_BUFFER_SIZE, dac_amplitude);
            break;
        case WAVE_SAWTOOTH:
            SigGen_GenerateSawtooth(waveform_buffer, DAC_BUFFER_SIZE, dac_amplitude);
            break;
        case WAVE_DC:
            SigGen_GenerateDC(waveform_buffer, DAC_BUFFER_SIZE, dac_amplitude);
            break;
        default:
            SigGen_GenerateSine(waveform_buffer, DAC_BUFFER_SIZE, dac_amplitude);
            break;
    }
}

/**
  * @brief  生成正弦波
  * @param  buffer: 缓冲区
  * @param  size: 缓冲区大小
  * @param  amplitude: 幅度 (DAC值)
  * @retval None
  */
/* 正弦查表（四分之一周期，64点，Q15格式） */
static const int16_t sin_table[65] = {
      0,   804,  1608,  2410,  3212,  4011,  4808,  5602,
   6393,  7179,  7962,  8739,  9512, 10278, 11039, 11793,
  12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530,
  18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
  23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790,
  27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
  30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971,
  32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
  32767
};

static void SigGen_GenerateSine(uint16_t *buffer, uint32_t size, uint32_t amplitude)
{
    for (uint32_t i = 0; i < size; i++) {
        uint32_t phase = (i * 256) / size;
        int16_t sin_val;

        if (phase < 64) {
            sin_val = sin_table[phase];
        } else if (phase < 128) {
            sin_val = sin_table[128 - phase];
        } else if (phase < 192) {
            sin_val = -sin_table[phase - 128];
        } else {
            sin_val = -sin_table[256 - phase];
        }

        buffer[i] = (uint16_t)(((int32_t)sin_val + 32767) * amplitude / 65534);
    }
}

/**
  * @brief  生成方波
  * @param  buffer: 缓冲区
  * @param  size: 缓冲区大小
  * @param  amplitude: 幅度 (DAC值)
  * @param  duty_cycle: 占空比 (‰)
  * @retval None
  */
static void SigGen_GenerateSquare(uint16_t *buffer, uint32_t size, uint32_t amplitude, uint16_t duty_cycle)
{
    uint32_t threshold = size * duty_cycle / 1000;

    for (uint32_t i = 0; i < size; i++) {
        if (i < threshold) {
            buffer[i] = amplitude;
        } else {
            buffer[i] = 0;
        }
    }
}

/**
  * @brief  生成三角波
  * @param  buffer: 缓冲区
  * @param  size: 缓冲区大小
  * @param  amplitude: 幅度 (DAC值)
  * @retval None
  */
static void SigGen_GenerateTriangle(uint16_t *buffer, uint32_t size, uint32_t amplitude)
{
    uint32_t half_size = size / 2;

    for (uint32_t i = 0; i < size; i++) {
        if (i < half_size) {
            buffer[i] = (uint16_t)(amplitude * i / half_size);
        } else {
            buffer[i] = (uint16_t)(amplitude * (size - i) / half_size);
        }
    }
}

/**
  * @brief  生成锯齿波
  * @param  buffer: 缓冲区
  * @param  size: 缓冲区大小
  * @param  amplitude: 幅度 (DAC值)
  * @retval None
  */
static void SigGen_GenerateSawtooth(uint16_t *buffer, uint32_t size, uint32_t amplitude)
{
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = (uint16_t)(amplitude * i / size);
    }
}

/**
  * @brief  生成直流
  * @param  buffer: 缓冲区
  * @param  size: 缓冲区大小
  * @param  amplitude: 幅度 (DAC值)
  * @retval None
  */
static void SigGen_GenerateDC(uint16_t *buffer, uint32_t size, uint32_t amplitude)
{
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = amplitude;
    }
}

/**
  * @brief  将电压转换为DAC值
  * @param  voltage_mv: 电压值 (mV)
  * @retval DAC值
  */
static uint16_t SigGen_VoltageToDac(uint32_t voltage_mv)
{
    /* DAC值 = 电压 * 分辨率 / 参考电压 */
    return (uint16_t)((uint32_t)voltage_mv * DAC_RESOLUTION / DAC_REF_VOLTAGE);
}

/**
  * @brief  获取 TIM5 时钟频率（APB1 定时器时钟）
  * @retval 时钟频率 (Hz)
  */
static uint32_t SigGen_GetTimerClock(void)
{
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    uint32_t prescaler = RCC->CFGR & RCC_CFGR_PPRE1;
    /* APB1 prescaler > 1 时，定时器时钟 = PCLK1 × 2 */
    if (prescaler != RCC_CFGR_PPRE1_DIV1) {
        return pclk1 * 2;
    }
    return pclk1;
}

/**
  * @brief  应用信号发生器配置到 TIM5 硬件
  *         调用前必须持有 siggen_mutex
  * @retval None
  */
static void SigGen_ApplyConfig(void)
{
    if (siggen_config.frequency == 0) return;

    uint32_t timer_clk = SigGen_GetTimerClock();
    uint32_t arr = DAC_BUFFER_SIZE - 1;

    /* TRGO = timer_clk / (PSC+1) / (ARR+1)
     * 目标 TRGO = frequency * DAC_BUFFER_SIZE (每秒输出 frequency 个完整波形)
     * PSC+1 = timer_clk / (target_trgo * (ARR+1))
     *       = timer_clk / (frequency * DAC_BUFFER_SIZE^2)
     */
    uint32_t psc_plus_1 = timer_clk / (siggen_config.frequency * DAC_BUFFER_SIZE * DAC_BUFFER_SIZE);
    uint32_t psc = (psc_plus_1 > 0) ? (psc_plus_1 - 1) : 0;
    if (psc > 65535) psc = 65535;

    __HAL_TIM_SET_PRESCALER(&htim5, psc);
    __HAL_TIM_SET_AUTORELOAD(&htim5, arr);
    htim5.Instance->EGR = TIM_EGR_UG;
}

const uint16_t *SignalGen_GetWaveformBuffer(void)
{
    return waveform_buffer;
}

uint32_t SignalGen_GetWaveformBufferSize(void)
{
    return DAC_BUFFER_SIZE;
}
