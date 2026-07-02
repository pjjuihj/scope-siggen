# Commit Convention Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 创建自定义提交规范技能，包含硬件信息、测试结果、外设配置

**Architecture:** 扩展 Conventional Commits 格式，添加硬件元数据。工具自动扫描项目、检测硬件、生成提交信息。Git Hook + Claude Code Hook 集成。

**Tech Stack:** Python, Git Hooks, Claude Code Hooks

## Global Constraints

- 技能目录：`~/.agents/skills/commit-convention/`
- 提交格式：扩展 Conventional Commits
- 硬件信息：从 .ioc 和代码自动检测
- 集成：Git Hook + Claude Code Hook

---

## 文件结构

```
commit-convention/
├── SKILL.md
├── templates/
│   ├── commit-template.txt
│   ├── hardware-template.yaml
│   └── release-template.md
├── hooks/
│   ├── pre-commit
│   └── check_commit.py
├── tools/
│   ├── project_scanner.py
│   ├── hardware_detector.py
│   ├── commit_generator.py
│   ├── version_bumper.py
│   └── changelog_generator.py
└── config/
    └── commit-convention.json
```

---

### Task 1: 创建目录和 SKILL.md

**Files:**
- Create: `~/.agents/skills/commit-convention/SKILL.md`
- Create: `~/.agents/skills/commit-convention/templates/`
- Create: `~/.agents/skills/commit-convention/hooks/`
- Create: `~/.agents/skills/commit-convention/tools/`
- Create: `~/.agents/skills/commit-convention/config/`

**Interfaces:**
- Consumes: 无
- Produces: SKILL.md 入口文件

- [ ] **Step 1: 创建目录结构**

```bash
mkdir -p ~/.agents/skills/commit-convention/templates
mkdir -p ~/.agents/skills/commit-convention/hooks
mkdir -p ~/.agents/skills/commit-convention/tools
mkdir -p ~/.agents/skills/commit-convention/config
```

- [ ] **Step 2: 编写 SKILL.md**

创建 `~/.agents/skills/commit-convention/SKILL.md`，内容如下：

```markdown
---
name: commit-convention
description: >
  嵌入式项目自定义提交规范。扩展 Conventional Commits 格式，添加硬件信息、
  测试结果、外设配置。自动扫描项目、检测硬件、生成提交信息。
  触发词：提交、commit、提交规范、变更日志。
license: MIT
metadata:
  author: cmj156
  version: "1.0.0"
  domain: specialized
  triggers: commit, 提交, 提交规范, 变更日志, changelog, version
  role: specialist
  scope: workflow
  output-format: text
  related-skills: embedded-verification-methodology, verification-enforcement
---

# Commit Convention

嵌入式项目自定义提交规范。

## 提交格式

```
type(scope): description

[hardware]
mcu: STM32F407VET6
clock: 168MHz
peripherals:
  - ADC1: PA6, TIM8 trigger, DMA circular, 10kHz

[test]
result: pass
checks: 62/62
errors: 0

[breaking]
none
```

## 类型

| 类型 | 说明 |
|------|------|
| feat | 新功能 |
| fix | 修复 |
| refactor | 重构 |
| test | 测试 |
| docs | 文档 |
| chore | 杂项 |
| perf | 性能优化 |
| style | 代码风格 |
| ci | CI/CD |
| build | 构建系统 |
| revert | 回滚 |

## 范围

adc, dac, tim, dma, i2c, spi, uart, gpio, display, config, rtos, oled, key, flash, power, debug, system

## 工具

- project_scanner.py: 扫描项目配置
- hardware_detector.py: 检测硬件信息
- commit_generator.py: 生成提交信息
- version_bumper.py: 管理版本号
- changelog_generator.py: 生成变更日志

## 使用方法

```
用户: "提交代码"
→ 自动扫描项目
→ 检测硬件信息
→ 生成提交信息
→ 用户确认
→ 提交
```
```

- [ ] **Step 3: 验证目录结构**

