/**
  ******************************************************************************
  * @file           : upper_computer.c
  * @brief          : 上位机通信模块实现
  ******************************************************************************
  * @attention
  *
  * 上位机通信模块，负责将示波器数据发送到上位机
  * 数据格式：
  *   - 波形: WAVE:0A1B0C1D... (16进制)
  *   - 频率: FREQ:1234.56
  *   - 电压: VOLT:0.123,3.210,3.087
  *   - 状态: STATUS:{...}
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "upper_computer.h"
#include "debug.h"
#include "oscilloscope.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* 外部变量声明 */
extern UART_HandleTypeDef huart1;

/* 上位机通信状态 */
static UC_State_t uc_state = UC_STATE_IDLE;

/* 上位机通信配置 */
static UC_Config_t uc_config = {
    .enabled = 1,
    .stream_mode = 0,
    .stream_interval = UC_DEFAULT_STREAM_INTERVAL,
    .data_format = 0  /* hex格式 */
};

/* 发送缓冲区 */
static char tx_buffer[UC_MAX_PACKET_SIZE];

/* 初始化标志 */
static bool initialized = false;

/* Private function prototypes -----------------------------------------------*/

static ErrorCode_t UC_SendString(const char *str);
static ErrorCode_t UC_SendData(const uint8_t *data, uint16_t len);
static uint16_t UC_ConvertToHex(const uint16_t *data, uint16_t len, char *output, uint16_t max_len);

/* Private user code ---------------------------------------------------------*/

/* Exported function implementations -----------------------------------------*/

ErrorCode_t UpperComputer_Init(void)
{
    LOG_INFO("Initializing upper computer communication...");

    /* 初始化状态 */
    uc_state = UC_STATE_IDLE;

    /* 清空发送缓冲区 */
    memset(tx_buffer, 0, sizeof(tx_buffer));

    initialized = true;
    LOG_INFO("Upper computer communication initialized");

    return ERR_OK;
}

ErrorCode_t UC_SendWaveform(const uint16_t *data, uint16_t len)
{
    if (!initialized || data == NULL || len == 0) {
        return ERR_INVALID_PARAM;
    }

    /* 构建数据包: WAVE:0A1B0C1D... */
    int offset = snprintf(tx_buffer, UC_MAX_PACKET_SIZE, UC_PREFIX_WAVE);

    /* 转换为16进制字符串 */
    offset += UC_ConvertToHex(data, len, tx_buffer + offset, UC_MAX_PACKET_SIZE - offset);

    /* 添加换行 */
    if (offset < UC_MAX_PACKET_SIZE - 2) {
        tx_buffer[offset++] = '\r';
        tx_buffer[offset++] = '\n';
    }

    /* 发送数据 */
    return UC_SendData((uint8_t*)tx_buffer, offset);
}

ErrorCode_t UC_SendFrequency(uint32_t freq_hz)
{
    if (!initialized) {
        return ERR_NOT_INIT;
    }

    /* 构建数据包: FREQ:1234.56 */
    int offset = snprintf(tx_buffer, UC_MAX_PACKET_SIZE,
                          UC_PREFIX_FREQ "%.2f\r\n",
                          (float)freq_hz);

    return UC_SendData((uint8_t*)tx_buffer, offset);
}

ErrorCode_t UC_SendVoltage(uint32_t min_mv, uint32_t max_mv, uint32_t pp_mv)
{
    if (!initialized) {
        return ERR_NOT_INIT;
    }

    /* 转换为伏特 */
    float min_v = (float)min_mv / 1000.0f;
    float max_v = (float)max_mv / 1000.0f;
    float pp_v = (float)pp_mv / 1000.0f;

    /* 构建数据包: VOLT:0.123,3.210,3.087 */
    int offset = snprintf(tx_buffer, UC_MAX_PACKET_SIZE,
                          UC_PREFIX_VOLT "%.3f,%.3f,%.3f\r\n",
                          min_v, max_v, pp_v);

    return UC_SendData((uint8_t*)tx_buffer, offset);
}

ErrorCode_t UC_SendStatus(const char *status_json)
{
    if (!initialized || status_json == NULL) {
        return ERR_INVALID_PARAM;
    }

    /* 构建数据包: STATUS:{...} */
    int offset = snprintf(tx_buffer, UC_MAX_PACKET_SIZE,
                          UC_PREFIX_STATUS "%s\r\n",
                          status_json);

    return UC_SendData((uint8_t*)tx_buffer, offset);
}

ErrorCode_t UC_SendMeasurement(uint32_t freq_hz, uint32_t min_mv, uint32_t max_mv)
{
    if (!initialized) {
        return ERR_NOT_INIT;
    }

    /* 计算峰峰值 */
    uint32_t pp_mv = max_mv - min_mv;

    /* 转换为伏特 */
    float freq = (float)freq_hz;
    float min_v = (float)min_mv / 1000.0f;
    float max_v = (float)max_mv / 1000.0f;
    float pp_v = (float)pp_mv / 1000.0f;

    /* 构建数据包: FREQ:1234.56\nVOLT:0.123,3.210,3.087 */
    int offset = snprintf(tx_buffer, UC_MAX_PACKET_SIZE,
                          UC_PREFIX_WAVE "%.2f\r\n"
                          UC_PREFIX_VOLT "%.3f,%.3f,%.3f\r\n",
                          freq, min_v, max_v, pp_v);

    return UC_SendData((uint8_t*)tx_buffer, offset);
}

