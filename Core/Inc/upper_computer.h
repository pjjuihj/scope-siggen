/**
  ******************************************************************************
  * @file           : upper_computer.h
  * @brief          : 上位机通信模块头文件
  ******************************************************************************
  * @attention
  *
  * 上位机通信模块，负责将示波器数据发送到上位机
  * 支持波形数据、频率、电压、状态等信息的发送
  *
  ******************************************************************************
  */

#ifndef __UPPER_COMPUTER_H
#define __UPPER_COMPUTER_H

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
  * @brief 上位机通信状态
  */
typedef enum {
    UC_STATE_IDLE = 0,          /* 空闲状态 */
    UC_STATE_STREAMING,         /* 流式传输中 */
    UC_STATE_ERROR              /* 错误状态 */
} UC_State_t;

/**
  * @brief 上位机通信配置
  */
typedef struct {
    uint8_t enabled;            /* 是否启用 */
    uint8_t stream_mode;        /* 流模式: 0=手动, 1=自动 */
    uint32_t stream_interval;   /* 流式传输间隔(ms) */
    uint8_t data_format;        /* 数据格式: 0=hex, 1=base64 */
} UC_Config_t;

/* Exported constants --------------------------------------------------------*/

/* 默认配置 */
#define UC_DEFAULT_STREAM_INTERVAL    100     /* 默认传输间隔100ms */
#define UC_MAX_PACKET_SIZE            2048    /* 最大数据包大小 */

/* 数据前缀 */
#define UC_PREFIX_WAVE      "WAVE:"
#define UC_PREFIX_FREQ      "FREQ:"
#define UC_PREFIX_VOLT      "VOLT:"
#define UC_PREFIX_STATUS    "STATUS:"
#define UC_PREFIX_ERROR     "ERROR:"
#define UC_PREFIX_OK        "OK:"
#define UC_PREFIX_INFO      "INFO:"

/* Exported macro ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief 初始化上位机通信模块
  * @retval ErrorCode_t 错误码
  */
ErrorCode_t UpperComputer_Init(void);

/**
  * @brief 发送波形数据
  * @param data 波形数据数组
  * @param len 数据长度
  * @retval ErrorCode_t 错误码
  */
ErrorCode_t UC_SendWaveform(const uint16_t *data, uint16_t len);

/**
  * @brief 发送频率数据
  * @param freq_hz 频率值(Hz)
  * @retval ErrorCode_t 错误码
  */
ErrorCode_t UC_SendFrequency(uint32_t freq_hz);

/**
  * @brief 发送电压数据
  * @param min_mv 最小电压(mV)
  * @param max_mv 最大电压(mV)
  * @param pp_mv 峰峰值电压(mV)
  * @retval ErrorCode_t 错误码
  */
ErrorCode_t UC_SendVoltage(uint32_t min_mv, uint32_t max_mv, uint32_t pp_mv);

/**
  * @brief 发送状态信息
  * @param status_json JSON格式的状态字符串
  * @retval ErrorCode_t 错误码
  */
ErrorCode_t UC_SendStatus(const char *status_json);

/**
  * @brief 发送测量数据（包含频率和电压）
  * @param freq_hz 频率值(Hz)
  * @param min_mv 最小电压(mV)
  * @param max_mv 最大电压(mV)
  * @retval ErrorCode_t 错误码
  */
ErrorCode_t UC_SendMeasurement(uint32_t freq_hz, uint32_t min_mv, uint32_t max_mv);

/**
  * @brief 发送完整数据包（波形+测量）
  * @param waveform 波形数据
  * @param len 波形长度
  * @param freq_hz 频率
  * @param min_mv 最小电压
  * @param max_mv 最大电压
  * @retval ErrorCode_t 错误码
  */
ErrorCode_t UC_SendCompletePacket(const uint16_t *waveform, uint16_t len,
                                   uint32_t freq_hz, uint32_t min_mv, uint32_t max_mv);

/**
  * @brief 启用/禁用流式传输
  * @param enable true=启用, false=禁用
  * @retval ErrorCode_t 错误码
  */
ErrorCode_t UC_SetStreamMode(bool enable);

/**
  * @brief 设置流式传输间隔
  * @param interval_ms 间隔时间(ms)
  * @retval ErrorCode_t 错误码
  */
ErrorCode_t UC_SetStreamInterval(uint32_t interval_ms);

/**
  * @brief 获取上位机通信状态
  * @retval UC_State_t 当前状态
  */
UC_State_t UC_GetState(void);

/**
  * @brief 检查是否正在流式传输
  * @retval true 正在传输, false 未传输
  */
bool UC_IsStreaming(void);

/**
  * @brief 发送OK响应
  * @param msg 附加消息
  * @retval ErrorCode_t 错误码
  */
ErrorCode_t UC_SendOK(const char *msg);

/**
  * @brief 发送错误响应
  * @param msg 错误消息
  * @retval ErrorCode_t 错误码
  */
ErrorCode_t UC_SendError(const char *msg);

/**
  * @brief 发送信息响应
  * @param msg 信息消息
  * @retval ErrorCode_t 错误码
  */
ErrorCode_t UC_SendInfo(const char *msg);

#ifdef __cplusplus
}
#endif

#endif /* __UPPER_COMPUTER_H */
