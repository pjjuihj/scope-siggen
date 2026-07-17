# STM32 示波器/信号发生器开发全记录

**项目**: SCOPE-SIGGEN
**平台**: STM32F407VETx (ARM Cortex-M4, 168MHz)
**RTOS**: FreeRTOS
**版本**: v1.3.0
**开发周期**: 2026-06-26 ~ 2026-07-02（7 天）

---

## 一、项目概述

用 STM32F407 做一个带示波器和信号发生器功能的小仪器：

```
信号发生器: TIM5 → DAC → DMA → PA4 → 输出波形
                                    │
                                导线连接
                                    │
示波器:     PA6 → ADC → DMA → TIM8触发 → 波形处理 → OLED显示
```

功能清单：

- 示波器：ADC DMA 双缓冲采集、硬件触发(AWD)、频率/电压测量
- 信号发生器：DAC DMA 输出、5 种波形（正弦/方波/三角/锯齿/直流）
- 波形显示：SSD1306 OLED 128x64、EMA 缩放、min/max 包络
- 串口控制：21 条命令、中断接收环形缓冲区
- 配置管理：Flash 持久化、魔数+校验和双重验证
- 看门狗：IWDG 32 秒超时

代码规模约 7000 行 C，18 个源文件，16 个头文件。

---

## 二、架构设计

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

| 任务         | 优先级 | 栈  | 触发方式    | 职责              |
| ------------ | ------ | --- | ----------- | ----------------- |
| Oscilloscope | Normal | 4KB | DMA中断通知 | ADC采集、波形处理 |
| SignalGen    | Normal | 4KB | 1Hz定时     | DAC输出控制       |
| UART         | Normal | 4KB | 100Hz轮询   | 命令接收解析      |
| Display      | Normal | 4KB | 20Hz定时    | OLED渲染          |
| Key          | Normal | 1KB | 100Hz定时   | 按键扫描          |

### 2.3 模块接口

所有模块遵循统一接口：

```c
ErrorCode_t Module_Init(void);
ErrorCode_t Module_Start(void);
ErrorCode_t Module_Stop(void);
ErrorCode_t Module_GetStatus(Status_t *status);
ErrorCode_t Module_SelfTest(void);
```

---

## 三、开发过程

### Day 1（06-26）：盲目堆代码

一次性写了 400 多行代码，4 个模块同时推进。编译通过了，但没有任何功能经过硬件验证。OLED 乱码、DAC 不输出、ADC 不采集。

后来才明白：写代码和跑通功能是两回事。应该先用最丑的方式把一条链路跑通，再美化。

### Day 2（06-27）：3 小时修 8 个 bug

逐个外设验证，发现 8 个初始化配置 bug：

| # | Bug                             | 根因                 | 修复耗时 |
| - | ------------------------------- | -------------------- | -------- |
| 1 | HAL_ADC_MspInit 缺 DMA          | CubeMX 未选 DMA 模式 | 5分钟    |
| 2 | TIM8 TRGO = RESET               | 默认值不适合         | 2分钟    |
| 3 | DMAContinuousRequests = DISABLE | 默认值不适合         | 2分钟    |
| 4 | 从空 ring buffer 读             | 架构与实现脱节       | 10分钟   |
| 5 | OLED_DrawLine delta_y           | 第三方库 bug         | 15分钟   |
| 6 | 两个任务竞争 OLED               | 无互斥保护           | 20分钟   |
| 7 | TIM5 未启动                     | 启动链不完整         | 2分钟    |
| 8 | osMutexNew 在内核启动前         | 初始化顺序错误       | 10分钟   |

所有 bug 都是初始化问题。如果第一天做了外设自检，10 分钟就能全发现：

```
TIM8 TRGO = RESET    → 读 TIM8->CR2，发现 TGRS=000  → 5秒
DMAContinuousRequests → 读 ADC->CR2，发现 CONT=0     → 5秒
TIM5 未启动           → 读 TIM5->CNT，发现不变       → 5秒
DMA 未配置            → 读 DMA2_Stream0->CR，发现 EN=0 → 5秒
```

寄存器不会骗人。软件层面猜来猜去，不如直接读硬件状态。

### Day 2 下午：配置系统连接 + ADC 配置修复

连接 Config 系统与 ADC/DAC 硬件，修复 ADC 硬件触发配置。ADC 外部触发模式有 4 个必要配置，采样时间从 84 cycles 改为 480 cycles（高阻信号源必须），启动顺序改为先 ADC DMA 后 TIM8。

之前试错 2 小时没搞定，后来查 GitHub 5 分钟就找到了正确配置。

### Day 2 晚：功能扩展 + 代码审查

做了显示优化、触发系统、信号发生器优化。代码审查发现显示模块 8 项、触发系统 6 项、信号发生器 4 项问题。

代码审查比写代码更重要。每轮审查都发现了前一轮遗漏的问题。

### Day 3（06-28~29）：DMA 缓冲区安全

审查 DMA 缓冲区数据流，修复了 process_ptr/process_len 竞态窗口、HAL_TIM_Base_Stop_IT 用错、Oscilloscope_Task 栈从 2KB 增大到 4KB。

