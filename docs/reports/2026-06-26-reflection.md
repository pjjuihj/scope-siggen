# 2026-06-26 开发反思

## 今日概要

| 指标 | 数值 |
|------|------|
| 改动文件 | 20+ |
| 代码变化 | +411 / -202 行 |
| 编译状态 | 0 Error, 6 Warning |
| 实测验证 | ❌ 未进行 |
| 未提交文件 | 20+ |

## 完成的工作

### 示波器模块 (oscilloscope.c)

- [x] TIM8 作为 ADC 触发源
- [x] ADC DMA 启动代码
- [x] RTOS 任务通知机制（100ms 超时）
- [x] 集成 Display 模块调用
- [ ] ❌ 硬件验证未做

### 显示模块 (display.c)

- [x] OLED 初始化
- [x] 波形绘制函数框架
- [x] 电压/频率显示接口
- [ ] ❌ DrawChar/DrawString/DrawLine 未接入
- [ ] ❌ 未使用变量未清理

### 信号发生器 (signal_gen.c)

- [x] 基础框架改动
- [ ] ❌ 功能未验证

### 其他

- [x] 中断处理 (stm32f4xx_it.c)
- [x] MSP 初始化 (stm32f4xx_hal_msp.c)
- [x] 代码审查模块骨架 (code_reviewer.c) — 3 个 TODO 未填

---

## 问题分析

### 1. 编译警告未清零

```
display.c(421): warning: function "abs" declared implicitly        ← 缺 stdlib.h
display.c(34):  warning: variable "cursor_x" was set but never used
display.c(35):  warning: variable "cursor_y" was set but never used
display.c(370): warning: function "Display_DrawChar" was declared but never referenced
display.c(383): warning: function "Display_DrawString" was declared but never referenced
display.c(418): warning: function "Display_DrawLine" was declared but never referenced
```

**根因**：写代码时只关注功能，忽略了编译器反馈。

**后果**：
- `abs()` 隐式声明 → 负数坐标计算可能出错
- 未使用变量 → Code 段膨胀，维护困惑
- 未引用函数 → 死代码，增加理解成本

### 2. 功能蔓延

今天同时推进了 4 个模块，没有一条链路跑通：

```
示波器 ADC → DMA → 任务通知 → 显示      ← 假设链路，未验证
信号发生器改动                              ← 改了什么？
代码审查模块                                ← 骨架，3个TODO
```

**根因**：缺乏明确的"今日目标"，想到什么写什么。

**后果**：
- 无法判断哪段代码是对的
- 出问题时不知道从哪里排查
- 改动分散，难以形成有意义的 commit

### 3. 验证后置

写了 400+ 行代码，没有一行经过硬件验证。

**嵌入式开发的真相**：
```
写代码:   20% 时间
调试硬件: 80% 时间
```

**根因**：用 Web 开发的思维做嵌入式——先写功能，后测试。

**后果**：
- 如果 TIM8 TRGO 没配对，后面全部白写
- 如果 DMA buffer 对齐错了，会 HardFault
- 如果任务通知没发，任务会永远卡在 Waiting

### 4. TODO 债务

```c
// app_init.c
/* TODO: 禁用示波器、信号发生器等 */

// code_reviewer.c
/* TODO: 实现文件检查逻辑 */
/* TODO: 实现项目检查逻辑 */
/* TODO: 实现设计符合性检查 */

// config.c
/* TODO: 从Flash读取配置 */
/* TODO: 写入Flash */

// display.c
/* TODO: 设置显示器亮度 */
```

**根因**：写了骨架但没填内容，也没有记录到任务列表。

**后果**：代码里到处是"承诺"，但没人跟踪是否兑现。

### 5. 版本管理混乱

- 20+ 文件未提交
- 编译产物 (build.log, flash.log) 和源码混在一起
- .gitignore 未更新

**根因**：急于写功能，忽略了版本管理纪律。

**后果**：
- 无法回退到某个稳定状态
- 无法用 git bisect 定位问题
- 编译产物污染仓库

---

## 根因总结

| 问题 | 根因 | 影响 |
|------|------|------|
| 编译警告 | 忽略编译器反馈 | 潜在 bug |
| 功能蔓延 | 无明确目标 | 精力分散 |
| 验证后置 | Web 思维做嵌入式 | 代码不确定对不对 |
| TODO 债务 | 只开坑不填 | 代码腐化 |
| 版本混乱 | 急于开发 | 无法回退 |

**一句话根因**：今天的目标是"写代码"而不是"跑通功能"。

---

## 改进方案

### 原则 1: 一次只打通一条链路

```
❌ 同时改示波器 + 显示 + 信号发生器
✅ 先跑通 ADC → 串口打印，再接显示
```

### 原则 2: 先验证，后美化

```
❌ 直接写 Display_DrawWaveform
✅ 先 printf("ADC=%d\n", adc_val)，确认 ADC 活了
```

### 原则 3: 编译警告 = 编译错误

```
❌ 6 个警告？能跑就行
✅ 每次编译后第一件事：清零警告
```

### 原则 4: TODO 必须有跟踪

```
❌ 写在代码里就完事
✅ 要么立刻做，要么记到任务列表，要么标记 Won't Do
```

### 原则 5: 每完成一个原子功能就 commit

```
❌ 改了 20 个文件一起提交
✅ ADC 单次采样通过 → commit
   ADC DMA 通过 → commit
   显示波形通过 → commit
```

---

## 明日优先级

```
P0  [ ] 清零编译警告（10分钟）
P0  [ ] 烧录实测 ADC DMA 是否能采集到数据
P0  [ ] 未改动 commit 掉，保持 git 干净

P1  [ ] 验证 TIM8 TRGO → ADC 触发链路
P1  [ ] 验证 DMA buffer 数据正确性
P1  [ ] 验证 RTOS 任务通知能唤醒

P2  [ ] 决定 Display_Draw* 函数：接入或删除
P2  [ ] code_reviewer.c TODO：实现或标记 Won't Do
P2  [ ] config.c Flash 读写：实现或标记 Won't Do
```

---

## 经验教训

> **嵌入式开发的效率瓶颈不是打字速度，是"改完不知道对不对"的迷茫时间。**

> **先用最丑的方式把功能跑出来，再美化。printf 是最好的调试工具。**

> **编译器的警告比你聪明，它说有问题就一定有问题。**

> **TODO 是债务，不是笔记。要么做，要么记，要么放弃。**

> **git commit 不是"写完才提交"，是"每验证一步就提交"。**
