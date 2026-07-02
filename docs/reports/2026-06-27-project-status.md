# SCOPE-SIGGEN 项目状态报告

**日期**: 2026-06-27
**版本**: v1.2.0
**状态**: 开发中

---

## 1. 项目概述

SCOPE-SIGGEN 是基于 STM32F407VETx 的双通道信号仪器，集成了示波器和信号发生器功能。

---

## 2. 模块状态

| 模块 | 文件 | 行数 | 状态 | 说明 |
|------|------|------|------|------|
| oscilloscope.c | Core/Src | ~650 | ✅ | ADC DMA、硬件触发(AWD)、频率测量 |
| signal_gen.c | Core/Src | ~650 | ✅ | DAC DMA、LUT正弦波、7种波形 |
| display.c | Core/Src | ~900 | ✅ | OLED 帧管理、EMA缩放、触发指示、信息栏 |
| uart_protocol.c | Core/Src | ~700 | ✅ | 环形缓冲接收、20条命令 |
| config.c | Core/Src | ~280 | ✅ | Flash 读写、校验和 |
| debug.c | Core/Src | ~410 | ✅ | 日志、调试命令、静态缓冲区 |
| app_init.c | Core/Src | ~270 | ✅ | 启动流程、自检、降级模式 |
| key_handler.c | Core/Src | ~190 | ✅ | GPIO轮询按键、去抖 |
| error_tracker.c | Core/Src | ~240 | ✅ | 错误记录查询 |
| ring_buffer.c | Core/Src | ~240 | ✅ | 关中断保护、批量操作 |

---

## 3. 功能完整度

### 3.1 核心功能

| 功能 | 状态 | 说明 |
|------|------|------|
| ADC 采样 | ✅ | DMA 双缓冲、内存屏障保护 |
| DAC 输出 | ✅ | LUT正弦波、7种波形、DMA循环 |
| 波形显示 | ✅ | EMA 平滑缩放、min/max 包络、局部清屏 |
| 电压测量 | ✅ | 峰峰值计算 |
| 频率测量 | ✅ | 过零检测算法 |
| 硬件触发 | ✅ | ADC AWD 模拟看门狗、上升沿/下降沿 |
| 软件触发 | ✅ | Auto 模式备用 |
| 触发电平指示 | ✅ | 水平虚线显示 |
| 信息栏显示 | ✅ | 时基、触发模式、采样率 |
| 串口命令 | ✅ | 20条命令 |
| 配置存储 | ✅ | Flash 读写 |
| 按键输入 | ✅ | GPIO轮询、去抖、长短按 |

### 3.2 LUT 正弦波生成

| 特性 | 说明 |
|------|------|
| LUT 大小 | 256 点 |
| 存储位置 | Flash（const，零 RAM 占用） |
| 值范围 | 0.0~1.0（归一化） |
| 索引公式 | `(i * 256 / size) & 255` |
| 运行时开销 | 1次乘法 + 1次数组访问 ≈ 3 cycles/点 |
| 对比旧方案 | sinf() ≈ 50 cycles/点，提升 16 倍 |
| 精度 | ~16 位（256 点，超过 12-bit DAC） |

### 3.3 硬件触发系统（ADC AWD）

| 特性 | 说明 |
|------|------|
| 触发方式 | ADC 模拟看门狗（硬件比较器） |
| 响应精度 | 1 ADC 时钟 (~1μs) |
| CPU 开销 | 极低（中断驱动） |
| 触发模式 | Auto / Normal / Single |
| 边沿检测 | 上升沿 / 下降沿 |
| 状态机 | IDLE → ARMED → TRIGGERED |

**触发流程：**
```
上升沿检测：
  1. 配置 AWD: low=0, high=threshold → 检测信号低于阈值
  2. AWD 中断: 信号 ≤ threshold → 重配置 AWD
  3. 配置 AWD: low=threshold, high=4095 → 检测信号高于阈值
  4. AWD 中断: 信号 ≥ threshold → 触发！记录 DMA 位置
```

### 3.4 显示优化

| 优化项 | 说明 |
|--------|------|
| EMA 平滑缩放 | 均值±3σ 裁剪杂波，跨帧平滑过渡 |
| 跳变检测 | range 暴增 3x 时冻结缩放 |
| 局部清屏 | 只清除波形区域，保留测量值和信息栏 |
| 中心十字线 | y=31 水平实线 + x=64 垂直虚线 |
| 触发电平线 | 水平虚线标记触发电平位置 |
| 信息栏 | Page 6: 状态+时基, Page 7: 触发+采样率 |

