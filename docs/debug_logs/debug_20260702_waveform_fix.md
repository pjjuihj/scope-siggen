# 2026-07-02 波形修复调试日志

## 问题描述

OLED 显示的波形不正确：
- 正弦波显示为锯齿波
- 波形有大跳变（>500mV）
- 频率检测大部分为 0Hz

## 排查过程

### 1. 确认 DAC 输出

用方波测试（前半高电平，后半低电平）验证 DAC 和 ADC：
- DAC 输出正确：高电平 ~2040，低电平 ~116
- ADC 读取正确：min/max 分布在两个清晰的电平

**结论**: DAC 和 ADC 硬件正常。

### 2. 分析 ADC 缓冲区范围

正弦波每个 ADC 缓冲区（512 样本 @ 10kHz = 51.2ms）应包含 ~51 个完整周期，min/max 范围应接近 2048。

实际数据：平均范围只有 40-50，说明每个缓冲区只捕获了正弦波的一小段。

**结论**: DAC 输出频率远低于预期（~0.12 Hz vs 1 kHz）。

### 3. 定位 TIM5 配置

检查 `SigGen_ApplyConfig` 函数：

```c
// 错误公式
uint32_t psc = (timer_clk / target_trgo) - 1;
```

计算：`psc = (84MHz / 256kHz) - 1 = 327`

实际 TRGO：`84MHz / 328 / 256 = 1001 Hz`（不是 256kHz）

**根因**: 公式假设 ARR=0，但实际 ARR=255。正确公式：

```c
// 正确公式
uint32_t psc = timer_clk / (target_trgo * (ARR+1)) - 1;
// = 84MHz / (256kHz * 256) - 1 ≈ 0
```

### 4. 修复

修改 `SigGen_ApplyConfig`：

```c
uint32_t psc_plus_1 = timer_clk / (siggen_config.frequency * DAC_BUFFER_SIZE * DAC_BUFFER_SIZE);
uint32_t psc = (psc_plus_1 > 0) ? (psc_plus_1 - 1) : 0;
```

### 5. 验证

修复后 ADC 缓冲区范围从 40 提升到 1925，波形正确。

## 附带修复

| 问题 | 修复 |
|------|------|
| `sinf()` 返回错误值 | 用查表法替代 |
| `snprintf` 不输出 | 添加 `#include <stdio.h>` |
| UART 命令不稳定 | 改进换行符处理 |
| OLED 调试输出干扰 | 设置日志级别 ERROR |
| 频率检测为 0Hz | 自适应阈值 `(min+max)/2` |
| 波形渲染不平滑 | min/max 竖线改为平均值连线 |

## 关键教训

1. **定时器频率公式必须考虑 ARR**: `TRGO = clk / (PSC+1) / (ARR+1)`
2. **ADC 缓冲区范围是 DAC 频率的直接指标**: 范围 < 1000 说明频率太低
3. **方波测试是验证 DAC/ADC 链路的最快方法**
4. **ARM CC 浮点库不可靠**: 优先用查表法
