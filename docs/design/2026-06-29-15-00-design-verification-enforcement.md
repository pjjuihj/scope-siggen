# Verification Enforcement System Design

**日期**: 2026-06-29
**状态**: 实施中
**版本**: 1.0.0

---

## 1. 概述

### 1.1 目标

创建一个强制执行嵌入式开发验证流程的系统，解决"AI 不遵守原则"的问题。

### 1.2 核心方案

```
skill (规则) + statepilot (状态机) + Claude Code Hooks (拦截)
```

### 1.3 核心约束

**写代码约束：**
- 必须先读手册（Day 0）
- 每次最多写 20 行
- 必须编译通过
- 必须有检查清单
- 必须提供验证代码

**修复约束：**
- 每次最多改 10 行
- 必须编译通过
- 必须重新验证
- 不能引入新问题

**验证约束：**
- Layer 1-5 验证有具体规则
- 失败必须回到修复
- 循环上限：每层最多 3 次

---

## 2. 状态机设计

### 2.1 状态定义

```
Day 0: 读手册
├─ day0_read_manual — 读取本地 PDF 手册
└─ day0_search_community — 搜索社区问题

Day 1: 外设自检
├─ day1_write_code — 写外设自检代码
├─ day1_verify — 验证外设自检
└─ day1_fix — 修复外设自检问题

Day 2: 数据链路
├─ day2_write_code — 写数据链路代码
├─ day2_verify — 验证数据链路
└─ day2_fix — 修复数据链路问题

Day 3: 集成显示
├─ day3_write_code — 写集成显示代码
├─ day3_verify — 验证集成显示
└─ day3_fix — 修复集成显示问题

代码审查
├─ code_review — 3 维度审查
└─ fix_code — 修复审查问题

提交
└─ commit — 提交代码
```

### 2.2 状态转换

```
day0_read_manual → day0_search_community → day1_write_code
day1_write_code → day1_verify → day2_write_code (通过)
day1_verify → day1_fix → day1_verify (失败，闭环)
day2_write_code → day2_verify → day3_write_code (通过)
day2_verify → day2_fix → day2_verify (失败，闭环)
day3_write_code → day3_verify → code_review (通过)
day3_verify → day3_fix → day3_verify (失败，闭环)
code_review → commit (通过)
code_review → fix_code → code_review (失败，闭环)
```

### 2.3 约束定义

```python
constraints = {
    "write_code": {
        "max_lines": 20,
        "must_compile": True,
        "must_have_checklist": True,
        "must_have_verification_code": True,
    },
    "fix": {
        "max_diff_lines": 10,
        "must_compile": True,
        "must_verify": True,
        "no_new_issues": True,
    },
    "verify": {
        "max_retries": 3,
        "must_pass_all_checks": True,
    }
}
```

---

## 3. Hook 设计

### 3.1 PreToolUse Hook

```python
# 检查是否允许操作
def check_workflow_state(current_state, requested_tool, tool_input):
    # 检查是否先读手册
    if "write_code" in current_state and not manual_read:
        return {"block": True, "reason": "还没有读手册"}
    
    # 检查写代码行数
    if "write_code" in current_state:
        lines = count_lines(tool_input)
        if lines > 20:
            return {"block": True, "reason": f"一次写了 {lines} 行，最多 20 行"}
    
    # 检查修复行数
    if "fix" in current_state:
        diff_lines = count_diff_lines(tool_input)
        if diff_lines > 10:
            return {"block": True, "reason": f"改了 {diff_lines} 行，最多 10 行"}
    
    return {"allow": True}
```

### 3.2 PostToolUse Hook

```python
# 验证结果
def verify_result(tool_name, result):
    # 编译检查
    if tool_name == "verify_build":
        if "error" in result:
            return {"block": True, "reason": "编译失败"}
    
    # 配置检查
    if tool_name == "verify_config":
        for check in skill.checks:
            if not check.pass(result):
                return {"block": True, "reason": f"配置错误: {check.description}"}
    
    return {"allow": True}
```

---

## 4. 工具集成

### 4.1 读手册工具

```
PyPDF2 — 读取本地 PDF 手册
agent-reach-internet-access — 在线搜索
```

### 4.2 编译工具

```
keil-workflow — Keil MDK-ARM 编译
arm-none-eabi-gcc — GCC 编译
```

### 4.3 验证工具

```
cppcheck — 静态分析
stm32-serial-loop — 串口验证
```

---

## 5. 文件结构

```
verification-enforcement/
├── SKILL.md                    (技能定义)
├── state_machine.py            (状态机实现)
├── hooks/
│   ├── check_workflow_state.py (PreToolUse Hook)
│   └── verify_result.py        (PostToolUse Hook)
├── tools/
│   ├── verify_build.py         (编译验证)
│   ├── verify_config.py        (配置验证)
│   ├── verify_timing.py        (时序验证)
│   ├── verify_data.py          (数据验证)
│   └── verify_hardware.py      (硬件验证)
└── config/
    └── settings.json           (配置文件)
```

---

## 6. 实施计划

### Phase 1: 基础框架
- 创建状态机
- 创建 Hook 脚本
- 集成到 Claude Code

### Phase 2: 验证工具
- 编译验证工具
- 配置验证工具
- 时序验证工具

### Phase 3: 工具集成
- PyPDF2 集成
- agent-reach-internet-access 集成
- keil-workflow 集成

### Phase 4: 测试优化
- 测试完整流程
- 优化性能
- 修复问题

---

**设计完成。**
