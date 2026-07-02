/**
  ******************************************************************************
  * @file           : ring_buffer.c
  * @brief          : 环形缓冲模块实现
  ******************************************************************************
  * @attention
  *
  * 环形缓冲模块，用于生产者-消费者模式的数据缓冲
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "ring_buffer.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Exported function implementations -----------------------------------------*/

ErrorCode_t RingBuffer_Init(RingBuffer_t *rb, uint16_t *buffer, uint16_t size)
{
    if (rb == NULL || buffer == NULL || size == 0) {
        return ERR_INVALID_PARAM;
    }

    rb->buffer = buffer;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;

    /* 清空缓冲区 */
    memset(buffer, 0, size * sizeof(uint16_t));

    return ERR_OK;
}

ErrorCode_t RingBuffer_Put(RingBuffer_t *rb, uint16_t data)
{
    if (rb == NULL) {
        return ERR_INVALID_PARAM;
    }

    /* 检查缓冲区是否为满 */
    if (rb->count >= rb->size) {
        return ERR_BUFFER_FULL;
    }

    /* 写入数据 */
    rb->buffer[rb->head] = data;

    /* 更新写指针 */
    rb->head = (rb->head + 1) % rb->size;

    /* 更新计数 */
    rb->count++;

    return ERR_OK;
}

ErrorCode_t RingBuffer_Get(RingBuffer_t *rb, uint16_t *data)
{
    if (rb == NULL || data == NULL) {
        return ERR_INVALID_PARAM;
    }

    /* 检查缓冲区是否为空 */
    if (rb->count == 0) {
        return ERR_BUFFER_EMPTY;
    }

    /* 读取数据 */
    *data = rb->buffer[rb->tail];

    /* 更新读指针 */
    rb->tail = (rb->tail + 1) % rb->size;

    /* 更新计数 */
    rb->count--;

    return ERR_OK;
}

ErrorCode_t RingBuffer_Peek(RingBuffer_t *rb, uint16_t *data)
{
    if (rb == NULL || data == NULL) {
        return ERR_INVALID_PARAM;
    }

    /* 检查缓冲区是否为空 */
    if (rb->count == 0) {
        return ERR_BUFFER_EMPTY;
    }

    /* 读取数据（不移除） */
    *data = rb->buffer[rb->tail];

    return ERR_OK;
}

bool RingBuffer_IsEmpty(RingBuffer_t *rb)
{
    if (rb == NULL) {
        return true;
    }

    return (rb->count == 0);
}

bool RingBuffer_IsFull(RingBuffer_t *rb)
{
    if (rb == NULL) {
        return false;
    }

    return (rb->count >= rb->size);
}

uint16_t RingBuffer_Count(RingBuffer_t *rb)
{
    if (rb == NULL) {
        return 0;
    }

    return rb->count;
}

uint16_t RingBuffer_Free(RingBuffer_t *rb)
{
    if (rb == NULL) {
        return 0;
    }

    return (rb->size - rb->count);
}

uint16_t RingBuffer_PutBlock(RingBuffer_t *rb, const uint16_t *data, uint16_t len)
{
    if (rb == NULL || data == NULL || len == 0) {
        return 0;
    }

    uint16_t written = 0;

    /* 写入数据 */
    for (uint16_t i = 0; i < len; i++) {
        if (RingBuffer_Put(rb, data[i]) != ERR_OK) {
            break;
        }
        written++;
    }

    return written;
}

uint16_t RingBuffer_GetBlock(RingBuffer_t *rb, uint16_t *data, uint16_t len)
{
    if (rb == NULL || data == NULL || len == 0) {
        return 0;
    }

    uint16_t read = 0;

    /* 读取数据 */
    for (uint16_t i = 0; i < len; i++) {
        if (RingBuffer_Get(rb, &data[i]) != ERR_OK) {
            break;
        }
        read++;
    }

    return read;
}

void RingBuffer_Flush(RingBuffer_t *rb)
{
    if (rb == NULL) {
        return;
    }

    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

/* Private function implementations ------------------------------------------*/

