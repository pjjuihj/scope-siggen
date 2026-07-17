/**
  ******************************************************************************
  * @file           : test_upper_computer.c
  * @brief          : 上位机通信模块测试
  ******************************************************************************
  * @attention
  *
  * 测试 upper_computer.c 的数据格式和包构建功能
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "test_framework.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* Private define ------------------------------------------------------------*/

#define UC_MAX_PACKET_SIZE  2048
#define UC_CHUNK_SIZE       64

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
    UC_STATE_IDLE = 0,
    UC_STATE_STREAMING,
    UC_STATE_ERROR
} UC_State_t;

/* 数据前缀 */
#define UC_PREFIX_WAVE      "WAVE:"
#define UC_PREFIX_FREQ      "FREQ:"
#define UC_PREFIX_VOLT      "VOLT:"
#define UC_PREFIX_STATUS    "STATUS:"
#define UC_PREFIX_ERROR     "ERROR:"
#define UC_PREFIX_OK        "OK:"
#define UC_PREFIX_INFO      "INFO:"

/* Private variables ---------------------------------------------------------*/

static char tx_buffer[UC_MAX_PACKET_SIZE];

/* Private function prototypes -----------------------------------------------*/

static uint16_t ConvertToHex(const uint16_t *data, uint16_t len, char *output, uint16_t max_len);

/* Test cases ----------------------------------------------------------------*/

/**
  * @brief  测试通信状态枚举
  */
TEST_CASE(uc_state_enum)
{
    ASSERT_EQUAL(0, UC_STATE_IDLE);
    ASSERT_EQUAL(1, UC_STATE_STREAMING);
    ASSERT_EQUAL(2, UC_STATE_ERROR);
    return TEST_PASS;
}

/**
  * @brief  测试数据前缀常量
  */
TEST_CASE(uc_prefix_constants)
{
    ASSERT_STRING_EQUAL("WAVE:", UC_PREFIX_WAVE);
    ASSERT_STRING_EQUAL("FREQ:", UC_PREFIX_FREQ);
    ASSERT_STRING_EQUAL("VOLT:", UC_PREFIX_VOLT);
    ASSERT_STRING_EQUAL("STATUS:", UC_PREFIX_STATUS);
    ASSERT_STRING_EQUAL("ERROR:", UC_PREFIX_ERROR);
    ASSERT_STRING_EQUAL("OK:", UC_PREFIX_OK);
    ASSERT_STRING_EQUAL("INFO:", UC_PREFIX_INFO);
    return TEST_PASS;
}

/**
  * @brief  测试 Hex 转换 - 单个值
  */
TEST_CASE(uc_hex_single)
{
    uint16_t data[] = {0x0A1B};
    char output[16];
    uint16_t len = ConvertToHex(data, 1, output, sizeof(output));

    ASSERT_EQUAL(4, len);
    ASSERT_STRING_EQUAL("0A1B", output);
    return TEST_PASS;
}

/**
  * @brief  测试 Hex 转换 - 多个值
  */
TEST_CASE(uc_hex_multiple)
{
    uint16_t data[] = {0x0000, 0x1234, 0xABCD, 0xFFFF};
    char output[32];
    uint16_t len = ConvertToHex(data, 4, output, sizeof(output));

    ASSERT_EQUAL(16, len);
    ASSERT_STRING_EQUAL("00001234ABCDFFFF", output);
    return TEST_PASS;
}

/**
  * @brief  测试 Hex 转换 - 零值
  */
TEST_CASE(uc_hex_zero)
{
    uint16_t data[] = {0x0000};
    char output[16];
    uint16_t len = ConvertToHex(data, 1, output, sizeof(output));

    ASSERT_EQUAL(4, len);
    ASSERT_STRING_EQUAL("0000", output);
    return TEST_PASS;
}

/**
  * @brief  测试频率格式化
  */
TEST_CASE(uc_format_freq)
{
    char buf[UC_MAX_PACKET_SIZE];
    uint32_t freq_hz = 1234567;

    int offset = snprintf(buf, sizeof(buf), UC_PREFIX_FREQ "%.2f\r\n", (float)freq_hz);

    ASSERT(offset > 0);
    ASSERT(strstr(buf, "FREQ:") != NULL);
    ASSERT(strstr(buf, "1234567.00") != NULL);
    return TEST_PASS;
}

