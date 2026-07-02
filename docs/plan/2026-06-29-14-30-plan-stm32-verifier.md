# STM32 Verifier 技能实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 创建独立的 stm32-verifier 技能，深度融合 codegraph + verification-methodology + keil-workflow，提供 62 项自动化检查

**Architecture:** 三合一深度融合架构 — codegraph 提供代码理解，verification-methodology 提供检查规则，keil-workflow 提供工具链。验证引擎按 4 步执行：代码理解 → 规则检查 → 编译验证 → 硬件验证

**Tech Stack:** Markdown (SKILL.md), JSON (输出格式), codegraph MCP tools, keil-workflow scripts

## Global Constraints

- SKILL.md 文件必须 < 500 行
- 检查规则分 3 层：Layer 1 通用(20项) / Layer 2 ARM(12项) / Layer 3 STM32(30项)
- 输出格式为结构化 JSON
- 支持 4 种验证模式：quick / full / hardware / incremental
- 技能目录：`~/.agents/skills/stm32-verifier/`

---

## 文件结构

```
stm32-verifier/
├── SKILL.md                         (技能入口，< 500 行)
├── references/
│   ├── rules/
│   │   ├── layer1-general.md        (通用嵌入式规则 20 项)
│   │   ├── layer2-arm.md            (ARM Cortex-M 规则 12 项)
│   │   └── layer3-stm32.md          (STM32 专有规则 30 项)
│   ├── verification-flow.md         (验证流程详细说明)
│   └── output-schema.md             (JSON 输出格式说明)
└── evals/
    └── evals.json                   (测试用例)
```

---

### Task 1: 创建技能目录和 SKILL.md

**Files:**
- Create: `~/.agents/skills/stm32-verifier/SKILL.md`
- Create: `~/.agents/skills/stm32-verifier/references/rules/`
- Create: `~/.agents/skills/stm32-verifier/evals/`

**Interfaces:**
- Consumes: 无
- Produces: SKILL.md 入口文件，定义验证引擎的 4 个步骤

- [ ] **Step 1: 创建目录结构**

```bash
mkdir -p ~/.agents/skills/stm32-verifier/references/rules
mkdir -p ~/.agents/skills/stm32-verifier/evals
```

- [ ] **Step 2: 编写 SKILL.md**

创建 `~/.agents/skills/stm32-verifier/SKILL.md`，内容如下：

