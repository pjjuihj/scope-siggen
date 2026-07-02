/**
  ******************************************************************************
  * @file           : display.c
  * @brief          : 显示模块实现
  ******************************************************************************
  * @attention
  *
  * 显示模块，采用帧管理机制：
  *   - 所有 "setter" 函数只更新内部状态，不直接操作 GRAM
  *   - Display_Task 统一执行渲染：清屏 → 绘制所有元素 → 刷新
  *   - 互斥锁保护 OLED I2C 操作
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "display.h"
#include "oled.h"
#include "debug.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Private typedef -----------------------------------------------------------*/

/** 显示页面类型 */
typedef enum {
    PAGE_SPLASH,        /* 启动画面 */
    PAGE_OSCOPE,        /* 示波器页面 */
    PAGE_SIGGEN,        /* 信号发生器页面 */
    PAGE_SYSINFO,       /* 系统信息页面 */
    PAGE_MENU,          /* 菜单页面 */
    PAGE_MESSAGE,       /* 消息页面 */
    PAGE_COUNT          /* 页面总数（用于指示点） */
} DisplayPage_t;

/* Private define ------------------------------------------------------------*/

#define ADC_RESOLUTION      4096    /* 12位ADC */
#define ADC_REF_VOLTAGE     3300    /* 3.3V参考电压 (mV) */

#define WAVEFORM_Y_TOP      10      /* 波形区域顶部（留出测量值空间） */
#define WAVEFORM_Y_BOTTOM   47      /* 波形区域底部 (48像素高) */
#define STATUS_Y            6       /* 状态行 Y (page 6, 像素 48-55) */

/* Private variables ---------------------------------------------------------*/

/* 外部变量声明 */
extern UART_HandleTypeDef huart1;
extern u8 OLED_GRAM[128][8];

/* 任务句柄 */
osThreadId_t display_task_handle;

/* 初始化标志 */
static bool initialized = false;

/* OLED 互斥锁 — 保护所有 I2C 操作 */
static osMutexId_t oled_mutex = NULL;

/* 需要刷新标志（setter 设置，Display_Task 消费） */
static volatile bool dirty = false;

/* 当前显示页面 */
static DisplayPage_t current_page = PAGE_SPLASH;

/* --- 示波器页面状态 --- */
static uint16_t *waveform_data = NULL;
static uint16_t  waveform_len  = 0;
static uint32_t  voltage_mv    = 0;
static uint32_t  frequency_hz  = 0;
static bool      show_cursor   = false;
static uint16_t  cursor_x      = 0;
static uint16_t  cursor_y      = 0;
static char      status_text[22] = "";
static bool      waveform_valid = false;

/* --- 菜单页面状态 --- */
static const MenuItem_t *menu_items  = NULL;
static uint8_t           menu_count  = 0;
static uint8_t           menu_select = 0;

/* --- 测量结果 --- */
static uint32_t measured_vpp = 0;    /* 峰峰值 (mV) */
static uint32_t measured_period_us = 0;  /* 周期 (µs) */
static uint32_t measured_freq = 0;   /* 测量频率 (Hz) */
static uint16_t measured_duty = 0;   /* 占空比 (‰, 0-1000) */
static uint32_t measured_vrms = 0;   /* 有效值电压 (mV) */

/* --- 时间轴缩放 --- */
static uint8_t timebase_zoom = 4;    /* 1=自动, 2=2x, 4=4x, 8=8x */

/* --- 触发电平 --- */
static uint32_t trigger_level_mv = 0;  /* 触发电平 (mV), 0=不显示 */

/* --- 信号发生器页面状态 --- */
static uint32_t siggen_freq = 0;       /* 频率 (Hz) */
static uint32_t siggen_amp = 0;        /* 幅度 (mV) */
static uint8_t  siggen_wave = 0;       /* 波形类型 */
static uint16_t siggen_duty = 0;       /* 占空比 (‰) */
static bool     siggen_running = false; /* 运行状态 */

/* --- 消息页面状态 --- */
static char message_text[32] = "";

/* Private function prototypes -----------------------------------------------*/

static void Render_Splash(void);
static void Render_Scope(void);
static void Render_SigGen(void);
static void Render_SysInfo(void);
static void Render_Menu(void);
static void Render_Message(void);
static void Draw_WaveformPreview(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t wave_type);
static void Draw_Grid(void);
static void Draw_Waveform(void);
static void Draw_Measurements(void);
static void Draw_Cursor(void);
static void Draw_StatusBar(void);
static void Draw_TriggerLevel(void);
static void Draw_PageIndicator(void);
static void OLED_Lock(void);
static void OLED_Unlock(void);

