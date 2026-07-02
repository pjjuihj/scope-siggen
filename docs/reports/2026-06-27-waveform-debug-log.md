# 2026-06-27 波形显示调试日志

**日期**: 2026-06-27
**问题**: OLED 示波器页面无波形显示
**状态**: ✅ 已解决

---

## 1. 问题现象

### 1.1 初始状态

OLED 显示网格和 "Waiting..." 文字，但波形区域始终空白。ADC/DAC 自测线已连接（PA4→PA6）。

### 1.2 调试目标

1. 修复 OLED 图形重叠/残留问题
2. 实现完整波形显示
3. 验证 DAC→ADC 自测链路

---

## 2. 调试过程（按时间线）

### 2.1 09:03 — OLED 图形重叠问题

**现象**: 文字、波形、网格互相重叠，显示不干净
**根因**: 各 Display 函数直接操作 GRAM，无统一清屏→重绘流程

| 函数 | 问题 |
|------|------|
| `Display_DrawWaveform` | 调用 `OLED_Clear()` 清全屏，擦掉网格和状态栏 |
| `Display_ShowVoltage/Frequency` | 直接写文字，不清旧文字残留 |
| `Display_DrawCursor` | 只画新光标，不擦旧光标 |
| `Display_DrawGrid` | 不清屏就画，重复调用叠加像素 |

**修复**: 引入帧管理机制 — setter 只存状态，`Display_Update()` 统一 Clear→Draw→Refresh

### 2.2 09:21 — OLED 卡死不显示

**现象**: 改完 display.c 后 OLED 直接黑屏
**根因**: `osMutexNew()` 在 `Display_Init()` 中调用，但 `Display_Init()` 在 `main()` → `osKernelStart()` 之前执行，RTOS 内核未启动，互斥锁创建失败

**修复**: 互斥锁创建移到 `Display_Task()` 开头（调度器启动后）

### 2.3 09:38 — 两个任务竞争 OLED

**现象**: 偶尔显示正常，偶尔乱码或黑屏
**根因**: `Display_Task` 和 `Oscilloscope_Task` 同时操作 OLED I2C，无同步

**修复**: 去掉 `scope_owner_active` 机制，统一由 `Display_Task` 渲染，其他任务只更新状态

### 2.4 10:05 — 一直显示 "Waiting..."，无波形

**现象**: 网格和文字正常，但波形区空白
**根因**: ADC 数据链路完全断裂，4 个阻断性 Bug：

| Bug | 文件 | 问题 |
|-----|------|------|
| BUG 1 | `stm32f4xx_hal_msp.c` | `HAL_ADC_MspInit` 缺少 DMA 配置 |
| BUG 2 | `main.c:565` | `TIM_TRGO_RESET` 不产生触发脉冲 |
| BUG 3 | `main.c:346` | `DMAContinuousRequests = DISABLE` |
| BUG 4 | `oscilloscope.c:342` | 从空的 ring buffer 读数据 |

### 2.5 10:42 — 三角波变成梯形

**现象**: 测试三角波顶部被截断，变成梯形
**根因**: 三角波幅度超出显示区域（4096），被 `y > plot_h` 钳位

**修复**: 调整测试波形幅度，范围 0~4095 占满波形区

### 2.6 10:58 — OLED_DrawLine 线段画错

**现象**: 波形显示为乱线，不是平滑曲线
**根因**: `oled.c:169` 有笔误

```c
// BUG: delta_y 被错误赋值为 -delta_x
else {incy=-1;delta_y=-delta_x;}  // ← 错误
else {incy=-1;delta_y=-delta_y;}  // ← 正确
```

此 bug 导致所有向上画的线段（delta_y < 0）被错误处理。波形由大量短线段组成，全部画错。

### 2.7 11:15 — 看到三角波但"不会动"

**现象**: 测试三角波正常显示，但没有真实波形
**根因**: ADC DMA 回调触发了（无 TIMEOUT），但 ADC 读到平数据（~124），`Draw_Waveform` 自动显示测试波形

### 2.8 11:33 — DAC 没有输出

