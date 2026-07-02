# 2026-06-27 反思：为什么这么低效

## 核心问题

3 小时修了 8 个 bug，但其中 **6 个是项目创建时就存在的配置缺陷**。不是调试难，而是**代码从没跑过**。

---

## 1. 根因：AI 生成代码后没有验证

| 阶段 | 实际做了什么 | 应该做什么 |
|------|-------------|-----------|
| 项目生成 | CubeMX + AI 生成了全部代码 | 生成后立即编译烧录 |
| 模块开发 | AI 逐模块添加功能代码 | 每加一个模块就验证硬件 |
| 提交前 | 没有测试 | 至少跑一次自检 |

**所有 8 个 bug 都可以在项目创建时通过一次编译+烧录+串口自检发现。**

---

## 2. 逐个 bug 反思

### BUG 1: HAL_ADC_MspInit 缺少 DMA 配置

**为什么没配好**: CubeMX 生成 ADC 时可能没选 DMA 模式，AI 手动补代码时只补了 GPIO 和 NVIC，漏了 DMA。

**教训**: `HAL_ADC_Start_DMA()` 依赖 `__HAL_LINKDMA`，这在 CubeMX 勾选 DMA 时会自动生成。手动写 MSP 必须完整对照 CubeMX 输出。

### BUG 2: TIM8 TRGO = RESET

**为什么没配好**: CubeMX 默认 TRGO 是 RESET（不输出）。需要手动改为 UPDATE 才能触发 ADC。

**教训**: 定时器触发源是"配置项"不是"默认值"。生成代码后必须检查 `MasterOutputTrigger`。

### BUG 3: DMAContinuousRequests = DISABLE

**为什么没配好**: CubeMX 默认是 DISABLE。DMA 循环采集必须设为 ENABLE。

**教训**: 这个参数名字就是"DMA 连续请求"，和"循环采集"直接相关。对 HAL 参数不熟就会漏。

### BUG 4: 从空 ring buffer 读数据

**为什么没配好**: 代码架构设计了 ring buffer 做数据中转，但 DMA 直接写 adc_buffer，没人把数据搬到 ring buffer。

**教训**: 架构设计和实际实现脱节。设计了两层数据通路（DMA→ring buffer→处理），但只实现了第一层（DMA→adc_buffer）。

### BUG 5: OLED_DrawLine delta_y 笔误

**为什么没配好**: 这是第三方 OLED 驱动库的 bug，`delta_y = -delta_x` 应该是 `delta_y = -delta_y`。一个字符的差异。

**教训**: 第三方库不能盲信。关键绘图函数（DrawLine/DrawRect）应该有简单的单元测试。

### BUG 6: 两个任务竞争 OLED I2C

**为什么没配好**: Display_Task 和 Oscilloscope_Task 都直接调用 OLED 函数，没有考虑 RTOS 并发。

**教训**: FreeRTOS 项目中，任何共享外设（I2C/SPI/UART）必须有互斥保护。这不是"优化"，是"必须"。

### BUG 7: TIM5 未启动

**为什么没配好**: `SignalGen_Start()` 调用了 `HAL_DAC_Start_DMA()` 但没有调用 `HAL_TIM_Base_Start(&htim5)`。DAC 配置为 TIM5 触发，但没人启动定时器。

**教训**: HAL 的"启动"是分层的：时钟→定时器→外设→DMA。每层都要显式启动，HAL 不会自动帮你做。

### BUG 8: osMutexNew 在内核启动前调用

**为什么没配好**: `Display_Init()` 在 `main()` 中 `osKernelStart()` 之前调用，此时 RTOS 对象 API 不可用。

**教训**: RTOS 对象（mutex/semaphore/queue）只能在调度器运行后创建。初始化代码的执行顺序很重要。

---

## 3. 模式总结

### 3.1 三个反复出现的错误模式

| 模式 | 具体表现 | 出现次数 |
|------|---------|---------|
| **分层启动遗漏** | 启动了外设但没启动触发源/时钟/DMA | 3 次 (BUG 2,3,7) |
| **架构与实现脱节** | 设计了数据通路但没完整实现 | 2 次 (BUG 4,6) |
| **默认值未检查** | CubeMX 默认值不适合项目需求 | 2 次 (BUG 2,3) |
| **第三方代码未验证** | OLED 驱动有 bug 但没测试 | 1 次 (BUG 5) |

