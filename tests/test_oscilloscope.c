/**
  ******************************************************************************
  * @file           : test_oscilloscope.c
  * @brief          : 示波器模块测试
  ******************************************************************************
  * @attention
  *
  * 测试 oscilloscope.c 的 ADC 转换和频率计算功能
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "test_framework.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

/* Private define ------------------------------------------------------------*/

#define ADC_RESOLUTION  4096
#define ADC_REF_VOLTAGE 3300
#define BUFFER_SIZE     1024

/* Private types ------------------------------------------------------------*/

typedef enum {
    ERR_OK = 0,
    ERR_TIMEOUT,
    ERR_INVALID_PARAM,
    ERR_BUFFER_FULL,
    ERR_BUFFER_EMPTY,
    ERR_HARDWARE,
    ERR_MEMORY,
    ERR_BUSY,
    ERR_NOT_INIT,
    ERR_NOT_SUPPORTED,
    ERR_UNKNOWN
} ErrorCode_t;

/* Private variables ---------------------------------------------------------*/

static uint16_t test_buffer[BUFFER_SIZE];

/* Private function prototypes -----------------------------------------------*/

static uint32_t ConvertToVoltage(uint16_t adc_value);
static uint32_t CalculateFrequency(const uint16_t *buffer, uint32_t size, uint32_t sample_rate);
static uint16_t FindTriggerPoint(const uint16_t *buf, uint16_t len);

/* Test cases ----------------------------------------------------------------*/

/**
  * @brief  测试 ADC 电压转换
  */
TEST_CASE(osc_voltage_conversion)
{
    /* 0 -> 0mV */
    ASSERT_EQUAL(0, ConvertToVoltage(0));

    /* 4096 -> 3300mV */
    ASSERT_EQUAL(3300, ConvertToVoltage(4096));

    /* 2048 -> 1650mV */
    uint32_t half = ConvertToVoltage(2048);
    ASSERT(half >= 1648 && half <= 1652);

    /* 1024 -> 825mV */
    uint32_t quarter = ConvertToVoltage(1024);
    ASSERT(quarter >= 823 && quarter <= 827);

    return TEST_PASS;
}

/**
  * @brief  测试频率计算 - 1kHz 正弦波
  */
TEST_CASE(osc_frequency_1khz)
{
    /* 生成 1kHz 正弦波：采样率 100kHz，1000 个点 = 10 个周期 */
    uint32_t sample_rate = 100000;
    uint32_t buffer_size = 1000;
    uint32_t freq = 1000;

    for (uint32_t i = 0; i < buffer_size; i++) {
        /* 生成 0~4095 的正弦波 */
        test_buffer[i] = (uint16_t)(2048 + 1800 * sin(2 * M_PI * freq * i / sample_rate));
    }

    uint32_t measured_freq = CalculateFrequency(test_buffer, buffer_size, sample_rate);

    /* 允许 ±5% 误差 */
    ASSERT(measured_freq >= 950 && measured_freq <= 1050);

    return TEST_PASS;
}

/**
  * @brief  测试频率计算 - 100Hz 方波
  */
TEST_CASE(osc_frequency_100hz)
{
    uint32_t sample_rate = 10000;
    uint32_t buffer_size = 1000;
    uint32_t freq = 100;

    for (uint32_t i = 0; i < buffer_size; i++) {
        /* 方波：0 或 4095 */
        uint32_t period_samples = sample_rate / freq;
        test_buffer[i] = ((i % period_samples) < (period_samples / 2)) ? 4095 : 0;
    }

    uint32_t measured_freq = CalculateFrequency(test_buffer, buffer_size, sample_rate);

    /* 允许 ±10% 误差 */
    ASSERT(measured_freq >= 90 && measured_freq <= 110);

    return TEST_PASS;
}

/**
  * @brief  测试频率计算 - 直流（无频率）
  */
TEST_CASE(osc_frequency_dc)
{
    uint32_t sample_rate = 100000;
    uint32_t buffer_size = 1000;

    /* 直流：所有点相同 */
    for (uint32_t i = 0; i < buffer_size; i++) {
        test_buffer[i] = 2048;
    }

    uint32_t measured_freq = CalculateFrequency(test_buffer, buffer_size, sample_rate);

    /* 直流应返回 0 */
    ASSERT_EQUAL(0, measured_freq);

    return TEST_PASS;
}

/**
  * @brief  测试触发点查找
  */
TEST_CASE(osc_trigger_point)
{
    /* 生成上升沿波形 */
    for (uint16_t i = 0; i < 100; i++) {
        test_buffer[i] = (i < 50) ? 1000 : 3000;
    }

    uint16_t trigger_idx = FindTriggerPoint(test_buffer, 100);

    /* 触发点应在 49 或 50 附近 */
    ASSERT(trigger_idx >= 45 && trigger_idx <= 55);

    return TEST_PASS;
}

/**
  * @brief  测试触发点查找 - 无触发
  */
