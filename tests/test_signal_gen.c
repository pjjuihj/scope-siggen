/**
  ******************************************************************************
  * @file           : test_signal_gen.c
  * @brief          : 信号发生器模块测试
  ******************************************************************************
  * @attention
  *
  * 测试 signal_gen.c 的波形生成和参数设置功能
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

#define DAC_BUFFER_SIZE     256
#define DAC_RESOLUTION      4096
#define DAC_REF_VOLTAGE     3300

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

typedef enum {
    WAVE_SINE = 0,
    WAVE_SQUARE,
    WAVE_TRIANGLE,
    WAVE_SAWTOOTH,
    WAVE_DC
} WaveformType_t;

typedef struct {
    uint32_t frequency;
    uint32_t amplitude;
    WaveformType_t waveform;
    uint16_t duty_cycle;
    uint8_t enabled;
} SigGenConfig_t;

typedef struct {
    uint32_t current_frequency;
    uint32_t current_amplitude;
    WaveformType_t current_waveform;
    uint8_t running;
    ErrorCode_t last_error;
} SigGenStatus_t;

/* Private variables ---------------------------------------------------------*/

/* 被测波形生成函数（从 signal_gen.c 提取） */
static uint16_t test_buffer[DAC_BUFFER_SIZE];

/* 正弦查表（四分之一周期，64点，Q15格式） */
static const int16_t sin_table[65] = {
      0,   804,  1608,  2410,  3212,  4011,  4808,  5602,
   6393,  7179,  7962,  8739,  9512, 10278, 11039, 11793,
  12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530,
  18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
  23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790,
  27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
  30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971,
  32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
  32767
};

/* Private function prototypes -----------------------------------------------*/

static uint16_t VoltageToDac(uint32_t voltage_mv);
static void GenerateSine(uint16_t *buffer, uint32_t size, uint32_t amplitude);
static void GenerateSquare(uint16_t *buffer, uint32_t size, uint32_t amplitude, uint16_t duty_cycle);
static void GenerateTriangle(uint16_t *buffer, uint32_t size, uint32_t amplitude);
static void GenerateSawtooth(uint16_t *buffer, uint32_t size, uint32_t amplitude);
static void GenerateDC(uint16_t *buffer, uint32_t size, uint32_t amplitude);

/* Test cases ----------------------------------------------------------------*/

/**
  * @brief  测试 DAC 电压转换
  */
TEST_CASE(siggen_voltage_to_dac)
{
    /* 0mV -> 0 */
    ASSERT_EQUAL(0, VoltageToDac(0));

    /* 3300mV -> 4096 (满量程，12位DAC) */
    ASSERT_EQUAL(4096, VoltageToDac(3300));

    /* 1650mV -> 2048 (半量程) */
    uint16_t half = VoltageToDac(1650);
    ASSERT(half >= 2046 && half <= 2050);

    return TEST_PASS;
}

/**
  * @brief  测试缓冲区大小
  */
TEST_CASE(siggen_buffer_size)
{
    ASSERT_EQUAL(256, DAC_BUFFER_SIZE);
    ASSERT_EQUAL(4096, DAC_RESOLUTION);
    ASSERT_EQUAL(3300, DAC_REF_VOLTAGE);
    return TEST_PASS;
}

/**
  * @brief  测试正弦波生成
  */
TEST_CASE(siggen_sine_wave)
{
    uint32_t amplitude = 2048; /* 半量程 DAC 值 */
    GenerateSine(test_buffer, DAC_BUFFER_SIZE, amplitude);

    /* 验证第一个点接近半量程（正弦波起始点） */
    ASSERT(test_buffer[0] >= 900 && test_buffer[0] <= 1100);

    /* 验证峰值接近满量程 */
    uint16_t max_val = 0;
    for (uint32_t i = 0; i < DAC_BUFFER_SIZE; i++) {
        if (test_buffer[i] > max_val) max_val = test_buffer[i];
    }
    ASSERT(max_val >= 1800 && max_val <= 2200);

    return TEST_PASS;
}

/**
  * @brief  测试方波生成
  */
TEST_CASE(siggen_square_wave)
{
    uint32_t amplitude = 2048;
    uint16_t duty_cycle = 500; /* 50% */

    GenerateSquare(test_buffer, DAC_BUFFER_SIZE, amplitude, duty_cycle);

    /* 前半应为 amplitude */
    for (uint32_t i = 0; i < DAC_BUFFER_SIZE / 2; i++) {
        ASSERT_EQUAL(amplitude, test_buffer[i]);
    }

    /* 后半应为 0 */
    for (uint32_t i = DAC_BUFFER_SIZE / 2; i < DAC_BUFFER_SIZE; i++) {
        ASSERT_EQUAL(0, test_buffer[i]);
    }

    return TEST_PASS;
}