```markdown
---
name: stm32-verifier
description: >
  STM32 嵌入式项目自动化验证工具。深度融合 codegraph（代码图谱）、verification-methodology（验证方法论）
  和 keil-workflow（编译/烧录），提供 62 项自动化检查。支持 4 种验证模式：quick（快速静态检查）、
  full（全量验证含编译）、hardware（硬件验证含烧录）、incremental（增量检查）。
  适用于 STM32F0/F1/F2/F3/F4/F7/G0/G4/L0/L4/H7 系列。
  触发词：验证项目、检查代码、verify、validation、代码审查、初始化检查、并发安全检查。
license: MIT
metadata:
  author: cmj156
  version: "1.0.0"
  domain: specialized
  triggers: verify, validation, 验证, 检查, 代码审查, 初始化检查, 并发安全, STM32 验证
  role: specialist
  scope: verification
  output-format: json
  related-skills: codegraph, embedded-verification-methodology, stm32-keil-workflow
---

# STM32 Verifier

自动化嵌入式项目验证工具。深度融合代码图谱、验证方法论和工具链。

## 验证模式

| 模式 | 触发词 | 执行内容 | 耗时 |
|------|--------|---------|------|
| **quick** | "快速验证" | 代码理解 + 62 项静态检查 | ~30s |
| **full** | "全量验证" | + 编译验证 | ~2min |
| **hardware** | "硬件验证" | + 烧录 + 串口自检 | ~5min |
| **incremental** | "检查改动" | 只检查 git diff 文件 | ~10s |

## 验证引擎（4 步）

### Step 1: 代码理解

使用 codegraph 工具分析项目结构：

```
codegraph explore → 识别外设初始化函数、ISR、FreeRTOS 任务
codegraph callers → 找共享变量的所有访问点
codegraph impact  → 分析改动影响范围
```

构建"外设依赖图"：
- 哪些函数操作哪些外设（ADC1, DAC1, TIM5, TIM8...）
- 哪些 ISR 访问哪些全局变量
- 哪些任务共享哪些资源

### Step 2: 规则检查

基于依赖图，按 3 层规则逐项检查：

- **Layer 1: 通用嵌入式**（20 项）— volatile、互斥锁、看门狗、Flash
- **Layer 2: ARM Cortex-M**（12 项）— NVIC、内存屏障、FromISR
- **Layer 3: STM32 专有**（30 项）— HAL 初始化链、CubeMX 陷阱

每项检查输出：id / title / severity / status / evidence / fix

### Step 3: 编译验证（full/hardware 模式）

调用 keil-workflow 编译项目：

```bash
python workflow.py --auto . --steps compile,analyze
```

检查：
- 编译错误/警告数量
- cppcheck 静态分析结果
- .map 文件的 Flash/RAM 使用

### Step 4: 硬件验证（hardware 模式）

烧录固件并运行串口自检：

```bash
python workflow.py --auto . --steps flash --port COM3
```

串口发送自检命令，读取寄存器值，对比预期。

## 输出格式

JSON 结构化报告，包含：
- `meta`: 工具版本、项目名、平台、扫描模式
- `code_graph`: 文件数、符号数、外设列表、ISR 列表、任务列表
- `summary`: 通过/失败/警告统计
- `checks`: 62 项检查结果（每项有 id/title/severity/status/evidence/fix）
- `compile`: 编译结果（仅 full/hardware 模式）
- `recommendations`: 优先修复建议

详细格式参考：`references/output-schema.md`

## 检查规则

分 3 层，根据项目类型自动选择：
- STM32 项目：启用 Layer 1 + 2 + 3（62 项）
- 其他 Cortex-M：启用 Layer 1 + 2（32 项）
- 其他 MCU：启用 Layer 1（20 项）

详细规则参考：
- `references/rules/layer1-general.md`
- `references/rules/layer2-arm.md`
- `references/rules/layer3-stm32.md`

## 使用示例

```
用户: "快速验证这个项目"
→ 执行 quick 模式，输出 JSON 报告

用户: "全量验证 scope-siggen"
→ 执行 full 模式，包含编译验证

用户: "检查最近的改动"
→ 执行 incremental 模式，只检查 git diff 文件

用户: "验证 ADC 和 DAC 的初始化"
→ 执行 targeted 模式，只检查相关规则
```

## 与现有技能的集成

| 技能 | 集成方式 |
|------|---------|
| codegraph | Step 1 用 explore/callers/impact 分析代码 |
| verification-methodology | 检查规则来自 20 条教训 + 扩展 |
| keil-workflow | Step 3/4 调用 compile/analyze/flash |
```

- [ ] **Step 3: 验证 SKILL.md 行数**

```bash
wc -l ~/.agents/skills/stm32-verifier/SKILL.md
# 预期: < 500 行
```

- [ ] **Step 4: Commit**

```bash
git add ~/.agents/skills/stm32-verifier/
git commit -m "feat: create stm32-verifier skill with SKILL.md"
```

---

### Task 2: 编写 Layer 1 通用嵌入式规则

**Files:**
- Create: `~/.agents/skills/stm32-verifier/references/rules/layer1-general.md`

**Interfaces:**
- Consumes: 无
- Produces: 20 项通用嵌入式检查规则，供 Step 2 使用

- [ ] **Step 1: 编写 layer1-general.md**

创建 `~/.agents/skills/stm32-verifier/references/rules/layer1-general.md`，内容包含 20 项检查规则：

每项规则格式：
```markdown
### GEN-XXX: 检查项标题

**严重度**: Critical / High / Medium / Low
**适用平台**: 所有 MCU

**检查方法**:
- 搜索模式：`grep -rn "pattern" Core/Src/`
- 检查逻辑：描述如何判断 pass/fail

**通过标准**:
- 描述什么情况下算通过

**修复建议**:
- 描述如何修复

**参考**:
- 参考文档链接
```