```bash
ls -la ~/.agents/skills/commit-convention/
```

Expected: 看到 SKILL.md 和 4 个子目录

- [ ] **Step 4: Commit**

```bash
git add ~/.agents/skills/commit-convention/
git commit -m "feat: create commit-convention skill with SKILL.md"
```

---

### Task 2: 创建模板文件

**Files:**
- Create: `~/.agents/skills/commit-convention/templates/commit-template.txt`
- Create: `~/.agents/skills/commit-convention/templates/hardware-template.yaml`
- Create: `~/.agents/skills/commit-convention/templates/release-template.md`

**Interfaces:**
- Consumes: 无
- Produces: 模板文件，供 commit_generator.py 使用

- [ ] **Step 1: 创建 commit-template.txt**

创建 `~/.agents/skills/commit-convention/templates/commit-template.txt`：

```
{type}({scope}): {description}

[hardware]
mcu: {mcu}
clock: {clock}
peripherals:
{peripherals}

[test]
result: {test_result}
checks: {test_checks}
errors: {test_errors}

[breaking]
{breaking_description}
```

- [ ] **Step 2: 创建 hardware-template.yaml**

创建 `~/.agents/skills/commit-convention/templates/hardware-template.yaml`：

```yaml
mcu: STM32F407VET6
clock: 168MHz
voltage: 3.3V
peripherals:
  - ADC1: PA6, TIM8 trigger, DMA circular
  - DAC1: PA4, TIM5 trigger, DMA circular
  - TIM8: 10kHz trigger
  - TIM5: 328kHz trigger
  - I2C1: OLED display
  - USART2: Serial debug
pins:
  - PA6: ADC1_IN6
  - PA4: DAC1_OUT1
  - PB6: I2C1_SCL
  - PB7: I2C1_SDA
  - PA2: USART2_TX
  - PA3: USART2_RX
```

- [ ] **Step 3: 创建 release-template.md**

创建 `~/.agents/skills/commit-convention/templates/release-template.md`：

```markdown
# Release {version}

**日期**: {date}
**MCU**: {mcu}
**时钟**: {clock}

## 新功能

{features}

## Bug 修复

{fixes}

## 性能优化

{performance}

## 破坏性变更

{breaking_changes}

## 测试结果

- 测试通过: {test_pass}/{test_total}
- 编译警告: {warnings}
- 编译错误: {errors}
```

- [ ] **Step 4: Commit**

```bash
git add ~/.agents/skills/commit-convention/templates/
git commit -m "feat: add commit templates"
```

---

### Task 3: 创建配置文件

**Files:**
- Create: `~/.agents/skills/commit-convention/config/commit-convention.json`

**Interfaces:**
- Consumes: 无
- Produces: 配置文件，供工具使用

- [ ] **Step 1: 创建 commit-convention.json**

创建 `~/.agents/skills/commit-convention/config/commit-convention.json`：

```json
{
  "version": "1.0.0",
  "types": [
    "feat", "fix", "refactor", "test", "docs",
    "chore", "perf", "style", "ci", "build", "revert"
  ],
  "scopes": [
    "adc", "dac", "tim", "dma", "i2c", "spi", "uart",
    "gpio", "display", "config", "rtos", "oled", "key",
    "flash", "power", "debug", "system"
  ],
  "hardware": {
    "auto_detect": true,
    "sources": [".ioc", "main.c", "stm32f4xx_hal_msp.c"]
  },
  "test": {
    "auto_detect": true,
    "source": "verification-enforcement"
  },
  "hooks": {
    "pre_commit": true,
    "claude_code": true
  }
}
```

- [ ] **Step 2: Commit**

```bash
git add ~/.agents/skills/commit-convention/config/
git commit -m "feat: add commit convention config"
```

---

### Task 4: 实现项目扫描器

**Files:**
- Create: `~/.agents/skills/commit-convention/tools/project_scanner.py`

**Interfaces:**
- Consumes: 项目路径
- Produces: 项目配置信息（MCU、时钟、外设）