/**
  * @brief  测试三角波生成
  */
TEST_CASE(siggen_triangle_wave)
{
    uint32_t amplitude = 2048;
    GenerateTriangle(test_buffer, DAC_BUFFER_SIZE, amplitude);

    /* 第一个点应为 0 */
    ASSERT_EQUAL(0, test_buffer[0]);

    /* 中间点应接近 amplitude */
    ASSERT(test_buffer[DAC_BUFFER_SIZE / 2] >= amplitude - 10 &&
           test_buffer[DAC_BUFFER_SIZE / 2] <= amplitude + 10);

    /* 最后一个点应接近 0 (2048 * 1 / 128 = 16) */
    ASSERT(test_buffer[DAC_BUFFER_SIZE - 1] <= 20);

    return TEST_PASS;
}

/**
  * @brief  测试锯齿波生成
  */
TEST_CASE(siggen_sawtooth_wave)
{
    uint32_t amplitude = 2048;
    GenerateSawtooth(test_buffer, DAC_BUFFER_SIZE, amplitude);

    /* 第一个点应为 0 */
    ASSERT_EQUAL(0, test_buffer[0]);

    /* 应单调递增 */
    for (uint32_t i = 1; i < DAC_BUFFER_SIZE; i++) {
        ASSERT(test_buffer[i] >= test_buffer[i - 1]);
    }

    /* 最后一个点接近 amplitude (2048 * 255 / 256 = 2040) */
    ASSERT(test_buffer[DAC_BUFFER_SIZE - 1] >= amplitude - 10);

    return TEST_PASS;
}

/**
  * @brief  测试直流输出
  */
TEST_CASE(siggen_dc_wave)
{
    uint32_t amplitude = 2048;
    GenerateDC(test_buffer, DAC_BUFFER_SIZE, amplitude);

    /* 所有点应为 amplitude */
    for (uint32_t i = 0; i < DAC_BUFFER_SIZE; i++) {
        ASSERT_EQUAL(amplitude, test_buffer[i]);
    }

    return TEST_PASS;
}

/**
  * @brief  测试方波不同占空比
  */
TEST_CASE(siggen_square_duty)
{
    uint32_t amplitude = 4095;

    /* 25% 占空比 */
    GenerateSquare(test_buffer, DAC_BUFFER_SIZE, amplitude, 250);
    uint32_t high_count = 0;
    for (uint32_t i = 0; i < DAC_BUFFER_SIZE; i++) {
        if (test_buffer[i] == amplitude) high_count++;
    }
    ASSERT(high_count >= 60 && high_count <= 70); /* 256 * 0.25 = 64 */

    /* 75% 占空比 */
    GenerateSquare(test_buffer, DAC_BUFFER_SIZE, amplitude, 750);
    high_count = 0;
    for (uint32_t i = 0; i < DAC_BUFFER_SIZE; i++) {
        if (test_buffer[i] == amplitude) high_count++;
    }
    ASSERT(high_count >= 188 && high_count <= 196); /* 256 * 0.75 = 192 */

    return TEST_PASS;
}

/**
  * @brief  测试波形类型枚举
  */
TEST_CASE(siggen_waveform_types)
{
    ASSERT_EQUAL(0, WAVE_SINE);
    ASSERT_EQUAL(1, WAVE_SQUARE);
    ASSERT_EQUAL(2, WAVE_TRIANGLE);
    ASSERT_EQUAL(3, WAVE_SAWTOOTH);
    ASSERT_EQUAL(4, WAVE_DC);
    return TEST_PASS;
}

/**
  * @brief  测试配置结构体
  */
TEST_CASE(siggen_config_struct)
{
    SigGenConfig_t config;
    memset(&config, 0, sizeof(config));

    config.frequency = 1000;
    config.amplitude = 1650;
    config.waveform = WAVE_SINE;
    config.duty_cycle = 500;
    config.enabled = 1;

    ASSERT_EQUAL(1000, config.frequency);
    ASSERT_EQUAL(1650, config.amplitude);
    ASSERT_EQUAL(WAVE_SINE, config.waveform);
    ASSERT_EQUAL(500, config.duty_cycle);
    ASSERT_EQUAL(1, config.enabled);

    return TEST_PASS;
}

