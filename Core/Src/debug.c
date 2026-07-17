/**
  ******************************************************************************
  * @file           : debug.c
  * @brief          : 调试工具模块实现
  ******************************************************************************
  * @attention
  *
  * 调试工具模块，提供日志输出、调试命令、性能监测等功能
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "debug.h"
#include "version.h"
#include "error_tracker.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* 外部变量声明 */
extern UART_HandleTypeDef huart1;

/**
  * @brief 日志缓冲区
  */
static char log_buffer[LOG_BUFFER_SIZE];

/**
  * @brief 当前日志级别
  */
static LogLevel_t current_log_level = LOG_INFO;

/**
  * @brief 初始化标志
  */
static bool initialized = false;

/* Private function prototypes -----------------------------------------------*/

static void Debug_PrintHelp(void);

/* Private user code ---------------------------------------------------------*/

/* Exported function implementations -----------------------------------------*/

ErrorCode_t Debug_Init(void)
{
    /* 设置日志级别 - 输出 INFO 及以上级别 */
    current_log_level = LOG_INFO;
    initialized = true;

    return ERR_OK;
}

void Debug_Log(LogLevel_t level, const char *file, uint32_t line,
               const char *fmt, ...)
{
    if (!initialized) {
        return;
    }

    /* 检查日志级别 */
    if (level < current_log_level) {
        return;
    }

    /* 格式化时间戳 */
    uint32_t tick = HAL_GetTick();
    int offset = snprintf(log_buffer, LOG_BUFFER_SIZE,
                          "[%lu.%03lu] ", tick / 1000, tick % 1000);

    /* 添加日志级别 */
    const char *level_str = Debug_GetLogLevelString(level);
    offset += snprintf(log_buffer + offset, LOG_BUFFER_SIZE - offset,
                       "[%s] ", level_str);

    /* 添加文件和行号（仅DEBUG级别） */
    if (level == LOG_DEBUG) {
        offset += snprintf(log_buffer + offset, LOG_BUFFER_SIZE - offset,
                           "%s:%lu ", file, line);
    }

    /* 添加用户消息 */
    va_list args;
    va_start(args, fmt);
    offset += vsnprintf(log_buffer + offset, LOG_BUFFER_SIZE - offset,
                        fmt, args);
    va_end(args);

    /* 添加换行 */
    if (offset < LOG_BUFFER_SIZE - 2) {
        log_buffer[offset++] = '\r';
        log_buffer[offset++] = '\n';
    }

    /* 输出到串口 */
    HAL_UART_Transmit(&huart1, (uint8_t*)log_buffer, offset, 100);
}

void Debug_SetLogLevel(LogLevel_t level)
{
    if (level >= LOG_DEBUG && level <= LOG_FATAL) {
        current_log_level = level;
        LOG_INFO("Log level set to %s", Debug_GetLogLevelString(level));
    }
}

LogLevel_t Debug_GetLogLevel(void)
{
    return current_log_level;
}

void Debug_ProcessCommand(const char *cmd)
{
    if (cmd == NULL) {
        return;
    }

    /* 去除前导空格 */
    while (*cmd == ' ') {
        cmd++;
    }

    /* 处理命令 */
    if (strcmp(cmd, "help") == 0) {
        Debug_PrintHelp();
    }
    else if (strcmp(cmd, "status") == 0) {
        Debug_PrintSystemStatus();
    }
    else if (strcmp(cmd, "tasks") == 0) {
        Debug_PrintTaskStatus();
    }
    else if (strcmp(cmd, "memory") == 0) {
        Debug_PrintMemoryStatus();
    }
    else if (strcmp(cmd, "version") == 0) {
        Version_Print();
    }
    else if (strcmp(cmd, "errors") == 0) {
        ErrorTracker_PrintHistory();
    }
    else if (strncmp(cmd, "log ", 4) == 0) {
        int level = atoi(cmd + 4);
        Debug_SetLogLevel((LogLevel_t)level);
    }
    else if (strcmp(cmd, "reset") == 0) {
        LOG_INFO("System reset...");
        HAL_Delay(100);
        NVIC_SystemReset();
    }
    else {
        LOG_WARNING("Unknown command: %s", cmd);
        LOG_INFO("Type 'help' for available commands");
    }
}

