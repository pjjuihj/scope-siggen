/**
  ******************************************************************************
  * @file           : uart_protocol.c
  * @brief          : 串口协议模块实现
  ******************************************************************************
  * @attention
  *
  * 串口协议模块，用于命令控制和数据传输
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "uart_protocol.h"
#include "oscilloscope.h"
#include "signal_gen.h"
#include "display.h"
#include "config.h"
#include "app_init.h"
#include "version.h"
#include "debug.h"
#include "error_tracker.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* 外部变量声明 */
extern UART_HandleTypeDef huart1;

/* 协议模式 */
static ProtocolMode_t protocol_mode = PROTOCOL_TEXT;

/* 命令缓冲区 */
static char cmd_buffer[CMD_MAX_LENGTH];
static uint16_t cmd_index = 0;

/* 任务句柄 */
osThreadId_t uart_task_handle;

/* 初始化标志 */
static bool initialized = false;

/* UART 接收环形缓冲区 */
#define UART_RX_BUF_SIZE 512
static uint8_t uart_rx_buf[UART_RX_BUF_SIZE];
static volatile uint16_t uart_rx_head = 0;  /* 写指针（中断） */
static volatile uint16_t uart_rx_tail = 0;  /* 读指针（任务） */
static uint8_t uart_rx_byte;                /* 单字节接收缓冲 */

/* Private function prototypes -----------------------------------------------*/

/* 命令处理器 */
static void UART_HandleHelp(const char *param);
static void UART_HandleStatus(const char *param);
static void UART_HandleVersion(const char *param);
static void UART_HandleStartOsc(const char *param);
static void UART_HandleStopOsc(const char *param);
static void UART_HandleSetFreq(const char *param);
static void UART_HandleSetWave(const char *param);
static void UART_HandleSetAmp(const char *param);
static void UART_HandleStartGen(const char *param);
static void UART_HandleStopGen(const char *param);
static void UART_HandleStreamDac(const char *param);
static void UART_HandleZoom(const char *param);
static void UART_HandleBrightness(const char *param);
static void UART_HandleMeasure(const char *param);
static void UART_HandlePage(const char *param);
static void UART_HandleKey(const char *param);
static void UART_HandleCursor(const char *param);
static void UART_HandleSave(const char *param);
static void UART_HandleLoad(const char *param);
static void UART_HandleShutdown(const char *param);
static void UART_HandleReset(const char *param);
static void UART_HandleTasks(const char *param);
static void UART_HandleMemory(const char *param);
static void UART_HandleErrors(const char *param);
static void UART_HandleLogLevel(const char *param);

/* 环形缓冲区操作 */
static void UART_RxBuffer_Put(uint8_t byte);
static bool UART_RxBuffer_Get(uint8_t *byte);
static uint16_t UART_RxBuffer_Count(void);

/* Private user code ---------------------------------------------------------*/

/* 环形缓冲区实现 */
static void UART_RxBuffer_Put(uint8_t byte)
{
    uint16_t next = (uart_rx_head + 1) % UART_RX_BUF_SIZE;
    if (next != uart_rx_tail) {
        uart_rx_buf[uart_rx_head] = byte;
        uart_rx_head = next;
    }
}

static bool UART_RxBuffer_Get(uint8_t *byte)
{
    if (uart_rx_head == uart_rx_tail) {
        return false;
    }
    *byte = uart_rx_buf[uart_rx_tail];
    uart_rx_tail = (uart_rx_tail + 1) % UART_RX_BUF_SIZE;
    return true;
}

static uint16_t UART_RxBuffer_Count(void)
{
    return (uart_rx_head - uart_rx_tail + UART_RX_BUF_SIZE) % UART_RX_BUF_SIZE;
}

/* UART 接收完成回调 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        UART_RxBuffer_Put(uart_rx_byte);
        /* 继续接收下一个字节 */
        HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
    }
}

/* ========================================================================== */
/*                            命令表                                           */
/* ========================================================================== */

