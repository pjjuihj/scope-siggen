# Verification Enforcement Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 实现分层验证系统，强制 AI 按流程执行，自动修复 Layer 1 问题

**Architecture:** 三层验证（Layer 1 必须/Layer 2 应该/Layer 3 可以）+ 自动闭环 + 自动修复

**Tech Stack:** Python, statepilot, PyMuPDF, keil-workflow, stm32-serial-loop

## Global Constraints

- 验证目录：`~/.agents/skills/verification-enforcement/`
- 状态机：statepilot
- PDF 读取：PyMuPDF
- 编译验证：keil-workflow
- 调试验证：stm32-serial-loop
- 每层最多 3 次重试

---

## File Structure

```
verification-enforcement/
├── SKILL.md
├── hooks/
│   ├── check_workflow_state.py
│   ├── check_tool_usage.py
│   ├── check_output_evidence.py
│   └── verify_result.py
├── tools/
│   ├── verify_build.py
│   ├── verify_config.py
│   ├── verify_timing.py
│   ├── verify_data.py
│   └── verify_hardware.py
└── config/
    └── settings.json
```

---

### Task 1: 创建 SKILL.md

**Files:**
- Create: `~/.agents/skills/verification-enforcement/SKILL.md`

**Interfaces:**
- Consumes: 无
- Produces: SKILL.md 入口文件

- [ ] **Step 1: 创建目录结构**

```bash
mkdir -p ~/.agents/skills/verification-enforcement/hooks
mkdir -p ~/.agents/skills/verification-enforcement/tools
mkdir -p ~/.agents/skills/verification-enforcement/config
```

- [ ] **Step 2: 编写 SKILL.md**

创建 `~/.agents/skills/verification-enforcement/SKILL.md`，内容：

```markdown
---
name: verification-enforcement
description: >
  分层验证系统。Layer 1 必须验证（编译、配置、启动顺序），Layer 2 应该验证（时序、内存），
  Layer 3 可以验证（EMC、温度）。自动闭环，自动修复 Layer 1 问题。
license: MIT
metadata:
  author: cmj156
  version: "1.0.0"
  domain: specialized
  triggers: verification, validation, 验证, 检查
  role: specialist
  scope: enforcement
---

# Verification Enforcement System

分层验证系统，强制 AI 按流程执行。

## 三层验证

| 层级 | 验证内容 | 闭环 | 自动修复 |
|------|---------|------|---------|
| Layer 1 | 编译、配置、启动顺序 | ✅ | ✅ |
| Layer 2 | 时序、内存、功耗 | ❌ | ❌ |
| Layer 3 | EMC、温度、长期稳定性 | ❌ | ❌ |

## 工作流

```
AI 生成代码
  → Layer 1 验证（闭环）
  → Layer 2 验证（报告）
  → Layer 3 验证（报告）
  → 烧录验证
  → 调试验证
  → 用户确认
  → 提交代码
```

## 验收标准

每层验证必须输出具体证据（页码、寄存器值、测试结果）。
没有证据 = 没有验证。
```

- [ ] **Step 3: 验证 SKILL.md 行数**

```bash
wc -l ~/.agents/skills/verification-enforcement/SKILL.md
# 预期: < 200 行
```

- [ ] **Step 4: Commit**

```bash
git add ~/.agents/skills/verification-enforcement/SKILL.md
git commit -m "feat: create verification-enforcement SKILL.md"
```

---

### Task 2: 创建状态机

**Files:**
- Create: `~/.agents/skills/verification-enforcement/state_machine.py`

**Interfaces:**
- Consumes: 无
- Produces: statepilot 状态机

- [ ] **Step 1: 编写 state_machine.py**

```python
from statepilot import StateMachine, Pilot

def create_verification_machine():
    """创建验证流程状态机"""
    machine = (
        StateMachine.builder()
        .initial("write_code")

        # Layer 1: 必须验证（闭环）
        .transition("write_code", "verify_compile", tool="verify_build")
        .transition("verify_compile", "verify_config", tool="verify_config")
        .transition("verify_compile", "fix_code", tool="analyze_errors")
        .transition("verify_config", "verify_timing", tool="verify_timing")
        .transition("verify_config", "fix_code", tool="analyze_errors")
        .transition("verify_timing", "flash", tool="verify_timing")
        .transition("verify_timing", "fix_code", tool="analyze_errors")

        # 闭环：修复 → 重新验证
        .transition("fix_code", "verify_compile", tool="verify_build")

        # Layer 2: 应该验证（报告）
        .transition("flash", "verify_data", tool="verify_data")
        .transition("verify_data", "verify_hardware", tool="verify_hardware")

        # Layer 3: 可以验证（报告）
        .transition("verify_hardware", "user_confirm", tool="confirm")

        # 用户确认
        .transition("user_confirm", "done", tool="commit")

        # 终止
        .terminal("done")
        .build()
    )
    return machine
```

