# Debug Log: ADC 波形修复

**Date:** 2026-06-27
**Status:** 部分修复

## 问题

ADC 读数和万用表不一致，波形显示不正确。

## 根因分析

### 问题 1：ADC 采样时间太短
- **症状**：DMA 模式 ADC 读数范围小（1167-1690），轮询模式正确（506-2450）
- **根因**：ADC 采样时间 84 cycles 不够，DAC 输出缓冲器阻抗高
- **修复**：改为 480 cycles（最长采样时间）

### 问题 2：TIM8 触发频率未配置
- **症状**：`Osc_ApplyConfig()` 是空函数，TIM8 用默认配置（50kHz）
- **根因**：采样率配置没有应用到 TIM8 硬件
- **修复**：实现 `Osc_ApplyConfig()`，根据 `sample_rate` 计算 PSC/ARR

### 问题 3：480 cycles + 50kHz = 转换溢出
- **症状**：ADC 读数仍然错误
- **根因**：480 cycles 转换时间 23.4µs > TIM8 触发间隔 20µs
- **修复**：TIM8 频率降为 10kHz（PSC=0, ARR=16799）

## 修改的文件

| 文件 | 修改 |
|------|------|
| `Core/Src/main.c` | ADC 采样时间 84→480 cycles |
| `Core/Src/oscilloscope.c` | 实现 `Osc_ApplyConfig()` + `Osc_GetTimerClock()` |
| `Core/Src/uart_protocol.c` | 添加 `test_adc` 命令（轮询模式 ADC 测试） |
| `Core/Inc/uart_protocol.h` | 添加 `CMD_TEST_ADC` 枚举 |
| `Core/Src/config.c` | 添加 `Config_CreateMutex()` 实现 |
| `Core/Src/display.c` | 添加 `ADC_REF_VOLTAGE` 本地定义 |
| `Drivers/OLED/oled.c` | 移除调试 UART 输出 |

## 验证结果

### test_adc 轮询模式 ✅
```
ADC[0]=932 (0.750V)
ADC[1]=1588 (1.279V)
ADC[2]=2264 (1.824V)
ADC[3]=2450 (1.973V)
...
```
ADC 读数范围 506-2450，符合 DAC 输出范围。

### TIM8 配置 ✅
```
TIM8: PSC=0, ARR=16799, rate=10000 Hz
```
采样率正确配置为 10kHz。

### stream_dac DMA 模式 ⚠️
待验证（设备启动不稳定）。

## 未解决的问题

1. **设备启动不稳定** — I2C 总线卡死导致启动循环
2. **DMA 模式波形验证** — 需要设备稳定启动后测试
3. **频率检测** — 中点阈值算法已实现，待验证

## 关键发现

| 模式 | ADC 范围 | 状态 |
|------|---------|------|
| 轮询模式 (480 cycles) | 506-2450 | ✅ 正确 |
| DMA 模式 (84 cycles) | 1167-1690 | ❌ 错误 |
| DMA 模式 (480 cycles + 10kHz) | 待验证 | ⏳ |

## 下一步

1. 解决 I2C 启动问题
2. 验证 DMA 模式波形
3. 验证频率检测