完整 20 项规则：
- GEN-001: 共享变量 volatile (Critical)
- GEN-002: ISR 保持简短 (High)
- GEN-003: 互斥锁保护共享资源 (Critical)
- GEN-004: 互斥锁内无耗时操作 (High)
- GEN-005: 看门狗配置 (High)
- GEN-006: 看门狗定期刷新 (High)
- GEN-007: 栈溢出检测 (High)
- GEN-008: 堆分配失败处理 (High)
- GEN-009: 错误处理函数 (Medium)
- GEN-010: 复位原因检查 (Low)
- GEN-011: I2C 总线恢复 (High)
- GEN-012: I2C 超时处理 (Medium)
- GEN-013: SPI 片选管理 (Medium)
- GEN-014: UART 接收缓冲区初始化 (Medium)
- GEN-015: UART 命令解析防御 (Medium)
- GEN-016: Flash 写入保护 (High)
- GEN-017: Flash 写入对齐 (Medium)
- GEN-018: 时钟频率不要硬编码 (Medium)
- GEN-019: 除零防御 (Medium)
- GEN-020: 资源使用记录 (Low)

- [ ] **Step 2: Commit**

```bash
git add ~/.agents/skills/stm32-verifier/references/rules/layer1-general.md
git commit -m "feat: add Layer 1 general embedded rules (20 items)"
```

---

### Task 3: 编写 Layer 2 ARM Cortex-M 规则

**Files:**
- Create: `~/.agents/skills/stm32-verifier/references/rules/layer2-arm.md`

**Interfaces:**
- Consumes: 无
- Produces: 12 项 ARM Cortex-M 检查规则，供 Step 2 使用

- [ ] **Step 1: 编写 layer2-arm.md**

创建 `~/.agents/skills/stm32-verifier/references/rules/layer2-arm.md`，内容包含 12 项检查规则：

- ARM-001: NVIC 优先级配置 (High)
- ARM-002: 中断优先级 ≤ configLIBRARY_MAX (Critical)
- ARM-003: __DMB() 内存屏障 (Critical)
- ARM-004: portYIELD_FROM_ISR() (High)
- ARM-005: FromISR API 使用 (Critical)
- ARM-006: HardFault 处理 (High)
- ARM-007: 栈水位检查 (Low)
- ARM-008: 位带操作原子性 (Medium)
- ARM-009: SysTick 配置 (Low)
- ARM-010: 低功耗模式配置 (Low)
- ARM-011: 电源监控 PVD (Low)
- ARM-012: 温度传感器读取 (Low)

- [ ] **Step 2: Commit**

```bash
git add ~/.agents/skills/stm32-verifier/references/rules/layer2-arm.md
git commit -m "feat: add Layer 2 ARM Cortex-M rules (12 items)"
```

---

### Task 4: 编写 Layer 3 STM32 专有规则

**Files:**
- Create: `~/.agents/skills/stm32-verifier/references/rules/layer3-stm32.md`

**Interfaces:**
- Consumes: 无
- Produces: 30 项 STM32 专有检查规则，供 Step 2 使用

- [ ] **Step 1: 编写 layer3-stm32.md**

创建 `~/.agents/skills/stm32-verifier/references/rules/layer3-stm32.md`，内容包含 30 项检查规则：

- STM-001 ~ STM-030: 完整的 STM32 专有检查规则

重点规则：
- STM-007: TIM TRGO 配置（CubeMX 默认值陷阱）
- STM-008: DMAContinuousRequests（CubeMX 默认值陷阱）
- STM-011: 启动顺序（先 ADC DMA 后 TIM）
- STM-015: TIM5 启动（最容易遗漏）
- STM-016: Start/Stop 对称
- STM-030: RTOS 对象创建时机

- [ ] **Step 2: Commit**

```bash
git add ~/.agents/skills/stm32-verifier/references/rules/layer3-stm32.md
git commit -m "feat: add Layer 3 STM32-specific rules (30 items)"
```

---

### Task 5: 编写验证流程和输出格式文档

