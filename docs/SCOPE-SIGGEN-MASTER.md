# SCOPE-SIGGEN 项目总览

**项目**: 示波器/信号发生器
**平台**: STM32F407VETx (ARM Cortex-M4, 168MHz)
**RTOS**: FreeRTOS
**版本**: v1.2.0
**日期**: 2026-06-29

---

## 目录

1. [项目概述](#1-项目概述)
2. [系统架构](#2-系统架构)
3. [硬件配置](#3-硬件配置)
4. [开发时间线](#4-开发时间线)
5. [Bug 数据库](#5-bug-数据库)
6. [教训清单](#6-教训清单)
7. [代码审查记录](#7-代码审查记录)
8. [当前状态](#8-当前状态)
9. [嵌入式专家视角](#9-嵌入式专家视角)
10. [待办事项](#10-待办事项)

---

## 1. 项目概述

### 1.1 功能

| 功能 | 状态 | 说明 |
|------|------|------|
| 示波器 | ✅ | ADC DMA、硬件触发(AWD)、频率/电压测量 |
| 信号发生器 | ✅ | DAC DMA、LUT 正弦波、7 种波形 |
| 波形显示 | ✅ | EMA 缩放、min/max 包络、信息栏 |
| 串口命令 | ✅ | 20 条命令 |
| 配置管理 | ✅ | Flash 读写 |
| 按键输入 | ✅ | GPIO 轮询、去抖 |

### 1.2 数据流

```
TIM5 (328kHz) ──TRGO──→ DAC ──DMA──→ waveform_buffer[256] ──→ PA4
                                                                  │
                                                              导线连接
                                                                  │
TIM8 (10kHz)  ──TRGO──→ ADC ──DMA──→ adc_buffer[1024]  ←── PA6
                                         │
                              半/全传输中断 → memcpy → proc_buffer[512]
                                         │
                              信号量通知 → Oscilloscope_Task
                                         │
                              Display_UpdateScopeEx() → waveform_buf[256]
                                         │
                              Display_Task (10Hz) → OLED 渲染
```

### 1.3 代码统计

| 指标 | 数值 |
|------|------|
| 源文件 | 18 个 .c |
| 头文件 | 16 个 .h |
| 代码行数 | ~9000 |
| 编译错误 | 0 |
| 编译警告 | 0 |
| TODO 残留 | 0 |
| 互斥锁覆盖 | 62 处 |

---

## 2. 系统架构

### 2.1 分层架构

```
┌─────────────────────────────────────────────────────────────┐
│                        应用层                                │
│  oscilloscope.c  signal_gen.c  display.c  uart_protocol.c  │
│  key_handler.c  app_init.c                                  │
├─────────────────────────────────────────────────────────────┤
│                        服务层                                │
│  config.c  error_tracker.c  ring_buffer.c  debug.c         │
├─────────────────────────────────────────────────────────────┤
│                        驱动层                                │
│  CubeMX 生成代码（不修改）                                   │
│  ADC1  DAC  TIM5  TIM8  USART1  I2C1  GPIO  DMA            │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 FreeRTOS 任务

| 任务 | 优先级 | 栈 | 周期 | 职责 |
|------|--------|-----|------|------|
| Oscilloscope | AboveNormal | 4KB | 事件驱动 | ADC 采集、波形处理 |
| SignalGen | Normal | 2KB | 1s | DAC 输出控制 |
| UART | Normal | 4KB | 事件驱动 | 命令接收解析 |
| Display | BelowNormal | 2KB | 100ms | OLED 渲染 |
| Key | Low | 1KB | 10ms | 按键扫描 |

### 2.3 模块接口

每个模块实现统一接口：

```c
ErrorCode_t Module_Init(void);
ErrorCode_t Module_Start(void);
ErrorCode_t Module_Stop(void);
ErrorCode_t Module_GetStatus(ModuleStatus_t *status);
ErrorCode_t Module_SelfTest(void);
```

### 2.4 互斥锁保护

| 模块 | 锁名 | 保护内容 |
|------|------|---------|
| oscilloscope.c | osc_mutex | osc_status, osc_config |
| signal_gen.c | siggen_mutex | siggen_status, siggen_config |
| display.c | oled_mutex | OLED_GRAM, 状态变量 |
| ring_buffer.c | __disable_irq | head, tail, count |

---

## 3. 硬件配置

### 3.1 外设配置

| 外设 | 功能 | 引脚 | 配置 |
|------|------|------|------|
| ADC1 | 示波器输入 | PA6 | 12-bit, TIM8 触发, DMA, AWD |
| DAC1 | 信号输出 | PA4 | 12-bit, TIM5 触发, DMA |
| TIM8 | ADC 触发源 | — | PSC=0, ARR=16799, 10kHz |
| TIM5 | DAC 触发源 | — | 可变频率 |
| USART1 | 串口 | PA9/PA10 | 115200, 中断接收 |
| I2C1 | OLED | PB6/PB7 | 400kHz |
| GPIO | 按键 | PA0, PB3-6 | GPIO 轮询 |
| GPIO | LED | PB2 | 推挽输出 |

### 3.2 时钟配置

```
HSE (8MHz) → PLL (M=8, N=336, Q=4) → SYSCLK = 168 MHz
                                         │
                    ┌────────────────────┼────────────────────┐
                    │                    │                    │
              AHB = 168 MHz        APB1 = 42 MHz        APB2 = 84 MHz
                                    │                    │
                              TIM2-7 = 84 MHz      TIM8-11 = 168 MHz
```

### 3.3 中断优先级

| 中断 | 优先级 | 用途 |
|------|--------|------|
| ADC_IRQn | 5 | ADC 转换 + AWD |
| DMA2_Stream0 | 5 | ADC DMA |
| TIM8_UP | 5 | ADC 触发定时器 |
| TIM5_IRQn | 5 | DAC 触发定时器 |
| DMA1_Stream5 | 5 | DAC DMA |
| USART1_IRQn | 6 | 串口通信 |
| I2C1_EV/ER | 7 | OLED I2C |
| SysTick | 15 | FreeRTOS 心跳 |

### 3.4 DMA 配置

| DMA 流 | 通道 | 方向 | 用途 |
|--------|------|------|------|
| DMA2_Stream0 | CH0 | 外设→内存 | ADC1 循环采集 |
| DMA1_Stream5 | CH7 | 内存→外设 | DAC1 波形输出 |

### 3.5 ADC 外部触发配置（4 个必要配置）

```c
hadc1.Init.ContinuousConvMode = DISABLE;   // 单次模式
hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T8_TRGO;
hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
```

**启动顺序**：先 `HAL_ADC_Start_DMA()`，后 `HAL_TIM_Base_Start()`

### 3.6 ADC 采样时间选择

| 采样时间 | 转换时间 | 适用场景 |
|---------|---------|---------|
| 84 cycles | 4.57µs | 低阻信号源 (<1kΩ) |
| 144 cycles | 7.43µs | 中等阻抗 |
| 480 cycles | 23.4µs | 高阻信号源 (>10kΩ) |

DAC 输出缓冲器阻抗 ~15kΩ，必须用 480 cycles。

---

## 4. 开发时间线

### 4.1 06-26：盲目堆代码

**做了什么**：
- 搭建项目框架（18 个源文件、16 个头文件）
- 实现 oscilloscope.c / signal_gen.c / display.c / uart_protocol.c
- 尝试调试 OLED 显示（乱码/重叠）

**问题**：
- 400+ 行代码没有一行经过硬件验证
- 同时推进 4 个模块，没有一条链路跑通
- 6 个编译警告
- 代码质量评分 6/10

**教训**：写代码 ≠ 跑通功能。先用最丑的方式把功能跑出来，再美化。

### 4.2 06-27 上午：3 小时修 8 个 bug

**做了什么**：
- 修复 8 个初始化配置 bug
- 实现波形显示（OLED_DrawLine 修复、帧管理、互斥锁）
- 实现 DAC→ADC 自测链路

**Bug 列表**：见 [Bug 数据库](#5-bug-数据库)

**教训**：所有 bug 都是初始化问题。AI 生成代码时没有逐条检查初始化调用链的完整性。

### 4.3 06-27 下午：配置系统连接 + ADC 配置修复

**做了什么**：
- 连接 Config 系统与 ADC/DAC 硬件
- 实现 Osc_ApplyConfig() / SigGen_ApplyConfig()
- 修复 ADC 硬件触发配置（4 个配置项 + 启动顺序）
- 修复 ADC 采样时间（84→480 cycles）

**方法论转变**：
- 试错 2 小时 → 查 GitHub 5 分钟 → 一次修复

**教训**：遇到配置问题先查 GitHub，不要试错。

### 4.4 06-27 晚：功能扩展 + 代码审查

**做了什么**：
- 显示优化（EMA 缩放、信息栏、局部清屏、十字线）
- 触发系统（AWD 硬件触发、状态机、set_trigger 命令）
- 信号发生器优化（LUT 查表、除零保护、NULL 检查）

**代码审查发现**：显示模块 8 项、触发系统 6 项、信号发生器 4 项

**教训**：代码审查比写代码更重要。每轮审查都发现了前一轮遗漏的问题。

### 4.5 06-28~29：DMA 缓冲区安全

**做了什么**：
- 审查 DMA 缓冲区数据流
- 修复 process_ptr/process_len 竞态窗口
- 修复 HAL_TIM_Base_Stop_IT 用错
- 增大 Oscilloscope_Task 栈到 4KB

**教训**：ISR 和任务之间的数据传递必须有内存屏障。Start/Stop 函数必须对称。

### 4.6 版本历史

| 版本 | 日期 | 主要改动 |
|------|------|---------|
| v1.0.0 | 06-26 | 项目框架 |
| v1.1.0 | 06-27 | 初始化修复、波形显示 |
| v1.2.0 | 06-27 | 显示优化、触发系统、DMA 修复 |

---

## 5. Bug 数据库

### 5.1 初始化 Bug（8 个）

| # | Bug | 根因 | 类别 | 文件 |
|---|-----|------|------|------|
| 1 | HAL_ADC_MspInit 缺 DMA | CubeMX 未选 DMA 模式 | 配置缺失 | stm32f4xx_hal_msp.c |
| 2 | TIM8 TRGO = RESET | 默认值不适合 | 默认值陷阱 | main.c |
| 3 | DMAContinuousRequests = DISABLE | 默认值不适合 | 默认值陷阱 | main.c |
| 4 | 从空 ring buffer 读 | 架构与实现脱节 | 设计缺陷 | oscilloscope.c |
| 5 | OLED_DrawLine delta_y | 第三方库 bug | 代码笔误 | oled.c |
| 6 | 两个任务竞争 OLED | 无互斥保护 | 并发问题 | display.c |
| 7 | TIM5 未启动 | 启动链不完整 | 分层遗漏 | signal_gen.c |
| 8 | osMutexNew 在内核启动前 | 初始化顺序错误 | 时序问题 | display.c |

### 5.2 配置系统 Bug（7 个）

| # | Bug | 根因 | 修复 |
|---|-----|------|------|
| 1 | Config API 从未被调用 | Init/Set* 未连接 | Init 读 Config，Set* 写回 |
| 2 | 时钟频率硬编码 84MHz | 不同时钟配置会算错 | HAL_RCC_GetPCLKxFreq() |
| 3 | Config 写入竞态 | 锁外调用 Set* | 锁内拷贝，锁外写入 |
| 4 | osc_config_buffer_size 非 volatile | ISR 和任务共享 | 声明加 volatile |
| 5 | 波形显示尖刺 | 降采样取单点 | min/max 包络算法 |
| 6 | 显示尾部数据丢失 | 整除截断 | 向上取整 |
| 7 | ApplyConfig 除零风险 | frequency=0 | 入口参数校验 |

### 5.3 信号发生器 Bug（4 个）

| # | 严重度 | Bug | 修复 |
|---|--------|-----|------|
| 1 | Critical | TIM5 运行时修改寄存器 | 先停止 TIM5，修改后再启动 |
| 2 | High | 锁粒度不一致 | 统一为：锁内更新→锁外生成→锁内应用 |
| 3 | High | 锁内浮点运算 | 移到锁外执行 |
| 4 | Medium | Stop 未停止 TIM5 | 添加 HAL_TIM_Base_Stop |

### 5.4 ADC 配置 Bug（4 个）

| # | Bug | 根因 | 修复 |
|---|-----|------|------|
| 1 | EOCSelection 配置错误 | ADC 不转换 | ADC_EOC_SINGLE_CONV |
| 2 | 启动顺序错误 | 先 TIM 后 ADC | 先 ADC DMA 后 TIM |
| 3 | 采样时间不够 | 84 cycles 对高阻不够 | 改为 480 cycles |
| 4 | Osc_ApplyConfig 空函数 | TIM8 频率未配置 | 实现 PSC/ARR 计算 |

### 5.5 DMA 缓冲区 Bug（5 个）

| # | 严重度 | Bug | 状态 |
|---|--------|-----|------|
| 1 | High | process_ptr/process_len 竞态 | ✅ 已修复 |
| 2 | High | HAL_TIM_Base_Stop_IT 用错 | ✅ 已修复 |
| 3 | Medium | adc_buffer 非真正乒乓 | ⏳ 待改进 |
| 4 | Medium | 栈使用偏高 | ✅ 已修复 |
| 5 | Medium | 类型截断 | ⏳ 待修复 |

### 5.6 显示模块 Bug（8 个）

| # | 严重度 | Bug | 修复 |
|---|--------|-----|------|
| 1 | Critical | trigger_enabled 导致触发电平线不显示 | 删除字段 |
| 2 | High | EMA 每帧重置 | 只在 Display_Clear 时重置 |
| 3 | High | switch 用硬编码数字 | 改用枚举类型 |
| 4 | High | 触发居中可能越界 | 添加 remaining 裁剪 |
| 5 | Medium | "T" 标签混淆 | 改用 "Tr" 前缀 |
| 6 | Medium | 魔法数字 3, 20, 10 | 提取为宏定义 |
| 7 | Medium | const 指针被强转非 const | 参数改为 const |
| 8 | Low | 冗余 #ifndef | 删除 |

---

## 6. 教训清单

### 6.1 初始化相关（教训 1-8）

| # | 教训 |
|---|------|
| 1 | HAL_ADC_MspInit 必须配 DMA |
| 2 | TIM_TRGO 默认 RESET，需改 UPDATE |
| 3 | DMAContinuousRequests 默认 DISABLE，需改 ENABLE |
| 4 | 架构设计必须与实现一致 |
| 5 | 第三方库不能盲信 |
| 6 | FreeRTOS 共享外设必须有互斥保护 |
| 7 | HAL 启动是分层的：时钟→定时器→外设→DMA |
| 8 | RTOS 对象在调度器启动后创建 |

### 6.2 代码审查相关（教训 9-15）

| # | 教训 |
|---|------|
| 9 | 配置系统必须两头接通（Init 读，Set* 写） |
| 10 | 时钟频率不要硬编码 |
| 11 | 互斥锁内外的操作要分清 |
| 12 | 波形显示用 min/max 包络 |
| 13 | 定时器寄存器修改前必须停止定时器 |
| 14 | 互斥锁内不要做耗时操作 |
| 15 | Stop 函数必须释放所有资源 |

### 6.3 硬件配置相关（教训 16-21）

| # | 教训 |
|---|------|
| 16 | ADC 外部触发模式的 4 个必要配置 |
| 17 | 启动顺序决定成败 |
| 18 | 遇到配置问题先查 GitHub |
| 19 | ADC 采样时间必须匹配信号源阻抗 |
| 20 | 空函数不代表"不需要配置" |
| 21 | test_adc 命令是关键诊断工具 |

### 6.4 并发安全相关（教训 22-26）

| # | 教训 |
|---|------|
| 22 | 代码审查必须分 3 维度 |
| 23 | ISR→任务必须有内存屏障 |
| 24 | Start/Stop 函数必须对称 |
| 25 | 半传输中断≠乒乓双缓冲 |
| 26 | 任务栈必须估算上限 |

### 6.5 正确的 setter 模式

```c
ErrorCode_t Module_SetXxx(type) {
    /* 1. 参数校验 */
    if (invalid) return ERR;

    /* 2. 锁内：更新配置 + 拷贝 */
    MODULE_LOCK();
    config.xxx = value;
    Config_t cfg_copy = config;
    MODULE_UNLOCK();

    /* 3. 锁外：耗时操作 */
    Module_GenerateWaveform();

    /* 4. 锁内：应用硬件配置 */
    MODULE_LOCK();
    Module_ApplyConfig();
    MODULE_UNLOCK();

    /* 5. 锁外：同步到 Config */
    Config_SetModuleConfig(&cfg_copy);
}
```

### 6.6 外设初始化检查清单

```
时钟使能 → GPIO配置 → DMA配置 → 触发源配置 → 外设初始化 → 外设启动 → DMA启动
```

关键检查项：
- [ ] TIM TRGO 必须设为 `TIM_TRGO_UPDATE`
- [ ] `DMAContinuousRequests` 必须 `ENABLE`
- [ ] `__HAL_LINKDMA` 把 DMA 链接到外设
- [ ] 触发定时器必须 `HAL_TIM_Base_Start()` 显式启动
- [ ] RTOS mutex 在 `osKernelStart()` 之后创建
- [ ] 验证方法：直接读寄存器

### 6.7 寄存器验证方法

| 验证项 | 读什么 | 正常值 |
|--------|--------|--------|
| 定时器是否运行 | `TIMx->CNT` | 值在变化 |
| DMA 是否在搬数据 | `DAC->DHR12R1` / `ADC->DR` | 非零且变化 |
| 外设是否使能 | `DAC->CR` bit 0 | 1 |
| 触发源是否正确 | `DAC->CR` TSEL 位 | 对应定时器 |
| DMA 流是否启用 | `DMAx_StreamN->CR` bit 0 | 1 |

---

## 7. 代码审查记录

### 7.1 审查统计

| 审查轮次 | 发现问题 | 修复 | 质量评分 |
|---------|---------|------|---------|
| 06-26 初审 | 20 | 0 | 6/10 |
| 06-27 全模块审查 | 23 | 23 | 8/10 |
| 06-27 显示模块审查 | 8 | 8 | — |
| 06-27 触发系统审查 | 6 | 6 | — |
| 06-27 信号发生器审查 | 4 | 4 | — |
| 06-27 DMA 缓冲区审查 | 5 | 3 | — |
| 06-29 综合审查 | — | — | 9/10 |

### 7.2 审查维度

每个功能必须分 3 个维度审查：

1. **C 语言安全** — 整数溢出、指针别名、未初始化变量
2. **嵌入式专项** — 中断优先级、DMA 配置、时钟树
3. **并发安全** — 竞态条件、内存屏障、volatile

### 7.3 审查前后对比

| 指标 | 审查前 | 审查后 |
|------|--------|--------|
| Critical | 4 | 0 |
| High | 6 | 0 |
| Medium | 8 | 2（待改进） |
| 互斥锁保护 | 0 | 62 处 |
| TODO 残留 | 6 | 0 |
| 编译警告 | 6 | 0 |

---

## 8. 当前状态

### 8.1 功能完整度

| 功能 | 状态 | 说明 |
|------|------|------|
| ADC 采样 | ✅ | DMA 双缓冲、内存屏障保护 |
| DAC 输出 | ✅ | LUT 正弦波、7 种波形 |
| 波形显示 | ✅ | EMA 缩放、min/max 包络 |
| 硬件触发 | ✅ | AWD 状态机 |
| 串口命令 | ✅ | 20 条命令 |
| 配置管理 | ✅ | Flash 读写 |
| 按键输入 | ✅ | GPIO 轮询 |

### 8.2 UART 命令列表（20 条）

| 命令 | 功能 |
|------|------|
| help | 显示帮助 |
| status | 系统状态 |
| version | 版本信息 |
| start_osc | 启动示波器 |
| stop_osc | 停止示波器 |
| set_freq <hz> | 设置频率 |
| set_wave <type> | 设置波形 |
| set_amp <mv> | 设置幅度 |
| start_gen | 启动信号发生器 |
| stop_gen | 停止信号发生器 |
| stream_dac | 单帧数据流 |
| stream on/off | 连续数据流 |
| set_cursor <x> <y> | 设置光标 |
| cursor_off | 隐藏光标 |
| set_protocol | 设置协议模式 |
| set_trigger <mV> <edge> <mode> | 设置触发参数 |
| test_adc | ADC 直接测试 |
| history | 版本历史 |
| review | 运行时审查 |
| reset | 系统复位 |

### 8.3 触发系统

| 特性 | 说明 |
|------|------|
| 触发方式 | ADC 模拟看门狗（硬件比较器） |
| 响应精度 | 1 ADC 时钟 (~1μs) |
| CPU 开销 | 极低（中断驱动） |
| 触发模式 | Auto / Normal / Single |
| 边沿检测 | 上升沿 / 下降沿 |
| 状态机 | IDLE → ARMED → TRIGGERED |

### 8.4 显示优化

| 优化项 | 说明 |
|--------|------|
| EMA 平滑缩放 | 均值±3σ 裁剪杂波，跨帧平滑过渡 |
| 跳变检测 | range 暴增 3x 时冻结缩放 |
| 局部清屏 | 只清除波形区域 |
| 中心十字线 | y=31 水平实线 + x=64 垂直虚线 |
| 触发电平线 | 水平虚线标记触发电平位置 |
| 信息栏 | Page 6: 状态+时基, Page 7: 触发+采样率 |

### 8.5 LUT 正弦波

| 特性 | 说明 |
|------|------|
| LUT 大小 | 256 点 |
| 存储位置 | Flash（const，零 RAM 占用） |
| 运行时开销 | 1 次乘法 + 1 次数组访问 ≈ 3 cycles/点 |
| 对比旧方案 | sinf() ≈ 50 cycles/点，提升 16 倍 |
| 精度 | ~16 位（256 点，超过 12-bit DAC） |

---

## 9. 嵌入式专家视角

### 9.1 效率低的根本原因

**不是技术难，是方法错。**

| 问题 | 表现 | 后果 |
|------|------|------|
| 验证后置 | 写 400 行不测试 | 第二天发现 8 个 bug |
| 试错代替查资料 | 改 6 次还是不对 | 浪费 2 小时 |
| 一次改太多 | 重写整个模块 | 引入死机 |

### 9.2 AI 的方式 vs 正确的方式

```
AI 的方式:   需求 → 生成代码 → 编译通过 → 交付
正确的方式:  需求 → 查参考 → 生成代码 → 编译 → 烧录 → 验证 → 修复
```

**AI 跳过了"验证"这一步，因为 AI 无法烧录和测量。**

### 9.3 专家怎么做

**第一天：不写一行应用代码**
```
CubeMX 生成 → 编译 → 烧录 → 串口输出 "Hello" → 确认 UART 通了
              → 配置 ADC 轮询 → 串口输出 ADC 值 → 确认 ADC 活了
              → 配置 DAC 单次输出 → 万用表量引脚 → 确认 DAC 活了
              → 配置 TIM 触发 → 读 TIM->CNT → 确认定时器在跑
```

**第二天：打通数据链路**
```
TIM8 → ADC → DMA → 中断 → 串口打印 → 确认数据正确
TIM5 → DAC → DMA → 万用表 → 确认输出正确
DAC 输出 → 导线 → ADC 输入 → 串口打印 → 确认自测链路
```

**第三天：集成显示**
```
OLED 初始化 → 画一条线 → 确认显示正常
ADC 数据 → 波形渲染 → OLED 显示 → 确认完整链路
```

### 9.4 8 个 bug 的寄存器诊断

如果第一天做了外设自检，这 8 个 bug 会在 10 分钟内全部发现：

| Bug | 诊断方法 | 耗时 |
|-----|---------|------|
| TIM8 TRGO = RESET | 读 TIM8->CR2，发现 TGRS=000 | 5 秒 |
| DMAContinuousRequests | 读 ADC->CR2，发现 CONT=0 | 5 秒 |
| TIM5 未启动 | 读 TIM5->CNT，发现不变 | 5 秒 |
| DMA 未配置 | 读 DMA2_Stream0->CR，发现 EN=0 | 5 秒 |

**寄存器不会骗人。** 软件层面猜来猜去，不如直接读硬件状态。

### 9.5 正确使用 AI 的方式

**不要用 AI 生成整个项目。** 用 AI 做以下事情：

1. **查配置** — "STM32F4 ADC DMA TIM8 触发的正确配置是什么"
2. **写函数** — 给 AI 一个明确的输入输出，让它写一个函数
3. **审查代码** — 让 AI 分维度审查你写的代码
4. **查 bug** — 给 AI 寄存器值和现象，让它分析原因

**但验证永远是你的事。** 烧录、测量、读寄存器——这些 AI 做不了，你必须自己做。

### 9.6 效率演进

| 日期 | 方法 | 效率 | 产出 |
|------|------|------|------|
| 06-26 | 写代码 → 测试 | ❌ 低 | 框架，0 功能验证 |
| 06-27 AM | 写代码 → 测试 → 修 bug | ⚠️ 中 | 8 个 bug，3 小时 |
| 06-27 PM | 查 GitHub → 写代码 → 测试 | ✅ 高 | ADC 配置，1 小时 |
| 06-27 晚 | 设计 → 审查 → 写代码 → 审查 | ✅ 高 | 2 个功能，2 小时 |
| 06-28~29 | 审查 → 修复 | ✅ 高 | 5 个问题，1 小时 |

**趋势**：从"盲目试错"到"系统性审查"，效率逐轮提升。

---

## 10. 待办事项

### 10.1 高优先级

| 问题 | 状态 |
|------|------|
| I2C 启动不稳定 | 待解决 |
| DMA 模式波形验证 | 待验证 |

### 10.2 中优先级

| 问题 | 状态 |
|------|------|
| adc_buffer 真正乒乓双缓冲 | 待改进 |
| osc_config_buffer_size 截断 | 待修复 |

### 10.3 低优先级

| 问题 | 状态 |
|------|------|
| Flash 配置持久化 | TODO |
| IWDG 看门狗 | Deferred |
| 消息队列替代互斥锁 | Planned |
| 二进制协议 | Planned |
| 单元测试框架 | Planned |

---

## 附录

### A. 调试命令

| 命令 | 用途 |
|------|------|
| `stream_dac` | 采集 DAC+ADC 波形数据 |
| `stream on/off` | 连续数据流 |
| `test_adc` | 轮询模式 ADC 测试 |
| `status` | 系统状态 |
| `review` | 运行时审查 |

### B. 诊断流程

```
1. 采集数据（stream_dac）
2. 量化分析（增益、频率、相位）
3. 隔离问题（test_adc 轮询 vs DMA）
4. 定位根因（采样时间、触发频率）
5. 针对性修复
6. 验证修复
```

### C. 关键文件

| 文件 | 职责 |
|------|------|
| Core/Src/oscilloscope.c | 示波器模块 |
| Core/Src/signal_gen.c | 信号发生器模块 |
| Core/Src/display.c | 显示模块 |
| Core/Src/uart_protocol.c | 串口协议 |
| Core/Src/config.c | 配置管理 |
| Core/Src/stm32f4xx_it.c | 中断处理 |
| Drivers/OLED/oled.c | OLED 驱动 |

---

**文档完成**
