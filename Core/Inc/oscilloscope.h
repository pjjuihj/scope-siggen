/**
  ******************************************************************************
  * @file           : oscilloscope.h
  * @brief          : 示波器模块接口
  ******************************************************************************
  * @attention
  *
  * 示波器模块，用于ADC数据采集、波形处理和测量
  *
  ******************************************************************************
  */

#ifndef __OSCILLOSCOPE_H
#define __OSCILLOSCOPE_H

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
  * @brief 示波器配置结构体
  */
typedef struct {
    uint32_t sample_rate;       /* 采样率 (Hz) */
    uint32_t buffer_size;       /* 缓冲区大小 */
    uint32_t trigger_level;     /* 触发电平 (mV) */
    uint8_t trigger_edge;       /* 触发边沿 (0=下降沿, 1=上升沿) */
    uint8_t enabled;            /* 使能标志 */
} OscConfig_t;

/**
  * @brief 示波器状态结构体
  */
typedef struct {
    uint32_t voltage_mv;        /* 电压 (mV) */
    uint32_t frequency_hz;      /* 频率 (Hz) */
    uint32_t sample_count;      /* 采样计数 */
    uint8_t running;            /* 运行状态 */
    ErrorCode_t last_error;     /* 最后错误 */
} OscStatus_t;

/* Exported constants --------------------------------------------------------*/

/* 默认配置 */
#define OSC_DEFAULT_SAMPLE_RATE     1000000     /* 1MHz */
#define OSC_DEFAULT_BUFFER_SIZE     1024        /* 1024个采样点 */
#define OSC_DEFAULT_TRIGGER_LEVEL   1650        /* 1.65V */
#define OSC_DEFAULT_TRIGGER_EDGE    1           /* 上升沿 */

/* ADC配置 */
#define ADC_RESOLUTION              4096        /* 12位ADC */
#define ADC_REF_VOLTAGE             3300        /* 3.3V参考电压 (mV) */

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  初始化示波器模块
  * @retval 错误码
  */
ErrorCode_t Oscilloscope_Init(void);

/**
  * @brief  启动示波器
  * @retval 错误码
  */
ErrorCode_t Oscilloscope_Start(void);

/**
  * @brief  停止示波器
  * @retval 错误码
  */
ErrorCode_t Oscilloscope_Stop(void);

/**
  * @brief  获取示波器状态
  * @param  status: 状态结构体指针
  * @retval 错误码
  */
ErrorCode_t Oscilloscope_GetStatus(OscStatus_t *status);

/**
  * @brief  检查示波器是否运行
  * @retval true=运行, false=停止
  */
bool Oscilloscope_IsRunning(void);

/**
  * @brief  设置示波器配置
  * @param  config: 配置结构体指针
  * @retval 错误码
  */
ErrorCode_t Oscilloscope_SetConfig(const OscConfig_t *config);

/**
  * @brief  获取示波器配置
  * @param  config: 配置结构体指针
  * @retval 错误码
  */
ErrorCode_t Oscilloscope_GetConfig(OscConfig_t *config);

/**
  * @brief  获取电压值
  * @param  voltage_mv: 电压值指针 (mV)
  * @retval 错误码
  */
ErrorCode_t Oscilloscope_GetVoltage(uint32_t *voltage_mv);

/**
  * @brief  获取频率值
  * @param  frequency_hz: 频率值指针 (Hz)
  * @retval 错误码
  */
ErrorCode_t Oscilloscope_GetFrequency(uint32_t *frequency_hz);

/**
  * @brief  设置波形流输出开关
  * @param  enabled: true=开启, false=关闭
  * @retval None
  */
void Oscilloscope_SetStreamEnabled(bool enabled);

/**
  * @brief  自检
  * @retval 错误码
  */
ErrorCode_t Oscilloscope_SelfTest(void);

/**
  * @brief  Day 2 验证: ADC+DMA 数据链路测试
  *         打印前 16 个采样值，检查数据有效性
  * @retval None
  */
void Oscilloscope_ValidateDay2(void);

/**
  * @brief  验证采样率: TIM8 配置 + 实测 DMA 中断频率
  * @retval None
  */
void Oscilloscope_ValidateSampleRate(void);

/**
  * @brief  获取ADC缓冲区指针
  * @retval 缓冲区指针
  */
const uint16_t *Oscilloscope_GetAdcBuffer(void);

/**
  * @brief  获取ADC缓冲区大小
  * @retval 缓冲区大小
  */
uint32_t Oscilloscope_GetAdcBufferSize(void);

/**
  * @brief  示波器任务
  * @param  argument: 任务参数
  * @retval None
  */
void Oscilloscope_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* __OSCILLOSCOPE_H */
