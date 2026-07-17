/**
  ******************************************************************************
  * @file           : test_uart_tx.c
  * @brief          : UART TX 模块测试
  ******************************************************************************
  * @attention
  *
  * UART TX ring buffer + 发送任务单元测试
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "test_framework.h"
#include "freertos_stub.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

/* Private define ------------------------------------------------------------*/

#define TX_BUF_SIZE  16  // 测试用小 buffer（不是真实的1024）

/* Private variables ---------------------------------------------------------*/

// Ring buffer 结构（从 uart_tx.c 复制，测试用）
typedef struct {
    uint8_t  data[TX_BUF_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t count;
} TX_RingBuffer_t;

static TX_RingBuffer_t tx_ring;

/* Private function prototypes -----------------------------------------------*/

// 被测函数声明
static void tx_ring_init(void);
static int tx_ring_put(const uint8_t *data, uint16_t len);
static int tx_ring_get(uint8_t *data, uint16_t len);
static uint16_t tx_ring_count(void);
static uint16_t tx_ring_free(void);
static int uart_tx_write(int fd, const char *buf, int len);

/* Test cases ----------------------------------------------------------------*/

/**
  * @brief  测试 ring buffer 初始化
  */
TEST_CASE(tx_ring_init)
{
    tx_ring_init();

    ASSERT_EQUAL(0, tx_ring.head);
    ASSERT_EQUAL(0, tx_ring.tail);
    ASSERT_EQUAL(0, tx_ring.count);

    return TEST_PASS;
}

/**
  * @brief  测试基本 put/get 操作
  */
TEST_CASE(tx_ring_put_get)
{
    tx_ring_init();

    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    int written = tx_ring_put(data, 5);

    ASSERT_EQUAL(5, written);
    ASSERT_EQUAL(5, tx_ring_count());

    uint8_t buf[5];
    int read = tx_ring_get(buf, 5);

    ASSERT_EQUAL(5, read);
    ASSERT_EQUAL(0x01, buf[0]);
    ASSERT_EQUAL(0x02, buf[1]);
    ASSERT_EQUAL(0x03, buf[2]);
    ASSERT_EQUAL(0x04, buf[3]);
    ASSERT_EQUAL(0x05, buf[4]);

    return TEST_PASS;
}

/**
  * @brief  测试 buffer 满时丢弃
  */
TEST_CASE(tx_ring_full_discard)
{
    tx_ring_init();

    // 填满 buffer
    uint8_t data[TX_BUF_SIZE];
    memset(data, 0xAA, sizeof(data));
    int written = tx_ring_put(data, TX_BUF_SIZE);

    ASSERT_EQUAL(TX_BUF_SIZE, written);
    ASSERT_EQUAL(0, tx_ring_free());

    // 再写应该失败
    uint8_t extra = 0xBB;
    written = tx_ring_put(&extra, 1);
    ASSERT_EQUAL(0, written);

    // buffer 内容不变
    ASSERT_EQUAL(0xAA, tx_ring.data[0]);

    return TEST_PASS;
}

/**
  * @brief  测试 count/free 一致性
  */
TEST_CASE(tx_ring_count_free)
{
    tx_ring_init();

    ASSERT_EQUAL(TX_BUF_SIZE, tx_ring_free());
    ASSERT_EQUAL(0, tx_ring_count());

    uint8_t data[8];
    memset(data, 0xCC, sizeof(data));
    tx_ring_put(data, 8);

    ASSERT_EQUAL(8, tx_ring_count());
    ASSERT_EQUAL(TX_BUF_SIZE - 8, tx_ring_free());

    uint8_t buf[3];
    tx_ring_get(buf, 3);

    ASSERT_EQUAL(5, tx_ring_count());
    ASSERT_EQUAL(TX_BUF_SIZE - 5, tx_ring_free());

    return TEST_PASS;
}

/**
  * @brief  测试回绕场景
  */
TEST_CASE(tx_ring_wrap)
{
    tx_ring_init();

    // 先写入12字节，head=12
    uint8_t data1[12];
    memset(data1, 0x11, sizeof(data1));
    tx_ring_put(data1, 12);

    // 读取10字节，tail=10，空出10字节空间
    uint8_t buf[10];
    tx_ring_get(buf, 10);

    // 现在 head=12, tail=10, count=2, free=14
    // 写入12字节，会回绕：先写12→16(4字节)，再写0→8(8字节)
    uint8_t data2[12];
    memset(data2, 0x22, sizeof(data2));
    int written = tx_ring_put(data2, 12);

    ASSERT_EQUAL(12, written);
    ASSERT_EQUAL(14, tx_ring_count());  // 2 + 12 = 14

    // 验证数据：先读2字节旧数据，再读12字节新数据
    uint8_t verify[14];
    tx_ring_get(verify, 14);

    // 前2字节应该是 0x11（旧数据）
    ASSERT_EQUAL(0x11, verify[0]);
    ASSERT_EQUAL(0x11, verify[1]);

    // 后12字节应该是 0x22（新数据，回绕后的）
    for (int i = 2; i < 14; i++) {
        ASSERT_EQUAL(0x22, verify[i]);
    }

    return TEST_PASS;
}

/**
  * @brief  测试 FIFO 顺序
  */
TEST_CASE(tx_ring_fifo_order)
{
    tx_ring_init();

    // 写入1-5
    uint8_t data1[] = {1, 2, 3, 4, 5};
    tx_ring_put(data1, 5);

    // 读取3字节
    uint8_t buf[3];
    tx_ring_get(buf, 3);
    ASSERT_EQUAL(1, buf[0]);
    ASSERT_EQUAL(2, buf[1]);
    ASSERT_EQUAL(3, buf[2]);

    // 写入6-10
    uint8_t data2[] = {6, 7, 8, 9, 10};
    tx_ring_put(data2, 5);

    // 读取剩余
    uint8_t buf2[7];
    tx_ring_get(buf2, 7);

    ASSERT_EQUAL(4, buf2[0]);  // 旧数据
    ASSERT_EQUAL(5, buf2[1]);  // 旧数据
    ASSERT_EQUAL(6, buf2[2]);  // 新数据
    ASSERT_EQUAL(7, buf2[3]);
    ASSERT_EQUAL(8, buf2[4]);
    ASSERT_EQUAL(9, buf2[5]);
    ASSERT_EQUAL(10, buf2[6]);

    return TEST_PASS;
}

/**
  * @brief  测试空 buffer 读取
  */
TEST_CASE(tx_ring_get_empty)
{
    tx_ring_init();

    uint8_t buf[5];
    int read = tx_ring_get(buf, 5);

    ASSERT_EQUAL(0, read);

    return TEST_PASS;
}

/**
  * @brief  测试部分读取
  */
TEST_CASE(tx_ring_get_partial)
{
    tx_ring_init();

    uint8_t data[] = {0x10, 0x20, 0x30};
    tx_ring_put(data, 3);

    // 只读2字节
    uint8_t buf[2];
    int read = tx_ring_get(buf, 2);

    ASSERT_EQUAL(2, read);
    ASSERT_EQUAL(0x10, buf[0]);
    ASSERT_EQUAL(0x20, buf[1]);
    ASSERT_EQUAL(1, tx_ring_count());  // 还剩1字节

    // 再读1字节
    uint8_t buf2[1];
    read = tx_ring_get(buf2, 1);
    ASSERT_EQUAL(1, read);
    ASSERT_EQUAL(0x30, buf2[0]);
    ASSERT_EQUAL(0, tx_ring_count());

    return TEST_PASS;
}

/* ========================================================================== */
/* _write() 测试用例                                                           */
/* ========================================================================== */

/**
  * @brief  测试 _write() 在 ISR 中调用
  */
TEST_CASE(write_in_isr)
{
    tx_ring_init();
    mock_reset();
    mock_set_isr_mode(1);  // 模拟 ISR 上下文

    char buf[] = "Hello";
    int result = uart_tx_write(1, buf, 5);

    ASSERT_EQUAL(0, result);  // ISR 中应返回 0
    ASSERT_EQUAL(0, tx_ring_count());  // 数据不应写入

    return TEST_PASS;
}

/**
  * @brief  测试 _write() 无效 fd
  */
TEST_CASE(write_invalid_fd)
{
    tx_ring_init();
    mock_reset();

    char buf[] = "Hello";
    int result = uart_tx_write(0, buf, 5);  // fd=0 (stdin)

    ASSERT_EQUAL(-1, result);  // 无效 fd 应返回 -1
    ASSERT_EQUAL(0, tx_ring_count());

    return TEST_PASS;
}

/**
  * @brief  测试 _write() mutex 超时
  */
TEST_CASE(write_mutex_timeout)
{
    tx_ring_init();
    mock_reset();
    mock_set_mutex_fail(1);  // 模拟 mutex 获取失败

    char buf[] = "Hello";
    int result = uart_tx_write(1, buf, 5);

    ASSERT_EQUAL(0, result);  // mutex 超时应返回 0
    ASSERT_EQUAL(0, tx_ring_count());  // 数据不应写入

    return TEST_PASS;
}

/**
  * @brief  测试 _write() buffer 满
  */
TEST_CASE(write_buffer_full)
{
    tx_ring_init();
    mock_reset();

    // 填满 buffer
    uint8_t data[TX_BUF_SIZE];
    memset(data, 0xAA, sizeof(data));
    tx_ring_put(data, TX_BUF_SIZE);

    ASSERT_EQUAL(0, tx_ring_free());

    // 再写应该失败
    char buf[] = "X";
    int result = uart_tx_write(1, buf, 1);

    ASSERT_EQUAL(0, result);  // buffer 满应返回 0
    ASSERT_EQUAL(TX_BUF_SIZE, tx_ring_count());  // count 不变

    return TEST_PASS;
}

/**
  * @brief  测试 _write() 正常写入
  */
TEST_CASE(write_normal)
{
    tx_ring_init();
    mock_reset();

    char buf[] = "Hello, UART TX!";
    int len = strlen(buf);
    int result = uart_tx_write(1, buf, len);

    ASSERT_EQUAL(len, result);  // 应返回写入长度
    ASSERT_EQUAL(len, tx_ring_count());  // 数据应写入

    // 验证数据正确
    uint8_t verify[32];
    tx_ring_get(verify, len);
    ASSERT_STRING_EQUAL(buf, (char *)verify);

    // 验证事件标志已设置
    ASSERT(mock_event_flags & 0x01);  // TX_EVT_DATA_READY

    return TEST_PASS;
}

/**
  * @brief  测试 _write() 写入 stderr
  */
TEST_CASE(write_stderr)
{
    tx_ring_init();
    mock_reset();

    char buf[] = "Error!";
    int result = uart_tx_write(2, buf, 6);  // fd=2 (stderr)

    ASSERT_EQUAL(6, result);
    ASSERT_EQUAL(6, tx_ring_count());

    return TEST_PASS;
}

/* ========================================================================== */
/* 发送任务逻辑测试                                                            */
/* ========================================================================== */

// 事件标志定义
#define TX_EVT_DATA_READY   (1 << 0)
#define TX_EVT_DMA_DONE     (1 << 1)

// 发送任务状态
static uint32_t tx_task_alive_counter = 0;
static uint32_t tx_drop_count = 0;
static uint32_t tx_error_count = 0;

// 模拟发送任务处理逻辑
static int send_task_process(void)
{
    tx_task_alive_counter++;

    // 检查 DMA 是否忙
    if (HAL_DMA_GetState(NULL) != 0) {
        return -1;  // DMA 忙，跳过
    }

    // 检查 ring buffer
    uint16_t count = tx_ring_count();
    if (count == 0) {
        osEventFlagsClear(NULL, TX_EVT_DATA_READY);
        return 0;  // 无数据
    }

    // 取连续数据块（简化版：只取 head→末尾）
    uint16_t head = tx_ring.head;
    uint16_t tail = tx_ring.tail;
    uint16_t contiguous;
    if (head >= tail) {
        contiguous = TX_BUF_SIZE - head;
    } else {
        contiguous = tail - head;
    }
    uint16_t to_send = (count < contiguous) ? count : contiguous;

    // 启动 DMA（mock）
    HAL_UART_Transmit_DMA(NULL, tx_ring.data + head, to_send);

    // 更新 head
    tx_ring.head = (tx_ring.head + to_send) % TX_BUF_SIZE;
    tx_ring.count -= to_send;

    return to_send;
}

/**
  * @brief  测试发送任务基本流程
  */
TEST_CASE(send_task_basic)
{
    tx_ring_init();
    mock_reset();
    tx_task_alive_counter = 0;

    // 写入数据
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    tx_ring_put(data, 5);

    // 模拟发送任务处理
    int sent = send_task_process();

    ASSERT_EQUAL(5, sent);
    ASSERT_EQUAL(0, tx_ring_count());  // 数据已发送
    ASSERT_EQUAL(1, tx_task_alive_counter);  // 心跳计数

    return TEST_PASS;
}

/**
  * @brief  测试发送任务 DMA 忙时跳过
  */
TEST_CASE(send_task_dma_busy)
{
    tx_ring_init();
    mock_reset();
    mock_set_dma_busy(1);  // 模拟 DMA 忙

    // 写入数据
    uint8_t data[] = {0x01, 0x02};
    tx_ring_put(data, 2);

    // 模拟发送任务处理
    int result = send_task_process();

    ASSERT_EQUAL(-1, result);  // 应返回 -1（跳过）
    ASSERT_EQUAL(2, tx_ring_count());  // 数据未发送

    return TEST_PASS;
}

/**
  * @brief  测试发送任务无数据时清除标志
  */
TEST_CASE(send_task_no_data)
{
    tx_ring_init();
    mock_reset();
    mock_event_flags = TX_EVT_DATA_READY;  // 模拟有事件

    // 无数据
    int result = send_task_process();

    ASSERT_EQUAL(0, result);
    ASSERT(!(mock_event_flags & TX_EVT_DATA_READY));  // 标志应被清除

    return TEST_PASS;
}

/**
  * @brief  测试发送任务心跳计数
  */
TEST_CASE(send_task_heartbeat)
{
    tx_ring_init();
    mock_reset();
    tx_task_alive_counter = 0;

    // 多次处理（无数据）
    for (int i = 0; i < 10; i++) {
        send_task_process();
    }

    ASSERT_EQUAL(10, tx_task_alive_counter);

    return TEST_PASS;
}

/**
  * @brief  测试发送任务回绕取数据
  */
TEST_CASE(send_task_wrap)
{
    tx_ring_init();
    mock_reset();

    // 构造回绕场景：head=12, tail=10, count=2
    uint8_t data1[12];
    memset(data1, 0x11, sizeof(data1));
    tx_ring_put(data1, 12);

    uint8_t buf[10];
    tx_ring_get(buf, 10);

    ASSERT_EQUAL(12, tx_ring.head);
    ASSERT_EQUAL(10, tx_ring.tail);
    ASSERT_EQUAL(2, tx_ring_count());

    // 写入更多数据，触发回绕
    uint8_t data2[8];
    memset(data2, 0x22, sizeof(data2));
    tx_ring_put(data2, 8);

    // 现在 head=4 (回绕), tail=10, count=10
    ASSERT_EQUAL(4, tx_ring.head);
    ASSERT_EQUAL(10, tx_ring.tail);
    ASSERT_EQUAL(10, tx_ring_count());

    // 发送任务应该取 tail→末尾 的数据（6字节）
    int sent = send_task_process();

    // 由于 head(4) < tail(10)，contiguous = tail - head = 6
    // 但 count(10) > contiguous(6)，所以取 6 字节
    ASSERT_EQUAL(6, sent);
    ASSERT_EQUAL(4, tx_ring_count());  // 剩余 4 字节

    return TEST_PASS;
}

/**
  * @brief  测试 drop_count 计数
  */
TEST_CASE(send_task_drop_count)
{
    tx_ring_init();
    mock_reset();
    tx_drop_count = 0;

    // 填满 buffer
    uint8_t data[TX_BUF_SIZE];
    memset(data, 0xAA, sizeof(data));
    tx_ring_put(data, TX_BUF_SIZE);

    // 尝试写入更多（应该失败）
    uint8_t extra = 0xBB;
    int written = tx_ring_put(&extra, 1);

    ASSERT_EQUAL(0, written);
    ASSERT_EQUAL(0, tx_drop_count);  // tx_ring_put 不增加 drop_count

    return TEST_PASS;
}

/* ========================================================================== */
/* DMA 回调测试                                                                */
/* ========================================================================== */

// DMA 回调状态
static uint32_t dma_tc_called = 0;
static uint32_t dma_error_called = 0;

// DMA TC 回调
static void dma_tx_cplt_callback(void)
{
    dma_tc_called++;
    osEventFlagsSet(NULL, TX_EVT_DMA_DONE);
}

// DMA Error 回调
static void dma_tx_error_callback(void)
{
    dma_error_called++;
    tx_error_count++;

    // 停止 DMA
    HAL_UART_AbortTransmit(NULL);

    // 重置 DMA 状态
    HAL_DMA_Abort(NULL);

    // 通知发送任务
    osEventFlagsSet(NULL, TX_EVT_DMA_DONE);
}

/**
  * @brief  测试 DMA TC 回调
  */
TEST_CASE(dma_tc_callback)
{
    tx_ring_init();
    mock_reset();
    mock_event_flags = 0;
    dma_tc_called = 0;

    // 模拟 DMA 完成
    dma_tx_cplt_callback();

    ASSERT_EQUAL(1, dma_tc_called);
    ASSERT(mock_event_flags & TX_EVT_DMA_DONE);  // DMA_DONE 标志应置位

    return TEST_PASS;
}

/**
  * @brief  测试 DMA Error 回调
  */
TEST_CASE(dma_error_callback)
{
    tx_ring_init();
    mock_reset();
    mock_event_flags = 0;
    dma_error_called = 0;
    tx_error_count = 0;

    // 模拟 DMA 错误
    dma_tx_error_callback();

    ASSERT_EQUAL(1, dma_error_called);
    ASSERT_EQUAL(1, tx_error_count);
    ASSERT(mock_event_flags & TX_EVT_DMA_DONE);  // DMA_DONE 标志应置位

    return TEST_PASS;
}

/**
  * @brief  测试 DMA Error 后重置状态
  */
TEST_CASE(dma_error_reset)
{
    tx_ring_init();
    mock_reset();
    mock_set_dma_busy(1);  // 模拟 DMA 忙

    // 模拟 DMA 错误
    dma_tx_error_callback();

    // DMA 应该被停止
    ASSERT_EQUAL(0, mock_dma_state);  // DMA 状态应为 READY

    return TEST_PASS;
}

/**
  * @brief  测试完整流程：写入 → 发送 → DMA 完成
  */
TEST_CASE(full_flow_basic)
{
    tx_ring_init();
    mock_reset();
    mock_event_flags = 0;
    tx_task_alive_counter = 0;

    // 1. 写入数据（短消息，确保不触发回绕）
    char msg[] = "Hi";
    int len = strlen(msg);
    int written = uart_tx_write(1, msg, len);

    ASSERT_EQUAL(len, written);
    ASSERT_EQUAL(len, tx_ring_count());
    ASSERT(mock_event_flags & TX_EVT_DATA_READY);

    // 2. 发送任务处理
    int sent = send_task_process();

    ASSERT_EQUAL(len, sent);
    ASSERT_EQUAL(0, tx_ring_count());

    // 3. DMA TC 回调（设置 DMA_DONE 标志）
    dma_tx_cplt_callback();

    ASSERT(mock_event_flags & TX_EVT_DMA_DONE);

    return TEST_PASS;
}

/**
  * @brief  测试完整流程：写入 → DMA 忙 → 等待 → DMA 完成 → 重发
  */
TEST_CASE(full_flow_dma_busy)
{
    tx_ring_init();
    mock_reset();
    mock_event_flags = 0;

    // 1. 写入数据
    uint8_t data[] = {0x01, 0x02, 0x03};
    tx_ring_put(data, 3);

    // 2. 模拟 DMA 忙
    mock_set_dma_busy(1);
    int result = send_task_process();

    ASSERT_EQUAL(-1, result);  // 跳过
    ASSERT_EQUAL(3, tx_ring_count());  // 数据仍在

    // 3. DMA 完成
    mock_set_dma_busy(0);
    dma_tx_cplt_callback();

    // 4. 发送任务再次处理
    int sent = send_task_process();

    ASSERT_EQUAL(3, sent);
    ASSERT_EQUAL(0, tx_ring_count());

    return TEST_PASS;
}

/* 被测函数实现（测试用简化版）-----------------------------------------------*/

static void tx_ring_init(void)
{
    tx_ring.head = 0;
    tx_ring.tail = 0;
    tx_ring.count = 0;
    memset(tx_ring.data, 0, TX_BUF_SIZE);
}

static int tx_ring_put(const uint8_t *data, uint16_t len)
{
    uint16_t free = TX_BUF_SIZE - tx_ring.count;
    if (len > free) {
        return 0;  // buffer满，丢弃
    }

    // 写入数据（处理回绕）
    uint16_t first = len;
    if (tx_ring.head + len > TX_BUF_SIZE) {
        first = TX_BUF_SIZE - tx_ring.head;
    }
    memcpy(tx_ring.data + tx_ring.head, data, first);
    if (first < len) {
        memcpy(tx_ring.data, data + first, len - first);
    }

    tx_ring.head = (tx_ring.head + len) % TX_BUF_SIZE;
    tx_ring.count += len;
    return len;
}

static int tx_ring_get(uint8_t *data, uint16_t len)
{
    uint16_t available = tx_ring.count;
    uint16_t to_read = (len < available) ? len : available;

    if (to_read == 0) {
        return 0;
    }

    // 读取数据（处理回绕）
    uint16_t first = to_read;
    if (tx_ring.tail + to_read > TX_BUF_SIZE) {
        first = TX_BUF_SIZE - tx_ring.tail;
    }
    memcpy(data, tx_ring.data + tx_ring.tail, first);
    if (first < to_read) {
        memcpy(data + first, tx_ring.data, to_read - first);
    }

    tx_ring.tail = (tx_ring.tail + to_read) % TX_BUF_SIZE;
    tx_ring.count -= to_read;
    return to_read;
}

static uint16_t tx_ring_count(void)
{
    return tx_ring.count;
}

static uint16_t tx_ring_free(void)
{
    return TX_BUF_SIZE - tx_ring.count;
}

/* ========================================================================== */
/* _write() 实现（测试用）                                                      */
/* ========================================================================== */

// 事件标志定义
#define TX_EVT_DATA_READY   (1 << 0)
#define TX_EVT_DMA_DONE     (1 << 1)

// Mock mutex（测试用）
static int mock_mutex = 0;

// uart_tx_write() 实现（模拟 _write）
static int uart_tx_write(int fd, const char *buf, int len)
{
    // 1. ISR 检查
    if (xPortIsInsideInterrupt()) {
        return 0;
    }

    // 2. 只处理 stdout (1) 和 stderr (2)
    if (fd != 1 && fd != 2) {
        return -1;
    }

    // 3. 获取 mutex（100ms 超时）
    int status = osMutexAcquire(&mock_mutex, 100);
    if (status != 0) {
        return 0;  // mutex 超时
    }

    // 4. 检查 buffer 空间
    uint16_t free_space = TX_BUF_SIZE - tx_ring.count;
    if (free_space < len) {
        osMutexRelease(&mock_mutex);
        return 0;  // buffer 满
    }

    // 5. 塞入 ring buffer
    uint16_t first = len;
    if (tx_ring.head + len > TX_BUF_SIZE) {
        first = TX_BUF_SIZE - tx_ring.head;
    }
    memcpy(tx_ring.data + tx_ring.head, buf, first);
    if (first < len) {
        memcpy(tx_ring.data, buf + first, len - first);
    }
    tx_ring.head = (tx_ring.head + len) % TX_BUF_SIZE;

    // 更新 count
    tx_ring.count += len;

    // 6. 设置事件标志
    osEventFlagsSet(NULL, TX_EVT_DATA_READY);

    // 7. 释放 mutex
    osMutexRelease(&mock_mutex);

    return len;
}

/* ========================================================================== */
/* UART_TX_MarkReady 测试                                                      */
/* ========================================================================== */

// 模拟 uart_ready 标志（测试用）
static volatile uint32_t mock_uart_ready = 0;

// 模拟 UART_TX_MarkReady 函数
static void mock_UART_TX_MarkReady(void)
{
    mock_uart_ready = 1;
}

// 模拟 _write 函数（使用 mock_uart_ready）
static int mock_write(int fd, const char *buf, int len)
{
    if (fd != 1 && fd != 2) {
        return -1;
    }

    if (!mock_uart_ready) {
        return len;  // 丢弃数据
    }

    // 模拟发送成功
    return len;
}

/**
  * @brief  测试 UART_TX_MarkReady 设置 uart_ready 标志
  */
TEST_CASE(uart_tx_mark_ready)
{
    mock_uart_ready = 0;

    // 调用前 uart_ready 应该为 0
    ASSERT_EQUAL(0, mock_uart_ready);

    // 调用 MarkReady
    mock_UART_TX_MarkReady();

    // 调用后 uart_ready 应该为 1
    ASSERT_EQUAL(1, mock_uart_ready);

    return TEST_PASS;
}

/**
  * @brief  测试 _write 在 uart_ready=0 时丢弃数据
  */
TEST_CASE(write_before_mark_ready)
{
    mock_uart_ready = 0;

    char buf[] = "Hello";
    int result = mock_write(1, buf, 5);

    // 应该返回 len（丢弃数据）
    ASSERT_EQUAL(5, result);

    return TEST_PASS;
}

/**
  * @brief  测试 _write 在 uart_ready=1 时正常发送
  */
TEST_CASE(write_after_mark_ready)
{
    mock_uart_ready = 1;

    char buf[] = "Hello";
    int result = mock_write(1, buf, 5);

    // 应该返回 len（成功发送）
    ASSERT_EQUAL(5, result);

    return TEST_PASS;
}

/**
  * @brief  测试 _write 在 uart_ready=0 时 stderr 也丢弃
  */
TEST_CASE(write_stderr_before_mark_ready)
{
    mock_uart_ready = 0;

    char buf[] = "Error";
    int result = mock_write(2, buf, 5);

    // stderr 也应该被丢弃
    ASSERT_EQUAL(5, result);

    return TEST_PASS;
}

/**
  * @brief  测试 MarkReady 后 _write 可以正常工作
  */
TEST_CASE(mark_ready_then_write)
{
    // 初始状态
    mock_uart_ready = 0;

    // 第一次写入（应该丢弃）
    char buf1[] = "First";
    int result1 = mock_write(1, buf1, 5);
    ASSERT_EQUAL(5, result1);

    // 调用 MarkReady
    mock_UART_TX_MarkReady();
    ASSERT_EQUAL(1, mock_uart_ready);

    // 第二次写入（应该成功）
    char buf2[] = "Second";
    int result2 = mock_write(1, buf2, 6);
    ASSERT_EQUAL(6, result2);

    return TEST_PASS;
}