/* Private user code ---------------------------------------------------------*/

static void OLED_Lock(void)
{
    if (oled_mutex != NULL) {
        osMutexAcquire(oled_mutex, osWaitForever);
    }
}

static void OLED_Unlock(void)
{
    if (oled_mutex != NULL) {
        osMutexRelease(oled_mutex);
    }
}

static void Draw_Grid(void)
{
    /* 网格点阵（每 16px 水平，每 4px 垂直） */
    for (uint16_t x = 0; x < OLED_WIDTH; x += 16) {
        for (uint16_t y = WAVEFORM_Y_TOP; y <= WAVEFORM_Y_BOTTOM; y += 4) {
            OLED_DrawPoint(x, y, 1);
        }
    }
    for (uint16_t y = WAVEFORM_Y_TOP; y <= WAVEFORM_Y_BOTTOM; y += 16) {
        for (uint16_t x = 0; x < OLED_WIDTH; x += 4) {
            OLED_DrawPoint(x, y, 1);
        }
    }

    /* 中心十字线 */
    uint16_t cx = OLED_WIDTH / 2;                          /* X 中心 = 64 */
    uint16_t cy = (WAVEFORM_Y_TOP + WAVEFORM_Y_BOTTOM) / 2; /* Y 中心 = 28 */

    /* 水平中心线（实线） */
    OLED_DrawLine(0, cy, OLED_WIDTH - 1, cy, 1);

    /* 垂直中心线（虚线，每隔 2 像素画点） */
    for (uint16_t y = WAVEFORM_Y_TOP; y <= WAVEFORM_Y_BOTTOM; y += 2) {
        OLED_DrawPoint(cx, y, 1);
    }

    /* 波形区域边框 */
    OLED_DrawLine(0, WAVEFORM_Y_TOP, OLED_WIDTH - 1, WAVEFORM_Y_TOP, 1);
    OLED_DrawLine(0, WAVEFORM_Y_BOTTOM, OLED_WIDTH - 1, WAVEFORM_Y_BOTTOM, 1);
}