### 3.5 UART 命令（20条）

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

---

## 4. 并发安全

### 4.1 互斥锁保护

| 模块 | 锁名 | 保护内容 |
|------|------|---------|
| oscilloscope.c | osc_mutex | osc_status, osc_config |
| signal_gen.c | siggen_mutex | siggen_status, siggen_config |
| display.c | oled_mutex | OLED_GRAM, 状态变量 |
| ring_buffer.c | __disable_irq | head, tail, count |

### 4.2 FreeRTOS 任务

| 任务 | 优先级 | 栈 | 周期 |
|------|--------|-----|------|
| Oscilloscope | Normal | 2KB | 事件驱动 |
| SignalGen | Normal | 2KB | 1s |
| UART | Normal | 4KB | 事件驱动 |
| Display | Normal | 2KB | 100ms |
| Key | Normal | 1KB | 10ms |

### 4.3 DMA 缓冲区安全

| 保护机制 | 说明 |
|----------|------|
| 乒乓模式 | 半传输/全传输中断，任务处理一半时 DMA 写另一半 |
| 内存屏障 | __DMB() 确保 process_ptr/process_len 读取顺序 |
| volatile | process_ptr, process_len 声明为 volatile |
| 局部拷贝 | 任务将 process_ptr 拷贝到局部变量后再处理 |

---

## 5. 硬件配置

### 5.1 外设配置

| 外设 | 功能 | 引脚 | 配置 |
|------|------|------|------|
| ADC1 | 示波器输入 | PA6 | 12-bit, TIM8触发, DMA, AWD |
| DAC1 | 信号输出 | PA4 | 12-bit, TIM5触发, DMA |
| TIM8 | ADC触发源 | — | PSC=4, ARR=671, 50kHz |
| TIM5 | DAC触发源 | — | ARR=255 |
| USART1 | 串口 | PA9/PA10 | 115200, 中断接收 |
| I2C1 | OLED | PB6/PB7 | 400kHz |
| IWDG | 看门狗 | — | PSC=256 (暂时禁用) |
| GPIO | 按键 | PA0, PB3-6 | GPIO轮询 |
| GPIO | LED | PB2 | 推挽输出 |
| SWD | 调试 | PA13/PA14 | SWD模式 |

### 5.2 时钟配置

```
HSE (8MHz) → PLL (M=8, N=336, Q=4) → SYSCLK = 168 MHz
                                         │
                    ┌────────────────────┼────────────────────┐
                    │                    │                    │
              AHB = 168 MHz        APB1 = 42 MHz        APB2 = 84 MHz
                                    │                    │
                              TIM2-7 = 84 MHz      TIM8-11 = 168 MHz
```

### 5.3 中断优先级

| 中断 | 优先级 | 用途 |
|------|--------|------|
| ADC_IRQn | 5 | ADC 转换 + AWD（实时采样） |
| DMA2_Stream0 | 5 | ADC DMA（实时采样） |
| TIM8_UP | 5 | ADC 触发定时器 |
| TIM5_IRQn | 5 | DAC 触发定时器 |
| DMA1_Stream5 | 5 | DAC DMA |
| USART1_IRQn | 6 | 串口通信 |
| I2C1_EV/ER | 7 | OLED I2C 显示 |
| SysTick | 15 | FreeRTOS 心跳 |
| PendSV | 15 | 上下文切换 |
| TIM6_DAC | 15 | HAL 时基 |

### 5.4 DMA 配置

| DMA 流 | 通道 | 方向 | 用途 |
|--------|------|------|------|
| DMA2_Stream0 | CH0 | 外设→内存 | ADC1 循环采集 |
| DMA1_Stream5 | CH7 | 内存→外设 | DAC1 波形输出 |

---

## 6. 代码质量

| 指标 | 数值 |
|------|------|
| 编译错误 | 0 |
| 编译警告 | 0 |
| TODO 残留 | 0 |
| 互斥锁覆盖 | 62 处 |
| HAL 返回值检查 | 关键路径全部检查 |
| 全局变量 | 已封装为结构体 |
| const 正确性 | 所有只读指针标注 const |
| 枚举类型安全 | switch 使用枚举类型 |
| NULL 指针检查 | 所有公共函数检查 |
| 除零保护 | 所有波形生成函数检查 |

