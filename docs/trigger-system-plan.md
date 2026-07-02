# 触发系统实现方案 ✅ 已完成

**完成日期**: 2026-06-27

## 实现内容

### 1. OscConfig_t 扩展触发模式 ✅
```c
typedef enum {
    TRIG_AUTO,      /* 自动：无触发也显示 */
    TRIG_NORMAL,    /* 普通：有触发才显示 */
    TRIG_SINGLE,    /* 单次：触发一次后停止更新 */
} TriggerMode_t;
```

### 2. 硬件触发（ADC AWD）✅
- ADC 模拟看门狗（AWD）硬件比较器
- 两阶段边沿检测状态机
- 响应精度：1 ADC 时钟 (~1μs)
- CPU 开销：极低（中断驱动）

**状态机：**
```
IDLE → Osc_ConfigureAWD() → ARMED
ARMED → AWD中断(第一阈值) → 重配置AWD → TRIGGERED
TRIGGERED → AWD中断(第二阈值) → 记录触发位置 → IDLE
```

### 3. 软件触发（Auto 模式备用）✅
- `Osc_FindTrigger()` 在缓冲区中扫描触发点
- 仅在 Auto 模式下使用
- Normal/Single 模式使用硬件触发

### 4. Osc_HandleAdcData 改造 ✅
- Auto 模式：直接使用半缓冲区数据
- Normal/Single 模式：从 DMA 环形缓冲区提取以触发点为中心的完整波形
- 内存屏障保护 process_ptr/process_len 读取

### 5. UART 命令 ✅
```
set_trigger <mV> <edge> <mode>
  edge: 0=下降沿, 1=上升沿
  mode: 0=Auto, 1=Normal, 2=Single
```

### 6. 运行时重配置 ✅
- Oscilloscope_SetConfig 支持运行时修改触发设置
- 安全停止/重启 DMA 和 AWD

---

## 代码审查修复

| # | 严重度 | 问题 | 修复 |
|---|--------|------|------|
| 1 | High | HAL_TIM_Base_Stop_IT 用错 | 改为 HAL_TIM_Base_Stop |
| 2 | High | process_ptr/process_len 竞态 | 反序读取 + __DMB() 内存屏障 |
| 3 | Medium | Oscilloscope_Task 栈 2KB 偏小 | 增大到 4KB |
| 4 | High | 运行时改 config 导致 DMA 不一致 | 安全停止/重启 DMA |
| 5 | High | switch 用硬编码数字 | 改用 TriggerMode_t 枚举类型 |
| 6 | High | 触发居中可能越界 | 添加 remaining 裁剪 |

---

## 涉及文件

| 文件 | 改动 |
|------|------|
| Core/Inc/oscilloscope.h | 新增 TriggerMode_t, HwTrigState_t, HwTrigCtx_t |
| Core/Src/oscilloscope.c | 新增 AWD 配置/回调，改造 Osc_HandleAdcData |
| Core/Inc/uart_protocol.h | 新增 CMD_SET_TRIGGER |
| Core/Src/uart_protocol.c | 新增 set_trigger 命令解析和处理 |
| Core/Src/config.c | 默认配置增加 trigger_mode |
| Core/Src/stm32f4xx_it.c | 新增 HAL_ADC_LevelOutOfWindowCallback |

---

**文档完成**