- [ ] **Step 1: 编写 project_scanner.py**

创建 `~/.agents/skills/commit-convention/tools/project_scanner.py`：

```python
#!/usr/bin/env python3
"""项目扫描器 - 扫描项目配置"""

import sys
import json
import os
import re


def scan_project(project_path):
    """扫描项目配置"""
    result = {
        "tool": "project_scanner",
        "project_path": project_path,
        "mcu": None,
        "clock": None,
        "peripherals": [],
        "pins": []
    }

    # 扫描 .ioc 文件
    ioc_path = os.path.join(project_path, "*.ioc")
    import glob
    ioc_files = glob.glob(ioc_path)
    if ioc_files:
        result.update(scan_ioc(ioc_files[0]))

    # 扫描 main.c
    main_c_path = os.path.join(project_path, "Core", "Src", "main.c")
    if os.path.exists(main_c_path):
        result.update(scan_main_c(main_c_path))

    # 扫描 MSP 初始化
    msp_path = os.path.join(project_path, "Core", "Src", "stm32f4xx_hal_msp.c")
    if os.path.exists(msp_path):
        result.update(scan_msp(msp_path))

    return result


def scan_ioc(ioc_path):
    """扫描 .ioc 文件"""
    result = {}

    with open(ioc_path, 'r') as f:
        content = f.read()

    # 提取 MCU
    mcu_match = re.search(r'Mcu\.STM32F(\d+)', content)
    if mcu_match:
        result["mcu"] = f"STM32F{mcu_match.group(1)}"

    # 提取时钟
    clock_match = re.search(r'RCC\.HSE_VALUE=(\d+)', content)
    if clock_match:
        result["clock"] = f"{int(clock_match.group(1)) // 1000000}MHz"

    return result


def scan_main_c(main_c_path):
    """扫描 main.c"""
    result = {"peripherals": []}

    with open(main_c_path, 'r') as f:
        content = f.read()

    # 检测 ADC
    if "ADC_HandleTypeDef hadc1" in content:
        result["peripherals"].append("ADC1")

    # 检测 DAC
    if "DAC_HandleTypeDef hdac" in content:
        result["peripherals"].append("DAC1")

    # 检测 TIM
    if "TIM_HandleTypeDef htim8" in content:
        result["peripherals"].append("TIM8")
    if "TIM_HandleTypeDef htim5" in content:
        result["peripherals"].append("TIM5")

    # 检测 I2C
    if "I2C_HandleTypeDef hi2c1" in content:
        result["peripherals"].append("I2C1")

    # 检测 UART
    if "UART_HandleTypeDef huart2" in content:
        result["peripherals"].append("USART2")

    return result


def scan_msp(msp_path):
    """扫描 MSP 初始化"""
    result = {"pins": []}

    with open(msp_path, 'r') as f:
        content = f.read()

    # 检测 GPIO 配置
    gpio_matches = re.findall(r'GPIO_PIN_(\d+)', content)
    for pin in set(gpio_matches):
        result["pins"].append(f"PA{pin}")

    return result


def main():
    """主函数"""
    if len(sys.argv) < 2:
        print(json.dumps({
            "status": "fail",
            "reason": "用法: project_scanner.py <project_path>"
        }))
        sys.exit(1)

    project_path = sys.argv[1]
    result = scan_project(project_path)

    print(json.dumps(result, indent=2))
    sys.exit(0)


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: 测试项目扫描器**

```bash
python ~/.agents/skills/commit-convention/tools/project_scanner.py "d:/stm32 _project_hal/scope-siggen"
```

Expected: 输出项目配置信息（MCU、时钟、外设列表）

- [ ] **Step 3: Commit**

```bash
git add ~/.agents/skills/commit-convention/tools/project_scanner.py
git commit -m "feat: add project scanner tool"
```

---

### Task 5: 实现硬件检测器

**Files:**
- Create: `~/.agents/skills/commit-convention/tools/hardware_detector.py`

**Interfaces:**
- Consumes: 项目路径
- Produces: 硬件信息（MCU、时钟、外设、引脚）

- [ ] **Step 1: 编写 hardware_detector.py**

创建 `~/.agents/skills/commit-convention/tools/hardware_detector.py`：

```python
#!/usr/bin/env python3
"""硬件检测器 - 检测硬件信息"""

