/**
  ******************************************************************************
  * @file           : test_ring_buffer.c
  * @brief          : 环形缓冲模块测试
  ******************************************************************************
  * @attention
  *
  * 环形缓冲模块单元测试
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "test_framework.h"
#include "ring_buffer.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

#define TEST_BUFFER_SIZE 16

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static uint16_t test_buffer[TEST_BUFFER_SIZE];
static RingBuffer_t rb;

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Exported function implementations -----------------------------------------*/

TEST_CASE(ring_buffer_init)
{
    ErrorCode_t err = RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);
    ASSERT_EQUAL(ERR_OK, err);
    ASSERT(RingBuffer_IsEmpty(&rb));
    ASSERT_EQUAL(0, RingBuffer_Count(&rb));
    ASSERT_EQUAL(TEST_BUFFER_SIZE, RingBuffer_Free(&rb));

    return TEST_PASS;
}

TEST_CASE(ring_buffer_put_get)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);

    /* 写入数据 */
    ErrorCode_t err = RingBuffer_Put(&rb, 42);
    ASSERT_EQUAL(ERR_OK, err);
    ASSERT_EQUAL(1, RingBuffer_Count(&rb));

    /* 读取数据 */
    uint16_t data;
    err = RingBuffer_Get(&rb, &data);
    ASSERT_EQUAL(ERR_OK, err);
    ASSERT_EQUAL(42, data);
    ASSERT(RingBuffer_IsEmpty(&rb));

    return TEST_PASS;
}

TEST_CASE(ring_buffer_full)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);

    /* 填满缓冲区 */
    for (uint16_t i = 0; i < TEST_BUFFER_SIZE; i++) {
        ErrorCode_t err = RingBuffer_Put(&rb, i);
        ASSERT_EQUAL(ERR_OK, err);
    }

    /* 检查是否满 */
    ASSERT(RingBuffer_IsFull(&rb));
    ASSERT_EQUAL(0, RingBuffer_Free(&rb));

    /* 尝试写入应该失败 */
    ErrorCode_t err = RingBuffer_Put(&rb, 99);
    ASSERT_EQUAL(ERR_BUFFER_FULL, err);

    return TEST_PASS;
}

TEST_CASE(ring_buffer_empty)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);

    /* 尝试读取应该失败 */
    uint16_t data;
    ErrorCode_t err = RingBuffer_Get(&rb, &data);
    ASSERT_EQUAL(ERR_BUFFER_EMPTY, err);

    return TEST_PASS;
}

TEST_CASE(ring_buffer_peek)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);

    /* 写入数据 */
    RingBuffer_Put(&rb, 42);

    /* Peek应该不移除数据 */
    uint16_t data;
    ErrorCode_t err = RingBuffer_Peek(&rb, &data);
    ASSERT_EQUAL(ERR_OK, err);
    ASSERT_EQUAL(42, data);
    ASSERT_EQUAL(1, RingBuffer_Count(&rb));

    return TEST_PASS;
}

TEST_CASE(ring_buffer_wrap)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);

    /* 写入并读取，测试环绕 */
    for (uint16_t i = 0; i < TEST_BUFFER_SIZE * 2; i++) {
        RingBuffer_Put(&rb, i);
        uint16_t data;
        RingBuffer_Get(&rb, &data);
        ASSERT_EQUAL(i, data);
    }

    return TEST_PASS;
}

TEST_CASE(ring_buffer_block)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);

    /* 批量写入 */
    uint16_t write_data[4] = {1, 2, 3, 4};
    uint16_t written = RingBuffer_PutBlock(&rb, write_data, 4);
    ASSERT_EQUAL(4, written);
    ASSERT_EQUAL(4, RingBuffer_Count(&rb));

    /* 批量读取 */
    uint16_t read_data[4];
    uint16_t read = RingBuffer_GetBlock(&rb, read_data, 4);
    ASSERT_EQUAL(4, read);
    ASSERT_EQUAL(1, read_data[0]);
    ASSERT_EQUAL(2, read_data[1]);
    ASSERT_EQUAL(3, read_data[2]);
    ASSERT_EQUAL(4, read_data[3]);

    return TEST_PASS;
}

TEST_CASE(ring_buffer_flush)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);

    /* 写入数据 */
    RingBuffer_Put(&rb, 1);
    RingBuffer_Put(&rb, 2);
    RingBuffer_Put(&rb, 3);

    /* 清空 */
    RingBuffer_Flush(&rb);
    ASSERT(RingBuffer_IsEmpty(&rb));
    ASSERT_EQUAL(0, RingBuffer_Count(&rb));

    return TEST_PASS;
}

/* Private function implementations ------------------------------------------*/
