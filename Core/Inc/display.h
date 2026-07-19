/**
  ******************************************************************************
  * @file           : display.h
  * @brief          : 显示模块接口
  ******************************************************************************
  * @attention
  *
  * 显示模块，用于OLED/LCD显示
  *
  ******************************************************************************
  */

#ifndef __DISPLAY_H
#define __DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "error_tracker.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief 显示坐标结构体
  */
typedef struct {
    uint16_t x;
    uint16_t y;
} Display_Point_t;

/**
  * @brief 菜单项结构体
  */
typedef struct {
    const char *text;
    void (*callback)(void);
} MenuItem_t;

/**
  * @brief 显示页面类型
  */
typedef enum {
    PAGE_SPLASH,        /* 启动画面 */
    PAGE_OSCOPE,        /* 示波器页面 */
    PAGE_SIGGEN,        /* 信号发生器页面 */
    PAGE_SYSINFO,       /* 系统信息页面 */
    PAGE_MENU,          /* 菜单页面 */
    PAGE_MESSAGE,       /* 消息页面 */
    PAGE_OTA,           /* OTA 更新页面 */
    PAGE_COUNT          /* 页面总数 */
} DisplayPage_t;

/* Exported constants --------------------------------------------------------*/

/* 显示尺寸 */
#define DISPLAY_WIDTH       128
#define DISPLAY_HEIGHT      64

/* 字体大小 */
#define FONT_WIDTH          6
#define FONT_HEIGHT         8

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  初始化显示模块
  * @retval 错误码
  */
ErrorCode_t Display_Init(void);

/**
  * @brief  清屏
  * @retval 错误码
  */
ErrorCode_t Display_Clear(void);

/**
  * @brief  更新显示（带1秒节流）
  * @retval 错误码
  */
ErrorCode_t Display_Update(void);

/**
  * @brief  强制立即刷新显示（不受节流限制）
  * @retval 错误码
  */
ErrorCode_t Display_ForceUpdate(void);

/**
  * @brief  通过串口发送OLED显示数据
  * @retval 错误码
  */
ErrorCode_t Display_SendToSerial(void);

/**
  * @brief  设置亮度
  * @param  brightness: 亮度值 (0-255)
  * @retval 错误码
  */
ErrorCode_t Display_SetBrightness(uint8_t brightness);

/**
  * @brief  绘制波形
  * @param  data: 数据缓冲区
  * @param  len: 数据长度
  * @retval 错误码
  */
ErrorCode_t Display_DrawWaveform(uint16_t *data, uint16_t len);

/**
  * @brief  绘制网格
  * @retval 错误码
  */
ErrorCode_t Display_DrawGrid(void);

/**
  * @brief  绘制光标
  * @param  x: X坐标
  * @param  y: Y坐标
  * @retval 错误码
  */
ErrorCode_t Display_DrawCursor(uint16_t x, uint16_t y);

/**
  * @brief  显示状态信息
  * @param  status: 状态字符串
  * @retval 错误码
  */
ErrorCode_t Display_ShowStatus(const char *status);

/**
  * @brief  显示电压值
  * @param  voltage_mv: 电压值 (mV)
  * @retval 错误码
  */
ErrorCode_t Display_ShowVoltage(uint32_t voltage_mv);

/**
  * @brief  显示频率值
  * @param  frequency_hz: 频率值 (Hz)
  * @retval 错误码
  */
ErrorCode_t Display_ShowFrequency(uint32_t frequency_hz);

/**
  * @brief  显示消息
  * @param  message: 消息字符串
  * @retval 错误码
  */
ErrorCode_t Display_ShowMessage(const char *message);

/**
  * @brief  显示菜单
  * @param  menu: 菜单项数组
  * @param  count: 菜单项数量
  * @retval 错误码
  */
ErrorCode_t Display_ShowMenu(const MenuItem_t *menu, uint8_t count);

/**
  * @brief  更新菜单选择
  * @param  selected: 选中的菜单项
  * @retval 错误码
  */