import sys
import json
import os
import re


def detect_hardware(project_path):
    """检测硬件信息"""
    result = {
        "tool": "hardware_detector",
        "project_path": project_path,
        "mcu": "Unknown",
        "clock": "Unknown",
        "voltage": "3.3V",
        "peripherals": [],
        "pins": []
    }

    # 检测 MCU
    result["mcu"] = detect_mcu(project_path)

    # 检测时钟
    result["clock"] = detect_clock(project_path)

    # 检测外设
    result["peripherals"] = detect_peripherals(project_path)

    # 检测引脚
    result["pins"] = detect_pins(project_path)

    return result


def detect_mcu(project_path):
    """检测 MCU 型号"""
    # 从 .ioc 文件检测
    import glob
    ioc_files = glob.glob(os.path.join(project_path, "*.ioc"))
    if ioc_files:
        with open(ioc_files[0], 'r') as f:
            content = f.read()
            mcu_match = re.search(r'Mcu\.STM32F(\d+)', content)
            if mcu_match:
                return f"STM32F{mcu_match.group(1)}"

    # 从启动文件检测
    startup_files = glob.glob(os.path.join(project_path, "startup_stm32f*.s"))
    if startup_files:
        match = re.search(r'startup_stm32f(\d+)', startup_files[0])
        if match:
            return f"STM32F{match.group(1)}"

    return "Unknown"


def detect_clock(project_path):
    """检测时钟配置"""
    main_c = os.path.join(project_path, "Core", "Src", "main.c")
    if not os.path.exists(main_c):
        return "Unknown"

    with open(main_c, 'r') as f:
        content = f.read()

    # 检测 PLL 配置
    pll_match = re.search(r'PLLN\s*=\s*(\d+)', content)
    if pll_match:
        plln = int(pll_match.group(1))
        # 假设 HSE = 8MHz, PLLM = 8, PLLP = 2
        clock = (8000000 / 8 * plln / 2) / 1000000
        return f"{int(clock)}MHz"

    return "Unknown"


def detect_peripherals(project_path):
    """检测使用的外设"""
    peripherals = []

    main_c = os.path.join(project_path, "Core", "Src", "main.c")
    if not os.path.exists(main_c):
        return peripherals

    with open(main_c, 'r') as f:
        content = f.read()

    # ADC
    if "ADC_HandleTypeDef hadc1" in content:
        peripherals.append("ADC1")

    # DAC
    if "DAC_HandleTypeDef hdac" in content:
        peripherals.append("DAC1")

    # TIM
    if "TIM_HandleTypeDef htim8" in content:
        peripherals.append("TIM8")
    if "TIM_HandleTypeDef htim5" in content:
        peripherals.append("TIM5")

    # I2C
    if "I2C_HandleTypeDef hi2c1" in content:
        peripherals.append("I2C1")

    # UART
    if "UART_HandleTypeDef huart2" in content:
        peripherals.append("USART2")

    return peripherals


def detect_pins(project_path):
    """检测使用的引脚"""
    pins = []

    msp_path = os.path.join(project_path, "Core", "Src", "stm32f4xx_hal_msp.c")
    if not os.path.exists(msp_path):
        return pins

    with open(msp_path, 'r') as f:
        content = f.read()

    # 检测 GPIO 配置
    gpio_matches = re.findall(r'GPIO_PIN_(\d+)', content)
    for pin in set(gpio_matches):
        pins.append(f"PA{pin}")

    return pins


def main():
    """主函数"""
    if len(sys.argv) < 2:
        print(json.dumps({
            "status": "fail",
            "reason": "用法: hardware_detector.py <project_path>"
        }))
        sys.exit(1)

    project_path = sys.argv[1]
    result = detect_hardware(project_path)

    print(json.dumps(result, indent=2))
    sys.exit(0)


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: 测试硬件检测器**