**现象**: 串口诊断 `DOR:0 DHR:0 TIM5:0`
**根因**: `SignalGen_Start()` 调用了 `HAL_DAC_Start_DMA()` 但**没有启动 TIM5**（DAC 触发源）

```c
// 修复前
SigGen_GenerateWaveform();
HAL_DAC_Start_DMA(&hdac, ...);  // DAC 等待 TIM5 触发，但 TIM5 没运行

// 修复后
SigGen_GenerateWaveform();
HAL_TIM_Base_Start(&htim5);      // 先启动 TIM5
HAL_DAC_Start_DMA(&hdac, ...);   // DAC 收到触发，开始输出
```

### 2.9 11:47 — 波形正常显示

**验证**: 串口输出 `TIM5` 值在变化，`DOR` 有非零值，OLED 显示正弦波

### 2.10 12:02 — 添加串口数据流

**需求**: 串口查看 DAC 输出波形数据
**实现**: 添加 `stream_dac` 命令，输出格式：`dac_val,adc_val\r\n`

### 2.11 12:15 — 串口刷屏问题

**现象**: 串口持续输出 `0.82V` `0Hz`，刷满屏幕
**根因**: `Display_ShowVoltage/Frequency` 每次调用都往串口发数据，示波器任务每 20ms 调用一次
**修复**: 移除 display.c 中所有自动串口输出，只保留 OLED 显示

### 2.12 12:25 — 命令解析失败

**现象**: 串口发送 `stream on` 返回 `ERROR:Unknown command`
**根因**: `UART_ReceiveCommand` 中 `cmd[0]` 未初始化，收到 `\r\n` 时返回空字符串
**修复**: 添加 `cmd[0] = '\0'` 初始化，增加接收超时从 10ms 到 100ms

### 2.13 12:35 — OLED 停在启动画面

**现象**: OLED 始终显示 "SCOPE-SIGGEN"，不切换到示波器页面
**根因**: `Display_Task` 的 `osDelay(1500)` 执行后，示波器页面内容不明显（网格太小、文字太小）
**验证**: 串口输出 `[DSP] -> SCOPE` 确认页面切换正常
**修复**: 添加大字 "SCOPE" 标识确认页面切换，后改为正常显示

### 2.14 12:45 — 波形与测量值重叠

**现象**: 波形顶部和频率/电压文字重叠
**根因**: 波形区从 y=0 开始，和测量值在同一位置
**修复**: 波形区从 y=10 开始，给测量值留 10 像素空间

### 2.15 12:55 — 波形是尖刺

**现象**: OLED 波形像一堆尖刺，不是平滑曲线
**根因**: DAC 频率 1000Hz，ADC 采样率 50kHz，一个缓冲区包含 20 个周期，32 个降采样点跨多个周期
**分析**: 串口数据范围 124~2042，每周期仅 14-16 采样点
**修复**: 降低 DAC 频率到 100Hz，一个缓冲区包含 2 个周期

### 2.16 13:10 — 波形幅度太小

**现象**: ADC 数据范围只有 328（1365~1693），波形很扁
**根因**: DAC 输出幅度太小，或 TIM5 预分频器配置错误导致 DAC 没有正确触发
**分析**: 串口数据呈阶梯状下降，不是正弦波
**修复**: 修正 TIM5 预分频器公式，添加 `EGR = TIM_EGR_UG` 使预分频器立即生效

### 2.17 13:20 — 设备死机

**现象**: 烧录后设备无响应，串口无输出
**根因**: 代码修改过多，引入了未定义符号（`__HAL_TIM_GENERATE_EVENT`、UART 变量）
**修复**: 回退到上次提交状态，重新编译

---

## 3. 根因分析

### 3.1 为什么这么久才找到问题？

| 原因 | 反思 |
|------|------|
| **一次改太多** | 重写 display.c 同时引入帧管理、互斥锁、页面机制，出问题时难以定位 |
| **没有分层验证** | 应该先验证 ADC 数据 → 再验证显示渲染 → 最后集成 |
| **忽视硬件层** | 花大量时间在软件架构上，但根因是 DMA/定时器配置缺失 |
| **调试手段不足** | 直到加串口诊断才发现 TIM5=0、DHR=0 |

