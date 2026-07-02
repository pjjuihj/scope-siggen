# 示波器数据链路与显示渲染修复设计

**日期:** 2026-06-28
**状态:** 已批准
**范围:** oscilloscope.c, stm32f4xx_it.c, display.c, uart_protocol.c

---

## 1. 问题描述

示波器波形不工作，存在 4 层问题：

| 层 | 问题 | 影响 |
|---|------|------|
| 硬件配置 | Osc_ApplyConfig() 可能没生效，TIM8 用默认值 | 采样率不对 |
| 采样质量 | ADC 和 DAC 不同步，ADC 在 DAC 更新瞬间采样 | 值不稳定 |
| 数据处理 | 没有触发对齐，每帧起始相位不同 | 波形漂移 |
| 显示渲染 | EMA 自适应缩放过度压缩 + 单点绘制 | 看起来像水平线 |

本设计修复第 1 层（数据链路竞争）和第 4 层（显示渲染）。第 2、3 层（采样同步、触发对齐）需要单独设计。

### 1.1 竞争点清单（第 1 层：数据链路）

| # | 位置 | 问题 | 严重度 |
|---|------|------|--------|
| 1 | ISR → Osc_HandleAdcData | 回调覆盖 process_ptr，任务处理慢时读到新半区数据 | 高 |
| 2 | ISR 写 process_ptr/process_len | 分两步写，非原子，任务可能读到指针新+长度旧 | 高 |
| 3 | UART_HandleStreamDac | 直接读 adc_buffer 活缓冲区，无任何同步 | 中 |
| 4 | Display_DrawWaveform | 存裸指针不拷贝，调用者缓冲区可能被 DMA 覆盖 | 中 |
| 5 | ISR 回调 | 不检查上一帧是否已处理完，静默覆盖 | 中 |

### 1.2 显示渲染问题（第 4 层）

**屏幕布局 (128×64 OLED):**
```
Page 0 (y=0-7):   [频率]              [电压]        ← 测量值
Page 1 (y=8-15):  ┌─网格─┬─波形区─┬─网格─┐
Page 2 (y=16-23): │      │        │      │
Page 3 (y=24-31): │ 网格 │ 波形区 │ 网格 │           ← 波形画在这里
Page 4 (y=32-39): │      │        │      │              (y=10~47, 38像素高)
Page 5 (y=40-47): └──────┴────────┴──────┘
Page 6 (y=48-55): [状态: Running]                     ← 状态栏
Page 7 (y=56-63): [A:sine]           [T:1ms] [500mV] ← 信息栏
```

**Draw_Waveform 渲染流程 (display.c:148):**
```
输入: waveform_data[0..len-1]  (uint16_t, 0~4095)

1. 计算数据范围: dmin, dmax
2. EMA 自适应缩放:
   ├── mean = Σdata[i] / len
   ├── stddev = √(Σ(data[i]-mean)² / len)
   ├── cur_min = max(mean - 3*stddev, 0)     ← ±3σ 裁剪杂波
   ├── cur_max = min(mean + 3*stddev, 4095)
   ├── ema_min = ema_min * 0.7 + cur_min * 0.3  ← 跨帧平滑
   ├── ema_max = ema_max * 0.7 + cur_max * 0.3
   └── ema_range = ema_max - ema_min
3. X轴降采样 (128像素): 每列取平均值
4. Y轴映射 (38像素): y = (avg - ema_min) * 38 / ema_range
5. 画点: OLED_DrawPoint(x, py, 1)  ← 只画单点，不连线
```

**显示层问题清单:**

| # | 问题 | 原因 | 影响 |
|---|------|------|------|
| D1 | EMA 范围过小时波形被压缩 | `ema_range` 可能 < 20 甚至 = 0，38 像素中只用几像素 | 小信号/DC 看起来像水平线 |
| D2 | EMA 冷启动从第一帧数据初始化 | 如果第一帧恰好是 DC/小信号，后续帧从极小范围开始 | 需要很多帧才能"拉伸"到正确范围 |
| D3 | ±3σ 裁剪对 DC+小信号过度 | DC=2048, 小信号幅度=50 → 3σ≈105，显示范围=2048±105 | 波形被压缩到范围中间 |
| D4 | 单点绘制不连线 | 相邻像素列 y 值差距大时呈散点 | 剧烈变化信号看起来像噪声 |

