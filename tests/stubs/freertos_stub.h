/**
  ******************************************************************************
  * @file           : freertos_stub.h
  * @brief          : FreeRTOS stub for unit testing
  ******************************************************************************
  * @attention
  *
  * Mock FreeRTOS functions for host-based testing
  *
  ******************************************************************************
  */

#ifndef FREERTOS_STUB_H
#define FREERTOS_STUB_H

#include <stdint.h>
#include <string.h>

/* Mock state ---------------------------------------------------------------*/

static uint32_t mock_in_isr = 0;
static uint32_t mock_mutex_acquired = 0;
static uint32_t mock_mutex_fail_count = 0;
static uint32_t mock_event_flags = 0;
static uint32_t mock_dma_state = 0;  // 0=READY, 1=BUSY

/* Stub functions -----------------------------------------------------------*/

static inline uint32_t xPortIsInsideInterrupt(void)
{
    return mock_in_isr;
}

static inline int osMutexAcquire(void *mutex, uint32_t timeout)
{
    (void)mutex;
    (void)timeout;

    if (mock_mutex_fail_count > 0) {
        mock_mutex_fail_count--;
        return 1;  // osErrorTimeout
    }
    mock_mutex_acquired = 1;
    return 0;  // osOK
}

static inline int osMutexRelease(void *mutex)
{
    (void)mutex;
    mock_mutex_acquired = 0;
    return 0;
}

static inline uint32_t osEventFlagsSet(void *flags, uint32_t mask)
{
    (void)flags;
    mock_event_flags |= mask;
    return 0;
}

static inline uint32_t osEventFlagsClear(void *flags, uint32_t mask)
{
    (void)flags;
    mock_event_flags &= ~mask;
    return 0;
}

static inline uint32_t osEventFlagsGet(void *flags)
{
    (void)flags;
    return mock_event_flags;
}

static inline uint32_t osEventFlagsWait(void *flags, uint32_t mask,
                                        uint32_t option, uint32_t timeout)
{
    (void)flags;
    (void)option;
    (void)timeout;

    uint32_t result = mock_event_flags & mask;
    mock_event_flags &= ~mask;
    return result;
}

static inline uint32_t HAL_DMA_GetState(void *hdma)
{
    (void)hdma;
    return mock_dma_state;
}

static inline int HAL_UART_Transmit_DMA(void *huart, uint8_t *data, uint16_t len)
{
    (void)huart;
    (void)data;
    (void)len;
    mock_dma_state = 1;  // 模拟DMA开始传输
    return 0;
}

static inline int HAL_UART_AbortTransmit(void *huart)
{
    (void)huart;
    mock_dma_state = 0;  // 模拟DMA停止
    return 0;
}

static inline int HAL_DMA_Abort(void *hdma)
{
    (void)hdma;
    return 0;
}

/* Helper functions ---------------------------------------------------------*/

static inline void mock_reset(void)
{
    mock_in_isr = 0;
    mock_mutex_acquired = 0;
    mock_mutex_fail_count = 0;
    mock_event_flags = 0;
    mock_dma_state = 0;
}

static inline void mock_set_isr_mode(uint32_t in_isr)
{
    mock_in_isr = in_isr;
}

static inline void mock_set_mutex_fail(uint32_t fail_count)
{
    mock_mutex_fail_count = fail_count;
}

static inline void mock_set_dma_busy(uint32_t busy)
{
    mock_dma_state = busy ? 1 : 0;
}

#endif /* FREERTOS_STUB_H */