- [ ] **Step 2: 测试状态机**

```python
python -c "
from state_machine import create_verification_machine
m = create_verification_machine()
print('Initial:', m.initial)
print('Transitions:', len(m.transitions))
print('Terminal:', m.terminal)
"
```

Expected: 正常输出状态机信息

- [ ] **Step 3: Commit**

```bash
git add ~/.agents/skills/verification-enforcement/state_machine.py
git commit -m "feat: add verification state machine"
```

---

### Task 3: 创建验证工具

**Files:**
- Create: `~/.agents/skills/verification-enforcement/tools/verify_build.py`
- Create: `~/.agents/skills/verification-enforcement/tools/verify_config.py`
- Create: `~/.agents/skills/verification-enforcement/tools/verify_timing.py`
- Create: `~/.agents/skills/verification-enforcement/tools/verify_data.py`
- Create: `~/.agents/skills/verification-enforcement/tools/verify_hardware.py`

**Interfaces:**
- Consumes: 项目路径、外设类型
- Produces: 验证结果（JSON）

- [ ] **Step 1: 编写 verify_build.py**

```python
#!/usr/bin/env python3
import subprocess
import json

def verify_build(project_path):
    """验证编译"""
    result = {"tool": "verify_build", "checks": []}

    # 检查是否有 Makefile 或 CMakeLists.txt
    makefile = os.path.join(project_path, "Makefile")
    cmake = os.path.join(project_path, "CMakeLists.txt")

    if os.path.exists(makefile):
        cmd = ["make", "-C", project_path]
    elif os.path.exists(cmake):
        cmd = ["cmake", "--build", project_path]
    else:
        result["status"] = "fail"
        result["reason"] = "No build system found"
        return result

    # 执行编译
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
        result["return_code"] = proc.returncode
        result["stdout"] = proc.stdout
        result["stderr"] = proc.stderr

        # 分析错误
        errors = [line for line in proc.stderr.split('\n') if 'error:' in line.lower()]
        warnings = [line for line in proc.stderr.split('\n') if 'warning:' in line.lower()]

        result["errors"] = errors
        result["warnings"] = warnings
        result["error_count"] = len(errors)
        result["warning_count"] = len(warnings)

        if proc.returncode == 0:
            result["status"] = "pass"
        else:
            result["status"] = "fail"

    except Exception as e:
        result["status"] = "fail"
        result["reason"] = str(e)

    return result
```

- [ ] **Step 2: 编写 verify_config.py**

```python
#!/usr/bin/env python3
import os
import re

def verify_config(project_path):
    """验证配置"""
    result = {"tool": "verify_config", "checks": []}

    # 读取 main.c
    main_c = os.path.join(project_path, "Core", "Src", "main.c")
    if not os.path.exists(main_c):
        result["status"] = "fail"
        result["reason"] = "main.c not found"
        return result

    with open(main_c, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # 检查 ADC 配置
    checks = []
    if "ContinuousConvMode = DISABLE" in content:
        checks.append({"id": "ADC-001", "status": "pass", "name": "ContinuousConvMode"})
    else:
        checks.append({"id": "ADC-001", "status": "fail", "name": "ContinuousConvMode", "fix": "Set to DISABLE"})

    if "DMAContinuousRequests = ENABLE" in content:
        checks.append({"id": "ADC-002", "status": "pass", "name": "DMAContinuousRequests"})
    else:
        checks.append({"id": "ADC-002", "status": "fail", "name": "DMAContinuousRequests", "fix": "Set to ENABLE"})

    result["checks"] = checks
    result["status"] = "pass" if all(c["status"] == "pass" for c in checks) else "fail"

    return result
```

- [ ] **Step 3: 编写 verify_timing.py**

```python
#!/usr/bin/env python3
import os
import re

def verify_timing(project_path):
    """验证时序"""
    result = {"tool": "verify_timing", "checks": []}

    main_c = os.path.join(project_path, "Core", "Src", "main.c")
    with open(main_c, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # 提取 TIM8 配置
    psc_match = re.search(r'Prescaler\s*=\s*(\d+)', content)
    arr_match = re.search(r'Period\s*=\s*(\d+)', content)

    if psc_match and arr_match:
        psc = int(psc_match.group(1))
        arr = int(arr_match.group(1))
        timer_clk = 168000000  # 168MHz
        freq = timer_clk / ((psc + 1) * (arr + 1))

        checks.append({
            "id": "TIMING-001",
            "name": "触发频率",
            "status": "pass" if 9000 <= freq <= 11000 else "fail",
            "value": f"{freq:.0f} Hz",
            "expected": "10000 Hz"
        })

    result["checks"] = checks
    result["status"] = "pass" if all(c["status"] == "pass" for c in checks) else "fail"

    return result
```