```bash
python ~/.agents/skills/commit-convention/tools/hardware_detector.py "d:/stm32 _project_hal/scope-siggen"
```

Expected: 输出硬件信息（MCU、时钟、外设、引脚）

- [ ] **Step 3: Commit**

```bash
git add ~/.agents/skills/commit-convention/tools/hardware_detector.py
git commit -m "feat: add hardware detector tool"
```

---

### Task 6: 实现提交生成器

**Files:**
- Create: `~/.agents/skills/commit-convention/tools/commit_generator.py`

**Interfaces:**
- Consumes: 项目路径、提交类型、范围、描述
- Produces: 符合规范的提交信息

- [ ] **Step 1: 编写 commit_generator.py**

创建 `~/.agents/skills/commit-convention/tools/commit_generator.py`：

```python
#!/usr/bin/env python3
"""提交生成器 - 生成符合规范的提交信息"""

import sys
import json
import os


def generate_commit(project_path, commit_type, scope, description, test_result=None):
    """生成提交信息"""
    # 读取模板
    template_path = os.path.join(os.path.dirname(os.path.dirname(__file__)),
                                  "templates", "commit-template.txt")
    with open(template_path, 'r') as f:
        template = f.read()

    # 扫描项目
    sys.path.insert(0, os.path.dirname(__file__))
    from project_scanner import scan_project
    from hardware_detector import detect_hardware

    project_info = scan_project(project_path)
    hardware_info = detect_hardware(project_path)

    # 生成外设列表
    peripherals = ""
    for p in hardware_info.get("peripherals", []):
        peripherals += f"  - {p}\n"

    # 生成测试结果
    test_checks = "0/0"
    test_errors = "0"
    if test_result:
        test_checks = f"{test_result.get('pass', 0)}/{test_result.get('total', 0)}"
        test_errors = str(test_result.get('errors', 0))

    # 填充模板
    commit_msg = template.format(
        type=commit_type,
        scope=scope,
        description=description,
        mcu=hardware_info.get("mcu", "Unknown"),
        clock=hardware_info.get("clock", "Unknown"),
        peripherals=peripherals,
        test_result=test_result.get("result", "unknown") if test_result else "unknown",
        test_checks=test_checks,
        test_errors=test_errors,
        breaking_description="none"
    )

    return commit_msg


def main():
    """主函数"""
    if len(sys.argv) < 5:
        print(json.dumps({
            "status": "fail",
            "reason": "用法: commit_generator.py <project_path> <type> <scope> <description>"
        }))
        sys.exit(1)

    project_path = sys.argv[1]
    commit_type = sys.argv[2]
    scope = sys.argv[3]
    description = sys.argv[4]

    commit_msg = generate_commit(project_path, commit_type, scope, description)

    print(commit_msg)
    sys.exit(0)


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: 测试提交生成器**

```bash
python ~/.agents/skills/commit-convention/tools/commit_generator.py "d:/stm32 _project_hal/scope-siggen" feat adc "add DMA circular mode"
```

Expected: 输出符合规范的提交信息

- [ ] **Step 3: Commit**

```bash
git add ~/.agents/skills/commit-convention/tools/commit_generator.py
git commit -m "feat: add commit generator tool"
```

---

### Task 7: 实现版本号管理

**Files:**
- Create: `~/.agents/skills/commit-convention/tools/version_bumper.py`

**Interfaces:**
- Consumes: 版本号、变更类型
- Produces: 新版本号

- [ ] **Step 1: 编写 version_bumper.py**

创建 `~/.agents/skills/commit-convention/tools/version_bumper.py`：

```python
#!/usr/bin/env python3
"""版本号管理 - 自动管理版本号"""

import sys
import json
import re