**数值示例（D1 问题）:**
```
数据: [2045, 2048, 2046, 2047, 2045, ...]  (几乎 DC)
mean = 2046, stddev ≈ 1
cur_min = 2043, cur_max = 2049, cur_range = 6
ema_range = 6 (< 20, 用实际值)

Y映射: (2047 - 2043) * 38 / 6 = 25 像素
       (2045 - 2043) * 38 / 6 = 12 像素
→ 波形在 y=22~35 之间，只有 13 像素变化
→ 看起来几乎是一条线
```

---

## 2. 设计方案

采用 **方案 B：独立处理缓冲区 + 信号量同步**。

### 2.1 数据流（改动后）

```
DMA → adc_buffer[1024]    ← 仅 DMA 写入，任务不直接读取做处理
         ↓ ISR 回调（临界区内 memcpy）
    proc_buffer[512]       ← 独立处理缓冲区
         ↓ xSemaphoreGiveFromISR
    Oscilloscope_Task      ← xSemaphoreTake → 从 proc_buffer 处理
         ↓ Display_UpdateScopeEx（已有 memcpy 到 waveform_buf）
    Display_Task           ← 从 waveform_buf 渲染
```

### 2.2 核心原则

- **adc_buffer 是纯硬件缓冲区** — 任务永不直接读取它做处理
- **proc_buffer 是任务数据源** — ISR 写入，任务读取，通过信号量同步
- **临界区保护短小** — 仅保护 memcpy + data_ready 标志设置
- **丢帧可检测** — 信号量已给出时 ISR 跳过拷贝，递增计数器

---

## 3. 详细改动

### 3.1 新增全局变量（oscilloscope.c）

```c
/* 独立处理缓冲区 — ISR 拷贝半区数据到此处 */
static uint16_t proc_buffer[OSC_DEFAULT_BUFFER_SIZE / 2];

/* 二值信号量 — ISR→Task 同步 */
static SemaphoreHandle_t data_semaphore = NULL;

/* 数据就绪标志 — ISR 设置，任务清除 */
static volatile bool data_ready = false;

/* 丢帧计数 */
static volatile uint32_t drop_count = 0;
```

**RAM 开销:** +1028 字节（proc_buffer 1024B + 信号量 ~4B）

### 3.2 废弃的全局变量

- `process_ptr` — 不再使用（ISR 直接拷贝到 proc_buffer）
- `process_len` — 不再使用

### 3.3 ISR 回调改动（stm32f4xx_it.c）

两个回调共用统一处理逻辑：

```c
/**
 * @brief ADC DMA 半区完成处理（ISR 上下文）
 * @param src 源数据指针（半区起始地址）
 *
 * 在临界区内将数据拷贝到 proc_buffer，然后通过信号量通知任务。
 * 如果上一帧尚未被任务取走（data_ready == true），跳过并计数。
 */
static void Osc_IsrProcessHalf(uint16_t *src)
{
    UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();

    if (data_ready) {
        drop_count++;
        taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
        return;
    }

    uint16_t half = osc_config_buffer_size / 2;
    memcpy(proc_buffer, src, half * sizeof(uint16_t));
    data_ready = true;

    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);

    if (data_semaphore != NULL) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(data_semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        Osc_IsrProcessHalf(&adc_buffer[0]);
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        uint16_t half = osc_config_buffer_size / 2;
        Osc_IsrProcessHalf(&adc_buffer[half]);
    }
}
```

### 3.4 Oscilloscope_Task 改动（oscilloscope.c）

**信号量初始化：**
```c
data_semaphore = xSemaphoreCreateBinary();
```

**任务主循环：**
```c
void Oscilloscope_Task(void *argument)
{
    // ... 初始化不变 ...

    for (;;) {
        if (xSemaphoreTake(data_semaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
            /* proc_buffer 已被 ISR 填充，直接处理 */
            Osc_HandleAdcData(proc_buffer, osc_config.buffer_size / 2);

            /* 标记已处理，允许 ISR 写入下一帧 */
            taskENTER_CRITICAL();
            data_ready = false;
            taskEXIT_CRITICAL();
        } else {
            Display_ShowStatus("Waiting...");
        }
    }
}
```