### 3.2 为什么会这样？

**因为代码是"写"出来的，不是"跑"出来的。**

AI 生成代码的能力很强，但 AI 无法：
- 烧录到真实硬件
- 用示波器测量引脚
- 观察 OLED 实际显示
- 用万用表测电压

所以 AI 生成的代码一定是"理论上正确"但"实际上可能有配置遗漏"。

---

## 4. 改进方案

### 4.1 项目创建时

```
CubeMX 生成 → 立即编译 → 烧录 → 串口自检 → 再开始写应用代码
```

不要一口气生成所有模块代码再测试。每个外设（ADC/DAC/I2C/TIM）生成后单独验证。

### 4.2 新增外设时

检查清单：
- [ ] 时钟使能 (`__HAL_RCC_xxx_CLK_ENABLE`)
- [ ] GPIO 配置（模式、上下拉）
- [ ] DMA 配置（流、通道、方向、链接）
- [ ] 触发源配置（定时器 TRGO）
- [ ] 定时器启动 (`HAL_TIM_Base_Start`)
- [ ] 外设启动 (`HAL_xxx_Start` / `HAL_xxx_Start_DMA`)
- [ ] 中断优先级（FreeRTOS 兼容）

### 4.3 RTOS 项目时

- [ ] 共享外设有互斥保护
- [ ] RTOS 对象在调度器启动后创建
- [ ] 中断优先级 ≤ `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`
- [ ] 任务栈大小足够（512*4 = 2KB 最小）

### 4.4 调试时

**先硬件后软件**：
1. 串口读寄存器值（CR/DOR/CNT）
2. 判断硬件是否在工作
3. 再看软件逻辑

不要在软件层面猜来猜去，直接读硬件状态。

---

## 5. 时间分析

| 阶段 | 时间 | 占比 | 效率 |
|------|------|------|------|
| OLED 架构重写 | 09:03~10:05 | 62min | ❌ 一次改太多 |
| ADC 配置修复 | 10:05~10:42 | 37min | ❌ 不知道该查什么 |
| 波形渲染调试 | 10:42~11:15 | 33min | ⚠️ 走了弯路 |
| DAC 链路修复 | 11:15~11:47 | 32min | ✅ 串口诊断有效 |
| 功能扩展 | 11:47~12:02 | 15min | ✅ 正常 |
| 串口刷屏修复 | 12:15~12:25 | 10min | ✅ 快速定位 |
| 命令解析修复 | 12:25~12:35 | 10min | ✅ 快速定位 |
| OLED 页面切换 | 12:35~12:45 | 10min | ⚠️ 调试手段不足 |
| 波形重叠修复 | 12:45~12:55 | 10min | ✅ 快速定位 |
| 波形频率适配 | 12:55~13:10 | 15min | ⚠️ 分析不充分 |
| TIM5 预分频器 | 13:10~13:20 | 10min | ❌ 引入死机 |
| 回退恢复 | 13:20~13:30 | 10min | ✅ git 回退 |

**高效时段**: 12:15~12:55（问题明确，快速修复）
**低效时段**: 13:10~13:20（改太多代码导致死机）

## 6. 新增教训

### 6.1 串口刷屏

**问题**: Display_ShowVoltage/Frequency 每次调用都往串口发数据，示波器任务每 20ms 调用一次
**教训**: 显示模块只负责 OLED 显示，不要自动发串口数据。需要串口输出时用专门的命令（如 `stream on`）

### 6.2 命令解析失败

**问题**: `cmd[0]` 未初始化，收到 `\r\n` 时返回空字符串
**教训**: 所有缓冲区使用前必须初始化。串口接收要考虑各种输入情况（空行、回车、换行）

### 6.3 波形频率适配

**问题**: DAC 频率 1000Hz，ADC 采样率 50kHz，一个缓冲区包含 20 个周期
**教训**: 波形显示要考虑采样率和信号频率的关系。一个缓冲区应包含 1-2 个完整周期

### 6.4 TIM5 预分频器