### 3.2 Bug 分类统计

| 类别 | 数量 | Bug |
|------|------|-----|
| 硬件配置缺失 | 3 | ADC DMA、TIM8 TRGO、TIM5 未启动 |
| 代码笔误 | 2 | OLED_DrawLine delta_y、DMAContinuousRequests |
| 架构缺陷 | 3 | 无帧管理、任务竞争 OLED、锁粒度不一致 |
| 数据路径错误 | 2 | 从空 ring buffer 读取、DMA 缓冲区竞争 |
| 定时器配置 | 2 | TIM5 运行时修改寄存器、Stop 未停止 TIM5 |
| 波形显示 | 3 | 频率不适配、幅度太小、自动缩放缺失 |

### 3.3 正确的调试顺序应该是

1. **先验证硬件**: 串口输出 ADC 原始值、DAC 寄存器状态
2. **单独测试每个模块**: ADC 单次转换 → DAC 单次输出 → DMA 循环
3. **再集成显示**: 确认数据正确后再接 OLED
4. **最后优化**: 帧管理、互斥锁、性能优化

---

## 4. 最终架构

```
TIM5 (328kHz) ──TRGO──→ DAC ──DMA──→ waveform_buffer[256] ──→ PA4
                                                                  │
                                                              导线连接
                                                                  │
TIM8 (50kHz)  ──TRGO──→ ADC ──DMA──→ adc_buffer[1024]  ←── PA6
                                         │
                              回调通知 → Oscilloscope_Task
                                         │
                              Display_DrawWaveform() → dirty=true
                                         │
                              Display_Task (10Hz) → OLED 渲染
```

---

## 5. 改动文件清单

### 第一轮（上午 09:03~13:30）

| 文件 | 改动内容 |
|------|----------|
| `Core/Src/display.c` | 重写为帧管理架构，互斥锁保护 |
| `Core/Inc/display.h` | 新增 `Display_ForceUpdate()` |
| `Core/Src/oscilloscope.c` | 修复数据读取路径，添加诊断代码 |
| `Core/Src/signal_gen.c` | 添加 TIM5 启动，波形 buffer getter |
| `Core/Inc/signal_gen.h` | 新增 getter 声明 |
| `Core/Inc/oscilloscope.h` | 新增 ADC buffer getter 声明 |
| `Core/Src/main.c` | TIM8 TRGO→UPDATE，DMAContinuousRequests→ENABLE |
| `Core/Src/stm32f4xx_hal_msp.c` | ADC1 添加 DMA2_Stream0 配置 |
| `Core/Src/stm32f4xx_it.c` | 添加 DMA2_Stream0 中断 + 半传输回调 |
| `Core/Inc/stm32f4xx_it.h` | 添加中断声明 |
| `Core/Src/uart_protocol.c` | 添加 `stream_dac` 命令 |
| `Core/Inc/uart_protocol.h` | 添加 `CMD_STREAM_DAC` |
| `Drivers/OLED/oled.c` | 修复 `OLED_DrawLine` delta_y 笔误 |

### 第二轮（下午，配置系统连接 + 波形优化）

| 文件 | 改动内容 |
|------|----------|
| `Core/Src/signal_gen.c` | Init 读 Config，Set* 同步 Config，动态更新 TIM5，提取 ApplyConfig |
| `Core/Src/oscilloscope.c` | Init 读 Config，SetConfig 动态更新 TIM8，volatile 修饰，提取 ApplyConfig |
| `Core/Src/stm32f4xx_it.c` | `osc_config_buffer_size` extern 加 volatile |
| `Core/Src/display.c` | Draw_Waveform 改用 min/max 包络算法，step 向上取整 |

**关键改进**：
- 时钟频率从硬编码 `84000000` 改为 `HAL_RCC_GetPCLKxFreq()` 动态获取
- 所有 `Config_Set*()` 调用在互斥锁内拷贝，修复竞态条件
- `ApplyConfig()` 函数增加除零/下溢防御
- 波形显示从取单点改为 min/max 包络，消除多周期尖刺

