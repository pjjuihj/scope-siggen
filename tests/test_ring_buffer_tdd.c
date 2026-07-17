/**
  ******************************************************************************
  * @file           : test_ring_buffer_tdd.c
  * @brief          : ring_buffer TDD 测试（逐切片红灯）
  ******************************************************************************
  *
  *  TDD 流程：每个切片先写红灯测试，再写最小实现
  *
  ******************************************************************************
  */

#include "test_framework.h"
#include "ring_buffer.h"
#include <string.h>

#define TEST_BUFFER_SIZE 16

static uint16_t test_buffer[TEST_BUFFER_SIZE];
static RingBuffer_t rb;

/* ============================================================
 *  Slice 1: Init
 *  红灯测试 → 写 Init 实现 → 绿灯
 * ============================================================ */

/* S1-T1: NULL rb 应返回 ERR_INVALID_PARAM */
TEST_CASE(s1_init_null_rb)
{
    uint16_t buf[4];
    ErrorCode_t err = RingBuffer_Init(NULL, buf, 4);
    ASSERT_EQUAL(ERR_INVALID_PARAM, err);
    return TEST_PASS;
}

/* S1-T2: NULL buffer 应返回 ERR_INVALID_PARAM */
TEST_CASE(s1_init_null_buffer)
{
    RingBuffer_t r;
    ErrorCode_t err = RingBuffer_Init(&r, NULL, 4);
    ASSERT_EQUAL(ERR_INVALID_PARAM, err);
    return TEST_PASS;
}

/* S1-T3: size=0 应返回 ERR_INVALID_PARAM */
TEST_CASE(s1_init_zero_size)
{
    RingBuffer_t r;
    uint16_t buf[4];
    ErrorCode_t err = RingBuffer_Init(&r, buf, 0);
    ASSERT_EQUAL(ERR_INVALID_PARAM, err);
    return TEST_PASS;
}

/* S1-T4: 正常初始化应返回 ERR_OK，缓冲区为空 */
TEST_CASE(s1_init_valid)
{
    ErrorCode_t err = RingBuffer_Init(&rb, test_buffer, TEST_BUFFER_SIZE);
    ASSERT_EQUAL(ERR_OK, err);
    ASSERT(RingBuffer_IsEmpty(&rb));
    return TEST_PASS;
}