### Day 7（07-02）：维护改进 v1.3.0

架构师审视后做了 7 项改进：

| # | 改进项                   | 改动                         |
| - | ------------------------ | ---------------------------- |
| 1 | SignalGen_Stop 不停 TIM5 | 加 HAL_TIM_Base_Stop         |
| 2 | main.c 调试代码          | 删除 I2C 扫描和 UART 测试    |
| 3 | uart_protocol 命令表     | 命令表替代 if-else 链        |
| 4 | mutex 初始化断言         | 加 configASSERT              |
| 5 | 锁内波形生成             | 波形生成移到锁外             |
| 6 | Config 魔数验证          | ConfigHeader_t 加 magic 字段 |
| 7 | OLED 调试输出            | 删除所有 UART 调试输出       |

另外启用了 IWDG 看门狗（32秒超时）和 16 条自动化回归测试。

---

## 四、踩坑记录

### 4.1 ADC 外部触发配置

```c
hadc1.Init.ContinuousConvMode = DISABLE;   // 必须禁用，否则忽略触发
hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T8_TRGO;
hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
```

默认值不适合外部触发模式，必须逐条检查。

### 4.2 启动顺序

```
正确: HAL_ADC_Start_DMA() → HAL_TIM_Base_Start()
错误: HAL_TIM_Base_Start() → HAL_ADC_Start_DMA()
```

RM0090 Section 11.3.15 要求 ADC 先就绪，定时器后启动。

### 4.3 ADC 采样时间

| 采样时间   | 转换时间 | 适用场景            |
| ---------- | -------- | ------------------- |
| 84 cycles  | 4.57µs  | 低阻信号源 (<1kΩ)  |
| 480 cycles | 23.4µs  | 高阻信号源 (>10kΩ) |

DAC 输出缓冲器阻抗约 15kΩ，用 84 cycles 采样值只有 60% 正确。

### 4.4 定时器预分频公式

```
TRGO = timer_clk / (PSC+1) / (ARR+1)
```

忘记考虑 ARR，导致 DAC 频率偏低 256 倍。

### 4.5 波形显示

```c
// 错误：取单点，多周期时产生尖刺
uint16_t y = data[x * step];

// 正确：min/max 包络
uint16_t col_min = data[x * step], col_max = col_min;
for (uint16_t i = x * step + 1; i < (x+1) * step; i++) {
    if (data[i] < col_min) col_min = data[i];
    if (data[i] > col_max) col_max = data[i];
}
OLED_DrawLine(x, col_min, x, col_max, 1);
```

### 4.6 FreeRTOS 互斥锁

```c
// 错误：锁内做耗时操作
SIGGEN_LOCK();
SigGen_GenerateWaveform();  // 768 次乘法
SIGGEN_UNLOCK();

// 正确：锁内拷贝，锁外生成
SIGGEN_LOCK();
SigGenConfig_t cfg = siggen_config;
SIGGEN_UNLOCK();
SigGen_GenerateWaveform();  // 锁外执行
```

---

## 五、效率变化

| 日期     | 方法                           | 效率 | 产出             |
| -------- | ------------------------------ | ---- | ---------------- |
| 06-26    | 写代码 → 测试                 | 低   | 框架，0 功能验证 |
| 06-27 AM | 写代码 → 测试 → 修 bug       | 中   | 8 个 bug，3 小时 |
| 06-27 PM | 查 GitHub → 写代码 → 测试    | 高   | ADC 配置，1 小时 |
| 06-27 晚 | 设计 → 审查 → 写代码 → 审查 | 高   | 2 个功能，2 小时 |
| 06-28~29 | 审查 → 修复                   | 高   | 5 个问题，1 小时 |
| 07-02    | 架构审视 → 系统改进           | 高   | 7 项改进，2 小时 |

从盲目试错到系统性审查，效率逐轮提升。

---

## 六、AI 能做什么，不能做什么

AI 能做的：

- 查配置："STM32F4 ADC DMA TIM8 触发的正确配置是什么"
- 写函数：给一个明确的输入输出，让它写一个函数
- 审查代码：分维度审查你写的代码
- 查 bug：给寄存器值和现象，让它分析原因

AI 不能做的：

- 验证：无法烧录和测量
- 读寄存器：无法读硬件状态
- 判断信号质量：无法用示波器看波形

```
专家:  需求 → 查参考 → 生成代码 → 编译 → 烧录 → 验证 → 修复
AI:    需求 → 生成代码 → 编译通过 → 交付（跳过验证）
```

验证永远是你的事。烧录、测量、读寄存器，这些 AI 做不了。

---

## 七、项目资源

- GitHub: https://github.com/pjjuihj/scope-siggen
- 参考手册: RM0090 (STM32F4 Reference Manual)
- HAL 库手册: UM1725 (STM32Cube HAL User Manual)
- 开发工具: Keil MDK-ARM V5.32, STM32CubeMX, ST-LINK

---

**最后更新**: 2026-07-02
**版本**: v1.3.0