### 第三轮（14:00~15:30，ADC 配置修复）

| 时间 | 事件 | 根因 | 修复 |
|------|------|------|------|
| 14:05 | ADC 直接读=0，DMA 缓冲区=4095 | ADC 外部触发模式，但 TIM8 没产生触发 | 改为软件触发+连续模式 |
| 14:20 | 波形显示杂波 | 连续模式采样率太高（~219kHz），噪声放大 | 改回 TIM8 硬件触发 |
| 14:35 | 参考 GitHub 示例 | EOCSelection 配置错误，启动顺序错误 | 修复 EOC + 启动顺序 |
| 14:50 | ADC 配置最终修复 | 4 个配置项全部需要修改 | 见下方配置清单 |

**ADC 配置修复清单**：

| 配置项 | 错误值 | 正确值 | 原因 |
|--------|--------|--------|------|
| `ContinuousConvMode` | ENABLE | DISABLE | 单次模式：每个 TIM8 触发一次转换 |
| `ExternalTrigConvEdge` | NONE | RISING | 需要上升沿触发 |
| `ExternalTrigConv` | SOFTWARE_START | T8_TRGO | 用 TIM8 触发，不用软件触发 |
| `EOCSelection` | SEQ_CONV | SINGLE_CONV | 每次转换完就触发 DMA，更可靠 |

**启动顺序修复**：
```c
// 错误：先启动 TIM8，后启动 ADC DMA
HAL_TIM_Base_Start(&htim8);      // 触发信号来了但 ADC 没准备好
HAL_ADC_Start_DMA(&hadc1, ...);  // 错过触发

// 正确：先启动 ADC DMA，后启动 TIM8
HAL_ADC_Start_DMA(&hadc1, ...);  // ADC 等待触发
HAL_TIM_Base_Start(&htim8);      // 第一个触发立即转换
```

**关键教训**：参考 GitHub 类似项目（如 `mervinnguyen/adc-dma-driver`）可以快速找到正确配置，避免反复试错。

### 第四轮（16:00~17:30，ADC 采样修复）

| 时间 | 事件 | 根因 | 修复 |
|------|------|------|------|
| 16:05 | ADC 读数偏小（1167-1690） | 采样时间 84 cycles 不够 | 改为 480 cycles |
| 16:20 | TIM8 配置未更新 | Osc_ApplyConfig() 为空函数 | 实现 PSC/ARR 计算 |
| 16:35 | 480 cycles + 50kHz = 溢出 | 转换时间 23.4µs > 触发间隔 20µs | TIM8 降为 10kHz |
| 16:50 | test_adc 轮询模式正常 | DMA 模式配置问题 | 添加 test_adc 命令诊断 |
| 17:00 | TIM8 配置生效 | PSC=0, ARR=16799, rate=10kHz | 日志确认 |

**关键发现**：

| 模式 | ADC 范围 | 状态 |
|------|---------|------|
| 轮询模式 (480 cycles) | 506-2450 | ✅ 正确 |
| DMA 模式 (84 cycles, 50kHz) | 1167-1690 | ❌ 错误 |
| DMA 模式 (480 cycles, 10kHz) | 待验证 | ⏳ |

**ADC 采样时间选择**：
- 84 cycles (4µs) — 适合低阻信号源
- 144 cycles (6.9µs) — 中等阻抗
- 480 cycles (23.4µs) — 适合高阻信号源（DAC 输出缓冲器 ~15kΩ）

**修改文件清单**：

| 文件 | 改动 |
|------|------|
| `Core/Src/main.c` | ADC 采样时间 84→480 cycles |
| `Core/Src/oscilloscope.c` | 实现 Osc_ApplyConfig(), Osc_GetTimerClock() |
| `Core/Src/uart_protocol.c` | 添加 test_adc 命令 |
| `Core/Inc/uart_protocol.h` | 添加 CMD_TEST_ADC |
| `Core/Src/config.c` | 添加 Config_CreateMutex() |
| `Core/Src/display.c` | 添加 ADC_REF_VOLTAGE 本地定义 |
| `Drivers/OLED/oled.c` | 移除调试 UART 输出 |