def bump_version(current_version, bump_type):
    """递增版本号"""
    # 解析版本号
    match = re.match(r'(\d+)\.(\d+)\.(\d+)', current_version)
    if not match:
        return {"status": "error", "reason": f"无效版本号: {current_version}"}

    major, minor, patch = int(match.group(1)), int(match.group(2)), int(match.group(3))

    # 递增
    if bump_type == "major":
        major += 1
        minor = 0
        patch = 0
    elif bump_type == "minor":
        minor += 1
        patch = 0
    elif bump_type == "patch":
        patch += 1
    else:
        return {"status": "error", "reason": f"无效递增类型: {bump_type}"}

    new_version = f"{major}.{minor}.{patch}"

    return {
        "status": "success",
        "old_version": current_version,
        "new_version": new_version,
        "bump_type": bump_type
    }


def main():
    """主函数"""
    if len(sys.argv) < 3:
        print(json.dumps({
            "status": "error",
            "reason": "用法: version_bumper.py <current_version> <major|minor|patch>"
        }))
        sys.exit(1)

    current_version = sys.argv[1]
    bump_type = sys.argv[2]

    result = bump_version(current_version, bump_type)

    print(json.dumps(result, indent=2))
    sys.exit(0 if result["status"] == "success" else 1)


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: 测试版本号管理**

```bash
python ~/.agents/skills/commit-convention/tools/version_bumper.py "1.0.0" minor
```

Expected: `{"status": "success", "old_version": "1.0.0", "new_version": "1.1.0", "bump_type": "minor"}`

- [ ] **Step 3: Commit**

```bash
git add ~/.agents/skills/commit-convention/tools/version_bumper.py
git commit -m "feat: add version bumper tool"
```

---

### Task 8: 实现变更日志生成器

**Files:**
- Create: `~/.agents/skills/commit-convention/tools/changelog_generator.py`

**Interfaces:**
- Consumes: Git 提交历史
- Produces: 变更日志

- [ ] **Step 1: 编写 changelog_generator.py**

创建 `~/.agents/skills/commit-convention/tools/changelog_generator.py`：

```python
#!/usr/bin/env python3
"""变更日志生成器 - 基于提交历史生成变更日志"""

import sys
import json
import subprocess
import re


def generate_changelog(project_path, since_tag=None):
    """生成变更日志"""
    # 获取提交历史
    cmd = ["git", "-C", project_path, "log", "--oneline"]
    if since_tag:
        cmd.append(f"{since_tag}..HEAD")

    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        return {"status": "error", "reason": f"Git 错误: {result.stderr}"}

    commits = result.stdout.strip().split('\n')

    # 分类提交
    features = []
    fixes = []
    performance = []
    other = []

    for commit in commits:
        if not commit:
            continue

        # 解析提交信息
        match = re.match(r'^[a-f0-9]+ (feat|fix|perf|refactor|test|docs|chore|style|ci|build|revert)\((\w+)\): (.+)$', commit)
        if match:
            commit_type = match.group(1)
            scope = match.group(2)
            description = match.group(3)

            if commit_type == "feat":
                features.append(f"- {description} ({scope})")
            elif commit_type == "fix":
                fixes.append(f"- {description} ({scope})")
            elif commit_type == "perf":
                performance.append(f"- {description} ({scope})")
            else:
                other.append(f"- {description} ({scope})")

    # 生成变更日志
    changelog = "# Changelog\n\n"

    if features:
        changelog += "## 新功能\n\n"
        changelog += "\n".join(features) + "\n\n"

    if fixes:
        changelog += "## Bug 修复\n\n"
        changelog += "\n".join(fixes) + "\n\n"

    if performance:
        changelog += "## 性能优化\n\n"
        changelog += "\n".join(performance) + "\n\n"

    if other:
        changelog += "## 其他变更\n\n"
        changelog += "\n".join(other) + "\n\n"

    return {
        "status": "success",
        "changelog": changelog,
        "features": len(features),
        "fixes": len(fixes),
        "performance": len(performance),
        "other": len(other)
    }


def main():
    """主函数"""
    if len(sys.argv) < 2:
        print(json.dumps({
            "status": "error",
            "reason": "用法: changelog_generator.py <project_path> [since_tag]"
        }))
        sys.exit(1)

    project_path = sys.argv[1]
    since_tag = sys.argv[2] if len(sys.argv) > 2 else None

    result = generate_changelog(project_path, since_tag)

    if result["status"] == "success":
        print(result["changelog"])
    else:
        print(json.dumps(result))

    sys.exit(0 if result["status"] == "success" else 1)


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: 测试变更日志生成器**

```bash
python ~/.agents/skills/commit-convention/tools/changelog_generator.py "d:/stm32 _project_hal/scope-siggen"
```

Expected: 输出变更日志

- [ ] **Step 3: Commit**

```bash
git add ~/.agents/skills/commit-convention/tools/changelog_generator.py
git commit -m "feat: add changelog generator tool"
```

---

### Task 9: 实现 Git Hook

**Files:**
- Create: `~/.agents/skills/commit-convention/hooks/pre-commit`

**Interfaces:**
- Consumes: 提交信息
- Produces: 检查结果

- [ ] **Step 1: 编写 pre-commit hook**

创建 `~/.agents/skills/commit-convention/hooks/pre-commit`：

```bash
#!/bin/bash
# Pre-commit hook: 检查提交信息格式