static const CommandEntry_t cmd_table[] = {
    /* name         min_len  has_param  handler             */
    {"help",        4,       false,     UART_HandleHelp},
    {"status",      6,       false,     UART_HandleStatus},
    {"version",     7,       false,     UART_HandleVersion},
    {"start_osc",   9,       false,     UART_HandleStartOsc},
    {"stop_osc",    8,       false,     UART_HandleStopOsc},
    {"set_freq",    8,       true,      UART_HandleSetFreq},
    {"set_wave",    8,       true,      UART_HandleSetWave},
    {"set_amp",     7,       true,      UART_HandleSetAmp},
    {"start_gen",   9,       false,     UART_HandleStartGen},
    {"stop_gen",    8,       false,     UART_HandleStopGen},
    {"stream_dac",  10,      false,     UART_HandleStreamDac},
    {"zoom",        4,       true,      UART_HandleZoom},
    {"brightness",  10,      true,      UART_HandleBrightness},
    {"measure",     7,       false,     UART_HandleMeasure},
    {"page",        4,       true,      UART_HandlePage},
    {"key",         3,       true,      UART_HandleKey},
    {"cursor",      6,       true,      UART_HandleCursor},
    {"save",        4,       false,     UART_HandleSave},
    {"load",        4,       false,     UART_HandleLoad},
    {"shutdown",    8,       false,     UART_HandleShutdown},
    {"reset",       5,       false,     UART_HandleReset},
    /* 调试命令 */
    {"tasks",       5,       false,     UART_HandleTasks},
    {"memory",      6,       false,     UART_HandleMemory},
    {"errors",      6,       false,     UART_HandleErrors},
    {"log",         3,       true,      UART_HandleLogLevel},
};
#define CMD_TABLE_SIZE (sizeof(cmd_table) / sizeof(cmd_table[0]))

/* Exported function implementations -----------------------------------------*/

ErrorCode_t UART_Protocol_Init(void)
{
    LOG_INFO("Initializing UART protocol module...");

    /* 初始化缓冲区 */
    memset(cmd_buffer, 0, sizeof(cmd_buffer));
    cmd_index = 0;

    /* 清空环形缓冲区 */
    uart_rx_head = 0;
    uart_rx_tail = 0;

    /* 启动中断接收 */
    HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);

    initialized = true;
    LOG_INFO("UART protocol module initialized");
    return ERR_OK;
}

ErrorCode_t UART_Protocol_Start(void)
{
    if (!initialized) {
        LOG_ERROR("UART protocol not initialized");
        return ERR_NOT_INIT;
    }

    LOG_INFO("UART protocol started");
    return ERR_OK;
}

ErrorCode_t UART_Protocol_Stop(void)
{
    if (!initialized) {
        return ERR_NOT_INIT;
    }

    LOG_INFO("UART protocol stopped");
    return ERR_OK;
}

ErrorCode_t UART_SendText(const char *text)
{
    if (text == NULL) {
        return ERR_INVALID_PARAM;
    }

    /* 发送文本 */
    HAL_UART_Transmit(&huart1, (uint8_t*)text, strlen(text), 100);
    return ERR_OK;
}

ErrorCode_t UART_SendData(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) {
        return ERR_INVALID_PARAM;
    }

    /* 发送数据 */
    HAL_UART_Transmit(&huart1, (uint8_t*)data, len, 100);
    return ERR_OK;
}

ErrorCode_t UART_SendResponse(const char *cmd, ErrorCode_t result)
{
    if (cmd == NULL) {
        return ERR_INVALID_PARAM;
    }

    char response[RESP_MAX_LENGTH];

    if (result == ERR_OK) {
        snprintf(response, sizeof(response), "OK:%s\r\n", cmd);
    } else {
        snprintf(response, sizeof(response), "ERROR:%s:%d\r\n", cmd, result);
    }

    return UART_SendText(response);
}

ErrorCode_t UART_ReceiveCommand(char *cmd, uint16_t max_len)
{
    if (cmd == NULL || max_len == 0) {
        return ERR_INVALID_PARAM;
    }

    /* 从环形缓冲区读取数据 */
    uint8_t byte;
    uint16_t i = 0;

    /* 跳过前导换行符 */
    while (UART_RxBuffer_Get(&byte)) {
        if (byte != '\r' && byte != '\n') {
            cmd[i++] = byte;
            break;
        }
    }

    /* 如果没有有效数据，返回超时 */
    if (i == 0) {
        return ERR_TIMEOUT;
    }

    /* 读取完整命令（直到遇到换行符） */
    while (i < max_len - 1 && UART_RxBuffer_Get(&byte)) {
        if (byte == '\r' || byte == '\n') {
            cmd[i] = '\0';
            return ERR_OK;
        }
        cmd[i++] = byte;
    }

    /* 如果缓冲区为空但命令未结束，等待更多数据 */
    if (i < max_len - 1) {
        /* 等待最多 100ms 让更多数据到达 */
        uint32_t start = HAL_GetTick();
        while (HAL_GetTick() - start < 100) {
            if (UART_RxBuffer_Get(&byte)) {
                if (byte == '\r' || byte == '\n') {
                    cmd[i] = '\0';
                    return ERR_OK;
                }
                if (i < max_len - 1) {
                    cmd[i++] = byte;
                }
            }
            osDelay(1);
        }
    }

    cmd[i] = '\0';
    return (i > 0) ? ERR_OK : ERR_TIMEOUT;
}