void Debug_PrintSystemStatus(void)
{
    LOG_INFO("=== System Status ===");
    LOG_INFO("Uptime: %lu s", HAL_GetTick() / 1000);
    LOG_INFO("CPU Usage: %lu%%", Debug_GetCPUUsage());
    LOG_INFO("Free Heap: %lu bytes", Debug_GetFreeHeapSize());
    LOG_INFO("Min Free Heap: %lu bytes", Debug_GetMinFreeHeapSize());
    LOG_INFO("SYSCLK: %lu Hz", HAL_RCC_GetSysClockFreq());
    LOG_INFO("HCLK: %lu Hz", HAL_RCC_GetHCLKFreq());
    LOG_INFO("PCLK1: %lu Hz", HAL_RCC_GetPCLK1Freq());
    LOG_INFO("PCLK2: %lu Hz", HAL_RCC_GetPCLK2Freq());
    LOG_INFO("Error Count: %d", ErrorTracker_GetCount());
    LOG_INFO("Log Level: %s", Debug_GetLogLevelString(current_log_level));
}

void Debug_PrintTaskStatus(void)
{
    LOG_INFO("=== Task Status ===");

#if (configUSE_TRACE_FACILITY == 1)
    /* 获取任务数量 */
    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    LOG_INFO("Task count: %lu", task_count);

    /* 获取任务状态 */
    TaskStatus_t *task_status = pvPortMalloc(task_count * sizeof(TaskStatus_t));
    if (task_status == NULL) {
        LOG_ERROR("Failed to allocate memory for task status");
        return;
    }

    uint32_t total_runtime;
    task_count = uxTaskGetSystemState(task_status, task_count, &total_runtime);

    /* 打印任务信息 */
    LOG_INFO("Name            State Prio  Stack Num");
    for (int i = 0; i < task_count; i++) {
        char state = '?';
        switch (task_status[i].eCurrentState) {
            case eRunning:   state = 'R'; break;
            case eReady:     state = 'Y'; break;
            case eBlocked:   state = 'B'; break;
            case eSuspended: state = 'S'; break;
            case eDeleted:   state = 'D'; break;
            default:         state = '?'; break;
        }

        LOG_INFO("%-15s %c     %lu    %lu    %lu",
                 task_status[i].pcTaskName,
                 state,
                 task_status[i].uxCurrentPriority,
                 task_status[i].usStackHighWaterMark,
                 task_status[i].xTaskNumber);
    }

    vPortFree(task_status);
#else
    LOG_INFO("Task trace not enabled");
#endif
}

void Debug_PrintMemoryStatus(void)
{
    LOG_INFO("=== Memory Status ===");
    LOG_INFO("Free Heap: %lu bytes", xPortGetFreeHeapSize());
    LOG_INFO("Min Free Heap: %lu bytes", xPortGetMinimumEverFreeHeapSize());
    LOG_INFO("Heap Size: %lu bytes", (uint32_t)configTOTAL_HEAP_SIZE);
}

uint32_t Debug_GetCPUUsage(void)
{
    /* 简化的CPU使用率计算 */
    static uint32_t last_tick = 0;
    static uint32_t idle_count = 0;
    static uint32_t total_count = 0;

    uint32_t current_tick = HAL_GetTick();
    uint32_t elapsed = current_tick - last_tick;

    if (elapsed >= 1000) {
        /* 每秒更新一次 */
        last_tick = current_tick;
        /* 这里需要实际的空闲任务计数 */
        /* 暂时返回估算值 */
        return 0;
    }

    return 0;
}

uint32_t Debug_GetFreeHeapSize(void)
{
    return xPortGetFreeHeapSize();
}

uint32_t Debug_GetMinFreeHeapSize(void)
{
    return xPortGetMinimumEverFreeHeapSize();
}

const char* Debug_GetLogLevelString(LogLevel_t level)
{
    switch (level) {
        case LOG_DEBUG:   return "DBG";
        case LOG_INFO:    return "INF";
        case LOG_WARNING: return "WRN";
        case LOG_ERROR:   return "ERR";
        case LOG_FATAL:   return "FTL";
        default:          return "UNK";
    }
}

/* Private function implementations ------------------------------------------*/

static void Debug_PrintHelp(void)
{
    LOG_INFO("=== Debug Commands ===");
    LOG_INFO("help           - Show this help");
    LOG_INFO("status         - Show system status");
    LOG_INFO("tasks          - Show task list");
    LOG_INFO("memory         - Show memory info");
    LOG_INFO("version        - Show version info");
    LOG_INFO("errors         - Show error history");
    LOG_INFO("log <level>    - Set log level (0-4)");
    LOG_INFO("reset          - System reset");
    LOG_INFO("");
    LOG_INFO("Log levels: 0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR, 4=FATAL");
}
