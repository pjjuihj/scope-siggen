# Debug Log: Waveform Analysis & Fix

**Date:** 2026-06-27
**Tool:** serial_loop.py --mode collect --send-cmd "stream_dac"
**Port:** COM3 @ 115200 bps

## Issues Found & Fixed

### Issue 1: OLED Clear Debug Output (FIXED)

**Problem:** `OLED_Clear()` and `OLED_Init()` had debug UART output that cluttered serial.

**Fix:** Removed debug UART output from `Drivers/OLED/oled.c`.

### Issue 2: ADC Sampling Time Too Short (FIXED)

**Problem:** ADC 采样时间 84 cycles 不够，DAC 输出缓冲器阻抗高导致采样不准确。

**Fix:** 改为 ADC_SAMPLETIME_480CYCLES。

### Issue 3: TIM8 Trigger Not Configured (FIXED)

**Problem:** `Osc_ApplyConfig()` 是空函数，TIM8 用默认配置（50kHz），不是配置的采样率。

**Fix:** 实现 `Osc_ApplyConfig()`，根据 `sample_rate` 计算 PSC 和 ARR。

### Issue 4: Conversion Time Overflow (FIXED)

**Problem:** 480 cycles 转换时间（23.4µs）> TIM8 触发间隔（20µs at 50kHz），导致 ADC 数据错误。

**Fix:** TIM8 频率降为 10kHz（PSC=0, ARR=16799）。

### Issue 5: Frequency Detection with DC Offset (FIXED)

**Problem:** 频率检测用固定阈值 2048，信号有直流偏移时检测失败。

**Fix:** 改用信号中点 `(min+max)/2` 作为阈值。

### Issue 6: Voltage Measurement (FIXED)

**Problem:** 电压测量用平均值，对 AC 信号不准确。

**Fix:** 改用峰峰值 `(max-min)`。

## Files Modified

| File | Change |
|------|--------|
| `Core/Src/main.c` | ADC 采样时间 84→480 cycles |
| `Core/Src/oscilloscope.c` | 实现 Osc_ApplyConfig(), Osc_GetTimerClock() |
| `Core/Src/uart_protocol.c` | 添加 test_adc 命令 |
| `Core/Inc/uart_protocol.h` | 添加 CMD_TEST_ADC |
| `Core/Src/config.c` | 添加 Config_CreateMutex() |
| `Core/Src/display.c` | 添加 ADC_REF_VOLTAGE 定义 |
| `Drivers/OLED/oled.c` | 移除调试 UART 输出 |

## Verification

### test_adc Polling Mode ✅
- ADC range: 506-2450 (matches DAC output)
- Sampling time: 480 cycles

### TIM8 Configuration ✅
- PSC=0, ARR=16799, rate=10000 Hz

### stream_dac DMA Mode ⚠️
- Pending verification (device boot instability)

## Key Findings

| Mode | ADC Range | Status |
|------|-----------|--------|
| Polling (480 cycles) | 506-2450 | ✅ Correct |
| DMA (84 cycles, 50kHz) | 1167-1690 | ❌ Wrong |
| DMA (480 cycles, 10kHz) | TBD | ⏳ Pending |

## Remaining Issues

1. **Device boot instability** — I2C bus stuck causing boot loop
2. **DMA waveform verification** — Need stable boot to test
3. **Frequency detection** — Mid-point threshold implemented, pending verification
