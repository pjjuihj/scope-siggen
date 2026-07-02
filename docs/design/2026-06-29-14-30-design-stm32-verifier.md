# STM32 Verifier 技能设计文档

**日期**: 2026-06-29
**状态**: 设计完成，待实现
**版本**: 1.0.0

---

## 1. 概述

### 1.1 目标

创建一个独立的 `stm32-verifier` 技能，深度融合 codegraph（代码图谱）、verification-methodology（验证方法论）和 keil-workflow（编译/烧录），提供自动化的嵌入式项目验证。

### 1.2 核心价值

| 维度 | 当前状态 | 使用技能后 |
|------|---------|-----------|
| 验证方式 | 人工逐项检查 | AI 自动扫描 62 项 |
| 覆盖范围 | 只检查记得住的 | 6 维度全覆盖 |
| 输出格式 | 自由文本 | 结构化 JSON |
| 跨文件分析 | 人工追踪 | codegraph 自动分析 |

### 1.3 适用范围

| 平台 | 支持程度 |
|------|---------|
| STM32F0/F1/F2/F3/F4/F7 | ✅ 完全支持 |
| STM32G0/G4/L0/L4/H7 | ✅ 完全支持 |
| ESP32 | ⚠️ 部分支持（Layer 1 通用规则） |
| 其他 Cortex-M | ⚠️ 部分支持（Layer 1+2） |

---

## 2. 架构设计

### 2.1 三合一深度融合

不是"三个技能串联调用"，而是"一个技能，三个能力内嵌"。

```
┌──────────────────────────────────────────────────────────────┐
│                    stm32-verifier 技能                         │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │              验证引擎 (Verification Engine)           │    │
│  │                                                     │    │
│  │  Step 1: 代码理解（内嵌 codegraph 能力）              │    │
│  │    • explore → 识别外设初始化函数、ISR、任务           │    │
│  │    • callers → 找共享变量访问点                       │    │
│  │    • impact → 分析改动影响范围                        │    │
│  │    • 构建"外设依赖图"                                │    │
│  │                                                     │    │
│  │  Step 2: 规则检查（内嵌 verification 规则）            │    │
│  │    • 基于依赖图，逐外设检查初始化链完整性              │    │
│  │    • 基于 callers 结果，检查共享变量 volatile          │    │
│  │    • 检查 CubeMX 默认值陷阱                           │    │
│  │    • 检查并发安全、通信协议、可靠性                    │    │
│  │                                                     │    │
│  │  Step 3: 编译验证（内嵌 keil-workflow 能力）           │    │
│  │    • 调用 keil-workflow compile 编译项目              │    │
│  │    • 运行 cppcheck 静态分析                           │    │
│  │    • 分析 .map 文件的 Flash/RAM 使用                  │    │
│  │                                                     │    │
│  │  Step 4: 硬件验证（可选）                             │    │
│  │    • keil-workflow flash 烧录固件                     │    │
│  │    • 串口发送自检命令，读取寄存器值                    │    │
│  │                                                     │    │
│  │  输出: JSON 验证报告                                  │    │
│  └─────────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────┘
```

### 2.2 三个技能的分工

| 技能 | 职责 | 提供什么 |
|------|------|---------|
| **codegraph** | 代码理解 | 调用图、依赖关系、符号查找 |
| **verification-methodology** | 检查规则 | 62 项检查清单、修复建议 |
| **keil-workflow** | 工具链 | 编译、静态分析、烧录、串口 |

### 2.3 验证模式

| 模式 | 触发词 | 执行内容 |
|------|--------|---------|
| **quick** | "快速验证" | Step 1 + Step 2（只做静态检查） |
| **full** | "全量验证" | Step 1 + 2 + 3（加编译验证） |
| **hardware** | "硬件验证" | Step 1 + 2 + 3 + 4（加烧录和串口自检） |
| **incremental** | "检查改动" | 只检查 git diff 涉及的文件 |

---

## 3. 检查规则体系

### 3.1 三层架构

