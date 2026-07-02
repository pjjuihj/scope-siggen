# ADC/DAC 配置系统修复设计

**日期**: 2026-06-27
**状态**: 已批准
**范围**: 连接配置系统与硬件定时器，修复6个bug

---

## 问题

设计文档要求 TIM5/TIM8 支持"可变频率"，但实际代码中定时器参数硬编码，Config API 从未被调用。

## 修复内容

### 1. signal_gen.c — 动态重配 TIM5

- 新增 `SigGen_ApplyConfig()`: 根据 `siggen_config.frequency` 计算 TIM5 PSC/ARR
- `SignalGen_Init()`: 从 `Config_GetSigGenConfig()` 读取配置，调用 `SigGen_ApplyConfig()`
- `SignalGen_SetFrequency()` / `SignalGen_SetConfig()`: 调用 `SigGen_ApplyConfig()` 动态更新 TIM5

### 2. oscilloscope.c — 动态重配 TIM8

- 新增 `Osc_ApplyConfig()`: 根据 `osc_config.sample_rate` 计算 TIM8 PSC/ARR
- `Oscilloscope_Init()`: 从 `Config_GetOscConfig()` 读取配置，调用 `Osc_ApplyConfig()`
- `Oscilloscope_SetConfig()`: 调用 `Osc_ApplyConfig()` 动态更新 TIM8
- `process_ptr` / `process_len`: 去掉 `static`（修复 ISR `extern` 链接）
- `Oscilloscope_Task`: `Display_UpdateScope` 传 `adc_buffer` 全缓冲区

### 3. uart_protocol.c — 命令同步 Config

- `UART_HandleSetFreq()`: 调用后同步 `Config_SetSigGenConfig()`
- `UART_HandleSetWave()`: 同上
- `UART_HandleSetAmp()`: 同上
- `UART_HandleStreamDac()`: 使用 `Oscilloscope_GetAdcBuffer()` getter（已有互斥保护）

### 4. 不改的部分

- `Config_Save()` 暂不接入（Flash 写入需谨慎）
- Ring buffer 保留但不强制使用
- `MX_TIM5_Init()` / `MX_TIM8_Init()` 硬编码保留为初始值

---

## 定时器计算公式

### TIM5 (DAC, APB1 timer clock = 84 MHz)

```
target_trgo = frequency × DAC_BUFFER_SIZE (256)
PSC = 84000000 / (target_trgo × 65536) + 1
ARR = 84000000 / (PSC × target_trgo) - 1
```

### TIM8 (ADC, APB2 timer clock = 168 MHz)

```
PSC = 168000000 / (sample_rate × 65536) + 1
ARR = 168000000 / (PSC × sample_rate) - 1
```

## 验证方法

- 串口 `status` 命令确认频率/采样率生效
- 串口 `stream_dac` 查看波形数据
- OLED 波形显示验证 ADC→DAC 闭环