**问题**: 动态修改预分频器后需要生成更新事件才能生效
**教训**: STM32 定时器预分频器是影子寄存器，需要 `EGR = TIM_EGR_UG` 或等待下一个更新事件

### 6.5 代码回退

**问题**: 改动过多导致死机
**教训**: 每次小改动后都要编译测试。不要一次性修改多个文件。用 `git checkout HEAD -- file` 可以快速回退单个文件

---

## 6. 初始化检查清单（必须逐条确认）

所有 8 个 bug 都是初始化问题。以下是 STM32 HAL 外设完整的初始化调用链，**每一步都要显式调用，HAL 不会自动帮你做**：

### 6.1 通用外设初始化链

```
时钟使能 → GPIO配置 → DMA配置 → 触发源配置 → 外设初始化 → 外设启动 → DMA启动
```

### 6.2 ADC + DMA 检查清单

- [ ] `__HAL_RCC_ADC1_CLK_ENABLE()` — ADC 时钟
- [ ] `__HAL_RCC_GPIOA_CLK_ENABLE()` — GPIO 时钟
- [ ] `__HAL_RCC_DMA2_CLK_ENABLE()` — DMA 时钟
- [ ] GPIO 设为 `GPIO_MODE_ANALOG`
- [ ] DMA 流/通道/方向/对齐 配置正确
- [ ] `__HAL_LINKDMA(hadc, DMA_Handle, hdma_xxx)` — DMA 链接到外设
- [ ] DMA 中断 NVIC 使能
- [ ] 定时器 TRGO 设为 `TIM_TRGO_UPDATE`（不是 RESET）
- [ ] `DMAContinuousRequests = ENABLE`（循环采集必须）
- [ ] `HAL_TIM_Base_Start(&htim)` — 启动触发定时器
- [ ] `HAL_ADC_Start_DMA()` — 启动 ADC DMA

### 6.3 DAC + DMA 检查清单

- [ ] `__HAL_RCC_DAC_CLK_ENABLE()` — DAC 时钟
- [ ] `__HAL_RCC_DMA1_CLK_ENABLE()` — DMA 时钟
- [ ] GPIO 设为 `GPIO_MODE_ANALOG`
- [ ] `__HAL_LINKDMA(hdac, DMA_Handle1, hdma_xxx)` — DMA 链接
- [ ] DAC 触发源配置正确（TSEL 位）
- [ ] `HAL_TIM_Base_Start(&htim)` — 启动触发定时器（**最容易漏**）
- [ ] `HAL_DAC_Start_DMA()` — 启动 DAC DMA

### 6.4 FreeRTOS 检查清单

- [ ] RTOS 对象（mutex/semaphore/queue）在 `osKernelStart()` 之后创建
- [ ] 共享外设（I2C/SPI/UART）有互斥保护
- [ ] 中断优先级 ≤ `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`（才能调用 FromISR API）
- [ ] 任务栈 ≥ 512*4 = 2KB

### 6.5 验证方法

不要猜，直接读寄存器：

| 验证项 | 读什么 | 正常值 |
|--------|--------|--------|
| 定时器是否运行 | `TIMx->CNT` | 值在变化 |
| DMA 是否在搬数据 | `DAC->DHR12R1` / `ADC->DR` | 非零且变化 |
| 外设是否使能 | `DAC->CR` bit 0 | 1 |
| 触发源是否正确 | `DAC->CR` TSEL 位 | 对应定时器 |
| DMA 流是否启用 | `DMAx_StreamN->CR` bit 0 | 1 |

---

## 7. 结论

> **最大的教训：所有 bug 都是初始化问题。AI 生成代码时没有逐条检查初始化调用链的完整性。**

以后新增外设时：
1. **先列完整初始化调用链**（时钟→GPIO→DMA→触发源→初始化→启动）
2. **逐条确认每一步都有对应代码**
3. **编译后立即烧录，串口读寄存器验证硬件状态**
4. **再写应用层代码**

不要堆到最后才测试，每加一个外设就验证一次。

---

## 8. 第二轮修复（配置系统连接 + 波形显示优化）

### 8.1 发现的新问题