**Osc_HandleAdcData 签名改动：**
```c
/* 改前 */
static void Osc_HandleAdcData(void);

/* 改后 — 接收明确的缓冲区指针和长度 */
static void Osc_HandleAdcData(const uint16_t *buf, uint16_t len);
```

函数内部逻辑不变，只是不再从全局 `process_ptr` 读取，改用参数 `buf`。

### 3.5 Display_DrawWaveform 修复（display.c）

改为内部拷贝，与 `Display_UpdateScopeEx` 行为一致：

```c
ErrorCode_t Display_DrawWaveform(const uint16_t *data, uint16_t len)
{
    if (!initialized || data == NULL || len == 0) return ERR_INVALID_PARAM;
    if (len > WAVEFORM_BUF_MAX) len = WAVEFORM_BUF_MAX;

    OLED_Lock();
    memcpy(waveform_buf, data, len * sizeof(uint16_t));
    waveform_data  = waveform_buf;
    waveform_len   = len;
    waveform_valid = true;
    current_page   = PAGE_OSCOPE;
    dirty          = true;
    OLED_Unlock();
    return ERR_OK;
}
```

### 3.6 UART 流输出修复（uart_protocol.c + oscilloscope.c）

**新增 API（oscilloscope.c）：**
```c
ErrorCode_t Oscilloscope_GetSnapshot(uint16_t *out, uint32_t max_len, uint32_t *actual_len)
{
    if (out == NULL || max_len == 0) return ERR_INVALID_PARAM;

    OSC_LOCK();  /* 互斥锁保护，避免关中断过久 */
    uint16_t half = osc_config.buffer_size / 2;
    uint32_t copy_len = (half < max_len) ? half : max_len;
    memcpy(out, proc_buffer, copy_len * sizeof(uint16_t));
    OSC_UNLOCK();

    if (actual_len) *actual_len = copy_len;
    return ERR_OK;
}
```

**UART_HandleStreamDac 改动：**
```c
static void UART_HandleStreamDac(void)
{
    uint16_t adc_snap[OSC_DEFAULT_BUFFER_SIZE / 2];
    uint32_t adc_len = 0;
    Oscilloscope_GetSnapshot(adc_snap, sizeof(adc_snap)/sizeof(adc_snap[0]), &adc_len);

    const uint16_t *dac_buf = SignalGen_GetWaveformBuffer();
    uint32_t dac_size = SignalGen_GetWaveformBufferSize();

    // ... 后续发送逻辑改用 adc_snap / adc_len ...
}
```

### 3.7 Oscilloscope_GetAdcBuffer 废弃

此函数返回 `adc_buffer` 裸指针，已无安全使用场景。标记 deprecated 或删除。UART 流输出改用 `Oscilloscope_GetSnapshot`。

### 3.8 Draw_Waveform 渲染修复（display.c）

**修复 D1/D2：EMA 范围下限保护 + 冷启动兜底**

```c
/* 改前 */
if (ema_range < EMA_MIN_RANGE) {
    ema_min = cur_min;
    ema_max = cur_max;
    ema_range = cur_range;          // 可能 = 0，波形被压缩
}

/* 改后 */
if (ema_range < EMA_MIN_RANGE) {
    /* 范围太小时，以 mean 为中心扩展到最小范围 */
    uint16_t center = (cur_min + cur_max) / 2;
    uint16_t half = EMA_MIN_RANGE / 2;
    ema_min = (center > half) ? center - half : 0;
    ema_max = ema_min + EMA_MIN_RANGE;
    if (ema_max > ADC_RESOLUTION) {
        ema_max = ADC_RESOLUTION;
        ema_min = ema_max - EMA_MIN_RANGE;
    }
    ema_range = EMA_MIN_RANGE;
}
```

效果：即使数据几乎 DC，波形也会在 38 像素高度中占据合理范围，不会被压缩成水平线。

**修复 D3：±3σ 裁剪范围保护**

