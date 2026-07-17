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

/* ===== TDD 新增：NULL 参数校验 ===== */

TEST_CASE(ring_buffer_init_null_rb)
{
    uint16_t buf[4];
    ErrorCode_t err = RingBuffer_Init(NULL, buf, 4);
    ASSERT_EQUAL(ERR_INVALID_PARAM, err);
    return TEST_PASS;
}

TEST_CASE(ring_buffer_init_null_buffer)
{
    RingBuffer_t r;
    ErrorCode_t err = RingBuffer_Init(&r, NULL, 4);
    ASSERT_EQUAL(ERR_INVALID_PARAM, err);
    return TEST_PASS;
}

TEST_CASE(ring_buffer_init_zero_size)
{
    RingBuffer_t r;
    uint16_t buf[4];
    ErrorCode_t err = RingBuffer_Init(&r, buf, 0);
    ASSERT_EQUAL(ERR_INVALID_PARAM, err);
    return TEST_PASS;
}

TEST_CASE(ring_buffer_put_null)
{
    ErrorCode_t err = RingBuffer_Put(NULL, 42);
    ASSERT_EQUAL(ERR_INVALID_PARAM, err);
    return TEST_PASS;
}

TEST_CASE(ring_buffer_get_null_rb)
{
    uint16_t data;
    ErrorCode_t err = RingBuffer_Get(NULL, &data);
    ASSERT_EQUAL(ERR_INVALID_PARAM, err);
    return TEST_PASS;
}

TEST_CASE(ring_buffer_get_null_data)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);
    ErrorCode_t err = RingBuffer_Get(&rb, NULL);
    ASSERT_EQUAL(ERR_INVALID_PARAM, err);
    return TEST_PASS;
}

TEST_CASE(ring_buffer_peek_null_rb)
{
    uint16_t data;
    ErrorCode_t err = RingBuffer_Peek(NULL, &data);
    ASSERT_EQUAL(ERR_INVALID_PARAM, err);
    return TEST_PASS;
}

TEST_CASE(ring_buffer_peek_null_data)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);
    ErrorCode_t err = RingBuffer_Peek(&rb, NULL);
    ASSERT_EQUAL(ERR_INVALID_PARAM, err);
    return TEST_PASS;
}

TEST_CASE(ring_buffer_peek_empty)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);
    uint16_t data;
    ErrorCode_t err = RingBuffer_Peek(&rb, &data);
    ASSERT_EQUAL(ERR_BUFFER_EMPTY, err);
    return TEST_PASS;
}

/* ===== TDD 新增：FIFO 顺序 ===== */

TEST_CASE(ring_buffer_fifo_order)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);

    /* 写入 1,2,3,4 */
    for (uint16_t i = 1; i <= 4; i++) {
        RingBuffer_Put(&rb, i);
    }

    /* 读取顺序应为 1,2,3,4 */
    for (uint16_t i = 1; i <= 4; i++) {
        uint16_t data;
        ErrorCode_t err = RingBuffer_Get(&rb, &data);
        ASSERT_EQUAL(ERR_OK, err);
        ASSERT_EQUAL(i, data);
    }

    return TEST_PASS;
}

/* ===== TDD 新增：Count/Free 一致性 ===== */

TEST_CASE(ring_buffer_count_free_consistency)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);

    /* 初始状态 */
    ASSERT_EQUAL(0, RingBuffer_Count(&rb));
    ASSERT_EQUAL(TEST_BUFFER_SIZE, RingBuffer_Free(&rb));

    /* 写入 5 个 */
    for (uint16_t i = 0; i < 5; i++) {
        RingBuffer_Put(&rb, i);
    }
    ASSERT_EQUAL(5, RingBuffer_Count(&rb));
    ASSERT_EQUAL(TEST_BUFFER_SIZE - 5, RingBuffer_Free(&rb));

    /* 读取 3 个 */
    uint16_t data;
    for (uint16_t i = 0; i < 3; i++) {
        RingBuffer_Get(&rb, &data);
    }
    ASSERT_EQUAL(2, RingBuffer_Count(&rb));
    ASSERT_EQUAL(TEST_BUFFER_SIZE - 2, RingBuffer_Free(&rb));

    /* Count + Free 应始终等于 size */
    ASSERT_EQUAL(TEST_BUFFER_SIZE, RingBuffer_Count(&rb) + RingBuffer_Free(&rb));

    return TEST_PASS;
}

/* ===== TDD 新增：环绕时 FIFO 顺序 ===== */

TEST_CASE(ring_buffer_wrap_fifo_order)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);

    /* 填满后读取一半，再写入，测试环绕后的 FIFO */
    for (uint16_t i = 0; i < TEST_BUFFER_SIZE; i++) {
        RingBuffer_Put(&rb, i);
    }
    /* 读取 8 个，腾出空间 */
    uint16_t data;
    for (uint16_t i = 0; i < TEST_BUFFER_SIZE / 2; i++) {
        RingBuffer_Get(&rb, &data);
    }

    /* 写入新数据 100..107，head 环绕到 0 */
    for (uint16_t i = 0; i < TEST_BUFFER_SIZE / 2; i++) {
        RingBuffer_Put(&rb, 100 + i);
    }

    /* 读取应先得到剩余旧数据 8..15，再得到新数据 100..107 */
    for (uint16_t i = TEST_BUFFER_SIZE / 2; i < TEST_BUFFER_SIZE; i++) {
        RingBuffer_Get(&rb, &data);
        ASSERT_EQUAL(i, data);
    }
    for (uint16_t i = 0; i < TEST_BUFFER_SIZE / 2; i++) {
        RingBuffer_Get(&rb, &data);
        ASSERT_EQUAL(100 + i, data);
    }

    return TEST_PASS;
}