```
Layer 1: 通用嵌入式规则（20 项）— 适用于所有 MCU
Layer 2: ARM Cortex-M 规则（12 项）— 适用于 Cortex-M 系列
Layer 3: STM32 专有规则（30 项）— 适用于 STM32 系列
```

技能根据项目类型自动选择适用的规则层。

### 3.2 Layer 1: 通用嵌入式规则（20 项）

| ID | 检查项 | 严重度 |
|----|--------|--------|
| GEN-001 | 共享变量 volatile | Critical |
| GEN-002 | ISR 保持简短 | High |
| GEN-003 | 互斥锁保护共享资源 | Critical |
| GEN-004 | 互斥锁内无耗时操作 | High |
| GEN-005 | 看门狗配置 | High |
| GEN-006 | 看门狗定期刷新 | High |
| GEN-007 | 栈溢出检测 | High |
| GEN-008 | 堆分配失败处理 | High |
| GEN-009 | 错误处理函数 | Medium |
| GEN-010 | 复位原因检查 | Low |
| GEN-011 | I2C 总线恢复 | High |
| GEN-012 | I2C 超时处理 | Medium |
| GEN-013 | SPI 片选管理 | Medium |
| GEN-014 | UART 接收缓冲区初始化 | Medium |
| GEN-015 | UART 命令解析防御 | Medium |
| GEN-016 | Flash 写入保护 | High |
| GEN-017 | Flash 写入对齐 | Medium |
| GEN-018 | 时钟频率不要硬编码 | Medium |
| GEN-019 | 除零防御 | Medium |
| GEN-020 | 资源使用记录 | Low |

### 3.3 Layer 2: ARM Cortex-M 规则（12 项）

| ID | 检查项 | 严重度 |
|----|--------|--------|
| ARM-001 | NVIC 优先级配置 | High |
| ARM-002 | 中断优先级 ≤ configLIBRARY_MAX | Critical |
| ARM-003 | __DMB() 内存屏障 | Critical |
| ARM-004 | portYIELD_FROM_ISR() | High |
| ARM-005 | FromISR API 使用 | Critical |
| ARM-006 | HardFault 处理 | High |
| ARM-007 | 栈水位检查 | Low |
| ARM-008 | 位带操作原子性 | Medium |
| ARM-009 | SysTick 配置 | Low |
| ARM-010 | 低功耗模式配置 | Low |
| ARM-011 | 电源监控 PVD | Low |
| ARM-012 | 温度传感器读取 | Low |

### 3.4 Layer 3: STM32 专有规则（30 项）

| ID | 检查项 | 严重度 |
|----|--------|--------|
| STM-001 | ADC 时钟使能 | Critical |
| STM-002 | ADC GPIO ANALOG | Critical |
| STM-003 | DMA 时钟使能 | Critical |
| STM-004 | DMA 流/通道配置 | Critical |
| STM-005 | __HAL_LINKDMA | Critical |
| STM-006 | DMA NVIC 使能 | High |
| STM-007 | TIM TRGO 配置 | Critical |
| STM-008 | DMAContinuousRequests | Critical |
| STM-009 | EOCSelection | High |
| STM-010 | ADC 采样时间 | Medium |
| STM-011 | 启动顺序 | Critical |
| STM-012 | DAC 时钟使能 | Critical |
| STM-013 | DAC DMA 链接 | Critical |
| STM-014 | DAC 触发源 | High |
| STM-015 | TIM5 启动 | Critical |
| STM-016 | Start/Stop 对称 | High |
| STM-017 | 定时器修改前停止 | High |
| STM-018 | EGR = TIM_EGR_UG | Medium |
| STM-019 | GPIO 模式正确 | High |
| STM-020 | UART AF 配置 | Medium |
| STM-021 | I2C 时钟速度 | Medium |
| STM-022 | SPI 时钟极性/相位 | Medium |
| STM-023 | PWM 死区配置 | Medium |
| STM-024 | 编码器模式 | Low |
| STM-025 | ADC 校准 | Low |
| STM-026 | Flash 解锁/锁定 | High |
| STM-027 | Flash 扇区擦除 | Medium |
| STM-028 | FreeRTOSConfig.h | Medium |
| STM-029 | 任务栈大小 | Medium |
| STM-030 | RTOS 对象创建时机 | Critical |