ErrorCode_t UC_SendCompletePacket(const uint16_t *waveform, uint16_t len,
                                   uint32_t freq_hz, uint32_t min_mv, uint32_t max_mv)
{
    if (!initialized || waveform == NULL || len == 0) {
        return ERR_INVALID_PARAM;
    }

    /* 计算峰峰值 */
    uint32_t pp_mv = max_mv - min_mv;

    /* 转换为伏特 */
    float freq = (float)freq_hz;
    float min_v = (float)min_mv / 1000.0f;
    float max_v = (float)max_mv / 1000.0f;
    float pp_v = (float)pp_mv / 1000.0f;

    /* 构建波形数据 */
    int offset = snprintf(tx_buffer, UC_MAX_PACKET_SIZE, UC_PREFIX_WAVE);
    offset += UC_ConvertToHex(waveform, len, tx_buffer + offset, UC_MAX_PACKET_SIZE - offset);

    /* 添加测量数据 */
    offset += snprintf(tx_buffer + offset, UC_MAX_PACKET_SIZE - offset,
                       "\r\n" UC_PREFIX_FREQ "%.2f\r\n"
                       UC_PREFIX_VOLT "%.3f,%.3f,%.3f\r\n",
                       freq, min_v, max_v, pp_v);

    return UC_SendData((uint8_t*)tx_buffer, offset);
}

ErrorCode_t UC_SetStreamMode(bool enable)
{
    if (!initialized) {
        return ERR_NOT_INIT;
    }

    uc_config.stream_mode = enable ? 1 : 0;
    uc_state = enable ? UC_STATE_STREAMING : UC_STATE_IDLE;

    LOG_INFO("Stream mode %s", enable ? "enabled" : "disabled");

    return ERR_OK;
}

ErrorCode_t UC_SetStreamInterval(uint32_t interval_ms)
{
    if (!initialized) {
        return ERR_NOT_INIT;
    }

    if (interval_ms < 10 || interval_ms > 10000) {
        return ERR_INVALID_PARAM;
    }

    uc_config.stream_interval = interval_ms;
    LOG_INFO("Stream interval set to %lu ms", interval_ms);

    return ERR_OK;
}

UC_State_t UC_GetState(void)
{
    return uc_state;
}

bool UC_IsStreaming(void)
{
    return (uc_state == UC_STATE_STREAMING);
}

ErrorCode_t UC_SendOK(const char *msg)
{
    if (!initialized) {
        return ERR_NOT_INIT;
    }

    int offset;
    if (msg != NULL) {
        offset = snprintf(tx_buffer, UC_MAX_PACKET_SIZE,
                          UC_PREFIX_OK "%s\r\n", msg);
    } else {
        offset = snprintf(tx_buffer, UC_MAX_PACKET_SIZE, "OK\r\n");
    }

    return UC_SendData((uint8_t*)tx_buffer, offset);
}

ErrorCode_t UC_SendError(const char *msg)
{
    if (!initialized || msg == NULL) {
        return ERR_INVALID_PARAM;
    }

    int offset = snprintf(tx_buffer, UC_MAX_PACKET_SIZE,
                          UC_PREFIX_ERROR "%s\r\n", msg);

    return UC_SendData((uint8_t*)tx_buffer, offset);
}

ErrorCode_t UC_SendInfo(const char *msg)
{
    if (!initialized || msg == NULL) {
        return ERR_INVALID_PARAM;
    }

    int offset = snprintf(tx_buffer, UC_MAX_PACKET_SIZE,
                          UC_PREFIX_INFO "%s\r\n", msg);

    return UC_SendData((uint8_t*)tx_buffer, offset);
}

/* Private function implementations ------------------------------------------*/

/**
  * @brief 发送字符串到串口
  * @param str 字符串
  * @retval ErrorCode_t 错误码
  */
static ErrorCode_t UC_SendString(const char *str)
{
    if (str == NULL) {
        return ERR_INVALID_PARAM;
    }

    uint16_t len = strlen(str);
    return UC_SendData((uint8_t*)str, len);
}

/**
  * @brief 发送数据到串口（分块发送，避免阻塞）
  * @param data 数据
  * @param len 数据长度
  * @retval ErrorCode_t 错误码
  */
static ErrorCode_t UC_SendData(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) {
        return ERR_INVALID_PARAM;
    }

    /* 分块发送，每块 64 字节，避免长时间阻塞 */
    #define UC_CHUNK_SIZE 64
    uint16_t offset = 0;

    while (offset < len) {
        uint16_t chunk = len - offset;
        if (chunk > UC_CHUNK_SIZE) {
            chunk = UC_CHUNK_SIZE;
        }

        HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1, (uint8_t*)(data + offset), chunk, 100);
        if (status != HAL_OK) {
            uc_state = UC_STATE_ERROR;
            return ERR_HARDWARE;
        }

        offset += chunk;

        /* 每发送一块后让出 CPU，避免阻塞其他任务 */
        if (offset < len) {
            osDelay(1);
        }
    }

    return ERR_OK;
}

/**
  * @brief 将ADC数据转换为16进制字符串
  * @param data ADC数据数组
  * @param len 数据长度
  * @param output 输出缓冲区
  * @param max_len 输出缓冲区最大长度
  * @retval uint16_t 输出字符串长度
  */
static uint16_t UC_ConvertToHex(const uint16_t *data, uint16_t len, char *output, uint16_t max_len)
{
    uint16_t pos = 0;

    for (uint16_t i = 0; i < len && pos < max_len - 4; i++) {
        /* 转换为4位16进制 */
        pos += snprintf(output + pos, max_len - pos, "%04X", data[i]);
    }

    return pos;
}