static void Draw_Waveform(void)
{
    if (!waveform_valid || waveform_data == NULL || waveform_len < 2) {
        return;
    }

    uint16_t *data = waveform_data;
    uint16_t len = waveform_len;
    uint16_t plot_h = WAVEFORM_Y_BOTTOM - WAVEFORM_Y_TOP;

    /* 时间轴缩放：选择显示范围 */
    uint16_t display_len = len;
    uint16_t offset = 0;

    if (timebase_zoom > 1 && len > OLED_WIDTH) {
        /* 放大时只显示中心部分 */
        display_len = len / timebase_zoom;
        if (display_len < OLED_WIDTH) display_len = OLED_WIDTH;
        offset = (len - display_len) / 2;
    }

    /* 自动缩放：找到显示范围内的数据范围 */
    uint16_t min_val = data[offset], max_val = data[offset];
    for (uint16_t i = offset + 1; i < offset + display_len; i++) {
        if (data[i] < min_val) min_val = data[i];
        if (data[i] > max_val) max_val = data[i];
    }

    uint16_t range = max_val - min_val;
    if (range < 10) return;  /* 数据太平，不画 */

    /* 计算 Vpp (mV): range * 3300 / 4096 */
    measured_vpp = (uint32_t)range * ADC_REF_VOLTAGE / ADC_RESOLUTION;

    /* 计算周期和频率（过零检测） */
    uint16_t threshold = (min_val + max_val) / 2;
    uint16_t zero_count = 0;
    uint16_t first_zero = 0;
    uint16_t last_zero = 0;

    for (uint16_t i = offset + 1; i < offset + display_len; i++) {
        if (data[i - 1] < threshold && data[i] >= threshold) {
            if (zero_count == 0) first_zero = i;
            last_zero = i;
            zero_count++;
        }
    }

    if (zero_count >= 2) {
        /* 周期 = (last_zero - first_zero) / (zero_count - 1) * 采样时间 */
        /* 假设采样率 1MHz，采样时间 = 1µs */
        uint16_t samples_per_period = (last_zero - first_zero) / (zero_count - 1);
        measured_period_us = samples_per_period;  /* 1MHz 采样率，1 样本 = 1µs */
        measured_freq = 1000000 / measured_period_us;

        /* 计算占空比（高电平时间 / 周期） */
        uint16_t high_count = 0;
        for (uint16_t i = offset; i < offset + display_len; i++) {
            if (data[i] >= threshold) high_count++;
        }
        measured_duty = (uint16_t)((uint32_t)high_count * 1000 / display_len);
    } else {
        measured_period_us = 0;
        measured_freq = 0;
        measured_duty = 0;
    }

    /* 计算 Vrms（均方根电压） */
    uint64_t sum_sq = 0;
    for (uint16_t i = offset; i < offset + display_len; i++) {
        int32_t val = (int32_t)data[i] - (int32_t)threshold;
        sum_sq += (uint64_t)(val * val);
    }
    uint32_t rms_adc = (uint32_t)(sum_sq / display_len);
    /* 简化开方：使用近似值 */
    uint32_t rms_approx = 0;
    for (uint32_t i = 1; i * i <= rms_adc; i++) {
        rms_approx = i;
    }
    measured_vrms = rms_approx * ADC_REF_VOLTAGE / ADC_RESOLUTION;

    /* X轴：将显示范围映射到屏幕宽度（使用定点数避免浮点） */
    /* 每个像素对应的样本数 = display_len / OLED_WIDTH (定点 x256) */
    uint32_t step_fp = ((uint32_t)display_len << 8) / OLED_WIDTH;

    uint16_t prev_y = 0;
    for (uint16_t x = 0; x < OLED_WIDTH; x++) {
        uint16_t idx_start = offset + (uint16_t)((x * step_fp) >> 8);
        uint16_t idx_end   = offset + (uint16_t)(((x + 1) * step_fp) >> 8);
        if (idx_start >= offset + display_len) break;
        if (idx_end > offset + display_len) idx_end = offset + display_len;

        /* 取该区间的平均值 */
        uint32_t sum = 0;
        uint16_t count = 0;
        for (uint16_t i = idx_start; i < idx_end; i++) {
            sum += data[i];
            count++;
        }
        uint16_t avg = (count > 0) ? (uint16_t)(sum / count) : data[idx_start];

        /* Y轴：映射到 [0, plot_h] */
        uint16_t y = (uint16_t)((uint32_t)(avg - min_val) * plot_h / range);
        uint16_t y_pixel = WAVEFORM_Y_BOTTOM - y;

        /* 画点 */
        OLED_DrawPoint(x, y_pixel, 1);

        /* 与前一个点连线（跳过第一个点） */
        if (x > 0) {
            OLED_DrawLine(x - 1, prev_y, x, y_pixel, 1);
        }
        prev_y = y_pixel;
    }
}

static void Draw_Measurements(void)
{
    char buf[22];

    if (frequency_hz >= 1000000) {
        snprintf(buf, sizeof(buf), "%lu.%02luMHz", frequency_hz / 1000000, (frequency_hz % 1000000) / 10000);
    } else if (frequency_hz >= 1000) {
        snprintf(buf, sizeof(buf), "%lu.%02lukHz", frequency_hz / 1000, (frequency_hz % 1000) / 10);
    } else {
        snprintf(buf, sizeof(buf), "%luHz", frequency_hz);
    }
    OLED_ShowString(0, 0, (u8 *)buf, 8, 1);

    snprintf(buf, sizeof(buf), "%lu.%02luV", voltage_mv / 1000, (voltage_mv % 1000) / 10);
    uint8_t len = (uint8_t)strlen(buf);
    uint8_t x_pos = (len * 6 < OLED_WIDTH) ? (OLED_WIDTH - len * 6) : 0;
    OLED_ShowString(x_pos, 0, (u8 *)buf, 8, 1);
}

static void Draw_Cursor(void)
{
    if (!show_cursor) return;

    for (int i = -3; i <= 3; i++) {
        int16_t px = (int16_t)cursor_x + i;
        int16_t py = (int16_t)cursor_y + i;
        if (px >= 0 && px < OLED_WIDTH) {
            OLED_DrawPoint((u8)px, cursor_y, 1);
        }
        if (py >= 0 && py < OLED_HEIGHT) {
            OLED_DrawPoint(cursor_x, (u8)py, 1);
        }
    }
}