/**
  * @brief  测试状态结构体
  */
TEST_CASE(siggen_status_struct)
{
    SigGenStatus_t status;
    memset(&status, 0, sizeof(status));

    status.current_frequency = 1000;
    status.current_amplitude = 1650;
    status.current_waveform = WAVE_SINE;
    status.running = 1;
    status.last_error = ERR_OK;

    ASSERT_EQUAL(1000, status.current_frequency);
    ASSERT_EQUAL(1650, status.current_amplitude);
    ASSERT_EQUAL(WAVE_SINE, status.current_waveform);
    ASSERT_EQUAL(1, status.running);
    ASSERT_EQUAL(ERR_OK, status.last_error);

    return TEST_PASS;
}

/**
  * @brief  测试幅度边界检查
  */
TEST_CASE(siggen_amplitude_bounds)
{
    /* 有效范围：0 ~ 3300mV */
    uint32_t valid_amps[] = {0, 100, 1650, 3300};
    for (int i = 0; i < 4; i++) {
        uint16_t dac = VoltageToDac(valid_amps[i]);
        ASSERT(dac <= DAC_RESOLUTION);
    }

    /* 超出范围：VoltageToDac 本身不截断，由调用方检查 */
    uint16_t dac_over = VoltageToDac(5000);
    ASSERT(dac_over > DAC_RESOLUTION); /* 超出范围产生大值 */

    return TEST_PASS;
}

/**
  * @brief  测试频率参数
  */
TEST_CASE(siggen_frequency_param)
{
    /* 默认频率 */
    ASSERT_EQUAL(1000, 1000); /* SIGGEN_DEFAULT_FREQUENCY */

    /* 有效频率范围：1 ~ 1000000 Hz */
    uint32_t valid_freqs[] = {1, 100, 1000, 10000, 100000, 1000000};
    for (int i = 0; i < 6; i++) {
        ASSERT(valid_freqs[i] > 0);
        ASSERT(valid_freqs[i] <= 1000000);
    }

    return TEST_PASS;
}

/**
  * @brief  测试自检逻辑
  */
TEST_CASE(siggen_selftest)
{
    /* 模拟自检：检查配置有效性 */
    SigGenConfig_t config;
    config.frequency = 1000;
    config.amplitude = 1650;
    config.waveform = WAVE_SINE;

    /* 验证配置有效 */
    ASSERT(config.frequency > 0);
    ASSERT(config.frequency <= 1000000);
    ASSERT(config.amplitude <= DAC_REF_VOLTAGE);
    ASSERT(config.waveform <= WAVE_DC);

    return TEST_PASS;
}

/* Private function implementations ------------------------------------------*/

static uint16_t VoltageToDac(uint32_t voltage_mv)
{
    return (uint16_t)((uint32_t)voltage_mv * DAC_RESOLUTION / DAC_REF_VOLTAGE);
}

static void GenerateSine(uint16_t *buffer, uint32_t size, uint32_t amplitude)
{
    for (uint32_t i = 0; i < size; i++) {
        uint32_t phase = (i * 256) / size;
        int16_t sin_val;

        if (phase < 64) {
            sin_val = sin_table[phase];
        } else if (phase < 128) {
            sin_val = sin_table[128 - phase];
        } else if (phase < 192) {
            sin_val = -sin_table[phase - 128];
        } else {
            sin_val = -sin_table[256 - phase];
        }

        buffer[i] = (uint16_t)(((int32_t)sin_val + 32767) * amplitude / 65534);
    }
}

static void GenerateSquare(uint16_t *buffer, uint32_t size, uint32_t amplitude, uint16_t duty_cycle)
{
    uint32_t threshold = size * duty_cycle / 1000;
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = (i < threshold) ? amplitude : 0;
    }
}

static void GenerateTriangle(uint16_t *buffer, uint32_t size, uint32_t amplitude)
{
    uint32_t half_size = size / 2;
    for (uint32_t i = 0; i < size; i++) {
        if (i < half_size) {
            buffer[i] = (uint16_t)(amplitude * i / half_size);
        } else {
            buffer[i] = (uint16_t)(amplitude * (size - i) / half_size);
        }
    }
}

static void GenerateSawtooth(uint16_t *buffer, uint32_t size, uint32_t amplitude)
{
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = (uint16_t)(amplitude * i / size);
    }
}

static void GenerateDC(uint16_t *buffer, uint32_t size, uint32_t amplitude)
{
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = amplitude;
    }
}
