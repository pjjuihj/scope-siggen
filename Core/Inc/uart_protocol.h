/**
  ******************************************************************************
  * @file           : uart_protocol.h
  * @brief          : 串口协议模块接口
  ******************************************************************************
  * @attention
  *
  * 串口协议模块，用于命令控制和数据传输
  *
  ******************************************************************************
  */

#ifndef __UART_PROTOCOL_H
#define __UART_PROTOCOL_H

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
  * @brief 协议模式定义
  */
typedef enum {
    PROTOCOL_TEXT = 0,      /* 文本模式 */
    PROTOCOL_BINARY         /* 二进制模式 */
} ProtocolMode_t;

/**
  * @brief 命令处理器函数类型
  * @param  param: 参数字符串（无参数时为 NULL）
  */
typedef void (*CmdHandler_t)(const char *param);

/**
  * @brief 命令表条目
  */
typedef struct {
    const char *name;       /* 命令名称 */
    uint8_t min_len;        /* 最短匹配长度 */
    bool has_param;         /* 是否需要参数 */
    CmdHandler_t handler;   /* 处理函数 */
} CommandEntry_t;

/* Exported constants --------------------------------------------------------*/

/* 命令最大长度 */
#define CMD_MAX_LENGTH      64

/* 响应最大长度 */
#define RESP_MAX_LENGTH     128

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  初始化串口协议模块
  * @retval 错误码
  */
ErrorCode_t UART_Protocol_Init(void);

/**
  * @brief  启动串口协议
  * @retval 错误码
  */
ErrorCode_t UART_Protocol_Start(void);

/**
  * @brief  停止串口协议
  * @retval 错误码
  */
ErrorCode_t UART_Protocol_Stop(void);

/**
  * @brief  发送文本
  * @param  text: 文本字符串
  * @retval 错误码
  */
ErrorCode_t UART_SendText(const char *text);

/**
  * @brief  发送数据
  * @param  data: 数据缓冲区
  * @param  len: 数据长度
  * @retval 错误码
  */
ErrorCode_t UART_SendData(const uint8_t *data, uint16_t len);

/**
  * @brief  发送响应
  * @param  cmd: 命令字符串
  * @param  result: 结果
  * @retval 错误码
  */
ErrorCode_t UART_SendResponse(const char *cmd, ErrorCode_t result);

/**
  * @brief  接收命令
  * @param  cmd: 命令字符串缓冲区
  * @param  max_len: 最大长度
  * @retval 错误码
  */
ErrorCode_t UART_ReceiveCommand(char *cmd, uint16_t max_len);

/**
  * @brief  设置协议模式
  * @param  mode: 协议模式
  * @retval 错误码
  */
ErrorCode_t UART_SetProtocolMode(ProtocolMode_t mode);

/**
  * @brief  执行命令（通过命令表匹配并分发）
  * @param  cmd_str: 原始命令字符串
  * @retval 错误码
  */
ErrorCode_t UART_ExecuteCommand(const char *cmd_str);

/**
  * @brief  串口协议任务
  * @param  argument: 任务参数
  * @retval None
  */
void UART_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* __UART_PROTOCOL_H */