static void Draw_StatusBar(void)
{
    char buf[22];

    if (!waveform_valid || measured_vpp == 0) {
        /* 无波形时显示状态文字 */
        if (status_text[0] != '\0') {
            OLED_ShowString(0, STATUS_Y, (u8 *)status_text, 8, 1);
        }
        return;
    }

    /* 左侧：Vpp + Vrms */
    snprintf(buf, sizeof(buf), "%lu.%02luV %lu.%01luV",
             measured_vpp / 1000, (measured_vpp % 1000) / 10,
             measured_vrms / 1000, (measured_vrms % 1000) / 100);
    OLED_ShowString(0, STATUS_Y, (u8 *)buf, 8, 1);

    /* 右侧：周期 + 占空比 */
    if (measured_period_us > 0) {
        if (measured_period_us >= 1000) {
            snprintf(buf, sizeof(buf), "%lu.%02lums %lu%%",
                     measured_period_us / 1000, (measured_period_us % 1000) / 10,
                     measured_duty / 10);
        } else {
            snprintf(buf, sizeof(buf), "%luus %lu%%",
                     measured_period_us, measured_duty / 10);
        }
        uint8_t len = (uint8_t)strlen(buf);
        uint8_t x_pos = (len * 6 < OLED_WIDTH) ? (OLED_WIDTH - len * 6) : 0;
        OLED_ShowString(x_pos, STATUS_Y, (u8 *)buf, 8, 1);
    }
}

static void Draw_TriggerLevel(void)
{
    if (trigger_level_mv == 0 || trigger_level_mv >= ADC_REF_VOLTAGE) return;

    /* 将触发电平 (mV) 映射到 Y 像素坐标 */
    uint16_t plot_h = WAVEFORM_Y_BOTTOM - WAVEFORM_Y_TOP;
    uint16_t y = (uint16_t)((uint32_t)trigger_level_mv * plot_h / ADC_REF_VOLTAGE);
    uint16_t y_pixel = WAVEFORM_Y_BOTTOM - y;

    /* 水平虚线（每隔 3 像素画 1 像素） */
    for (uint16_t x = 0; x < OLED_WIDTH; x += 3) {
        OLED_DrawPoint(x, y_pixel, 1);
    }

    /* 左侧 "T" 标记 */
    OLED_ShowString(0, y_pixel / 8, (u8 *)"T", 8, 1);
}

static void Render_Splash(void)
{
    OLED_ShowString(0, 2, (u8 *)"SCOPE-SIGGEN", 12, 1);
    OLED_ShowString(0, 4, (u8 *)"v1.4.0", 12, 1);
}

static void Render_Scope(void)
{
    Draw_Grid();
    Draw_TriggerLevel();
    Draw_Waveform();
    Draw_Measurements();
    Draw_Cursor();
    Draw_StatusBar();
    Draw_PageIndicator();
}

/**
  * @brief  绘制波形预览图标（使用查表法避免浮点运算）
  * @param  x: 起始 X 坐标
  * @param  y: 起始 Y 坐标
  * @param  w: 宽度
  * @param  h: 高度
  * @param  wave_type: 波形类型 (0-4)
  * @retval None
  */
static void Draw_WaveformPreview(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t wave_type)
{
    uint8_t mid_y = y + h / 2;
    uint8_t top_y = y + 2;
    uint8_t bot_y = y + h - 2;
    uint8_t end_x = x + w;

    /* 绘制边框 */
    OLED_DrawLine(x, y, end_x, y, 1);
    OLED_DrawLine(x, y + h, end_x, y + h, 1);

    /* 绘制波形 */
    switch (wave_type) {
        case 0: { /* 正弦波 - 使用分段线性近似 */
            uint8_t quarter = w / 4;
            /* 上升段 */
            OLED_DrawLine(x, mid_y, x + quarter, top_y, 1);
            /* 下降段 */
            OLED_DrawLine(x + quarter, top_y, x + 3 * quarter, bot_y, 1);
            /* 上升段 */
            OLED_DrawLine(x + 3 * quarter, bot_y, end_x, mid_y, 1);
            break;
        }
        case 1: { /* 方波 */
            uint8_t half_w = w / 2;
            OLED_DrawLine(x, bot_y, x, top_y, 1);
            OLED_DrawLine(x, top_y, x + half_w, top_y, 1);
            OLED_DrawLine(x + half_w, top_y, x + half_w, bot_y, 1);
            OLED_DrawLine(x + half_w, bot_y, end_x, bot_y, 1);
            break;
        }
        case 2: { /* 三角波 */
            uint8_t half_w = w / 2;
            OLED_DrawLine(x, bot_y, x + half_w, top_y, 1);
            OLED_DrawLine(x + half_w, top_y, end_x, bot_y, 1);
            break;
        }
        case 3: { /* 锯齿波 */
            OLED_DrawLine(x, bot_y, end_x, top_y, 1);
            OLED_DrawLine(end_x, top_y, end_x, bot_y, 1);
            break;
        }
        case 4: { /* 直流 */
            OLED_DrawLine(x, mid_y, end_x, mid_y, 1);
            break;
        }
    }
}