ErrorCode_t UART_SetProtocolMode(ProtocolMode_t mode)
{
    protocol_mode = mode;
    LOG_INFO("Protocol mode set to: %d", mode);
    return ERR_OK;
}

ErrorCode_t UART_ExecuteCommand(const char *cmd_str)
{
    if (cmd_str == NULL || cmd_str[0] == '\0') {
        return ERR_INVALID_PARAM;
    }

    /* 记录收到的命令 */
    LOG_DEBUG("RX: %s", cmd_str);

    /* 遍历命令表 */
    for (uint64_t i = 0; i < CMD_TABLE_SIZE; i++) {
        uint8_t len = cmd_table[i].min_len;
        if (strncmp(cmd_str, cmd_table[i].name, len) == 0) {
            /* 确保完整匹配（下一个字符是空格或字符串结束） */
            char next = cmd_str[len];
            if (next == ' ' || next == '\0') {
                const char *param = NULL;
                if (cmd_table[i].has_param) {
                    const char *space = strchr(cmd_str, ' ');
                    param = (space != NULL) ? (space + 1) : NULL;
                }

                /* 记录命令执行 */
                LOG_DEBUG("CMD: %s (param: %s)", cmd_table[i].name,
                          param ? param : "none");

                cmd_table[i].handler(param);

                /* 记录命令完成 */
                LOG_DEBUG("CMD: %s done", cmd_table[i].name);
                return ERR_OK;
            }
        }
    }

    LOG_WARNING("Unknown command: %s", cmd_str);
    UART_SendText("ERROR:Unknown command\r\n");
    return ERR_UNKNOWN;
}

void UART_Task(void *argument)
{
    LOG_INFO("UART task started");

    /* 初始化 */
    UART_Protocol_Init();

    char cmd[CMD_MAX_LENGTH];

    for (;;) {
        /* 确保中断接收在运行 */
        if (huart1.RxState == HAL_UART_STATE_READY) {
            HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
        }

        /* 接收并执行命令 */
        if (UART_ReceiveCommand(cmd, sizeof(cmd)) == ERR_OK) {
            LOG_DEBUG("Received command: %s", cmd);
            UART_ExecuteCommand(cmd);
        }

        osDelay(10);
    }
}

/* Private function implementations ------------------------------------------*/

/**
  * @brief  处理帮助命令
  */
static void UART_HandleHelp(const char *param)
{
    (void)param;
    UART_SendText("=== Commands ===\r\n");
    UART_SendText("help           - Show help\r\n");
    UART_SendText("status         - Show status\r\n");
    UART_SendText("version        - Show version\r\n");
    UART_SendText("start_osc      - Start oscilloscope\r\n");
    UART_SendText("stop_osc       - Stop oscilloscope\r\n");
    UART_SendText("set_freq <hz>  - Set frequency\r\n");
    UART_SendText("set_wave <type>- Set waveform (sine/square/triangle)\r\n");
    UART_SendText("set_amp <mv>   - Set amplitude\r\n");
    UART_SendText("start_gen      - Start signal generator\r\n");
    UART_SendText("stop_gen       - Stop signal generator\r\n");
    UART_SendText("stream_dac     - Stream DAC+ADC waveform data\r\n");
    UART_SendText("zoom <1|2|4|8> - Set timebase zoom (1=auto)\r\n");
    UART_SendText("brightness <0-255> - Set OLED brightness\r\n");
    UART_SendText("measure        - Show measurements\r\n");
    UART_SendText("page <next|prev> - Switch page\r\n");
    UART_SendText("key <enter>    - Simulate key press\r\n");
    UART_SendText("cursor <x,y>   - Move cursor (0-127,0-63)\r\n");
    UART_SendText("cursor off     - Hide cursor\r\n");
    UART_SendText("save           - Save config to Flash\r\n");
    UART_SendText("load           - Load config from Flash\r\n");
    UART_SendText("shutdown       - Safe shutdown\r\n");
    UART_SendText("reset          - System reset\r\n");
    UART_SendText("\r\n=== Debug Commands ===\r\n");
    UART_SendText("tasks          - Show task status\r\n");
    UART_SendText("memory         - Show memory status\r\n");
    UART_SendText("errors         - Show error history\r\n");
    UART_SendText("log <0-4>      - Set log level (0=DEBUG, 4=FATAL)\r\n");
    UART_SendText("OK:help\r\n");
}