- [ ] **Step 4: 测试验证工具**

```bash
python ~/.agents/skills/verification-enforcement/tools/verify_config.py "d:/stm32 _project_hal/scope-siggen"
python ~/.agents/skills/verification-enforcement/tools/verify_timing.py "d:/stm32 _project_hal/scope-siggen"
```

Expected: 输出验证结果

- [ ] **Step 5: Commit**

```bash
git add ~/.agents/skills/verification-enforcement/tools/
git commit -m "feat: add verification tools"
```

---

### Task 4: 创建 Hook 脚本

**Files:**
- Create: `~/.agents/skills/verification-enforcement/hooks/check_workflow_state.py`
- Create: `~/.agents/skills/verification-enforcement/hooks/check_tool_usage.py`
- Create: `~/.agents/skills/verification-enforcement/hooks/check_output_evidence.py`
- Create: `~/.agents/skills/verification-enforcement/hooks/verify_result.py`

**Interfaces:**
- Consumes: 工具名称、工具输入、工具结果
- Produces: 检查结果（allow/block/warn）

- [ ] **Step 1: 编写 check_workflow_state.py**

```python
#!/usr/bin/env python3
import sys
import json

def check_workflow_state(tool_name, tool_input):
    """检查工作流状态"""
    # 加载状态
    state_file = os.path.join(os.path.dirname(__file__), "..", "config", "workflow_state.json")
    state = {}
    if os.path.exists(state_file):
        with open(state_file, 'r') as f:
            state = json.load(f)

    current_state = state.get("state", "day0_read_manual")

    # 检查是否允许操作
    allowed_tools = {
        "day0_read_manual": ["search_manual"],
        "day0_search_community": ["search_community"],
        "day1_write_code": ["write_code"],
        "day1_verify": ["verify_build", "verify_config"],
        # ... 更多状态
    }

    if tool_name not in allowed_tools.get(current_state, []):
        return {
            "block": True,
            "reason": f"当前状态 {current_state} 不允许 {tool_name}",
            "suggestion": f"应该先执行 {allowed_tools.get(current_state, [])}"
        }

    return {"allow": True}
```

- [ ] **Step 2: 测试 Hook**

```bash
echo '{"tool_name": "search_manual", "tool_input": {}}' | python ~/.agents/skills/verification-enforcement/hooks/check_workflow_state.py
```

Expected: `{"allow": true}`

- [ ] **Step 3: Commit**

```bash
git add ~/.agents/skills/verification-enforcement/hooks/
git commit -m "feat: add verification hooks"
```

---

### Task 5: 集成测试

**Files:**
- 无新文件

**Interfaces:**
- Consumes: 所有工具和 Hook
- Produces: 测试结果

- [ ] **Step 1: 测试完整流程**

```bash
# 测试状态机
python ~/.agents/skills/verification-enforcement/state_machine.py

# 测试验证工具
python ~/.agents/skills/verification-enforcement/tools/verify_config.py "d:/stm32 _project_hal/scope-siggen"
python ~/.agents/skills/verification-enforcement/tools/verify_timing.py "d:/stm32 _project_hal/scope-siggen"

# 测试 Hook
echo '{"tool_name": "search_manual", "tool_input": {}}' | python ~/.agents/skills/verification-enforcement/hooks/check_workflow_state.py
echo '{"tool_name": "verify_build", "tool_result": "0 errors"}' | python ~/.agents/skills/verification-enforcement/hooks/verify_result.py
```

Expected: 所有测试通过

- [ ] **Step 2: Commit**

```bash
git add ~/.agents/skills/verification-enforcement/
git commit -m "feat: verification-enforcement system complete"
```

---

## 任务依赖

```
Task 1 (SKILL.md)
    ↓
Task 2 (状态机)
    ↓
Task 3 (验证工具)
    ↓
Task 4 (Hook 脚本)
    ↓
Task 5 (集成测试)
```

## 验收标准

1. SKILL.md < 200 行
2. 状态机正常工作
3. 验证工具正常工作
4. Hook 正常工作
5. 集成测试通过