static void Render_SigGen(void)
{
    char buf[22];

    /* 标题 */
    OLED_ShowString(0, 0, (u8 *)"Signal Generator", 8, 1);

    /* 运行状态 */
    OLED_ShowString(0, 2, (u8 *)(siggen_running ? "Status: ON" : "Status: OFF"), 8, 1);

    /* 波形类型 */
    const char *wave_name = "Unknown";
    switch (siggen_wave) {
        case 0: wave_name = "Sine"; break;
        case 1: wave_name = "Square"; break;
        case 2: wave_name = "Triangle"; break;
        case 3: wave_name = "Sawtooth"; break;
        case 4: wave_name = "DC"; break;
    }
    snprintf(buf, sizeof(buf), "Wave: %s", wave_name);
    OLED_ShowString(0, 3, (u8 *)buf, 8, 1);

    /* 频率 */
    if (siggen_freq >= 1000000) {
        snprintf(buf, sizeof(buf), "Freq: %lu.%02luMHz",
                 siggen_freq / 1000000, (siggen_freq % 1000000) / 10000);
    } else if (siggen_freq >= 1000) {
        snprintf(buf, sizeof(buf), "Freq: %lu.%02lukHz",
                 siggen_freq / 1000, (siggen_freq % 1000) / 10);
    } else {
        snprintf(buf, sizeof(buf), "Freq: %luHz", siggen_freq);
    }
    OLED_ShowString(0, 4, (u8 *)buf, 8, 1);

    /* 幅度 */
    snprintf(buf, sizeof(buf), "Amp: %lu.%02luV",
             siggen_amp / 1000, (siggen_amp % 1000) / 10);
    OLED_ShowString(0, 5, (u8 *)buf, 8, 1);

    /* 占空比（仅方波显示） */
    if (siggen_wave == 1) {  /* Square */
        snprintf(buf, sizeof(buf), "Duty: %lu.%lu%%",
                 siggen_duty / 10, siggen_duty % 10);
        OLED_ShowString(0, 6, (u8 *)buf, 8, 1);
    }

    /* 波形预览（右侧居中） */
    Draw_WaveformPreview(85, 16, 40, 20, siggen_wave);

    Draw_PageIndicator();
}

static void Render_SysInfo(void)
{
    char buf[22];

    /* 标题 */
    OLED_ShowString(0, 0, (u8 *)"System Info", 8, 1);

    /* 版本 */
    OLED_ShowString(0, 2, (u8 *)"v1.4.0", 8, 1);

    /* 运行时间 */
    uint32_t uptime_sec = HAL_GetTick() / 1000;
    snprintf(buf, sizeof(buf), "Up: %lu:%02lu:%02lu",
             uptime_sec / 3600, (uptime_sec % 3600) / 60, uptime_sec % 60);
    OLED_ShowString(0, 3, (u8 *)buf, 8, 1);

    /* 堆内存使用 */
    extern size_t xPortGetFreeHeapSize(void);
    size_t free_heap = xPortGetFreeHeapSize();
    snprintf(buf, sizeof(buf), "Heap: %lu B free", (uint32_t)free_heap);
    OLED_ShowString(0, 4, (u8 *)buf, 8, 1);

    /* 任务数量 */
    snprintf(buf, sizeof(buf), "Tasks: %lu", (uint32_t)uxTaskGetNumberOfTasks());
    OLED_ShowString(0, 5, (u8 *)buf, 8, 1);

    /* 采样率 */
    snprintf(buf, sizeof(buf), "ADC: 1MHz 10kHz");
    OLED_ShowString(0, 6, (u8 *)buf, 8, 1);

    Draw_PageIndicator();
}