| 问题 | 根因 | 修复 |
|------|------|------|
| 配置系统与硬件脱节 | `Config_Get*()` 从未被调用，`Set*()` 不更新定时器 | Init 读 Config，Set* 动态更新 TIM + 同步 Config |
| 时钟频率硬编码 84MHz | 不同时钟配置下会算错 | 用 `HAL_RCC_GetPCLKxFreq()` 动态获取 |
| Config 写入竞态 | `Config_Set*()` 在锁外调用，可能写入脏数据 | 锁内拷贝 `cfg_copy`，锁外写入 |
| `osc_config_buffer_size` 非 volatile | ISR 和任务共享但未加修饰 | 声明和 extern 都加 `volatile` |
| 波形显示尖刺 | 降采样取单点，多周期时不同相位连成尖刺 | 改用 min/max 包络算法 |
| 显示尾部数据丢失 | `step = len / OLED_WIDTH` 整除截断 | 向上取整 `(len + OLED_WIDTH - 1) / OLED_WIDTH` |
| ApplyConfig 除零风险 | `frequency=0` 或 `sample_rate=0` 时除零 | 入口参数校验 |

### 8.2 新增教训

**教训 9: 配置系统必须两头接通**

设计了 Config API 但没有在 Init 和 Set* 中调用，等于没设计。配置系统的价值在于：
- Init 时从 Config 读取 → 确保 Flash 保存的配置生效
- Set* 时写回 Config → 确保运行时修改能持久化

**教训 10: 时钟频率不要硬编码**

STM32 的定时器时钟取决于 APBx 分频器。硬编码 `84000000` 在时钟配置变更时会静默失效。用 `HAL_RCC_GetPCLKxFreq()` + 分频器检查是正确做法。

**教训 11: 互斥锁内外的操作要分清**

`Config_Set*()` 是 memcpy 操作，很快。但放在锁外时，另一个任务可能修改了源数据。正确做法：锁内拷贝，锁外写入。

**教训 12: 波形显示用 min/max 包络**

低分辨率屏幕上显示高采样率波形时，取单点会因混叠产生尖刺。min/max 包络是数字示波器的标准做法。

### 8.3 代码审查驱动的修复

本轮通过 3 轮代码审查发现并修复了问题：

| 审查轮次 | 发现 | 修复 |
|---------|------|------|
| 第 1 轮 | 时钟硬编码、Config 竞态、未 volatile | 提取 GetTimerClock/ApplyConfig 函数 |
| 第 2 轮 | 除零风险、uint32_t 下溢 | ApplyConfig 入口防御 |
| 第 3 轮 | 整除截断 | 向上取整 |

**代码审查比写代码更重要。** 每轮审查都发现了前一轮遗漏的问题。

---

## 9. 第三轮修复（信号发生器代码审查）

### 9.1 发现的问题

| 严重度 | 问题 | 根因 | 修复 |
|--------|------|------|------|
| **Critical** | TIM5 运行时修改寄存器 | `__HAL_TIM_SET_PRESCALER` 在定时器运行时修改，可能产生 DAC 毛刺 | 先停止 TIM5，修改后再启动 |
| **High** | 锁粒度不一致 | 多次获取/释放 mutex，中间有 HAL 调用 | 统一为：锁内更新配置 → 锁外生成波形 → 锁内应用硬件 |
| **High** | 锁内浮点运算 | `SigGen_GenerateWaveform`（含 sinf）在 mutex 内执行 | 移到锁外执行 |
| **Medium** | Stop 未停止 TIM5 | `SignalGen_Stop` 只停 DMA，不停定时器 | 添加 `HAL_TIM_Base_Stop(&htim5)` |

### 9.2 修复模式

```c
/* 正确的 setter 模式 */
ErrorCode_t SignalGen_SetXxx(type) {
    /* 1. 参数校验 */
    if (invalid) return ERR;

    /* 2. 锁内：更新配置 + 拷贝 */
    SIGGEN_LOCK();
    siggen_config.xxx = value;
    SigGenConfig_t cfg_copy = siggen_config;
    SIGGEN_UNLOCK();

    /* 3. 锁外：波形生成（含浮点运算） */
    SigGen_GenerateWaveform();

    /* 4. 锁内：应用硬件配置 */
    SIGGEN_LOCK();
    SigGen_ApplyConfig();  /* 内部会停止/启动 TIM5 */
    SIGGEN_UNLOCK();

    /* 5. 锁外：同步到 Config */
    Config_SetSigGenConfig(&cfg_copy);
}
```