/**
  * @brief  处理状态命令
  */
static void UART_HandleStatus(const char *param)
{
    (void)param;
    char response[RESP_MAX_LENGTH];

    OscStatus_t osc_status;
    SigGenStatus_t siggen_status;

    Oscilloscope_GetStatus(&osc_status);
    SignalGen_GetStatus(&siggen_status);

    snprintf(response, sizeof(response),
             "=== Status ===\r\n"
             "Oscilloscope: %s\r\n"
             "Voltage: %lu mV\r\n"
             "Frequency: %lu Hz\r\n"
             "Signal Gen: %s\r\n"
             "OK:status\r\n",
             osc_status.running ? "Running" : "Stopped",
             osc_status.voltage_mv,
             osc_status.frequency_hz,
             siggen_status.running ? "Running" : "Stopped");

    UART_SendText(response);
}

/**
  * @brief  处理版本命令
  */
static void UART_HandleVersion(const char *param)
{
    (void)param;
    const VersionInfo_t *info = Version_GetInfo();
    char response[RESP_MAX_LENGTH];

    snprintf(response, sizeof(response),
             "=== Version ===\r\n"
             "Version: %s\r\n"
             "Built: %s %s\r\n"
             "OK:version\r\n",
             info->version_string,
             info->build_date,
             info->build_time);

    UART_SendText(response);
}

/**
  * @brief  处理启动示波器命令
  */
static void UART_HandleStartOsc(const char *param)
{
    (void)param;
    ErrorCode_t err = Oscilloscope_Start();
    UART_SendResponse("start_osc", err);
}

/**
  * @brief  处理停止示波器命令
  */
static void UART_HandleStopOsc(const char *param)
{
    (void)param;
    ErrorCode_t err = Oscilloscope_Stop();
    UART_SendResponse("stop_osc", err);
}

/**
  * @brief  处理设置频率命令
  */
static void UART_HandleSetFreq(const char *param)
{
    if (param == NULL || strlen(param) == 0) {
        UART_SendText("ERROR:set_freq:Missing parameter\r\n");
        return;
    }

    uint32_t freq = atoi(param);
    char debug[64];
    snprintf(debug, sizeof(debug), "DEBUG:set_freq=%lu\r\n", freq);
    UART_SendText(debug);
    ErrorCode_t err = SignalGen_SetFrequency(freq);
    UART_SendResponse("set_freq", err);
}

/**
  * @brief  处理设置波形命令
  */
static void UART_HandleSetWave(const char *param)
{
    if (param == NULL || strlen(param) == 0) {
        UART_SendText("ERROR:set_wave:Missing parameter\r\n");
        return;
    }

    WaveformType_t type;
    if (strcmp(param, "sine") == 0) {
        type = WAVE_SINE;
    } else if (strcmp(param, "square") == 0) {
        type = WAVE_SQUARE;
    } else if (strcmp(param, "triangle") == 0) {
        type = WAVE_TRIANGLE;
    } else if (strcmp(param, "sawtooth") == 0) {
        type = WAVE_SAWTOOTH;
    } else if (strcmp(param, "dc") == 0) {
        type = WAVE_DC;
    } else {
        UART_SendText("ERROR:set_wave:Invalid waveform\r\n");
        return;
    }

    ErrorCode_t err = SignalGen_SetWaveform(type);
    UART_SendResponse("set_wave", err);
}

/**
  * @brief  处理设置幅度命令
  */
static void UART_HandleSetAmp(const char *param)
{
    if (param == NULL || strlen(param) == 0) {
        UART_SendText("ERROR:set_amp:Missing parameter\r\n");
        return;
    }

    uint32_t amp = atoi(param);
    ErrorCode_t err = SignalGen_SetAmplitude(amp);
    UART_SendResponse("set_amp", err);
}