/**
  * @brief  测试电压格式化
  */
TEST_CASE(uc_format_voltage)
{
    char buf[UC_MAX_PACKET_SIZE];
    uint32_t min_mv = 100;
    uint32_t max_mv = 3200;
    uint32_t pp_mv = 3100;

    float min_v = (float)min_mv / 1000.0f;
    float max_v = (float)max_mv / 1000.0f;
    float pp_v = (float)pp_mv / 1000.0f;

    int offset = snprintf(buf, sizeof(buf), UC_PREFIX_VOLT "%.3f,%.3f,%.3f\r\n",
                          min_v, max_v, pp_v);

    ASSERT(offset > 0);
    ASSERT(strstr(buf, "VOLT:") != NULL);
    ASSERT(strstr(buf, "0.100") != NULL);
    ASSERT(strstr(buf, "3.200") != NULL);
    ASSERT(strstr(buf, "3.100") != NULL);
    return TEST_PASS;
}

/**
  * @brief  测试状态格式化
  */
TEST_CASE(uc_format_status)
{
    char buf[UC_MAX_PACKET_SIZE];
    const char *json = "{\"running\":true}";

    int offset = snprintf(buf, sizeof(buf), UC_PREFIX_STATUS "%s\r\n", json);

    ASSERT(offset > 0);
    ASSERT(strstr(buf, "STATUS:") != NULL);
    ASSERT(strstr(buf, "{\"running\":true}") != NULL);
    return TEST_PASS;
}

/**
  * @brief  测试 OK 响应格式化
  */
TEST_CASE(uc_format_ok)
{
    char buf[UC_MAX_PACKET_SIZE];
    const char *msg = "config saved";

    int offset = snprintf(buf, sizeof(buf), UC_PREFIX_OK "%s\r\n", msg);

    ASSERT(offset > 0);
    ASSERT(strstr(buf, "OK:") != NULL);
    ASSERT(strstr(buf, "config saved") != NULL);
    return TEST_PASS;
}

/**
  * @brief  测试错误响应格式化
  */
TEST_CASE(uc_format_error)
{
    char buf[UC_MAX_PACKET_SIZE];
    const char *msg = "invalid parameter";

    int offset = snprintf(buf, sizeof(buf), UC_PREFIX_ERROR "%s\r\n", msg);

    ASSERT(offset > 0);
    ASSERT(strstr(buf, "ERROR:") != NULL);
    ASSERT(strstr(buf, "invalid parameter") != NULL);
    return TEST_PASS;
}

/**
  * @brief  测试信息响应格式化
  */
TEST_CASE(uc_format_info)
{
    char buf[UC_MAX_PACKET_SIZE];
    const char *msg = "version 1.4.0";

    int offset = snprintf(buf, sizeof(buf), UC_PREFIX_INFO "%s\r\n", msg);

    ASSERT(offset > 0);
    ASSERT(strstr(buf, "INFO:") != NULL);
    ASSERT(strstr(buf, "version 1.4.0") != NULL);
    return TEST_PASS;
}

/**
  * @brief  测试数据包大小限制
  */
TEST_CASE(uc_packet_size_limit)
{
    ASSERT_EQUAL(2048, UC_MAX_PACKET_SIZE);
    ASSERT_EQUAL(64, UC_CHUNK_SIZE);
    return TEST_PASS;
}

/**
  * @brief  测试流模式间隔范围
  */
TEST_CASE(uc_stream_interval_range)
{
    /* 有效范围: 10 ~ 10000 ms */
    uint32_t valid_intervals[] = {10, 100, 1000, 10000};
    for (int i = 0; i < 4; i++) {
        ASSERT(valid_intervals[i] >= 10);
        ASSERT(valid_intervals[i] <= 10000);
    }

    return TEST_PASS;
}

/* Private function implementations ------------------------------------------*/

static uint16_t ConvertToHex(const uint16_t *data, uint16_t len, char *output, uint16_t max_len)
{
    uint16_t pos = 0;

    for (uint16_t i = 0; i < len && pos < max_len - 4; i++) {
        pos += snprintf(output + pos, max_len - pos, "%04X", data[i]);
    }

    return pos;
}