# 获取提交信息
commit_msg=$(cat "$1")

# 检查格式
if ! echo "$commit_msg" | grep -qE "^(feat|fix|refactor|test|docs|chore|perf|style|ci|build|revert)\([a-z]+\): .+"; then
    echo "错误: 提交信息格式不正确"
    echo "期望格式: type(scope): description"
    echo "示例: feat(adc): add DMA circular mode"
    exit 1
fi

# 检查硬件信息
if ! echo "$commit_msg" | grep -q "\[hardware\]"; then
    echo "警告: 缺少 [hardware] 部分"
fi

# 检查测试结果
if ! echo "$commit_msg" | grep -q "\[test\]"; then
    echo "警告: 缺少 [test] 部分"
fi

echo "提交信息格式检查通过"
exit 0
```

- [ ] **Step 2: 设置执行权限**

```bash
chmod +x ~/.agents/skills/commit-convention/hooks/pre-commit
```

- [ ] **Step 3: Commit**

```bash
git add ~/.agents/skills/commit-convention/hooks/pre-commit
git commit -m "feat: add pre-commit hook"
```

---

### Task 10: 实现 Claude Code Hook

**Files:**
- Create: `~/.agents/skills/commit-convention/hooks/check_commit.py`

**Interfaces:**
- Consumes: 提交信息
- Produces: 检查结果

- [ ] **Step 1: 编写 check_commit.py**

创建 `~/.agents/skills/commit-convention/hooks/check_commit.py`：

```python
#!/usr/bin/env python3
"""Claude Code Hook: 检查提交信息格式"""

import sys
import json
import re


def check_commit(commit_msg):
    """检查提交信息"""
    result = {
        "valid": True,
        "errors": [],
        "warnings": []
    }

    # 检查格式
    pattern = r"^(feat|fix|refactor|test|docs|chore|perf|style|ci|build|revert)\(([a-z]+)\): (.+)$"
    match = re.match(pattern, commit_msg.split('\n')[0])

    if not match:
        result["valid"] = False
        result["errors"].append("提交信息格式不正确")
        result["errors"].append("期望格式: type(scope): description")
        result["errors"].append("示例: feat(adc): add DMA circular mode")
        return result

    # 检查类型
    commit_type = match.group(1)
    valid_types = ["feat", "fix", "refactor", "test", "docs", "chore", "perf", "style", "ci", "build", "revert"]
    if commit_type not in valid_types:
        result["valid"] = False
        result["errors"].append(f"无效类型: {commit_type}")
        result["errors"].append(f"有效类型: {', '.join(valid_types)}")

    # 检查范围
    scope = match.group(2)
    valid_scopes = ["adc", "dac", "tim", "dma", "i2c", "spi", "uart", "gpio", "display", "config", "rtos", "oled", "key", "flash", "power", "debug", "system"]
    if scope not in valid_scopes:
        result["warnings"].append(f"未知范围: {scope}")
        result["warnings"].append(f"已知范围: {', '.join(valid_scopes)}")

    # 检查硬件信息
    if "[hardware]" not in commit_msg:
        result["warnings"].append("缺少 [hardware] 部分")

    # 检查测试结果
    if "[test]" not in commit_msg:
        result["warnings"].append("缺少 [test] 部分")

    return result