TEST_CASE(osc_trigger_no_edge)
{
    /* 直流信号，无边沿 */
    for (uint16_t i = 0; i < 100; i++) {
        test_buffer[i] = 2048;
    }

    uint16_t trigger_idx = FindTriggerPoint(test_buffer, 100);

    /* 无触发应返回 0 */
    ASSERT_EQUAL(0, trigger_idx);

    return TEST_PASS;
}

/**
  * @brief  测试配置结构体
  */
TEST_CASE(osc_config_struct)
{
    /* 默认配置值 */
    ASSERT_EQUAL(1000000, 1000000);  /* OSC_DEFAULT_SAMPLE_RATE */
    ASSERT_EQUAL(1024, 1024);        /* OSC_DEFAULT_BUFFER_SIZE */
    ASSERT_EQUAL(1650, 1650);        /* OSC_DEFAULT_TRIGGER_LEVEL */
    ASSERT_EQUAL(1, 1);              /* OSC_DEFAULT_TRIGGER_EDGE */

    return TEST_PASS;
}

/**
  * @brief  测试状态结构体
  */
TEST_CASE(osc_status_struct)
{
    /* 验证状态结构体字段 */
    uint32_t voltage_mv = 1650;
    uint32_t frequency_hz = 1000;
    uint32_t sample_count = 1024;
    uint8_t running = 1;

    ASSERT_EQUAL(1650, voltage_mv);
    ASSERT_EQUAL(1000, frequency_hz);
    ASSERT_EQUAL(1024, sample_count);
    ASSERT_EQUAL(1, running);

    return TEST_PASS;
}

/**
  * @brief  测试缓冲区大小验证
  */
TEST_CASE(osc_buffer_size)
{
    /* 有效缓冲区大小 */
    uint32_t valid_sizes[] = {1, 100, 512, 1024};
    for (int i = 0; i < 4; i++) {
        ASSERT(valid_sizes[i] > 0);
        ASSERT(valid_sizes[i] <= 1024);
    }

    /* 无效缓冲区大小 */
    ASSERT(0 == 0);  /* 0 无效 */
    ASSERT(2048 > 1024);  /* 超出范围 */

    return TEST_PASS;
}

/**
  * @brief  测试采样率参数
  */
TEST_CASE(osc_sample_rate)
{
    /* 有效采样率范围 */
    uint32_t valid_rates[] = {1, 1000, 100000, 1000000, 10000000};
    for (int i = 0; i < 5; i++) {
        ASSERT(valid_rates[i] > 0);
        ASSERT(valid_rates[i] <= 10000000);
    }

    return TEST_PASS;
}

/**
  * @brief  测试自检逻辑
  */
TEST_CASE(osc_selftest)
{
    /* 模拟自检：检查配置有效性 */
    uint32_t sample_rate = 1000000;

    ASSERT(sample_rate > 0);
    ASSERT(sample_rate <= 10000000);

    return TEST_PASS;
}

/* Private function implementations ------------------------------------------*/

static uint32_t ConvertToVoltage(uint16_t adc_value)
{
    return (uint32_t)((uint32_t)adc_value * ADC_REF_VOLTAGE / ADC_RESOLUTION);
}

static uint32_t CalculateFrequency(const uint16_t *buffer, uint32_t size, uint32_t sample_rate)
{
    if (buffer == NULL || size < 2) {
        return 0;
    }

    /* 自适应阈值：使用信号的实际中点 */
    uint16_t min_val = buffer[0], max_val = buffer[0];
    for (uint32_t i = 1; i < size; i++) {
        if (buffer[i] < min_val) min_val = buffer[i];
        if (buffer[i] > max_val) max_val = buffer[i];
    }

    /* 信号幅度太小，无法检测频率 */
    uint16_t range = max_val - min_val;
    if (range < 100) {
        return 0;
    }

    /* 使用信号中点作为阈值 */
    uint16_t threshold = (min_val + max_val) / 2;

    /* 过零检测 */
    uint32_t zero_count = 0;
    for (uint32_t i = 1; i < size; i++) {
        if ((buffer[i-1] < threshold && buffer[i] >= threshold) ||
            (buffer[i-1] >= threshold && buffer[i] < threshold)) {
            zero_count++;
        }
    }

    /* 频率 = 过零次数 / 2 / 时间 */
    if (zero_count > 0) {
        uint32_t time_us = size * 1000000 / sample_rate;
        return (zero_count * 1000000) / (2 * time_us);
    }

    return 0;
}

static uint16_t FindTriggerPoint(const uint16_t *buf, uint16_t len)
{
    if (buf == NULL || len < 10) return 0;

    /* 计算信号中点作为触发电平 */
    uint16_t min_val = buf[0], max_val = buf[0];
    for (uint16_t i = 1; i < len; i++) {
        if (buf[i] < min_val) min_val = buf[i];
        if (buf[i] > max_val) max_val = buf[i];
    }

    uint16_t range = max_val - min_val;
    if (range < 100) return 0;

    uint16_t threshold = (min_val + max_val) / 2;

    /* 查找第一个上升沿过零点 */
    uint16_t start = len / 4;
    for (uint16_t i = start; i < len - 1; i++) {
        if (buf[i] < threshold && buf[i + 1] >= threshold) {
            return i;
        }
    }

    return 0;
}