**Files:**
- Create: `~/.agents/skills/stm32-verifier/references/verification-flow.md`
- Create: `~/.agents/skills/stm32-verifier/references/output-schema.md`

**Interfaces:**
- Consumes: 无
- Produces: 验证流程详细说明和 JSON 输出格式定义

- [ ] **Step 1: 编写 verification-flow.md**

详细说明 4 种验证模式的执行流程：
- quick 模式：Step 1 + Step 2
- full 模式：Step 1 + 2 + 3
- hardware 模式：Step 1 + 2 + 3 + 4
- incremental 模式：git diff → 只检查改动文件

- [ ] **Step 2: 编写 output-schema.md**

定义 JSON 输出格式的完整 schema：
- meta 字段
- code_graph 字段
- summary 字段
- checks 数组（每个检查项的字段）
- compile 字段
- recommendations 字段

- [ ] **Step 3: Commit**

```bash
git add ~/.agents/skills/stm32-verifier/references/
git commit -m "docs: add verification flow and output schema"
```

---

### Task 6: 创建测试用例

**Files:**
- Create: `~/.agents/skills/stm32-verifier/evals/evals.json`

**Interfaces:**
- Consumes: 无
- Produces: 测试用例，用于验证技能效果

- [ ] **Step 1: 编写 evals.json**

创建 3 个测试用例：

```json
{
  "skill_name": "stm32-verifier",
  "evals": [
    {
      "id": 0,
      "prompt": "快速验证 scope-siggen 项目的初始化和并发安全",
      "expected_output": "JSON 报告，包含 62 项检查结果，识别出启动顺序、DMA 链接等已知问题"
    },
    {
      "id": 1,
      "prompt": "检查 oscilloscope.c 的 DMA 缓冲区代码",
      "expected_output": "JSON 报告，识别出 process_ptr/process_len 竞态、缺少 __DMB() 等问题"
    },
    {
      "id": 2,
      "prompt": "全量验证 STM32F407 项目的 ADC/DAC 配置",
      "expected_output": "JSON 报告，包含编译结果和静态检查结果"
    }
  ]
}
```

- [ ] **Step 2: Commit**

```bash
git add ~/.agents/skills/stm32-verifier/evals/
git commit -m "test: add evaluation cases for stm32-verifier"
```

---

### Task 7: 集成测试和优化

**Files:**
- Modify: `~/.agents/skills/stm32-verifier/SKILL.md`（如需要）

**Interfaces:**
- Consumes: SKILL.md + 所有 rules + evals
- Produces: 经过测试验证的完整技能

- [ ] **Step 1: 运行测试用例 0**

使用技能执行 eval 0，验证：
- codegraph explore 能否正确识别外设函数
- 62 项规则检查是否完整
- JSON 输出格式是否正确

- [ ] **Step 2: 运行测试用例 1**

使用技能执行 eval 1，验证：
- 并发安全检查是否准确
- 能否识别竞态条件和内存屏障问题

- [ ] **Step 3: 运行测试用例 2**

使用技能执行 eval 2，验证：
- full 模式是否正常工作
- 编译验证是否集成成功

- [ ] **Step 4: 根据测试结果优化**

如果测试发现问题：
- 调整 SKILL.md 的描述
- 补充缺失的检查规则
- 优化 JSON 输出格式

- [ ] **Step 5: 最终 Commit**

```bash
git add ~/.agents/skills/stm32-verifier/
git commit -m "feat: stm32-verifier v1.0.0 complete"
```

---

## 任务依赖

```
Task 1 (SKILL.md)
    ↓
Task 2 (Layer 1) ← Task 3 (Layer 2) ← Task 4 (Layer 3)
    ↓                   ↓                   ↓
Task 5 (文档) ←──────────────────────────────┘
    ↓
Task 6 (测试用例)
    ↓
Task 7 (集成测试)
```

## 验收标准

1. SKILL.md < 500 行
2. 62 项检查规则完整（Layer 1: 20, Layer 2: 12, Layer 3: 30）
3. JSON 输出格式符合 schema
4. 3 个测试用例全部通过
5. 与 codegraph、keil-workflow 集成正常