static void Draw_PageIndicator(void)
{
    /* 显示 3 个页面的指示点：示波器/信号发生器/系统信息 */
    uint8_t dot_y = OLED_HEIGHT - 2;  /* 倒数第二行，避免超出范围 */
    uint8_t dot_spacing = 16;
    uint8_t start_x = (OLED_WIDTH - 2 * dot_spacing) / 2;

    /* 第一个点：示波器页面 */
    uint8_t x1 = start_x;
    if (current_page == PAGE_OSCOPE) {
        OLED_FillRect(x1 - 1, dot_y - 1, 3, 3, 1);
    } else {
        OLED_DrawPoint(x1, dot_y, 1);
    }

    /* 第二个点：信号发生器页面 */
    uint8_t x2 = start_x + dot_spacing;
    if (current_page == PAGE_SIGGEN) {
        OLED_FillRect(x2 - 1, dot_y - 1, 3, 3, 1);
    } else {
        OLED_DrawPoint(x2, dot_y, 1);
    }

    /* 第三个点：系统信息页面 */
    uint8_t x3 = start_x + 2 * dot_spacing;
    if (current_page == PAGE_SYSINFO) {
        OLED_FillRect(x3 - 1, dot_y - 1, 3, 3, 1);
    } else {
        OLED_DrawPoint(x3, dot_y, 1);
    }
}

static void Render_Menu(void)
{
    if (menu_items == NULL || menu_count == 0) return;

    for (uint8_t i = 0; i < menu_count; i++) {
        /* 菜单项 Y 坐标检查（每项占 2 页，8 页总共 4 项） */
        if (i * 2 >= OLED_HEIGHT / 8) break;

        uint8_t x = 8;
        if (i == menu_select) {
            OLED_ShowString(0, i * 2, (u8 *)">", 12, 1);
            x = 12;
        }
        OLED_ShowString(x, i * 2, (u8 *)menu_items[i].text, 12, 1);
    }
}

static void Render_Message(void)
{
    if (message_text[0] == '\0') return;

    uint16_t x = (OLED_WIDTH - strlen(message_text) * 8) / 2;
    if (x > OLED_WIDTH) x = 0;
    OLED_ShowString(x, 3, (u8 *)message_text, 16, 1);
}

/**
  * @brief  内部帧渲染（调用前必须持有锁）
  */
static void Render_Frame(void)
{
    OLED_Clear();

    switch (current_page) {
    case PAGE_SPLASH:
        Render_Splash();
        break;
    case PAGE_OSCOPE:
        Render_Scope();
        break;
    case PAGE_SIGGEN:
        Render_SigGen();
        break;
    case PAGE_SYSINFO:
        Render_SysInfo();
        break;
    case PAGE_MENU:
        Render_Menu();
        break;
    case PAGE_MESSAGE:
        Render_Message();
        break;
    }

    OLED_Refresh();
}

/* Exported function implementations -----------------------------------------*/

ErrorCode_t Display_Init(void)
{
    LOG_INFO("Initializing display module...");

    OLED_Init();
    memset(OLED_GRAM, 0, sizeof(OLED_GRAM));

    current_page = PAGE_SPLASH;
    initialized = true;

    LOG_INFO("Display module initialized");
    return ERR_OK;
}

ErrorCode_t Display_Clear(void)
{
    if (!initialized) return ERR_NOT_INIT;

    OLED_Lock();

    waveform_data = NULL;
    waveform_len  = 0;
    waveform_valid = false;
    voltage_mv    = 0;
    frequency_hz  = 0;
    show_cursor   = false;
    cursor_x      = 0;
    cursor_y      = 0;
    status_text[0] = '\0';
    menu_items    = NULL;
    menu_count    = 0;
    menu_select   = 0;
    message_text[0] = '\0';
    current_page = PAGE_SPLASH;

    OLED_Unlock();
    return ERR_OK;
}

ErrorCode_t Display_Update(void)
{
    if (!initialized) return ERR_NOT_INIT;

    static uint32_t last_update = 0;
    uint32_t now = HAL_GetTick();

    /* 示波器页面使用 100ms 节流（10Hz），其他页面使用 1s 节流 */
    uint32_t throttle = (current_page == PAGE_OSCOPE) ? 100 : 1000;
    if (now - last_update < throttle) {
        return ERR_OK;
    }

    OLED_Lock();
    Render_Frame();
    OLED_Unlock();

    last_update = now;
    return ERR_OK;
}

ErrorCode_t Display_ForceUpdate(void)
{
    if (!initialized) return ERR_NOT_INIT;

    OLED_Lock();
    Render_Frame();
    OLED_Unlock();

    return ERR_OK;
}