/**
  * @brief  处理启动信号发生器命令
  */
static void UART_HandleStartGen(const char *param)
{
    (void)param;
    ErrorCode_t err = SignalGen_Start();
    UART_SendResponse("start_gen", err);
}

/**
  * @brief  处理停止信号发生器命令
  */
static void UART_HandleStopGen(const char *param)
{
    (void)param;
    ErrorCode_t err = SignalGen_Stop();
    UART_SendResponse("stop_gen", err);
}

/**
  * @brief  流式输出DAC波形数据 + ADC采样数据
  *         格式：每行一个样本 "dac_val,adc_val\r\n"
  *         适合串口绘图仪（如 Serial Plotter）使用
  */
static void UART_HandleStreamDac(const char *param)
{
    (void)param;
    const uint16_t *dac_buf = SignalGen_GetWaveformBuffer();
    uint32_t dac_size = SignalGen_GetWaveformBufferSize();
    const uint16_t *adc_buf = Oscilloscope_GetAdcBuffer();
    uint32_t adc_size = Oscilloscope_GetAdcBufferSize();

    /* 发送帧头：STREAM:dac_count,adc_count */
    char header[32];
    snprintf(header, sizeof(header), "STREAM:%lu,%lu\r\n", dac_size, adc_size);
    HAL_UART_Transmit(&huart1, (uint8_t *)header, strlen(header), 100);

    /* 逐样本发送：DAC值,ADC值 */
    uint32_t count = (dac_size < adc_size) ? dac_size : adc_size;
    for (uint32_t i = 0; i < count; i++) {
        char line[24];
        int len = snprintf(line, sizeof(line), "%u,%u\r\n", dac_buf[i], adc_buf[i]);
        HAL_UART_Transmit(&huart1, (uint8_t *)line, len, 50);
    }

    /* 发送帧尾 */
    HAL_UART_Transmit(&huart1, (uint8_t *)"END\r\n", 5, 100);
}

/**
  * @brief  处理时间轴缩放命令
  */
static void UART_HandleZoom(const char *param)
{
    if (param == NULL || strlen(param) == 0) {
        UART_SendText("ERROR:zoom:Missing parameter (1|2|4|8)\r\n");
        return;
    }

    uint8_t zoom = (uint8_t)atoi(param);
    if (zoom == 0) zoom = 1;
    if (zoom > 8) zoom = 8;

    ErrorCode_t err = Display_SetTimebaseZoom(zoom);
    if (err == ERR_OK) {
        char response[32];
        snprintf(response, sizeof(response), "OK:zoom %dx\r\n", (int)zoom);
        UART_SendText(response);
    } else {
        UART_SendResponse("zoom", err);
    }
}

/**
  * @brief  处理亮度设置命令
  */
static void UART_HandleBrightness(const char *param)
{
    if (param == NULL || strlen(param) == 0) {
        UART_SendText("ERROR:brightness:Missing parameter (0-255)\r\n");
        return;
    }

    uint8_t brightness = (uint8_t)atoi(param);
    ErrorCode_t err = Display_SetBrightness(brightness);
    if (err == ERR_OK) {
        char response[32];
        snprintf(response, sizeof(response), "OK:brightness %d\r\n", brightness);
        UART_SendText(response);
    } else {
        UART_SendResponse("brightness", err);
    }
}

/**
  * @brief  处理测量结果查询命令
  */
static void UART_HandleMeasure(const char *param)
{
    (void)param;
    Display_Measurements_t meas;
    ErrorCode_t err = Display_GetMeasurements(&meas);
    if (err != ERR_OK) {
        UART_SendResponse("measure", err);
        return;
    }

    char response[128];
    snprintf(response, sizeof(response),
             "=== Measurements ===\r\n"
             "Freq: %lu Hz\r\n"
             "Period: %lu us\r\n"
             "Voltage: %lu mV\r\n"
             "Vpp: %lu mV\r\n"
             "Vrms: %lu mV\r\n"
             "Duty: %lu.%lu%%\r\n"
             "OK:measure\r\n",
             meas.frequency_hz,
             meas.period_us,
             meas.voltage_mv,
             meas.vpp_mv,
             meas.vrms_mv,
             meas.duty_permille / 10, meas.duty_permille % 10);
    UART_SendText(response);
}

