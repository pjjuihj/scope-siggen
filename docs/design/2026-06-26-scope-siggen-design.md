# SCOPE-SIGGEN 设计文档

**项目名称**: SCOPE-SIGGEN（示波器/信号发生器）  
**版本**: 1.0.0  
**日期**: 2026-06-26  
**作者**: AI Assistant  
**状态**: 设计完成

---

## 目录

1. [项目概述](#1-项目概述)
2. [系统架构](#2-系统架构)
3. [硬件设计](#3-硬件设计)
4. [软件架构](#4-软件架构)
5. [模块设计](#5-模块设计)
6. [接口设计](#6-接口设计)
7. [数据流设计](#7-数据流设计)
8. [错误处理](#8-错误处理)
9. [版本管理](#9-版本管理)
10. [调试支持](#10-调试支持)
11. [代码审查](#11-代码审查)
12. [测试策略](#12-测试策略)
13. [部署流程](#13-部署流程)
14. [附录](#14-附录)

---

## 1. 项目概述

### 1.1 项目简介

SCOPE-SIGGEN 是一个基于 STM32F407VETx 的双通道信号仪器项目，集成了示波器和信号发生器功能。

**核心功能**:
- **示波器**: ADC 采样、波形显示、电压/频率测量
- **信号发生器**: DAC 输出、多种波形生成（正弦、方波、三角波）
- **串口通信**: 命令控制、数据传输
- **显示界面**: OLED/LCD 波形显示

### 1.2 设计目标

| 目标 | 描述 |
|------|------|
| **可维护性** | 模块化设计，职责单一，便于维护和扩展 |
| **可靠性** | 完善的错误处理和恢复机制 |
| **可测试性** | 清晰的接口设计，便于单元测试和集成测试 |
| **可调试性** | 完整的调试支持和日志系统 |
| **安全性** | 线程安全、中断安全、内存安全 |

### 1.3 术语定义

| 术语 | 定义 |
|------|------|
| ADC | 模数转换器 (Analog-to-Digital Converter) |
| DAC | 数模转换器 (Digital-to-Analog Converter) |
| DMA | 直接内存访问 (Direct Memory Access) |
| RTOS | 实时操作系统 (Real-Time Operating System) |
| ISR | 中断服务程序 (Interrupt Service Routine) |
| HAL | 硬件抽象层 (Hardware Abstraction Layer) |

---

## 2. 系统架构

### 2.1 整体架构

采用分层架构设计，分为三层：

```
┌─────────────────────────────────────────────────────────────┐
│                        应用层                                │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │              App_Init() 统一入口                        │ │
│  └─────────────────────────────────────────────────────────┘ │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ oscilloscope.c │  │ signal_gen.c │  │ uart_protocol.c │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  display.c   │  │ key_handler.c │  │   app_init.c │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
├─────────────────────────────────────────────────────────────┤
│                        服务层                                │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   config.c   │  │ error_tracker.c │  │ ring_buffer.c │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   debug.c    │  │   version.c  │  │ code_reviewer.c │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
├─────────────────────────────────────────────────────────────┤
│                        驱动层                                │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │              CubeMX 生成代码（不修改）                  │ │
│  │    ADC1  DAC  TIM5  TIM8  USART1  I2C1  GPIO  DMA     │ │
│  └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 架构原则

| 原则 | 描述 |
|------|------|
| **分层隔离** | 各层职责明确，层间通过接口通信 |
| **模块独立** | 每个模块独立编译，减少耦合 |
| **CubeMX兼容** | 不修改CubeMX生成的代码 |
| **接口统一** | 统一的模块接口设计规范 |
| **错误追踪** | 所有错误都有记录和追踪 |

### 2.3 目录结构

```
scope-siggen/
├── Core/
│   ├── Inc/                    # 头文件
│   │   ├── main.h              # CubeMX生成
│   │   ├── app_init.h          # 应用初始化接口
│   │   ├── oscilloscope.h      # 示波器模块接口
│   │   ├── signal_gen.h        # 信号发生器模块接口
│   │   ├── uart_protocol.h     # 串口协议接口
│   │   ├── display.h           # 显示模块接口
│   │   ├── key_handler.h       # 按键处理接口
│   │   ├── config.h            # 配置管理接口
│   │   ├── error_tracker.h     # 错误追踪接口
│   │   ├── ring_buffer.h       # 环形缓冲接口
│   │   ├── debug.h             # 调试工具接口
│   │   ├── version.h           # 版本管理接口
│   │   └── code_reviewer.h     # 代码审查接口
│   │
│   └── Src/                    # 源文件
│       ├── main.c              # CubeMX生成
│       ├── freertos.c          # CubeMX生成
│       ├── stm32f4xx_it.c      # 中断处理
│       ├── app_init.c          # 应用初始化实现
│       ├── oscilloscope.c      # 示波器模块实现
│       ├── signal_gen.c        # 信号发生器模块实现
│       ├── uart_protocol.c     # 串口协议实现
│       ├── display.c           # 显示模块实现
│       ├── key_handler.c       # 按键处理实现
│       ├── config.c            # 配置管理实现
│       ├── error_tracker.c     # 错误追踪实现
│       ├── ring_buffer.c       # 环形缓冲实现
│       ├── debug.c             # 调试工具实现
│       ├── version.c           # 版本管理实现
│       └── code_reviewer.c     # 代码审查实现
│
├── Drivers/                    # HAL驱动（CubeMX管理）
├── Middlewares/                # 中间件（FreeRTOS等）
├── MDK-ARM/                    # Keil工程文件
├── docs/                       # 文档目录
│   └── design/                 # 设计文档
└── scripts/                    # 脚本工具
```

---

## 3. 硬件设计

### 3.1 微控制器

| 参数 | 值 |
|------|-----|
| **型号** | STM32F407VETx |
| **封装** | LQFP100 |
| **内核** | ARM Cortex-M4 |
| **主频** | 168 MHz |
| **Flash** | 512 KB |
| **SRAM** | 192 KB |

### 3.2 时钟配置

```
HSE (8 MHz)
    ↓
PLL (M=8, N=336, P=2, Q=4)
    ↓
SYSCLK = 168 MHz
    ↓
┌───────────────┬───────────────┐
│   AHB (÷1)    │   APB1 (÷4)   │
│   168 MHz     │   42 MHz      │
├───────────────┼───────────────┤
│   APB2 (÷2)   │               │
│   84 MHz      │               │
└───────────────┴───────────────┘
```

### 3.3 外设配置

| 外设 | 功能 | 配置 | 引脚 |
|------|------|------|------|
| **ADC1** | 示波器输入 | 12位，TIM8触发，DMA | PA6 (CH6) |
| **DAC** | 信号输出 | 12位，TIM5触发，DMA | PA4 |
| **TIM5** | DAC触发源 | 内部时钟，可变频率 | - |
| **TIM8** | ADC触发源 | 内部时钟，可变频率 | - |
| **USART1** | 串口通信 | 115200, 8N1 | PA9(TX), PA10(RX) |
| **I2C1** | 显示接口 | 400kHz | PB6(SCL), PB7(SDA) |
| **GPIO** | LED/按键 | 推挽输出/上拉输入 | PB2(LED), PA0(KEY) |
| **IWDG** | 看门狗 | 预分频256，超时~26s | - |

### 3.4 DMA配置

| DMA通道 | 用途 | 方向 | 模式 |
|---------|------|------|------|
| DMA2_Stream0 | ADC1 | 外设→内存 | 循环模式 |
| DMA1_Stream5 | DAC | 内存→外设 | 循环模式 |

### 3.5 NVIC配置

**优先级组**: NVIC_PRIORITYGROUP_4（4位抢占优先级，0位子优先级）

| 中断 | 优先级 | 子优先级 | 说明 |
|------|--------|---------|------|
| ADC_IRQn | 5 | 0 | ADC完成中断 |
| DMA2_Stream0_IRQn | 5 | 0 | ADC DMA |
| DMA1_Stream5_IRQn | 5 | 0 | DAC DMA |
| TIM5_IRQn | 5 | 0 | DAC触发 |
| TIM8_UP_TIM13_IRQn | 5 | 0 | ADC触发 |
| USART1_IRQn | 6 | 0 | UART接收 |
| I2C1_EV_IRQn | 5 | 0 | I2C事件 |
| I2C1_ER_IRQn | 5 | 0 | I2C错误 |
| EXTI0_IRQn | 6 | 0 | 按键中断 |
| SysTick_IRQn | 15 | 0 | 系统滴答 |

**注意事项**:
- FreeRTOS要求：configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5
- 优先级数值 ≤ 5 的中断不能调用FreeRTOS API
- 优先级数值 > 5 的中断可以调用FromISR API
- USART1和EXTI0优先级为6，可以调用FromISR API

---

## 4. 软件架构

### 4.1 FreeRTOS任务设计

每个功能模块对应一个独立任务：

| 任务名称 | 优先级 | 栈大小 | 功能 |
|---------|--------|--------|------|
| **Oscilloscope_Task** | 24 | 512B | ADC数据采集和处理 |
| **SignalGen_Task** | 24 | 512B | DAC波形输出控制 |
| **UART_Task** | 24 | 1024B | 串口命令接收和解析 |
| **Display_Task** | 24 | 1024B | 显示更新和UI |
| **Key_Task** | 24 | 512B | 按键扫描和处理 |

### 4.2 模块间通信

采用消息队列进行模块间通信：

```
┌──────────────┐     消息队列      ┌──────────────┐
│ Oscilloscope │ ─────────────────→ │   Display    │
│    Task      │                    │    Task      │
└──────────────┘                    └──────────────┘

┌──────────────┐     消息队列      ┌──────────────┐
│   UART       │ ─────────────────→ │  SignalGen   │
│    Task      │                    │    Task      │
└──────────────┘                    └──────────────┘
```

**消息队列定义**:

```c
osMessageQueueId_t osc_data_queue;    // 示波器数据队列
osMessageQueueId_t cmd_queue;         // 命令队列
osMessageQueueId_t display_queue;     // 显示更新队列
```

### 4.3 中断处理

采用任务通知机制处理中断：

```c
// ADC完成中断
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(oscilloscope_task_handle, 
                           &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// UART接收中断
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(uart_task_handle, 
                           &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

---

## 5. 模块设计

### 5.1 示波器模块 (oscilloscope.c)

**职责**:
- ADC数据采集
- 波形处理和分析
- 电压/频率测量

**接口**:

```c
// 初始化和控制
ErrorCode_t Oscilloscope_Init(void);
ErrorCode_t Oscilloscope_Start(void);
ErrorCode_t Oscilloscope_Stop(void);

// 状态查询
ErrorCode_t Oscilloscope_GetStatus(OscStatus_t *status);
bool Oscilloscope_IsRunning(void);

// 配置
ErrorCode_t Oscilloscope_SetConfig(const OscConfig_t *config);
ErrorCode_t Oscilloscope_GetConfig(OscConfig_t *config);

// 测量结果
ErrorCode_t Oscilloscope_GetVoltage(uint32_t *voltage_mv);
ErrorCode_t Oscilloscope_GetFrequency(uint32_t *frequency_hz);

// 自检
ErrorCode_t Oscilloscope_SelfTest(void);
```

**数据结构**:

```c
typedef struct {
    uint32_t sample_rate;      // 采样率 (Hz)
    uint32_t buffer_size;      // 缓冲区大小
    uint32_t trigger_level;    // 触发电平 (mV)
    uint8_t trigger_edge;      // 触发边沿 (0=下降沿, 1=上升沿)
    uint8_t enabled;           // 使能标志
} OscConfig_t;

typedef struct {
    uint32_t voltage_mv;       // 电压 (mV)
    uint32_t frequency_hz;     // 频率 (Hz)
    uint32_t sample_count;     // 采样计数
    uint8_t running;           // 运行状态
    ErrorCode_t last_error;    // 最后错误
} OscStatus_t;
```

**FreeRTOS任务**:

```c
void Oscilloscope_Task(void *argument) {
    // 初始化
    Oscilloscope_Init();
    
    for(;;) {
        // 等待ADC数据就绪（任务通知）
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        // 处理数据
        Osc_ProcessData();
        
        // 更新测量结果
        Osc_UpdateMeasurements();
        
        // 发送数据到显示模块
        Osc_SendToDisplay();
    }
}
```

### 5.2 信号发生器模块 (signal_gen.c)

**职责**:
- DAC波形输出
- 波形类型选择
- 频率/幅度控制

**接口**:

```c
// 初始化和控制
ErrorCode_t SignalGen_Init(void);
ErrorCode_t SignalGen_Start(void);
ErrorCode_t SignalGen_Stop(void);

// 状态查询
ErrorCode_t SignalGen_GetStatus(SigGenStatus_t *status);
bool SignalGen_IsRunning(void);

// 配置
ErrorCode_t SignalGen_SetConfig(const SigGenConfig_t *config);
ErrorCode_t SignalGen_GetConfig(SigGenConfig_t *config);

// 波形控制
ErrorCode_t SignalGen_SetWaveform(WaveformType_t type);
ErrorCode_t SignalGen_SetFrequency(uint32_t freq_hz);
ErrorCode_t SignalGen_SetAmplitude(uint32_t amplitude_mv);
ErrorCode_t SignalGen_SetDutyCycle(uint16_t duty_permille);

// 自检
ErrorCode_t SignalGen_SelfTest(void);
```

**数据结构**:

```c
typedef enum {
    WAVE_SINE,      // 正弦波
    WAVE_SQUARE,    // 方波
    WAVE_TRIANGLE,  // 三角波
    WAVE_SAWTOOTH,  // 锯齿波
    WAVE_DC         // 直流
} WaveformType_t;

typedef struct {
    uint32_t frequency;        // 频率 (Hz)
    uint32_t amplitude;        // 幅度 (mV)
    WaveformType_t waveform;   // 波形类型
    uint16_t duty_cycle;       // 占空比 (‰)
    uint8_t enabled;           // 使能标志
} SigGenConfig_t;

typedef struct {
    uint32_t current_frequency; // 当前频率
    uint32_t current_amplitude; // 当前幅度
    WaveformType_t current_waveform; // 当前波形
    uint8_t running;           // 运行状态
    ErrorCode_t last_error;    // 最后错误
} SigGenStatus_t;
```

### 5.3 串口协议模块 (uart_protocol.c)

**职责**:
- 命令接收和解析
- 数据发送
- 协议切换（文本/二进制）

**接口**:

```c
// 初始化和控制
ErrorCode_t UART_Protocol_Init(void);
ErrorCode_t UART_Protocol_Start(void);
ErrorCode_t UART_Protocol_Stop(void);

// 发送
ErrorCode_t UART_SendText(const char *text);
ErrorCode_t UART_SendData(const uint8_t *data, uint16_t len);
ErrorCode_t UART_SendResponse(const char *cmd, ErrorCode_t result);

// 接收
ErrorCode_t UART_ReceiveCommand(char *cmd, uint16_t max_len);

// 协议切换
ErrorCode_t UART_SetProtocolMode(ProtocolMode_t mode);
```

**协议格式**:

```
文本模式:
  命令: @COMMAND\r\n
  响应: OK\r\n 或 ERROR:message\r\n

二进制模式:
  帧头: 0xAA 0x55
  命令: 1字节
  长度: 2字节（小端）
  数据: N字节
  校验: 1字节（异或）
```

**命令列表**:

| 命令 | 功能 | 参数 |
|------|------|------|
| @START_OSC | 启动示波器 | - |
| @STOP_OSC | 停止示波器 | - |
| @SET_FREQ | 设置频率 | 频率值 |
| @SET_WAVE | 设置波形 | 波形类型 |
| @GET_STATUS | 查询状态 | - |
| @VERSION | 查询版本 | - |
| @HELP | 帮助信息 | - |

### 5.4 配置管理模块 (config.c)

**职责**:
- 配置参数管理
- Flash存储
- 默认配置

**接口**:

```c
// 初始化
ErrorCode_t Config_Init(void);

// 加载/保存
ErrorCode_t Config_Load(void);
ErrorCode_t Config_Save(void);
ErrorCode_t Config_LoadDefaults(void);

// 访问配置
AppConfig_t* Config_Get(void);

// 模块配置访问
ErrorCode_t Config_SetOscConfig(const OscConfig_t *config);
ErrorCode_t Config_GetOscConfig(OscConfig_t *config);
ErrorCode_t Config_SetSigGenConfig(const SigGenConfig_t *config);
ErrorCode_t Config_GetSigGenConfig(SigGenConfig_t *config);
```

**配置结构**:

```c
typedef struct {
    // 配置头
    struct {
        uint32_t version;
        uint32_t checksum;
        uint32_t length;
    } header;
    
    // 示波器配置
    OscConfig_t osc;
    
    // 信号发生器配置
    SigGenConfig_t siggen;
    
    // 系统配置
    struct {
        uint32_t uart_baudrate;
        uint8_t display_brightness;
        uint8_t watchdog_timeout;
        uint8_t log_level;
    } sys;
    
    // 关机配置
    struct {
        uint8_t auto_save;
        uint8_t confirm_shutdown;
        uint8_t safe_state;
    } shutdown;
} AppConfig_t;
```

### 5.5 错误追踪模块 (error_tracker.c)

**职责**:
- 错误记录
- 错误查询
- 调试信息输出

**接口**:

```c
// 初始化
ErrorCode_t ErrorTracker_Init(void);

// 记录错误
void ErrorTracker_Record(ErrorCode_t code, const char *file, 
                         uint32_t line, const char *message);

// 查询错误
ErrorCode_t ErrorTracker_GetLastError(ErrorRecord_t *record);
ErrorCode_t ErrorTracker_GetHistory(ErrorRecord_t *records, 
                                     uint8_t max_count, uint8_t *actual_count);
void ErrorTracker_PrintHistory(void);

// 清除错误
void ErrorTracker_Clear(void);

// 宏定义
#define RECORD_ERROR(code, msg) \
    ErrorTracker_Record(code, __FILE__, __LINE__, msg)
```

**错误记录结构**:

```c
typedef struct {
    ErrorCode_t code;          // 错误码
    const char *file;          // 文件名
    uint32_t line;             // 行号
    uint32_t timestamp;        // 时间戳
    char message[64];          // 错误消息
} ErrorRecord_t;

#define ERROR_MAX_RECORDS 16
```

### 5.6 环形缓冲模块 (ring_buffer.c)

**职责**:
- 数据缓冲管理
- 生产者-消费者模式支持

**接口**:

```c
// 初始化
ErrorCode_t RingBuffer_Init(RingBuffer_t *rb, uint16_t *buffer, uint16_t size);

// 数据操作
ErrorCode_t RingBuffer_Put(RingBuffer_t *rb, uint16_t data);
ErrorCode_t RingBuffer_Get(RingBuffer_t *rb, uint16_t *data);
ErrorCode_t RingBuffer_Peek(RingBuffer_t *rb, uint16_t *data);

// 状态查询
bool RingBuffer_IsEmpty(RingBuffer_t *rb);
bool RingBuffer_IsFull(RingBuffer_t *rb);
uint16_t RingBuffer_Count(RingBuffer_t *rb);
uint16_t RingBuffer_Free(RingBuffer_t *rb);

// 批量操作
uint16_t RingBuffer_PutBlock(RingBuffer_t *rb, const uint16_t *data, uint16_t len);
uint16_t RingBuffer_GetBlock(RingBuffer_t *rb, uint16_t *data, uint16_t len);
```

**数据结构**:

```c
typedef struct {
    uint16_t *buffer;          // 缓冲区指针
    uint16_t size;             // 缓冲区大小
    volatile uint16_t head;    // 写指针
    volatile uint16_t tail;    // 读指针
    volatile uint16_t count;   // 数据计数
} RingBuffer_t;
```

### 5.7 显示模块 (display.c)

**职责**:
- 波形绘制
- 状态信息显示
- 菜单交互

**接口**:

```c
// 初始化
ErrorCode_t Display_Init(void);

// 控制
ErrorCode_t Display_Clear(void);
ErrorCode_t Display_Update(void);
ErrorCode_t Display_SetBrightness(uint8_t brightness);

// 波形绘制
ErrorCode_t Display_DrawWaveform(uint16_t *data, uint16_t len);
ErrorCode_t Display_DrawGrid(void);
ErrorCode_t Display_DrawCursor(uint16_t x, uint16_t y);

// 信息显示
ErrorCode_t Display_ShowStatus(const char *status);
ErrorCode_t Display_ShowVoltage(uint32_t voltage_mv);
ErrorCode_t Display_ShowFrequency(uint32_t frequency_hz);
ErrorCode_t Display_ShowMessage(const char *message);

// 菜单
ErrorCode_t Display_ShowMenu(const MenuItem_t *menu, uint8_t count);
ErrorCode_t Display_UpdateSelection(uint8_t selected);
```

### 5.8 按键处理模块 (key_handler.c)

**职责**:
- 按键扫描
- 按键去抖
- 按键事件生成

**接口**:

```c
// 初始化
ErrorCode_t KeyHandler_Init(void);

// 按键扫描
KeyCode_t Key_Scan(void);
bool Key_IsPressed(KeyCode_t key);

// 按键事件
KeyCode_t Key_WaitForKey(uint32_t timeout_ms);
void Key_RegisterCallback(KeyCallback_t callback);
```

**按键定义**:

```c
typedef enum {
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_ENTER,
    KEY_BACK,
    KEY_MENU
} KeyCode_t;
```

---

## 6. 接口设计

### 6.1 模块接口规范

每个模块必须实现以下基础接口：

```c
// 基础接口（必须实现）
ErrorCode_t Module_Init(void);
ErrorCode_t Module_Start(void);
ErrorCode_t Module_Stop(void);
ErrorCode_t Module_GetStatus(ModuleStatus_t *status);
ErrorCode_t Module_SelfTest(void);

// 扩展接口（可选实现）
ErrorCode_t Module_SetConfig(const ModuleConfig_t *config);
ErrorCode_t Module_GetConfig(ModuleConfig_t *config);
ErrorCode_t Module_Calibrate(void);
```

### 6.2 错误码定义

```c
typedef enum {
    ERR_OK = 0,                // 成功
    ERR_TIMEOUT,               // 超时
    ERR_INVALID_PARAM,         // 无效参数
    ERR_BUFFER_FULL,           // 缓冲区满
    ERR_BUFFER_EMPTY,          // 缓冲区空
    ERR_HARDWARE,              // 硬件错误
    ERR_MEMORY,                // 内存错误
    ERR_BUSY,                  // 忙
    ERR_NOT_INIT,              // 未初始化
    ERR_NOT_SUPPORTED,         // 不支持
    ERR_FILE_OPEN,             // 文件打开失败
    ERR_FILE_READ,             // 文件读取失败
    ERR_FILE_WRITE,            // 文件写入失败
    ERR_CANCELLED,             // 已取消
    ERR_UNKNOWN                // 未知错误
} ErrorCode_t;
```

### 6.3 版本信息接口

```c
typedef struct {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint16_t build;
    
    const char *version_string;
    const char *build_date;
    const char *build_time;
    
    const char *git_commit;
    const char *git_branch;
    uint8_t git_dirty;
} VersionInfo_t;

const VersionInfo_t* Version_GetInfo(void);
void Version_Print(void);
```

---

## 7. 数据流设计

### 7.1 示波器数据流

```
ADC DMA (循环模式)
    ↓
HAL_ADC_ConvCpltCallback() [中断]
    ↓
vTaskNotifyGiveFromISR() [任务通知]
    ↓
Oscilloscope_Task [任务]
    ↓
Ring Buffer [环形缓冲]
    ↓
Osc_ProcessData() [数据处理]
    ↓
osMessageQueuePut() [消息队列]
    ↓
Display_Task [显示更新]
```

### 7.2 信号发生器数据流

```
UART_Task [命令接收]
    ↓
UART_ParseCommand() [命令解析]
    ↓
osMessageQueuePut() [消息队列]
    ↓
SignalGen_Task [任务]
    ↓
SigGen_SetWaveform() [波形设置]
    ↓
HAL_DAC_Start_DMA() [DAC输出]
```

### 7.3 配置数据流

```
Config_Load() [Flash读取]
    ↓
AppConfig_t [配置结构体]
    ↓
各模块初始化
    ↓
Config_Save() [Flash写入]
```

---

## 8. 错误处理

### 8.1 错误分级

| 级别 | 描述 | 处理方式 |
|------|------|---------|
| **致命错误** | 系统无法继续运行 | 停止系统，显示错误信息 |
| **严重错误** | 模块无法正常工作 | 停止模块，记录错误 |
| **警告** | 功能受限但可继续 | 记录错误，继续运行 |
| **信息** | 状态信息 | 仅记录 |

### 8.2 错误恢复机制

**启动错误恢复**:
```c
ErrorCode_t App_Init(void) {
    // 自检（带重试）
    for (int retry = 0; retry < MAX_RETRY; retry++) {
        err = SelfTest_Run();
        if (err == ERR_OK) break;
        HAL_Delay(RETRY_DELAY);
    }
    
    // 进入降级模式
    if (err != ERR_OK) {
        App_EnterFallbackMode();
    }
}
```

**运行时错误恢复**:
```c
ErrorCode_t Osc_ProcessData(void) {
    err = HAL_ADC_Start_DMA(&hadc1, buffer, size);
    if (err != HAL_OK) {
        RECORD_ERROR(ERR_HARDWARE, "ADC start failed");
        
        // 重试
        HAL_Delay(10);
        err = HAL_ADC_Start_DMA(&hadc1, buffer, size);
        if (err != HAL_OK) {
            return ERR_HARDWARE;
        }
    }
    return ERR_OK;
}
```

### 8.3 HardFault处理

```c
void HardFault_Handler(void) {
    char buf[64];
    snprintf(buf, sizeof(buf), "HF CFSR:0x%lX HFSR:0x%lX\r\n", 
             SCB->CFSR, SCB->HFSR);
    HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), 10);
    
    if (SCB->CFSR & 0x00008000) {  // BFARVALID
        snprintf(buf, sizeof(buf), "BFAR:0x%lX\r\n", SCB->BFAR);
        HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), 10);
    }
    
    while (1);
}
```

---

## 9. 版本管理

### 9.1 版本号格式

```
主版本.次版本.补丁.构建号
例如: 1.0.0.42
```

**版本类型**:
- **Release**: 正式版 (1.0.0)
- **RC**: Release Candidate (1.0.0-rc1)
- **Beta**: Beta版 (1.0.0-beta2)
- **Alpha**: Alpha版 (1.0.0-alpha3)
- **Dev**: 开发版 (1.0.0-dev.42)

### 9.2 版本信息结构

```c
typedef struct {
    // 版本号
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint16_t build;
    
    // 版本类型
    VersionType_t type;
    
    // Git信息
    const char *git_commit;
    const char *git_branch;
    uint8_t git_dirty;
    
    // 编译信息
    const char *build_date;
    const char *build_time;
    const char *compiler;
    const char *target;
} VersionInfo_t;
```

### 9.3 版本历史

- 记录最近50个版本
- 包含启动次数、复位原因
- 支持版本对比

### 9.4 Git集成

**构建脚本** (build_version.py):
- 自动获取Git信息
- 更新version.h文件
- 支持自动提交

**预提交钩子**:
- 代码质量检查
- 版本号更新

---

## 10. 调试支持

### 10.1 日志系统

**日志级别**:
- LOG_DEBUG: 调试信息
- LOG_INFO: 一般信息
- LOG_WARNING: 警告
- LOG_ERROR: 错误
- LOG_FATAL: 致命错误

**日志格式**:
```
[时间戳] [级别] [文件:行号] 消息
[1234.567] [INF] [app_init.c:45] System initialized
```

**日志宏**:
```c
#ifdef DEBUG_EN
    #define LOG_DEBUG(fmt, ...)   Debug_Log(LOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...)    Debug_Log(LOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_WARNING(fmt, ...) Debug_Log(LOG_WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...)   Debug_Log(LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_FATAL(fmt, ...)   Debug_Log(LOG_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
    #define LOG_DEBUG(fmt, ...)   ((void)0)
    #define LOG_INFO(fmt, ...)    ((void)0)
    // ...
#endif
```

### 10.2 调试命令

| 命令 | 功能 |
|------|------|
| help | 显示帮助 |
| status | 系统状态 |
| tasks | 任务列表 |
| memory | 内存信息 |
| version | 版本信息 |
| history | 版本历史 |
| errors | 错误历史 |
| review | 代码审查 |
| log <level> | 设置日志级别 |
| reset | 系统复位 |

### 10.3 性能监测

**监测指标**:
- CPU使用率
- 任务状态
- 堆栈使用
- 内存使用

**实现**:
```c
uint32_t Debug_GetCPUUsage(void) {
    TaskStatus_t idle_status;
    vTaskGetInfo(xTaskGetIdleTaskHandle(), &idle_status, pdTRUE, eInvalid);
    // 计算CPU使用率
    return 100 - (idle_time * 100 / total_time);
}
```

---

## 11. 代码审查

### 11.1 检查规则

**命名规范**:
- 函数命名: 模块前缀_功能 (Oscilloscope_Init)
- 变量命名: 驼峰命名法 (sampleRate)
- 常量命名: 全大写 (MAX_BUFFER_SIZE)
- 类型命名: 驼峰+_t后缀 (OscConfig_t)

**错误处理**:
- 所有函数必须检查返回值
- 指针参数必须检查NULL
- 数组访问必须检查边界

**中断安全**:
- 中断中不能调用阻塞函数
- 中断处理时间不能过长

**线程安全**:
- 共享资源必须使用互斥锁
- 必须防止死锁

**内存安全**:
- 动态内存分配必须检查返回值
- 必须防止内存泄漏
- 必须防止栈溢出

### 11.2 评分标准

**分类权重**:
- 语法检查: 5%
- 代码风格: 5%
- 功能实现: 25%
- 完整性检查: 10%
- 性能检查: 10%
- 安全检查: 15%
- 内存检查: 15%
- 中断安全: 10%
- 线程安全: 10%
- 设计符合性: 10%

**扣分规则**:
- 警告: -2分 × 严重程度
- 失败: -10分 × 严重程度
- 错误: -20分 × 严重程度

### 11.3 报告格式

支持多种格式:
- 文本格式
- JSON格式
- HTML格式（带图表）
- CSV格式
- Markdown格式

---

## 12. 测试策略

### 12.1 单元测试

**测试框架**: 自定义轻量级框架

**测试覆盖**:
- 每个模块的核心函数
- 边界条件测试
- 错误处理测试

**示例**:
```c
void Test_RingBuffer(void) {
    RingBuffer_t rb;
    uint16_t buffer[16];
    uint16_t data;
    
    RingBuffer_Init(&rb, buffer, 16);
    
    // 测试空缓冲区
    assert(RingBuffer_IsEmpty(&rb));
    assert(RingBuffer_Get(&rb, &data) == ERR_BUFFER_EMPTY);
    
    // 测试写入
    assert(RingBuffer_Put(&rb, 42) == ERR_OK);
    assert(RingBuffer_Count(&rb) == 1);
    
    // 测试读取
    assert(RingBuffer_Get(&rb, &data) == ERR_OK);
    assert(data == 42);
}
```

### 12.2 集成测试

**测试内容**:
- 模块间通信测试
- 数据流测试
- 错误传播测试

### 12.3 系统测试

**测试内容**:
- 完整功能测试
- 性能测试
- 稳定性测试

---

## 13. 部署流程

### 13.1 编译流程

```bash
# 使用Keil编译
UV4.exe -b scope-siggen.uvprojx -t Debug -o build.log -j0

# 或使用工作流脚本
python workflow.py --auto . --steps compile
```

### 13.2 烧录流程

```bash
# ST-LINK烧录
UV4.exe -f scope-siggen.uvprojx -t Debug -o flash.log

# 或使用工作流脚本
python workflow.py --auto . --steps flash --port COM3
```

### 13.3 验证流程

```bash
# 串口验证
python serial_debug.py --auto . --port COM3 --proto printf --listen 30

# 发送测试命令
python serial_debug.py --auto . --port COM3 --proto text --send "@STATUS"
```

---

## 14. 附录

### 14.1 参考文档

- STM32F407参考手册 (RM0090)
- STM32F407数据手册
- FreeRTOS参考手册
- CubeMX用户手册

### 14.2 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-06-26 | 初始设计文档 |

### 14.3 术语表

| 术语 | 定义 |
|------|------|
| ADC | 模数转换器 |
| DAC | 数模转换器 |
| DMA | 直接内存访问 |
| GPIO | 通用输入输出 |
| HAL | 硬件抽象层 |
| ISR | 中断服务程序 |
| NVIC | 嵌套向量中断控制器 |
| RCC | 复位和时钟控制 |
| RTOS | 实时操作系统 |
| UART | 通用异步收发传输器 |

---

**文档结束**