ErrorCode_t Display_SendToSerial(void)
{
    if (!initialized) return ERR_NOT_INIT;

    char header[] = "OLED_DATA_START\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t *)header, strlen(header), 100);

    /* 按页批量发送（每页 128 字节），减少 UART 调用次数 */
    uint8_t page_buf[128];
    for (int page = 0; page < 8; page++) {
        for (int col = 0; col < 128; col++) {
            page_buf[col] = OLED_GRAM[col][page];
        }
        HAL_UART_Transmit(&huart1, page_buf, 128, 100);
    }

    char footer[] = "\r\nOLED_DATA_END\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t *)footer, strlen(footer), 100);

    return ERR_OK;
}

ErrorCode_t Display_SetBrightness(uint8_t brightness)
{
    if (!initialized) return ERR_NOT_INIT;

    /* SSD1306 对比度控制命令: 0x81 + 值 */
    OLED_WR_Byte(0x81, 0);  /* SET_CONTRAST */
    OLED_WR_Byte(brightness, 0);

    return ERR_OK;
}

ErrorCode_t Display_DrawWaveform(uint16_t *data, uint16_t len)
{
    if (!initialized || data == NULL || len == 0) {
        return ERR_INVALID_PARAM;
    }

    waveform_data = data;
    waveform_len  = len;
    waveform_valid = true;
    current_page   = PAGE_OSCOPE;
    dirty = true;

    return ERR_OK;
}

ErrorCode_t Display_UpdateScope(uint16_t *data, uint16_t len,
                                uint32_t voltage, uint32_t frequency)
{
    if (!initialized || data == NULL || len == 0) {
        return ERR_INVALID_PARAM;
    }

    /* 调试输出已禁用（避免干扰命令接收） */

    /* 原子更新全部示波器数据，避免 Display_Task 读到新旧混合值 */
    OLED_Lock();
    waveform_data  = data;
    waveform_len   = len;
    waveform_valid = true;
    voltage_mv     = voltage;
    frequency_hz   = frequency;
    snprintf(status_text, sizeof(status_text), "Running");
    dirty          = true;
    OLED_Unlock();

    return ERR_OK;
}

ErrorCode_t Display_DrawGrid(void)
{
    return ERR_OK;
}

ErrorCode_t Display_DrawCursor(uint16_t x, uint16_t y)
{
    if (!initialized) return ERR_NOT_INIT;

    cursor_x    = x;
    cursor_y    = y;
    show_cursor = true;
    dirty = true;

    return ERR_OK;
}

ErrorCode_t Display_ShowStatus(const char *status)
{
    if (!initialized || status == NULL) return ERR_INVALID_PARAM;

    snprintf(status_text, sizeof(status_text), "%s", status);
    dirty = true;

    return ERR_OK;
}

ErrorCode_t Display_ShowVoltage(uint32_t voltage)
{
    if (!initialized) return ERR_NOT_INIT;

    OLED_Lock();
    voltage_mv = voltage;
    dirty = true;
    OLED_Unlock();

    return ERR_OK;
}

ErrorCode_t Display_ShowFrequency(uint32_t frequency)
{
    if (!initialized) return ERR_NOT_INIT;

    OLED_Lock();
    frequency_hz = frequency;
    dirty = true;
    OLED_Unlock();

    return ERR_OK;
}

ErrorCode_t Display_ShowMessage(const char *message)
{
    if (!initialized || message == NULL) return ERR_INVALID_PARAM;

    int ret = snprintf(message_text, sizeof(message_text), "%s", message);
    if (ret >= (int)sizeof(message_text)) {
        LOG_WARNING("Message truncated (%d chars)", ret);
    }
    current_page = PAGE_MESSAGE;
    dirty = true;

    return ERR_OK;
}

ErrorCode_t Display_ShowMenu(const MenuItem_t *menu, uint8_t count)
{
    if (!initialized || menu == NULL || count == 0) return ERR_INVALID_PARAM;

    menu_items  = menu;
    menu_count  = count;
    menu_select = 0;
    current_page = PAGE_MENU;
    dirty = true;

    return ERR_OK;
}

ErrorCode_t Display_UpdateSelection(uint8_t selected)
{
    if (!initialized) return ERR_NOT_INIT;

    if (selected < menu_count) {
        menu_select = selected;
        dirty = true;
    }

    return ERR_OK;
}