### 9.3 新增教训

**教训 13: 定时器寄存器修改前必须停止定时器**

运行时修改预分频器/周期寄存器可能产生一个错误的触发脉冲，导致 DAC 输出毛刺。正确做法：
```c
HAL_TIM_Base_Stop(&htim5);
__HAL_TIM_SET_PRESCALER(&htim5, psc);
__HAL_TIM_SET_AUTORELOAD(&htim5, arr);
htim5.Instance->EGR = TIM_EGR_UG;  /* 生成更新事件 */
HAL_TIM_Base_Start(&htim5);
```

**教训 14: 互斥锁内不要做耗时操作**

浮点运算（sinf/cosf）、波形生成、UART 发送等耗时操作不应在 mutex 内执行。会导致：
- 其他任务等待时间过长
- 优先级反转风险增加
- 实时性下降

正确做法：锁内只做配置读写和状态拷贝，耗时操作在锁外执行。

**教训 15: Stop 函数必须释放所有资源**

`SignalGen_Stop` 只停了 DMA，没停 TIM5。定时器继续运行可能产生意外的 DAC 触发。Stop 函数应该与 Start 函数对称：Start 开启的所有资源，Stop 都要关闭。

---

## 10. 第四轮修复（ADC 硬件触发配置）

### 10.1 问题

ADC 读数异常：
- 软件触发+连续模式：采样率太高（~219kHz），噪声放大，波形杂乱
- 外部触发模式：DMA 缓冲区全是 4095，ADC 没有实际转换

### 10.2 根因

| 问题 | 根因 |
|------|------|
| ADC 不转换 | `EOCSelection = ADC_EOC_SEQ_CONV` 配置错误 |
| 触发丢失 | 启动顺序错误：先启动 TIM8，后启动 ADC DMA |
| 采样率不受控 | 使用软件触发+连续模式，ADC 自由运行 |

### 10.3 参考 GitHub 项目

搜索 `mervinnguyen/adc-dma-driver`（STM32F411 ADC+DMA 驱动），找到正确配置：

```c
// ADC 配置
hadc1.Init.ContinuousConvMode = DISABLE;   // 单次模式
hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T8_TRGO;
hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;  // 关键！

// 启动顺序（先 ADC DMA，后 TIM）
HAL_ADC_Start_DMA(&hadc1, buffer, size);   // 先启动，等待触发
HAL_TIM_Base_Start(&htim8);                // 后启动，产生触发
```

### 10.4 新增教训

**教训 16: ADC 外部触发模式的 4 个必要配置**

使用 TIM 触发 ADC 时，必须同时满足：
1. `ContinuousConvMode = DISABLE`（单次模式，受触发控制）
2. `ExternalTrigConvEdge = RISING`（上升沿触发）
3. `ExternalTrigConv = TIMx_TRGO`（指定触发源）
4. `EOCSelection = ADC_EOC_SINGLE_CONV`（每次转换完成触发 DMA）

**教训 17: 启动顺序决定成败**

ADC DMA 必须在 TIM 之前启动。原因：
- `HAL_ADC_Start_DMA()` 配置 ADC 等待触发
- `HAL_TIM_Base_Start()` 开始产生触发信号
- 如果 TIM 先启动，前几个触发信号会丢失，ADC 可能进入异常状态

**教训 18: 遇到配置问题先查 GitHub 类似项目**

花 2 小时试错不如花 5 分钟搜索 GitHub。搜索关键词：`STM32F4 ADC DMA TIM trigger example`，找到已验证的配置直接用。

### 10.5 时间分析

| 时间 | 事件 | 耗时 | 效率 |
|------|------|------|------|
| 14:05 | ADC 直接读=0 | 15min | ❌ 不知道查什么 |
| 14:20 | 改为连续模式，杂波 | 15min | ❌ 试错 |
| 14:35 | 查 GitHub 示例 | 15min | ✅ 找到正确配置 |
| 14:50 | 应用修复 | 15min | ✅ 快速修复 |