### 3.5 检查规则总览

| 层级 | 检查项数 | Critical | High | Medium | Low |
|------|---------|----------|------|--------|-----|
| Layer 1: 通用嵌入式 | 20 | 2 | 8 | 8 | 2 |
| Layer 2: ARM Cortex-M | 12 | 3 | 3 | 1 | 5 |
| Layer 3: STM32 专有 | 30 | 10 | 8 | 10 | 2 |
| **总计** | **62** | **15** | **19** | **19** | **9** |

---

## 4. 输出格式

### 4.1 JSON 报告结构

```json
{
  "meta": {
    "tool": "stm32-verifier",
    "version": "1.0.0",
    "timestamp": "2026-06-29T12:00:00",
    "project": "scope-siggen",
    "platform": "STM32F4",
    "scan_mode": "full",
    "rules_applied": ["layer1", "layer2", "layer3"]
  },
  "code_graph": {
    "files_scanned": 18,
    "symbols_found": 120,
    "peripherals": ["ADC1", "DAC1", "TIM5", "TIM8", "I2C1", "USART2"],
    "isrs": ["DMA2_Stream0_IRQHandler", "USART2_IRQHandler"],
    "tasks": ["Oscilloscope_Task", "Display_Task", "SignalGen_Task"],
    "shared_vars": ["process_ptr", "process_len", "osc_status"]
  },
  "summary": {
    "total": 62,
    "pass": 55,
    "fail": 5,
    "warn": 2,
    "skip": 0,
    "by_severity": {
      "critical": {"total": 15, "fail": 2},
      "high": {"total": 19, "fail": 2},
      "medium": {"total": 19, "fail": 1},
      "low": {"total": 8, "fail": 0}
    }
  },
  "checks": [
    {
      "id": "STM-011",
      "dimension": "init_chain",
      "layer": "layer3",
      "title": "启动顺序：先 ADC DMA 后 TIM",
      "severity": "critical",
      "status": "fail",
      "file": "oscilloscope.c",
      "line": 183,
      "evidence": "Oscilloscope_Start() 先调用 HAL_TIM_Base_Start(&htim8)，后调用 HAL_ADC_Start_DMA()",
      "fix": "交换顺序：先 HAL_ADC_Start_DMA()，后 HAL_TIM_Base_Start(&htim8)",
      "reference": "参考手册 RM0090 Section 13.3.15"
    }
  ],
  "compile": {
    "errors": 0,
    "warnings": 0,
    "flash_used": "45KB",
    "ram_used": "32KB"
  },
  "recommendations": [
    "优先修复 2 个 Critical 问题：启动顺序和 DMA 链接",
    "建议添加 I2C 总线恢复机制",
    "建议实现 vApplicationStackOverflowHook"
  ]
}
```

### 4.2 每个检查项的字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | string | 检查项 ID（如 "STM-011"） |
| `dimension` | string | 所属维度 |
| `layer` | string | 所属层级（layer1/layer2/layer3） |
| `title` | string | 检查项标题 |
| `severity` | string | 严重度（critical/high/medium/low） |
| `status` | string | 状态（pass/fail/warn/skip） |
| `file` | string | 相关文件 |
| `line` | number | 相关行号 |
| `evidence` | string | 证据（代码片段或分析结果） |
| `fix` | string | 修复建议 |
| `reference` | string | 参考文档 |

---

## 5. 与现有技能的集成

### 5.1 集成点