void Display_Task(void *argument)
{
    (void)argument;
    LOG_INFO("Display task started");

    /* 在调度器运行后创建互斥锁 */
    oled_mutex = osMutexNew(NULL);
    configASSERT(oled_mutex != NULL);

    /* 显示启动画面 1.5 秒 */
    OLED_Lock();
    Render_Frame();
    OLED_Unlock();
    osDelay(1500);

    /* 切换到示波器页面 */
    current_page = PAGE_OSCOPE;
    snprintf(status_text, sizeof(status_text), "Waiting...");
    dirty = true;

    for (;;) {
        /* 有数据变化时才刷新，节省 I2C 带宽 */
        if (dirty) {
            OLED_Lock();
            Render_Frame();
            OLED_Unlock();
            dirty = false;
        }
        osDelay(50);   /* 20Hz 巡检周期 */
    }
}

ErrorCode_t Display_Test(void)
{
    if (!initialized) return ERR_NOT_INIT;

    LOG_INFO("Running display test...");

    current_page = PAGE_SPLASH;
    Display_ForceUpdate();
    osDelay(500);

    Display_ShowMessage("Hello OLED!");
    Display_ForceUpdate();
    osDelay(1000);

    current_page = PAGE_OSCOPE;
    Display_ShowVoltage(3300);
    Display_ShowFrequency(1000);
    Display_ShowStatus("Test Mode");
    Display_ForceUpdate();
    osDelay(1000);

    Display_ShowMessage("Test Complete!");
    Display_ForceUpdate();

    LOG_INFO("Display test completed");
    return ERR_OK;
}

ErrorCode_t Display_SetTriggerLevel(uint32_t level_mv)
{
    if (!initialized) return ERR_NOT_INIT;

    trigger_level_mv = level_mv;
    dirty = true;

    return ERR_OK;
}

ErrorCode_t Display_SetTimebaseZoom(uint8_t zoom)
{
    if (!initialized) return ERR_NOT_INIT;
    if (zoom == 0) zoom = 1;
    if (zoom > 8) zoom = 8;

    timebase_zoom = zoom;
    dirty = true;

    LOG_INFO("Timebase zoom: %dx", zoom);
    return ERR_OK;
}

ErrorCode_t Display_GetMeasurements(Display_Measurements_t *meas)
{
    if (!initialized) return ERR_NOT_INIT;
    if (meas == NULL) return ERR_INVALID_PARAM;

    meas->frequency_hz = measured_freq;
    meas->period_us = measured_period_us;
    meas->voltage_mv = voltage_mv;
    meas->vpp_mv = measured_vpp;
    meas->vrms_mv = measured_vrms;
    meas->duty_permille = measured_duty;

    return ERR_OK;
}

ErrorCode_t Display_UpdateSigGen(uint32_t freq, uint32_t amp, uint8_t wave,
                                  uint16_t duty, bool running)
{
    if (!initialized) return ERR_NOT_INIT;

    siggen_freq = freq;
    siggen_amp = amp;
    siggen_wave = wave;
    siggen_duty = duty;
    siggen_running = running;

    /* 如果当前在信号发生器页面，标记需要刷新 */
    if (current_page == PAGE_SIGGEN) {
        dirty = true;
    }

    return ERR_OK;
}

ErrorCode_t Display_NextPage(void)
{
    if (!initialized) return ERR_NOT_INIT;

    current_page = (DisplayPage_t)((current_page + 1) % PAGE_COUNT);
    dirty = true;

    LOG_INFO("Page: %d", current_page);
    return ERR_OK;
}

ErrorCode_t Display_PrevPage(void)
{
    if (!initialized) return ERR_NOT_INIT;

    current_page = (DisplayPage_t)((current_page + PAGE_COUNT - 1) % PAGE_COUNT);
    dirty = true;

    LOG_INFO("Page: %d", current_page);
    return ERR_OK;
}

ErrorCode_t Display_MoveCursor(uint16_t x, uint16_t y)
{
    if (!initialized) return ERR_NOT_INIT;

    cursor_x = (x < OLED_WIDTH) ? x : OLED_WIDTH - 1;
    cursor_y = (y < OLED_HEIGHT) ? y : OLED_HEIGHT - 1;
    show_cursor = true;
    dirty = true;

    return ERR_OK;
}

ErrorCode_t Display_SetCursorEnabled(bool enabled)
{
    if (!initialized) return ERR_NOT_INIT;

    show_cursor = enabled;
    dirty = true;

    return ERR_OK;
}
