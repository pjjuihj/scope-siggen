/**
  ******************************************************************************
  * @file           : signal_gen.h
  * @brief          : 信号发生器模块接口
  ******************************************************************************
  * @attention
  *
  * 信号发生器模块，用于DAC波形输出
  *
  ******************************************************************************
  */

#ifndef __SIGNAL_GEN_H
#define __SIGNAL_GEN_H

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
  * @brief 波形类型定义
  */
typedef enum {
    WAVE_SINE = 0,      /* 正弦波 */
    WAVE_SQUARE,        /* 方波 */
    WAVE_TRIANGLE,      /* 三角波 */
    WAVE_SAWTOOTH,      /* 锯齿波 */
    WAVE_DC             /* 直流 */
} WaveformType_t;

/**
  * @brief 信号发生器配置结构体
  */
typedef struct {
    uint32_t frequency;         /* 频率 (Hz) */
    uint32_t amplitude;         /* 幅度 (mV) */
    WaveformType_t waveform;    /* 波形类型 */
    uint16_t duty_cycle;        /* 占空比 (‰, 0-1000) */
    uint8_t enabled;            /* 使能标志 */
} SigGenConfig_t;

/**
  * @brief 信号发生器状态结构体
  */
typedef struct {
    uint32_t current_frequency; /* 当前频率 */
    uint32_t current_amplitude; /* 当前幅度 */
    WaveformType_t current_waveform; /* 当前波形 */
    uint8_t running;            /* 运行状态 */
    ErrorCode_t last_error;     /* 最后错误 */
} SigGenStatus_t;

/* Exported constants --------------------------------------------------------*/

/* 默认配置 */
#define SIGGEN_DEFAULT_FREQUENCY    1000        /* 1kHz */
#define SIGGEN_DEFAULT_AMPLITUDE    1650        /* 1.65V */
#define SIGGEN_DEFAULT_WAVEFORM     WAVE_SINE   /* 正弦波 */
#define SIGGEN_DEFAULT_DUTY_CYCLE   500         /* 50% */

/* DAC配置 */
#define DAC_RESOLUTION              4096        /* 12位DAC */
#define DAC_REF_VOLTAGE             3300        /* 3.3V参考电压 (mV) */
#define DAC_BUFFER_SIZE             256         /* 波形缓冲区大小 */

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  初始化信号发生器模块
  * @retval 错误码
  */
ErrorCode_t SignalGen_Init(void);

/**
  * @brief  启动信号发生器
  * @retval 错误码
  */
ErrorCode_t SignalGen_Start(void);

/**
  * @brief  停止信号发生器
  * @retval 错误码
  */
ErrorCode_t SignalGen_Stop(void);

/**
  * @brief  获取信号发生器状态
  * @param  status: 状态结构体指针
  * @retval 错误码
  */
ErrorCode_t SignalGen_GetStatus(SigGenStatus_t *status);

/**
  * @brief  检查信号发生器是否运行
  * @retval true=运行, false=停止
  */
bool SignalGen_IsRunning(void);

/**
  * @brief  设置信号发生器配置
  * @param  config: 配置结构体指针
  * @retval 错误码
  */
ErrorCode_t SignalGen_SetConfig(const SigGenConfig_t *config);

/**
  * @brief  获取信号发生器配置
  * @param  config: 配置结构体指针
  * @retval 错误码
  */
ErrorCode_t SignalGen_GetConfig(SigGenConfig_t *config);

/**
  * @brief  设置波形类型
  * @param  type: 波形类型
  * @retval 错误码
  */
ErrorCode_t SignalGen_SetWaveform(WaveformType_t type);

/**
  * @brief  设置频率
  * @param  freq_hz: 频率值 (Hz)
  * @retval 错误码
  */
ErrorCode_t SignalGen_SetFrequency(uint32_t freq_hz);

/**
  * @brief  设置幅度
  * @param  amplitude_mv: 幅度值 (mV)
  * @retval 错误码
  */
ErrorCode_t SignalGen_SetAmplitude(uint32_t amplitude_mv);

/**
  * @brief  设置占空比
  * @param  duty_permille: 占空比 (‰, 0-1000)
  * @retval 错误码
  */
ErrorCode_t SignalGen_SetDutyCycle(uint16_t duty_permille);

/**
  * @brief  自检
  * @retval 错误码
  */
ErrorCode_t SignalGen_SelfTest(void);

/**
  * @brief  获取波形缓冲区指针
  * @retval 缓冲区指针
  */
const uint16_t *SignalGen_GetWaveformBuffer(void);

/**
  * @brief  获取波形缓冲区大小
  * @retval 缓冲区大小
  */
uint32_t SignalGen_GetWaveformBufferSize(void);

/**
  * @brief  信号发生器任务
  * @param  argument: 任务参数
  * @retval None
  */
void SignalGen_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* __SIGNAL_GEN_H */