/* ===== TDD 新增：部分 PutBlock ===== */

TEST_CASE(ring_buffer_putblock_partial)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);

    /* 先填满一半 */
    for (uint16_t i = 0; i < TEST_BUFFER_SIZE / 2; i++) {
        RingBuffer_Put(&rb, i);
    }

    /* 尝试写入超过剩余空间的数据 */
    uint16_t write_data[TEST_BUFFER_SIZE] = {0};
    for (uint16_t i = 0; i < TEST_BUFFER_SIZE; i++) {
        write_data[i] = 200 + i;
    }

    uint16_t written = RingBuffer_PutBlock(&rb, write_data, TEST_BUFFER_SIZE);

    /* 应只写入剩余空间 */
    ASSERT_EQUAL(TEST_BUFFER_SIZE / 2, written);
    ASSERT_EQUAL(TEST_BUFFER_SIZE, RingBuffer_Count(&rb));
    ASSERT(RingBuffer_IsFull(&rb));

    return TEST_PASS;
}

/* ===== TDD 新增：部分 GetBlock ===== */

TEST_CASE(ring_buffer_getblock_partial)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);

    /* 只写入 3 个 */
    RingBuffer_Put(&rb, 10);
    RingBuffer_Put(&rb, 20);
    RingBuffer_Put(&rb, 30);

    /* 尝试读取 10 个 */
    uint16_t read_data[10] = {0};
    uint16_t read = RingBuffer_GetBlock(&rb, read_data, 10);

    /* 应只读取 3 个 */
    ASSERT_EQUAL(3, read);
    ASSERT_EQUAL(10, read_data[0]);
    ASSERT_EQUAL(20, read_data[1]);
    ASSERT_EQUAL(30, read_data[2]);
    ASSERT(RingBuffer_IsEmpty(&rb));

    return TEST_PASS;
}

/* ===== TDD 新增：Flush 后可重新使用 ===== */

TEST_CASE(ring_buffer_flush_reuse)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);

    /* 写入数据 */
    for (uint16_t i = 0; i < TEST_BUFFER_SIZE; i++) {
        RingBuffer_Put(&rb, i);
    }

    /* 清空 */
    RingBuffer_Flush(&rb);
    ASSERT(RingBuffer_IsEmpty(&rb));

    /* 重新写入，应正常工作 */
    for (uint16_t i = 50; i < 50 + TEST_BUFFER_SIZE; i++) {
        ErrorCode_t err = RingBuffer_Put(&rb, i);
        ASSERT_EQUAL(ERR_OK, err);
    }

    /* 读取验证 */
    uint16_t data;
    for (uint16_t i = 50; i < 50 + TEST_BUFFER_SIZE; i++) {
        RingBuffer_Get(&rb, &data);
        ASSERT_EQUAL(i, data);
    }

    return TEST_PASS;
}

/* ===== TDD 新增：IsFull 在 NULL 时返回 false ===== */

TEST_CASE(ring_buffer_isfull_null)
{
    ASSERT(!RingBuffer_IsFull(NULL));
    return TEST_PASS;
}

/* ===== TDD 新增：IsEmpty 在 NULL 时返回 true ===== */

TEST_CASE(ring_buffer_isempty_null)
{
    ASSERT(RingBuffer_IsEmpty(NULL));
    return TEST_PASS;
}

/* ===== TDD 新增：Count/Free 在 NULL 时返回 0 ===== */

TEST_CASE(ring_buffer_count_null)
{
    ASSERT_EQUAL(0, RingBuffer_Count(NULL));
    return TEST_PASS;
}

TEST_CASE(ring_buffer_free_null)
{
    ASSERT_EQUAL(0, RingBuffer_Free(NULL));
    return TEST_PASS;
}

/* ===== TDD 新增：PutBlock/GetBlock NULL 参数 ===== */

TEST_CASE(ring_buffer_putblock_null_rb)
{
    uint16_t data[4] = {1, 2, 3, 4};
    uint16_t written = RingBuffer_PutBlock(NULL, data, 4);
    ASSERT_EQUAL(0, written);
    return TEST_PASS;
}

TEST_CASE(ring_buffer_putblock_null_data)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);
    uint16_t written = RingBuffer_PutBlock(&rb, NULL, 4);
    ASSERT_EQUAL(0, written);
    return TEST_PASS;
}

TEST_CASE(ring_buffer_putblock_zero_len)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);
    uint16_t data[4] = {1, 2, 3, 4};
    uint16_t written = RingBuffer_PutBlock(&rb, data, 0);
    ASSERT_EQUAL(0, written);
    return TEST_PASS;
}

TEST_CASE(ring_buffer_getblock_null_rb)
{
    uint16_t data[4];
    uint16_t read = RingBuffer_GetBlock(NULL, data, 4);
    ASSERT_EQUAL(0, read);
    return TEST_PASS;
}

TEST_CASE(ring_buffer_getblock_null_data)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);
    uint16_t read = RingBuffer_GetBlock(&rb, NULL, 4);
    ASSERT_EQUAL(0, read);
    return TEST_PASS;
}

TEST_CASE(ring_buffer_getblock_zero_len)
{
    RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);
    uint16_t data[4];
    uint16_t read = RingBuffer_GetBlock(&rb, data, 0);
    ASSERT_EQUAL(0, read);
    return TEST_PASS;
}

/* ===== TDD 新增：Flush NULL 安全 ===== */

TEST_CASE(ring_buffer_flush_null)
{
    /* 不应崩溃 */
    RingBuffer_Flush(NULL);
    return TEST_PASS;
}

/* Private function implementations ------------------------------------------*/