```c
/* 改前 */
uint16_t cur_min = (mean > 3 * stddev) ? (uint16_t)(mean - 3 * stddev) : 0;
uint16_t cur_max = (uint16_t)(mean + 3 * stddev);

/* 改后 — 确保裁剪后范围不小于 MIN_RANGE */
uint16_t cur_min = (mean > 3 * stddev) ? (uint16_t)(mean - 3 * stddev) : 0;
uint16_t cur_max = (uint16_t)(mean + 3 * stddev);
if (cur_max > ADC_RESOLUTION) cur_max = ADC_RESOLUTION;
uint16_t cur_range = cur_max - cur_min;
if (cur_range < EMA_MIN_RANGE) {
    uint16_t center = (cur_min + cur_max) / 2;
    uint16_t half = EMA_MIN_RANGE / 2;
    cur_min = (center > half) ? center - half : 0;
    cur_max = cur_min + EMA_MIN_RANGE;
    cur_range = EMA_MIN_RANGE;
}
```

**修复 D4：相邻点连线**

```c
/* 改前 */
OLED_DrawPoint((u8)x, (u8)py, 1);  // 单点

/* 改后 — 相邻列之间画连线 */
static uint16_t prev_py = 0;
if (x > 0 && ema_range >= EMA_MIN_RANGE) {
    OLED_DrawLine((u8)(x-1), (u8)prev_py, (u8)x, (u8)py, 1);
} else {
    OLED_DrawPoint((u8)x, (u8)py, 1);
}
prev_py = py;
```

注意：`prev_py` 需要改为 `static` 局部变量或文件级变量（与 `ema_initialized` 同级）。

**修复 D2 附加：EMA 冷启动保护**

```c
/* 改前 */
if (!ema_initialized) {
    ema_min = cur_min; ema_max = cur_max; prev_range = cur_range; ema_initialized = true;
}

/* 改后 — 首帧也强制使用最小范围 */
if (!ema_initialized) {
    if (cur_range < EMA_MIN_RANGE) {
        uint16_t center = (cur_min + cur_max) / 2;
        uint16_t half = EMA_MIN_RANGE / 2;
        ema_min = (center > half) ? center - half : 0;
        ema_max = ema_min + EMA_MIN_RANGE;
    } else {
        ema_min = cur_min;
        ema_max = cur_max;
    }
    prev_range = cur_range;
    ema_initialized = true;
}
```

---

## 4. 修复验证

### 4.1 数据链路验证

| 竞争点 | 修复方式 | 验证方法 |
|--------|----------|----------|
| #1 回调覆盖 | data_ready 标志 + 丢帧计数 | 查询 drop_count，应为 0 或少量 |
| #2 非原子更新 | 临界区内完成拷贝+标志设置 | 不再有 process_ptr/process_len |
| #3 UART 直读活缓冲 | Oscilloscope_GetSnapshot 拷贝 | 串口流输出无撕裂 |
| #4 Display_DrawWaveform 裸指针 | 内部 memcpy | 波形显示无撕裂 |
| #5 无丢帧检测 | drop_count 全局变量 | UART 命令查询 |

### 4.2 显示渲染验证

| 问题 | 修复方式 | 验证方法 |
|------|----------|----------|
| D1 EMA 范围过小 | 强制最小范围 EMA_MIN_RANGE=20 | DC 信号输入，波形应占据合理高度 |
| D2 EMA 冷启动 | 首帧也强制最小范围 | 冷启动后第一帧波形不被压缩 |
| D3 ±3σ 裁剪过度 | 裤剪后范围保护 | DC+小信号叠加，波形应可见 |
| D4 单点不连线 | OLED_DrawLine 连线 | 快速变化信号呈连续曲线 |

---

## 5. 不变部分

- **Display_UpdateScopeEx** — 已有 memcpy，无需改动
- **Display_Task** — dirty 标志 + 10Hz 巡检机制不变
- **OLED 驱动层** — 不涉及
- **信号发生器模块** — 不涉及
- **DMA 配置** — 循环模式 + 半传输/全传输回调不变
- **Render_Frame / Render_Scope** — 渲染流程不变，仅 Draw_Waveform 内部逻辑改动

---

## 6. 后续工作（不在本设计范围内）

| 层 | 问题 | 方向 |
|---|------|------|
| 采样质量 | ADC/DAC 不同步 | 参考 project_led：同一中断里同时做 DAC+ADC |
| 数据处理 | 无触发对齐 | 软件触发：找到过零点后对齐帧起始 |