| 集成点 | 如何集成 |
|--------|---------|
| **codegraph** | Step 1 用 `explore` 识别符号，`callers` 找访问点，`impact` 分析影响 |
| **keil-workflow** | Step 3 调用 `workflow.py --steps compile,analyze`，读取编译结果 |
| **verification-methodology** | 检查规则来自 methodology 的 20 条教训 + 扩展 |
| **stm32-hal-development** | HAL 初始化链检查规则 |

### 5.2 数据流

```
codegraph explore → 识别外设函数、ISR、任务
         ↓
codegraph callers → 找共享变量访问点
         ↓
verification rules → 逐项检查
         ↓
keil-workflow compile → 编译验证
         ↓
JSON 报告输出
```

---

## 6. 文件结构

```
stm32-verifier/
├── SKILL.md                    (技能入口，< 500 行)
├── references/
│   ├── rules/
│   │   ├── layer1-general.md   (通用嵌入式规则 20 项)
│   │   ├── layer2-arm.md       (ARM Cortex-M 规则 12 项)
│   │   └── layer3-stm32.md     (STM32 专有规则 30 项)
│   ├── check-templates.md      (检查代码模板)
│   └── output-schema.md        (JSON 输出格式说明)
└── scripts/
    └── verify.py               (可选：辅助脚本)
```

---

## 7. 验证模式

### 7.1 quick 模式

只做静态检查，不编译。

```
用户: "快速验证这个项目"
  ↓
Step 1: codegraph explore → 识别符号
Step 2: 62 项规则检查 → JSON 报告
```

**耗时**: ~30 秒
**适用**: 开发过程中频繁检查

### 7.2 full 模式

静态检查 + 编译验证。

```
用户: "全量验证这个项目"
  ↓
Step 1: codegraph explore → 识别符号
Step 2: 62 项规则检查
Step 3: keil-workflow compile → 编译结果
  ↓
JSON 报告
```

**耗时**: ~2 分钟
**适用**: 提交前验证

### 7.3 hardware 模式

静态检查 + 编译 + 烧录 + 串口自检。

```
用户: "硬件验证这个项目"
  ↓
Step 1-3: 同 full 模式
Step 4: keil-workflow flash → 烧录
        串口发送自检命令 → 读取寄存器值
  ↓
JSON 报告
```

**耗时**: ~5 分钟
**适用**: 版本发布前验证

### 7.4 incremental 模式

只检查 git diff 涉及的文件。

```
用户: "检查最近的改动"
  ↓
git diff --name-only → 获取改动文件列表
Step 1: codegraph explore → 只分析改动文件
Step 2: 只检查受影响的规则
  ↓
JSON 报告（增量）
```

**耗时**: ~10 秒
**适用**: 频繁提交时的快速检查

---

## 8. 实现计划

### Phase 1: 基础框架

1. 创建 stm32-verifier 技能目录
2. 编写 SKILL.md（< 500 行）
3. 编写 Layer 1 通用规则（20 项）
4. 实现 quick 模式

### Phase 2: 完整规则

1. 编写 Layer 2 ARM 规则（12 项）
2. 编写 Layer 3 STM32 规则（30 项）
3. 实现 full 模式

### Phase 3: 工具链集成

1. 集成 codegraph
2. 集成 keil-workflow
3. 实现 hardware 模式

### Phase 4: 优化

1. 实现 incremental 模式
2. 优化 JSON 输出格式
3. 添加更多检查规则

---

## 9. 预期效果

### 9.1 效率提升

| 场景 | 当前方式 | 使用技能后 |
|------|---------|-----------|
| 新项目验证 | 人工检查 2 小时 | 自动扫描 30 秒 |
| 提交前检查 | 人工审查 30 分钟 | full 模式 2 分钟 |
| 调试配置问题 | 试错 2 小时 | 快速定位 1 分钟 |

### 9.2 质量提升

| 维度 | 当前 | 使用技能后 |
|------|------|-----------|
| 检查覆盖 | 记得住的项 | 62 项全覆盖 |
| 跨文件分析 | 人工追踪 | codegraph 自动 |
| 输出一致性 | 自由文本 | 结构化 JSON |

---

**设计完成。**