**教训**：遇到硬件配置问题，先查 GitHub 类似项目，不要自己试错。

---

## 11. 深层反思：为什么技术规范给了错误的 CubeMX 配置

### 11.1 根因：AI 没有"硬件经验"

AI 生成代码的依据是：
- HAL 函数签名和注释
- 训练数据中的代码片段
- CubeMX 的默认值

但 AI 不知道：
- `TIM_TRGO_RESET` 实际上不输出触发信号
- `DMAContinuousRequests=DISABLE` 会导致 DMA 只跑一轮
- 启动顺序错了会丢失触发
- `EOCSelection` 的不同值对 DMA 请求时机的影响

**这些知识来自实际硬件调试，不是文档。**

### 11.2 CubeMX 默认值有陷阱

| 配置项 | CubeMX 默认值 | 实际需要 | 后果 |
|--------|--------------|---------|------|
| `TIM_TRGO` | RESET | UPDATE | 不输出触发信号 |
| `DMAContinuousRequests` | DISABLE | ENABLE | DMA 只跑一轮 |
| `EOCSelection` | SINGLE_CONV | 视场景而定 | DMA 请求时机错误 |

**AI 直接用了默认值，没有检查是否适合具体场景。**

### 11.3 没有"验证驱动开发"

AI 的工作方式：
```
需求 → 生成代码 → 编译通过 → 交付
```

正确的方式：
```
需求 → 查参考手册/示例 → 生成代码 → 编译 → 烧录 → 验证 → 修复
```

**AI 跳过了"验证"这一步，因为 AI 无法烧录和测量。**

### 11.4 效率对比

| 方法 | 耗时 | 结果 |
|------|------|------|
| AI 直接生成代码 | 5 分钟 | 编译通过，但硬件不工作 |
| 自己试错调试 | 2 小时 | 改了 6 次，还是不对 |
| 查 GitHub 类似项目 | 5 分钟 | 找到正确配置，一次修复 |

**效率差 24 倍。**

### 11.5 根本解决方案

**以后生成 STM32 代码时：**

1. **先查 GitHub 类似项目** — 搜索 `STM32F4 ADC DMA TIM trigger example`，找到已验证的配置
2. **对照参考手册** — 检查每个配置项的含义，不盲信默认值
3. **生成最小可运行示例** — 先让最简单的功能跑通，再逐步添加
4. **提供验证清单** — 告诉用户烧录后应该检查什么（读哪些寄存器，量哪些引脚）

**给用户的建议：**

> AI 生成的嵌入式代码，必须在真实硬件上验证。配置类 bug 不是"调试"能解决的，是"测试"能预防的。
>
> **每次 AI 生成新模块后：**
> 1. 编译烧录
> 2. 串口读关键寄存器值
> 3. 万用表量关键引脚电压
> 4. 确认硬件工作正常后再写应用层代码
>
> **不要信任 AI 生成的 CubeMX 配置，要对照参考手册逐项检查。**

---

## 12. 第五轮修复（ADC 采样时间 + TIM8 频率配置）

### 12.1 问题

ADC 在 DMA 模式下读数偏小（1167-1690），但轮询模式正确（506-2450）。

### 12.2 根因

| 问题 | 根因 | 修复 |
|------|------|------|
| ADC 读数偏小 | 采样时间 84 cycles 不够（DAC 输出缓冲器 ~15kΩ） | 改为 480 cycles |
| TIM8 频率不对 | `Osc_ApplyConfig()` 是空函数 | 实现 PSC/ARR 计算 |
| 480 cycles + 50kHz = 溢出 | 转换时间 23.4µs > 触发间隔 20µs | TIM8 降为 10kHz |
| 频率检测失败 | 固定阈值 2048，信号有直流偏移 | 改用信号中点 |
| 电压测量不准 | 用平均值，对 AC 信号不准确 | 改用峰峰值 |

### 12.3 关键发现

**ADC 采样时间选择**：

| 采样时间 | 转换时间 | 适用场景 |
|---------|---------|---------|
| 84 cycles | 4.57µs | 低阻信号源 (<1kΩ) |
| 144 cycles | 7.43µs | 中等阻抗 |
| 480 cycles | 23.4µs | 高阻信号源 (>10kΩ) |