def main():
    """主函数"""
    input_data = json.loads(sys.stdin.read())
    commit_msg = input_data.get("commit_message", "")

    result = check_commit(commit_msg)

    print(json.dumps(result))
    sys.exit(0 if result["valid"] else 1)


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: 测试 Claude Code Hook**

```bash
echo '{"commit_message": "feat(adc): add DMA circular mode"}' | python ~/.agents/skills/commit-convention/hooks/check_commit.py
```

Expected: `{"valid": true, "errors": [], "warnings": ["缺少 [hardware] 部分", "缺少 [test] 部分"]}`

- [ ] **Step 3: Commit**

```bash
git add ~/.agents/skills/commit-convention/hooks/check_commit.py
git commit -m "feat: add Claude Code hook"
```

---

### Task 11: 集成测试

**Files:**
- 无新文件

**Interfaces:**
- Consumes: 所有工具
- Produces: 测试结果

- [ ] **Step 1: 测试完整流程**

```bash
# 1. 扫描项目
python ~/.agents/skills/commit-convention/tools/project_scanner.py "d:/stm32 _project_hal/scope-siggen"

# 2. 检测硬件
python ~/.agents/skills/commit-convention/tools/hardware_detector.py "d:/stm32 _project_hal/scope-siggen"

# 3. 生成提交信息
python ~/.agents/skills/commit-convention/tools/commit_generator.py "d:/stm32 _project_hal/scope-siggen" feat adc "add DMA circular mode"

# 4. 测试版本号管理
python ~/.agents/skills/commit-convention/tools/version_bumper.py "1.0.0" minor

# 5. 测试变更日志生成
python ~/.agents/skills/commit-convention/tools/changelog_generator.py "d:/stm32 _project_hal/scope-siggen"
```

Expected: 所有工具正常工作

- [ ] **Step 2: 测试 Git Hook**

```bash
# 复制 hook 到项目
cp ~/.agents/skills/commit-convention/hooks/pre-commit "d:/stm32 _project_hal/scope-siggen/.git/hooks/"
chmod +x "d:/stm32 _project_hal/scope-siggen/.git/hooks/pre-commit"

# 测试提交
cd "d:/stm32 _project_hal/scope-siggen"
git commit --allow-empty -m "feat(adc): test commit"
```

Expected: 提交成功

- [ ] **Step 3: 测试 Claude Code Hook**

```bash
echo '{"commit_message": "feat(adc): add DMA circular mode\n\n[hardware]\nmcu: STM32F407VET6\nclock: 168MHz"}' | python ~/.agents/skills/commit-convention/hooks/check_commit.py
```

Expected: `{"valid": true, "errors": [], "warnings": []}`

- [ ] **Step 4: Commit**

```bash
git add ~/.agents/skills/commit-convention/
git commit -m "feat: commit-convention skill complete"
```

---

## 任务依赖

```
Task 1 (SKILL.md)
    ↓
Task 2 (模板) ← Task 3 (配置)
    ↓
Task 4 (项目扫描器) ← Task 5 (硬件检测器) ← Task 6 (提交生成器)
    ↓
Task 7 (版本号管理) ← Task 8 (变更日志生成器)
    ↓
Task 9 (Git Hook) ← Task 10 (Claude Code Hook)
    ↓
Task 11 (集成测试)
```

## 验收标准

1. 所有工具正常工作
2. Git Hook 正确检查格式
3. Claude Code Hook 正确检查格式
4. 完整流程测试通过