ErrorCode_t Display_UpdateSelection(uint8_t selected);

/**
  * @brief  获取当前页面
  * @retval 当前页面类型
  */
DisplayPage_t Display_GetCurrentPage(void);

/**
  * @brief  获取当前菜单选中项
  * @retval 当前选中的菜单项索引
  */
uint8_t Display_GetMenuSelect(void);

/**
  * @brief  选择当前菜单项
  * @retval 错误码
  */
ErrorCode_t Display_SelectMenuItem(void);

/**
  * @brief  记录操作到日志
  * @param  op: 操作描述字符串
  * @retval None
  */
void Display_LogOperation(const char *op);

/**
  * @brief  显示任务
  * @param  argument: 任务参数
  * @retval None
  */
void Display_Task(void *argument);

/**
  * @brief  显示测试
  * @retval 错误码
  */
ErrorCode_t Display_Test(void);

/**
  * @brief  设置触发电平（用于显示指示线）
  * @param  level_mv: 触发电平 (mV)
  * @retval 错误码
  */
ErrorCode_t Display_SetTriggerLevel(uint32_t level_mv);

/**
  * @brief  设置时间轴缩放
  * @param  zoom: 1=自动, 2=2倍放大, 4=4倍放大
  * @retval 错误码
  */
ErrorCode_t Display_SetTimebaseZoom(uint8_t zoom);

/**
  * @brief  测量结果结构体
  */
typedef struct {
    uint32_t frequency_hz;      /* 频率 (Hz) */
    uint32_t period_us;         /* 周期 (µs) */
    uint32_t voltage_mv;        /* 平均电压 (mV) */
    uint32_t vpp_mv;            /* 峰峰值 (mV) */
    uint32_t vrms_mv;           /* 有效值 (mV) */
    uint16_t duty_permille;     /* 占空比 (‰) */
} Display_Measurements_t;

/**
  * @brief  获取测量结果
  * @param  meas: 测量结果结构体指针
  * @retval 错误码
  */
ErrorCode_t Display_GetMeasurements(Display_Measurements_t *meas);

/**
  * @brief  更新信号发生器状态
  * @param  freq: 频率 (Hz)
  * @param  amp: 幅度 (mV)
  * @param  wave: 波形类型 (0-4)
  * @param  duty: 占空比 (‰)
  * @param  running: 运行状态
  * @retval 错误码
  */
ErrorCode_t Display_UpdateSigGen(uint32_t freq, uint32_t amp, uint8_t wave,
                                  uint16_t duty, bool running);

/**
  * @brief  切换到下一页
  * @retval 错误码
  */
ErrorCode_t Display_NextPage(void);

/**
  * @brief  切换到上一页
  * @retval 错误码
  */
ErrorCode_t Display_PrevPage(void);

/**
  * @brief  移动光标
  * @param  x: X 坐标 (0-127)
  * @param  y: Y 坐标 (0-63)
  * @retval 错误码
  */
ErrorCode_t Display_MoveCursor(uint16_t x, uint16_t y);

/**
  * @brief  启用/禁用光标
  * @param  enabled: true=显示, false=隐藏
  * @retval 错误码
  */
ErrorCode_t Display_SetCursorEnabled(bool enabled);

/**
  * @brief  显示 OTA 更新页面
  * @retval 错误码
  */
ErrorCode_t Display_ShowOTA(void);

/**
  * @brief  更新 OTA 进度显示
  * @param  progress: 进度 (0-100)
  * @param  status: 状态字符串
  * @retval 错误码
  */
ErrorCode_t Display_UpdateOTAProgress(uint8_t progress, const char *status);

/**
  * @brief  显示 OTA 错误信息
  * @param  error: 错误信息
  * @retval 错误码
  */
ErrorCode_t Display_ShowOTAError(const char *error);

/**
  * @brief  显示 OTA 完成信息
  * @retval 错误码
  */
ErrorCode_t Display_ShowOTAComplete(void);

#ifdef __cplusplus
}
#endif

#endif /* __DISPLAY_H */
