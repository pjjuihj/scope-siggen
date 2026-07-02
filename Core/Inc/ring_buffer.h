/**
  ******************************************************************************
  * @file           : ring_buffer.h
  * @brief          : 环形缓冲模块接口
  ******************************************************************************
  * @attention
  *
  * 环形缓冲模块，用于生产者-消费者模式的数据缓冲
  *
  ******************************************************************************
  */

#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

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
  * @brief 环形缓冲结构体
  */
typedef struct {
    uint16_t *buffer;           /*!< 缓冲区指针 */
    uint16_t size;              /*!< 缓冲区大小 */
    volatile uint16_t head;     /*!< 写指针 */
    volatile uint16_t tail;     /*!< 读指针 */
    volatile uint16_t count;    /*!< 数据计数 */
} RingBuffer_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  初始化环形缓冲
  * @param  rb: 环形缓冲指针
  * @param  buffer: 缓冲区指针
  * @param  size: 缓冲区大小
  * @retval 错误码
  */
ErrorCode_t RingBuffer_Init(RingBuffer_t *rb, uint16_t *buffer, uint16_t size);

/**
  * @brief  写入一个数据
  * @param  rb: 环形缓冲指针
  * @param  data: 数据
  * @retval 错误码
  */
ErrorCode_t RingBuffer_Put(RingBuffer_t *rb, uint16_t data);

/**
  * @brief  读取一个数据
  * @param  rb: 环形缓冲指针
  * @param  data: 数据指针
  * @retval 错误码
  */
ErrorCode_t RingBuffer_Get(RingBuffer_t *rb, uint16_t *data);

/**
  * @brief  查看一个数据（不移除）
  * @param  rb: 环形缓冲指针
  * @param  data: 数据指针
  * @retval 错误码
  */
ErrorCode_t RingBuffer_Peek(RingBuffer_t *rb, uint16_t *data);

/**
  * @brief  检查缓冲区是否为空
  * @param  rb: 环形缓冲指针
  * @retval true=空, false=非空
  */
bool RingBuffer_IsEmpty(RingBuffer_t *rb);

/**
  * @brief  检查缓冲区是否为满
  * @param  rb: 环形缓冲指针
  * @retval true=满, false=未满
  */
bool RingBuffer_IsFull(RingBuffer_t *rb);

/**
  * @brief  获取缓冲区中的数据数量
  * @param  rb: 环形缓冲指针
  * @retval 数据数量
  */
uint16_t RingBuffer_Count(RingBuffer_t *rb);

/**
  * @brief  获取缓冲区中的空闲空间
  * @param  rb: 环形缓冲指针
  * @retval 空闲空间
  */
uint16_t RingBuffer_Free(RingBuffer_t *rb);

/**
  * @brief  批量写入数据
  * @param  rb: 环形缓冲指针
  * @param  data: 数据数组
  * @param  len: 数据长度
  * @retval 实际写入的数据数量
  */
uint16_t RingBuffer_PutBlock(RingBuffer_t *rb, const uint16_t *data, uint16_t len);

/**
  * @brief  批量读取数据
  * @param  rb: 环形缓冲指针
  * @param  data: 数据数组
  * @param  len: 数据长度
  * @retval 实际读取的数据数量
  */
uint16_t RingBuffer_GetBlock(RingBuffer_t *rb, uint16_t *data, uint16_t len);

/**
  * @brief  清空缓冲区
  * @param  rb: 环形缓冲指针
  * @retval None
  */
void RingBuffer_Flush(RingBuffer_t *rb);

#ifdef __cplusplus
}
#endif

#endif /* __RING_BUFFER_H */