DAC 输出缓冲器阻抗 ~15kΩ，必须用 480 cycles。

**TIM8 触发频率计算**：

```
TIM8 频率 = timer_clk / (PSC+1) / (ARR+1)
         = 168 MHz / 1 / 16800
         = 10 kHz
```

转换时间必须 < 触发间隔：
- 480 cycles: 23.4µs < 100µs (10kHz) ✅
- 480 cycles: 23.4µs > 20µs (50kHz) ❌

### 12.4 新增教训

**教训 19: ADC 采样时间必须匹配信号源阻抗**

高阻信号源（DAC 输出缓冲器、传感器、分压器）需要更长的采样时间，让采样电容充分充电。84 cycles 对低阻信号源够用，但对 15kΩ 的 DAC 输出不够。

**教训 20: 空函数不代表"不需要配置"**

`Osc_ApplyConfig()` 的注释说"连续模式，无需定时器配置"，但 ADC 实际上是 TIM8 触发模式，必须配置 TIM8 频率。不要相信注释，要看实际配置。

**教训 21: test_adc 命令是关键诊断工具**

轮询模式绕过 DMA/TIM8，直接读 ADC 寄存器。如果轮询模式正确但 DMA 模式错误，问题在 DMA/TIM8 配置，不在 ADC 硬件。

### 12.5 调试效率分析

| 阶段 | 时间 | 效率 |
|------|------|------|
| 发现 ADC 读数偏小 | 5min | ✅ stream_dac 数据分析 |
| 定位采样时间问题 | 10min | ✅ test_adc 轮询模式 |
| 实现 Osc_ApplyConfig | 15min | ⚠️ 没先检查 TIM8 配置 |
| 发现转换溢出 | 5min | ✅ 日志分析 |
| 修复 TIM8 频率 | 10min | ✅ 快速修复 |

**高效原因**：用 stream_dac 采集数据 → 用 test_adc 隔离问题 → 针对性修复

**低效原因**：没先读 Osc_ApplyConfig() 就改代码，引入了转换溢出问题

### 12.6 代码改动

| 文件 | 改动 |
|------|------|
| `Core/Src/main.c` | ADC 采样时间 84→480 cycles |
| `Core/Src/oscilloscope.c` | 实现 Osc_ApplyConfig(), Osc_GetTimerClock() |
| `Core/Src/uart_protocol.c` | 添加 test_adc 命令 |
| `Core/Inc/uart_protocol.h` | 添加 CMD_TEST_ADC |
| `Core/Src/config.c` | 添加 Config_CreateMutex() |
| `Core/Src/display.c` | 添加 ADC_REF_VOLTAGE 本地定义 |
| `Drivers/OLED/oled.c` | 移除调试 UART 输出 |

### 12.7 验证结果

| 功能 | 状态 |
|------|------|
| test_adc 轮询模式 | ✅ ADC 506-2450，正确 |
| TIM8 配置 | ✅ PSC=0, ARR=16799, 10kHz |
| stream_dac DMA 模式 | ⏳ 待验证 |

### 12.8 未解决的问题

1. **I2C 启动问题** — 设备反复卡在 "Initializing display module..."，需要 I2C 总线恢复
2. **DMA 模式波形验证** — 需要设备稳定启动后测试

---

## 13. 总结：调试方法论

### 13.1 正确的调试流程

```
1. 采集数据（stream_dac）
2. 量化分析（增益、频率、相位）
3. 隔离问题（test_adc 轮询 vs DMA）
4. 定位根因（采样时间、触发频率）
5. 针对性修复
6. 验证修复
```

### 13.2 错误的调试流程

```
1. 猜测问题
2. 改代码
3. 烧录测试
4. 发现不对
5. 回到步骤 1
```

### 13.3 关键工具

| 工具 | 用途 |
|------|------|
| `stream_dac` | 采集 DAC+ADC 波形数据 |
| `test_adc` | 轮询模式 ADC 测试，隔离 DMA 问题 |
| `help` | 查看可用命令 |
| 万用表 | 测量实际电压，对比 ADC 读数 |

### 13.4 一句话总结

**先用数据定位问题，再用工具隔离问题，最后针对性修复。不要猜，不要试错。**