---

## 7. 代码审查修复记录

### 7.1 显示模块修复 (2026-06-27)

| # | 严重度 | 问题 | 修复 |
|---|--------|------|------|
| 1 | Critical | trigger_enabled 导致触发电平线不显示 | 删除字段，改用 trigger_level_mv == 0 |
| 2 | High | EMA 每帧重置，平滑效果失效 | 只在 Display_Clear 时重置 |
| 3 | High | switch 用硬编码数字 | 改用 TriggerMode_t 枚举类型 |
| 4 | High | 触发居中可能越界 | 添加 remaining 裁剪 |
| 5 | Medium | "T" 标签混淆 | 触发电平改用 "Tr" 前缀 |
| 6 | Medium | 魔法数字 3, 20, 10 | 提取为宏定义 |
| 7 | Medium | const 指针被强转非 const | 参数改为 const uint16_t * |
| 8 | Low | 冗余 #ifndef ADC_REF_VOLTAGE | 删除 |

### 7.2 DMA 缓冲模块修复 (2026-06-27)

| # | 严重度 | 问题 | 修复 |
|---|--------|------|------|
| 1 | High | HAL_TIM_Base_Stop_IT 用错 | 改为 HAL_TIM_Base_Stop |
| 2 | High | process_ptr/process_len 竞态 | 反序读取 + __DMB() 内存屏障 |
| 3 | Medium | Oscilloscope_Task 栈 2KB 偏小 | 增大到 4KB |
| 4 | High | 运行时改 config 导致 DMA 不一致 | Oscilloscope_SetConfig 安全停止/重启 DMA |

### 7.3 信号发生器修复 (2026-06-27)

| # | 严重度 | 问题 | 修复 |
|---|--------|------|------|
| 1 | Medium | sinf() 浮点运算开销大 | 改为 LUT 查表（256点） |
| 2 | Medium | 缺少除零保护 | 所有波形函数添加 size==0 检查 |
| 3 | Medium | 缺少 NULL 指针检查 | 所有波形函数添加 buffer==NULL 检查 |
| 4 | Low | 包含不必要的 <math.h> | 移除 |

### 7.4 硬件触发实现 (2026-06-27)

| # | 功能 | 说明 |
|---|------|------|
| 1 | AWD 配置 | Osc_ConfigureAWD() 配置 ADC 模拟看门狗 |
| 2 | AWD 回调 | Osc_AWD_Callback() 两阶段边沿检测 |
| 3 | 状态机 | IDLE → ARMED → TRIGGERED |
| 4 | 触发模式 | Auto(软件) / Normal(硬件) / Single(硬件) |
| 5 | 重配置 | Oscilloscope_SetConfig 运行时重配置 AWD |

### 7.5 中断优先级优化 (2026-06-27)

| 中断 | 旧优先级 | 新优先级 | 说明 |
|------|---------|---------|------|
| ADC/DMA2/TIM8 | 5 | 5 | 保持（实时采样） |
| TIM5/DMA1 | 5 | 5 | 保持（DAC输出） |
| USART1 | 5 | 6 | 降低（串口通信） |
| I2C1 | 5 | 7 | 降低（OLED显示） |

---

## 8. 移植兼容性

| 目标平台 | 兼容性 | 说明 |
|---------|--------|------|
| Cortex-M4F (STM32F4) | ✅ 完全兼容 | FPU 硬件加速 |
| Cortex-M4 (无FPU) | ✅ 兼容 | 软浮点 LUT |
| Cortex-M3 | ✅ 兼容 | 软浮点 LUT |
| Cortex-M0/M0+ | ✅ 兼容 | 建议改用 uint16_t LUT |
| ARM Compiler 5/6 | ✅ 兼容 | 标准 C 语法 |
| GCC ARM | ✅ 兼容 | 标准 C 语法 |
| IAR | ✅ 兼容 | 标准 C 语法 |

---

## 9. 待实现功能

| # | 功能 | 优先级 | 说明 |
|---|------|--------|------|
| 1 | IWDG 看门狗 | Medium | 发布前启用 |
| 2 | 消息队列 | Low | 当前用互斥锁替代 |
| 3 | 二进制协议 | Low | 仅实现文本模式 |
| 4 | 单元测试框架 | Low | 未实现 |
| 5 | uint16_t LUT | Low | 为 M0/M0+ 平台准备 |

---

**文档结束**
