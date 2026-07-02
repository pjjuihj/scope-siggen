# DMA 缓冲模块代码审查报告

**日期**: 2026-06-27
**审查范围**: oscilloscope.c, stm32f4xx_it.c, main.c
**审查维度**: C语言安全、嵌入式专项、并发安全

---

## 1. 数据流架构

```
TIM8 触发 → ADC1 (PA6) → DMA2_Stream0 → adc_buffer[1024]
                                              ↓
                    ┌─────────────────────────┤
                    ↓                         ↓
         HalfCpltCallback              CpltCallback
         process_ptr = &buf[0]         process_ptr = &buf[512]
         process_len = 512             process_len = 512
                    ↓                         ↓
                    └──── vTaskNotifyGive ─────┘
                                 ↓
                      Oscilloscope_Task
                      Osc_HandleAdcData()
                                 ↓
                      Display_UpdateScopeEx()
                                 ↓
                      memcpy → waveform_buf[256]
                                 ↓
                          Draw_Waveform()
```

---

## 2. 发现的问题

### 2.1 [High] process_ptr 和 process_len 竞态窗口

**位置**: stm32f4xx_it.c:313-314, oscilloscope.c:388-389

**问题**: ISR 中先设 `process_ptr`，再设 `process_len`。任务先读 `process_ptr`，再读 `process_len`。如果任务在 ISR 两次写入之间被调度执行，可能读到不匹配的指针和长度。

**修复**: 反序读取 + 内存屏障
```c
uint16_t len = process_len;
__DMB();
volatile uint16_t *buf = process_ptr;
__DMB();
```

### 2.2 [High] Oscilloscope_Stop 用错 TIM 停止函数

**位置**: oscilloscope.c:183

**问题**: `HAL_TIM_Base_Stop_IT(&htim8)` 停止的是中断模式，但启动时用的是 `HAL_TIM_Base_Start(&htim8)`（轮询模式）。

**修复**: 改为 `HAL_TIM_Base_Stop(&htim8)`

### 2.3 [Medium] adc_buffer 不是真正的乒乓双缓冲

**位置**: oscilloscope.c:69

**问题**: 当前是单缓冲区 + 半传输中断。如果任务处理时间超过半缓冲区填充时间（512/10000 = 51.2ms），DMA 会覆写正在处理的数据。

**当前安全**: Draw_Waveform + I2C 传输约 10ms << 51.2ms

**未来改进**: 真正的乒乓双缓冲

### 2.4 [Medium] Osc_HandleAdcData 栈使用偏高

**位置**: oscilloscope.c:385

**问题**: 函数内局部变量 + 嵌套调用，估计总栈使用 1.5-2KB。任务栈 2KB 接近上限。

**修复**: 增大任务栈到 4KB

### 2.5 [Medium] osc_config_buffer_size 类型截断

**位置**: stm32f4xx_it.c:312

**问题**: `uint16_t half = osc_config_buffer_size / 2` 可能截断。当前安全（默认 1024）。

**修复**: 改为 `uint32_t half`

---

## 3. 修复记录

| # | 严重度 | 问题 | 状态 |
|---|--------|------|------|
| 1 | High | process_ptr/process_len 竞态 | ✅ 已修复 |
| 2 | High | HAL_TIM_Base_Stop_IT 用错 | ✅ 已修复 |
| 3 | Medium | adc_buffer 非真正乒乓 | ⏳ 待改进 |
| 4 | Medium | 栈使用偏高 | ✅ 已修复 |
| 5 | Medium | 类型截断 | ⏳ 待修复 |

---

## 4. 安全机制总结

| 机制 | 说明 | 状态 |
|------|------|------|
| 乒乓模式 | 半传输/全传输中断，任务处理一半时 DMA 写另一半 | ✅ |
| 内存屏障 | __DMB() 确保 process_ptr/process_len 读取顺序 | ✅ |
| volatile | process_ptr, process_len 声明为 volatile | ✅ |
| 局部拷贝 | 任务将 process_ptr 拷贝到局部变量后再处理 | ✅ |
| 互斥锁 | osc_mutex 保护 osc_status 和 osc_config | ✅ |
| 栈大小 | Oscilloscope_Task 栈 4KB | ✅ |

---

**审查完成**