/**
  * @brief  处理页面切换命令
  */
static void UART_HandlePage(const char *param)
{
    if (param == NULL || strlen(param) == 0) {
        UART_SendText("ERROR:page:Missing parameter (next|prev|0|1)\r\n");
        return;
    }

    ErrorCode_t err = ERR_OK;
    if (strcmp(param, "next") == 0) {
        err = Display_NextPage();
    } else if (strcmp(param, "prev") == 0) {
        err = Display_PrevPage();
    } else {
        UART_SendText("ERROR:page:Invalid parameter (next|prev)\r\n");
        return;
    }

    if (err == ERR_OK) {
        UART_SendText("OK:page\r\n");
    } else {
        UART_SendResponse("page", err);
    }
}

/**
  * @brief  处理模拟按键命令
  */
static void UART_HandleKey(const char *param)
{
    if (param == NULL || strlen(param) == 0) {
        UART_SendText("ERROR:key:Missing parameter (enter)\r\n");
        return;
    }

    if (strcmp(param, "enter") == 0) {
        /* 模拟 ENTER 键短按 */
        Display_NextPage();
        UART_SendText("OK:key enter (page switched)\r\n");
    } else {
        UART_SendText("ERROR:key:Invalid parameter (enter)\r\n");
    }
}

/**
  * @brief  处理光标控制命令
  */
static void UART_HandleCursor(const char *param)
{
    if (param == NULL || strlen(param) == 0) {
        UART_SendText("ERROR:cursor:Missing parameter (x,y or off)\r\n");
        return;
    }

    if (strcmp(param, "off") == 0) {
        Display_SetCursorEnabled(false);
        UART_SendText("OK:cursor off\r\n");
        return;
    }

    /* 解析 x,y 坐标 */
    uint16_t x, y;
    if (sscanf(param, "%hu,%hu", &x, &y) == 2) {
        ErrorCode_t err = Display_MoveCursor(x, y);
        if (err == ERR_OK) {
            char response[32];
            snprintf(response, sizeof(response), "OK:cursor %d,%d\r\n", x, y);
            UART_SendText(response);
        } else {
            UART_SendResponse("cursor", err);
        }
    } else {
        UART_SendText("ERROR:cursor:Invalid format (use x,y)\r\n");
    }
}

/**
  * @brief  处理保存配置命令
  */
static void UART_HandleSave(const char *param)
{
    (void)param;
    ErrorCode_t err = Config_Save();
    if (err == ERR_OK) {
        UART_SendText("OK:config saved to Flash\r\n");
    } else {
        UART_SendResponse("save", err);
    }
}

/**
  * @brief  处理加载配置命令
  */
static void UART_HandleLoad(const char *param)
{
    (void)param;
    ErrorCode_t err = Config_Load();
    if (err == ERR_OK) {
        UART_SendText("OK:config loaded from Flash\r\n");
    } else {
        UART_SendResponse("load", err);
    }
}

/**
  * @brief  处理安全关机命令
  */
static void UART_HandleShutdown(const char *param)
{
    (void)param;
    UART_SendText("OK:shutting down...\r\n");
    App_Shutdown();
}

/**
  * @brief  处理系统复位命令
  */
static void UART_HandleReset(const char *param)
{
    (void)param;
    UART_SendText("OK:reset\r\n");
    HAL_Delay(100);
    NVIC_SystemReset();
}

static void UART_HandleTasks(const char *param)
{
    (void)param;
    Debug_PrintTaskStatus();
    UART_SendText("OK:tasks\r\n");
}

static void UART_HandleMemory(const char *param)
{
    (void)param;
    Debug_PrintMemoryStatus();
    UART_SendText("OK:memory\r\n");
}

static void UART_HandleErrors(const char *param)
{
    (void)param;
    ErrorTracker_PrintHistory();
    UART_SendText("OK:errors\r\n");
}

static void UART_HandleLogLevel(const char *param)
{
    if (param == NULL) {
        UART_SendText("ERROR:missing level\r\n");
        return;
    }

    int level = atoi(param);
    if (level < 0 || level > 4) {
        UART_SendText("ERROR:invalid level (0-4)\r\n");
        return;
    }

    Debug_SetLogLevel((LogLevel_t)level);
    UART_SendText("OK:log\r\n");
}
