# Embedded Workflow Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 创建通用嵌入式开发工作流技能，整合六个信息源，实现闭环验证

**Architecture:** 一个 SKILL.md 定义工作流，references/ 目录存储参考文档，tools/ 目录存储工具脚本。核心功能不依赖外部工具，网络资源是可选增强。

**Tech Stack:** Python, Markdown, Git

## Global Constraints

- 技能目录：`~/.agents/skills/embedded-workflow/`
- 核心原则：一切代码服务硬件
- 验证流程：每步有验收标准
- 防骗机制：AI 不能说"我验证了"
- 全局上下文：CLAUDE.md

---

## 文件结构

```
embedded-workflow/
├── SKILL.md
├── references/
│   ├── rm0090/
│   │   └── adc-config.md
│   ├── um1725/
│   │   └── hal-adc.md
│   ├── constraints/
│   │   └── stm32f407.md
│   └── templates/
│       ├── verification-code.md
│       └── checklist.md
└── tools/
    ├── manual_reader.py
    ├── constraint_checker.py
    └── verification_generator.py
```

---

### Task 1: 创建目录和 SKILL.md

**Files:**
- Create: `~/.agents/skills/embedded-workflow/SKILL.md`
- Create: `~/.agents/skills/embedded-workflow/references/`
- Create: `~/.agents/skills/embedded-workflow/tools/`

**Interfaces:**
- Consumes: 无
- Produces: SKILL.md 入口文件

- [ ] **Step 1: 创建目录结构**

```bash
mkdir -p ~/.agents/skills/embedded-workflow/references/rm0090
mkdir -p ~/.agents/skills/embedded-workflow/references/um1725
mkdir -p ~/.agents/skills/embedded-workflow/references/constraints
mkdir -p ~/.agents/skills/embedded-workflow/references/templates
mkdir -p ~/.agents/skills/embedded-workflow/tools
```

- [ ] **Step 2: 编写 SKILL.md**

创建 `~/.agents/skills/embedbed-workflow/SKILL.md`，内容如下：

```markdown
---
name: embedded-workflow
description: >
  通用嵌入式开发工作流。整合六个信息源（数据手册、HAL库、网络资源、项目文档、约束文档、时间约束），
  实现闭环验证。核心原则：一切代码服务硬件，先读手册再写代码，每写20行就验证。
  触发词：嵌入式开发、MCU编程、外设初始化、验证、代码审查。
license: MIT
metadata:
  author: cmj156
  version: "1.0.0"
  domain: specialized
  triggers: embedded, MCU, 外设初始化, 验证, 代码审查, 嵌入式开发
  role: specialist
  scope: workflow
  output-format: code + checklist
  related-skills: embedded-verification-methodology, stm32-verifier, perplexity-search, agent-reach-internet-access
---

# Embedded Workflow

通用嵌入式开发工作流。

## 核心原则

1. 一切代码服务硬件
2. 先读手册，再写代码
3. 每写 20 行就验证
4. 库有 bug，要了解已知问题
5. 先硬件后软件

## 开发规范

1. 小步迭代 MVP
2. 主动拆分模块
3. 限制 AI 修改范围
4. 死守安全底线
5. 科学应对报错
6. 两次修复无效 → 回滚

## 工作流

Step 0: 读取全局规则（CLAUDE.md）
Step 1: 判断任务类型
Step 2: 读数据手册（必须）
Step 3: 读 HAL 库手册（必须）
Step 4: 读项目文档（必须）
Step 5: 搜索网络资源（可选）
Step 6: 整合信息
Step 7: 生成验证代码（自动）
Step 8: 用户运行验证
Step 9: 修复（如果需要）
Step 10: 继续下一步

## 信息源

| 信息源 | 必须 | 说明 |
|--------|------|------|
| 数据手册 | ✅ | RM0090 - 寄存器定义、配置流程 |
| HAL 库手册 | ✅ | UM1725 - 函数签名、已知问题 |
| 项目文档 | ✅ | 设计文档、调试日志、检查清单 |
| 约束文档 | ✅ | 硬件约束、软件约束、流程约束 |
| 网络资源 | ⚠️ | GitHub Issues、社区讨论 |

## 防骗机制

AI 不能说"我验证了"：
- AI 只能输出代码和预期值
- AI 必须输出验证代码让用户运行
- AI 必须输出检查清单

## 验收标准

每步有验收标准，失败有闭环处理，循环最多 3 次。

## 使用方法

```
用户: "帮我配置 ADC+DMA"
→ 执行完整流程（Step 0-10）

用户: "修复这个 bug"
→ 执行简化流程（Step 0, 1, 4, 7, 8, 9, 10）

用户: "优化这段代码"
→ 执行快速流程（Step 0, 1, 7, 8, 10）
```
```

- [ ] **Step 3: 验证目录结构**

```bash
ls -la ~/.agents/skills/embedded-workflow/
```

Expected: 看到 SKILL.md 和 2 个子目录

- [ ] **Step 4: Commit**

```bash
git add ~/.agents/skills/embedded-workflow/
git commit -m "feat: create embedded-workflow skill with SKILL.md"
```

---

### Task 2: 创建参考文档

**Files:**
- Create: `~/.agents/skills/embedded-workflow/references/rm0090/adc-config.md`
- Create: `~/.agents/skills/embedded-workflow/references/um1725/hal-adc.md`
- Create: `~/.agents/skills/embedded-workflow/references/constraints/stm32f407.md`

**Interfaces:**
- Consumes: 无
- Produces: 参考文档，供 AI 读取

- [ ] **Step 1: 创建 adc-config.md**

创建 `~/.agents/skills/embedded-workflow/references/rm0090/adc-config.md`：

```markdown
# RM0090 ADC 配置参考

## ADC 外部触发配置

### EXTSEL[3:0] — 触发源选择

| 值 | 触发源 |
|----|--------|
| 0110 | Timer 2 TRGO |
| 0111 | Timer 3 CC1 |
| 1000 | Timer 3 TRGO |
| 1110 | Timer 8 TRGO |
| 1111 | SWSTART |

### EXTEN[1:0] — 触发极性

| 值 | 极性 |
|----|------|
| 00 | 禁用 |
| 01 | 上升沿 |
| 10 | 下降沿 |
| 11 | 双边沿 |

### SMPx[2:0] — 采样时间

| 值 | 采样时间 |
|----|---------|
| 000 | 3 cycles |
| 001 | 15 cycles |
| 010 | 28 cycles |
| 011 | 56 cycles |
| 100 | 84 cycles |
| 101 | 112 cycles |
| 110 | 144 cycles |
| 111 | 480 cycles |

### DDS — DMA 连续请求

| 值 | 说明 |
|----|------|
| 0 | 最后一次传输后不再发 DMA 请求 |
| 1 | 只要数据在转换就持续发 DMA 请求 |

### CONT — 连续模式

| 值 | 说明 |
|----|------|
| 0 | 单次模式（外部触发时必须用这个） |
| 1 | 连续模式（忽略外部触发） |

## 启动顺序

```
1. ADON = 1 (ADC on)
2. 配置 EXTSEL/EXTEN/DMA/DDS
3. HAL_ADC_Start_DMA() (等待触发)
4. HAL_TIM_Base_Start() (产生触发)
```

## 注意事项

- SWSTART 只能在 ADON = 1 时设置
- 触发源可以动态切换
- DDS 必须设为 1
- 外部触发时必须关闭 CONT
```

- [ ] **Step 2: 创建 hal-adc.md**

创建 `~/.agents/skills/embedded-workflow/references/um1725/hal-adc.md`：

```markdown
# UM1725 HAL ADC 参考

## HAL_ADC_Start_DMA 函数

```c
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef *hadc, uint32_t *pData, uint32_t Length);
```

### 参数

- `hadc`: ADC 句柄
- `pData`: 数据缓冲区指针
- `Length`: 传输长度

### 行为

1. 使能 ADC (ADON)
2. 配置 DMA 回调
3. 启动 DMA 传输
4. 如果是软件触发，立即启动转换

### 回调函数

| 回调 | 触发时机 |
|------|---------|
| HAL_ADC_ConvCpltCallback() | 转换完成 |
| HAL_ADC_ConvHalfCpltCallback() | 半传输完成 |
| HAL_ADC_ErrorCallback() | 错误 |

### 注意事项

- HAL_ADC_PollForConversion() 不能在 DMA 模式 + EOCSelection=SINGLE_CONV 时使用
- HAL_ADC_Stop() 会同时停止注入通道
- 溢出错误需要重新初始化 DMA
```

- [ ] **Step 3: 创建 stm32f407.md**

创建 `~/.agents/skills/embedded-workflow/references/constraints/stm32f407.md`：

```markdown
# STM32F407 约束

## 硬件约束

| 约束项 | 值 | 说明 |
|--------|-----|------|
| MCU | STM32F407VET6 | ARM Cortex-M4, 168MHz |
| Flash | 512KB | 程序存储 |
| RAM | 192KB | 数据存储（含 64KB CCM） |
| 电压 | 3.3V | 工作电压 |
| 时钟 | 168MHz | 系统时钟 |

## 外设约束

| 外设 | 引脚 | 配置 | 约束 |
|------|------|------|------|
| ADC1 | PA6 | TIM8 触发, DMA 循环 | 采样率 10kHz, 480 cycles |
| DAC1 | PA4 | TIM5 触发, DMA 循环 | 输出正弦波 |
| TIM8 | - | TRGO=UPDATE | 10kHz 触发 |
| TIM5 | - | TRGO=UPDATE | 328kHz 触发 |
| I2C1 | PB6/PB7 | 标准模式 | OLED 显示 |
| USART2 | PA2/PA3 | 115200 baud | 串口调试 |

## 配置约束

| 配置项 | 值 | 说明 |
|--------|-----|------|
| ADC 采样时间 | 480 cycles | 高阻信号源必须 |
| DMAContinuousRequests | ENABLE | 循环采集必须 |
| EOCSelection | SINGLE_CONV | DMA 模式必须 |
| TIM TRGO | UPDATE | 不是 RESET |
| 启动顺序 | 先 ADC DMA，后 TIM | RM0090 要求 |
```

- [ ] **Step 4: Commit**

```bash
git add ~/.agents/skills/embedded-workflow/references/
git commit -m "feat: add reference documents"
```

---

### Task 3: 创建工具脚本

**Files:**
- Create: `~/.agents/skills/embedded-workflow/tools/manual_reader.py`
- Create: `~/.agents/skills/embedded-workflow/tools/constraint_checker.py`
- Create: `~/.agents/skills/embedded-workflow/tools/verification_generator.py`

**Interfaces:**
- Consumes: 项目路径、外设类型
- Produces: 手册内容、约束检查结果、验证代码

- [ ] **Step 1: 创建 manual_reader.py**

创建 `~/.agents/skills/embedded-workflow/tools/manual_reader.py`：

```python
#!/usr/bin/env python3
"""手册读取器 - 读取本地 PDF 手册"""

import sys
import json
import os


def read_manual(manual_path, keyword):
    """读取手册并搜索关键词"""
    result = {
        "tool": "manual_reader",
        "manual_path": manual_path,
        "keyword": keyword,
        "pages": []
    }

    try:
        import PyPDF2

        if not os.path.exists(manual_path):
            result["status"] = "error"
            result["reason"] = f"手册不存在: {manual_path}"
            return result

        reader = PyPDF2.PdfReader(manual_path)
        result["total_pages"] = len(reader.pages)

        for i in range(len(reader.pages)):
            page = reader.pages[i]
            text = page.extract_text()
            if keyword.lower() in text.lower():
                result["pages"].append({
                    "page": i + 1,
                    "text": text[:500]
                })

        result["status"] = "success"
        result["found_pages"] = len(result["pages"])

    except ImportError:
        result["status"] = "error"
        result["reason"] = "未安装 PyPDF2"
    except Exception as e:
        result["status"] = "error"
        result["reason"] = str(e)

    return result


def main():
    """主函数"""
    if len(sys.argv) < 3:
        print(json.dumps({
            "status": "error",
            "reason": "用法: manual_reader.py <manual_path> <keyword>"
        }))
        sys.exit(1)

    manual_path = sys.argv[1]
    keyword = sys.argv[2]

    result = read_manual(manual_path, keyword)
    print(json.dumps(result, indent=2))
    sys.exit(0 if result["status"] == "success" else 1)


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: 创建 constraint_checker.py**

创建 `~/.agents/skills/embedded-workflow/tools/constraint_checker.py`：

```python
#!/usr/bin/env python3
"""约束检查器 - 检查项目约束"""

import sys
import json
import os
import re


def check_constraints(project_path):
    """检查项目约束"""
    result = {
        "tool": "constraint_checker",
        "project_path": project_path,
        "checks": []
    }

    # 检查硬件约束
    result["checks"].extend(check_hardware(project_path))

    # 检查软件约束
    result["checks"].extend(check_software(project_path))

    # 检查流程约束
    result["checks"].extend(check_process(project_path))

    # 统计结果
    pass_count = sum(1 for c in result["checks"] if c["status"] == "pass")
    fail_count = sum(1 for c in result["checks"] if c["status"] == "fail")

    if fail_count == 0:
        result["status"] = "pass"
        result["reason"] = f"约束检查通过，{pass_count} 项检查通过"
    else:
        result["status"] = "fail"
        result["reason"] = f"约束检查失败，{fail_count} 项检查失败"

    return result


def check_hardware(project_path):
    """检查硬件约束"""
    checks = []

    # 检查 MCU
    ioc_files = [f for f in os.listdir(project_path) if f.endswith('.ioc')]
    if ioc_files:
        with open(os.path.join(project_path, ioc_files[0]), 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
            if 'STM32F407' in content:
                checks.append({
                    "id": "HW-001",
                    "name": "MCU 型号",
                    "status": "pass",
                    "reason": "STM32F407"
                })
            else:
                checks.append({
                    "id": "HW-001",
                    "name": "MCU 型号",
                    "status": "fail",
                    "reason": "不是 STM32F407"
                })

    return checks


def check_software(project_path):
    """检查软件约束"""
    checks = []

    # 检查 HAL 库
    main_c = os.path.join(project_path, "Core", "Src", "main.c")
    if os.path.exists(main_c):
        with open(main_c, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
            if 'HAL_ADC_Start_DMA' in content:
                checks.append({
                    "id": "SW-001",
                    "name": "HAL 库使用",
                    "status": "pass",
                    "reason": "使用 HAL 库"
                })

    return checks


def check_process(project_path):
    """检查流程约束"""
    checks = []

    # 检查是否有 CLAUDE.md
    claude_md = os.path.join(project_path, "CLAUDE.md")
    if os.path.exists(claude_md):
        checks.append({
            "id": "PR-001",
            "name": "全局规则",
            "status": "pass",
            "reason": "CLAUDE.md 存在"
        })
    else:
        checks.append({
            "id": "PR-001",
            "name": "全局规则",
            "status": "fail",
            "reason": "CLAUDE.md 不存在"
        })

    return checks


def main():
    """主函数"""
    if len(sys.argv) < 2:
        print(json.dumps({
            "status": "error",
            "reason": "用法: constraint_checker.py <project_path>"
        }))
        sys.exit(1)

    project_path = sys.argv[1]
    result = check_constraints(project_path)

    print(json.dumps(result, indent=2))
    sys.exit(0 if result["status"] == "pass" else 1)


if __name__ == "__main__":
    main()
```

- [ ] **Step 3: 创建 verification_generator.py**

创建 `~/.agents/skills/embedded-workflow/tools/verification_generator.py`：

```python
#!/usr/bin/env python3
"""验证代码生成器 - 生成验证代码"""

import sys
import json
import os


def generate_verification(project_path, peripheral):
    """生成验证代码"""
    result = {
        "tool": "verification_generator",
        "project_path": project_path,
        "peripheral": peripheral,
        "code": "",
        "checklist": []
    }

    if peripheral == "adc":
        result["code"] = generate_adc_verification()
        result["checklist"] = get_adc_checklist()
    elif peripheral == "dac":
        result["code"] = generate_dac_verification()
        result["checklist"] = get_dac_checklist()
    elif peripheral == "tim":
        result["code"] = generate_tim_verification()
        result["checklist"] = get_tim_checklist()
    else:
        result["status"] = "error"
        result["reason"] = f"不支持的外设: {peripheral}"
        return result

    result["status"] = "success"
    return result


def generate_adc_verification():
    """生成 ADC 验证代码"""
    return '''
void Verify_ADC(void) {
    printf("=== ADC Verification ===\\r\\n");

    // 1. 检查 ADC 时钟
    uint32_t adc_en = RCC->APB2ENR & RCC_APB2ENR_ADC1EN;
    printf("[REG] ADC1 clock: %s\\r\\n", adc_en ? "ENABLED" : "DISABLED");

    // 2. 检查 GPIO 模式
    uint32_t pa6_mode = (GPIOA->MODER >> (6 * 2)) & 0x3;
    printf("[REG] PA6 mode: %s\\r\\n", pa6_mode == 0x3 ? "ANALOG" : "NOT ANALOG");

    // 3. 检查触发源
    uint32_t cr2 = ADC1->CR2;
    uint32_t extsel = (cr2 >> 24) & 0xF;
    printf("[REG] ADC_CR2 EXTSEL: %s (val=%lu)\\r\\n",
           extsel == 14 ? "TIM8 TRGO" : "NOT TIM8 TRGO", extsel);

    // 4. 检查采样时间
    uint32_t smpr2 = ADC1->SMPR2;
    uint32_t smp_ch6 = (smpr2 >> (6 * 3)) & 0x7;
    printf("[REG] ADC_SMPR2 CH6: %s (val=%lu)\\r\\n",
           smp_ch6 == 7 ? "480 CYCLES" : "NOT 480 CYCLES", smp_ch6);

    // 5. 检查 DMA 配置
    uint32_t dma_cr = DMA2_Stream0->CR;
    uint32_t dma_en = dma_cr & 0x1;
    uint32_t dma_circ = (dma_cr >> 8) & 0x1;
    printf("[REG] DMA2_S0 CR: EN=%lu, CIRC=%lu\\r\\n", dma_en, dma_circ);

    printf("=== Verification Complete ===\\r\\n");
}
'''


def get_adc_checklist():
    """获取 ADC 检查清单"""
    return [
        "ADC 时钟使能",
        "GPIO 模式为 ANALOG",
        "DMA 时钟使能",
        "触发源配置正确",
        "采样时间足够",
        "启动顺序正确"
    ]


def generate_dac_verification():
    """生成 DAC 验证代码"""
    return '''
void Verify_DAC(void) {
    printf("=== DAC Verification ===\\r\\n");

    // 1. 检查 DAC 时钟
    uint32_t dac_en = RCC->APB1ENR & RCC_APB1ENR_DACEN;
    printf("[REG] DAC clock: %s\\r\\n", dac_en ? "ENABLED" : "DISABLED");

    // 2. 检查 DAC 使能
    uint32_t cr = DAC->CR;
    uint32_t en = cr & 0x1;
    printf("[REG] DAC CR EN: %lu\\r\\n", en);

    // 3. 检查 DAC 输出
    uint32_t dor = DAC->DOR1;
    printf("[REG] DAC DOR1: %lu\\r\\n", dor);

    printf("=== Verification Complete ===\\r\\n");
}
'''


def get_dac_checklist():
    """获取 DAC 检查清单"""
    return [
        "DAC 时钟使能",
        "GPIO 模式为 ANALOG",
        "DMA 配置正确",
        "触发源配置正确",
        "TIM 启动"
    ]


def generate_tim_verification():
    """生成 TIM 验证代码"""
    return '''
void Verify_TIM(void) {
    printf("=== TIM Verification ===\\r\\n");

    // 1. 检查 TIM8 配置
    uint32_t psc = TIM8->PSC;
    uint32_t arr = TIM8->ARR;
    printf("[REG] TIM8 PSC=%lu, ARR=%lu\\r\\n", psc, arr);

    // 2. 检查 TRGO 配置
    uint32_t cr2 = TIM8->CR2;
    uint32_t mms = (cr2 >> 4) & 0x7;
    printf("[REG] TIM8 CR2 MMS: %s (val=%lu)\\r\\n",
           mms == 2 ? "UPDATE" : "NOT UPDATE", mms);

    // 3. 检查计数器
    uint32_t cnt1 = TIM8->CNT;
    HAL_Delay(100);
    uint32_t cnt2 = TIM8->CNT;
    printf("[REG] TIM8 CNT: %lu -> %lu\\r\\n", cnt1, cnt2);

    printf("=== Verification Complete ===\\r\\n");
}
'''


def get_tim_checklist():
    """获取 TIM 检查清单"""
    return [
        "TIM 时钟使能",
        "TRGO 配置正确",
        "PSC/ARR 配置正确",
        "计数器在变化"
    ]


def main():
    """主函数"""
    if len(sys.argv) < 3:
        print(json.dumps({
            "status": "error",
            "reason": "用法: verification_generator.py <project_path> <peripheral>"
        }))
        sys.exit(1)

    project_path = sys.argv[1]
    peripheral = sys.argv[2]

    result = generate_verification(project_path, peripheral)
    print(json.dumps(result, indent=2))
    sys.exit(0 if result["status"] == "success" else 1)


if __name__ == "__main__":
    main()
```

- [ ] **Step 4: Commit**

```bash
git add ~/.agents/skills/embedded-workflow/tools/
git commit -m "feat: add tool scripts"
```

---

### Task 4: 创建 CLAUDE.md 模板

**Files:**
- Create: `~/.agents/skills/embedded-workflow/references/templates/claude-md-template.md`

**Interfaces:**
- Consumes: 无
- Produces: CLAUDE.md 模板

- [ ] **Step 1: 创建 claude-md-template.md**

创建 `~/.agents/skills/embedded-workflow/references/templates/claude-md-template.md`：

```markdown
# 嵌入式项目全局规则

**每次开始任务时必须先读取本文档。**

---

## 核心原则

1. **一切代码服务硬件** — 代码不是独立存在的，是为了控制硬件
2. **先读手册，再写代码** — 没有手册就去下载，这是硬性要求
3. **每写 20 行就验证** — 不要写 400 行再测试
4. **库有 bug** — 写代码前必须了解库的已知问题
5. **先硬件后软件** — 读寄存器判断硬件状态

---

## 开发规范

### 小步迭代 MVP
- 每次只做一小块（~20 行）
- 做完就验证（编译 + 检查）
- 不要一次做太多（最多 50 行）

### 主动拆分模块
- 大功能拆成小模块
- 每个模块独立可测
- 模块之间接口清晰

### 限制 AI 修改范围
- 指定 AI 只改哪些文件
- 不让 AI 瞎改其他文件
- 每次改动范围明确

### 死守安全底线
- 不破坏已有功能
- 不引入新 bug
- 安全第一

### 科学应对报错
- 先分析原因
- 再修复代码
- 不要盲目试错

### 两次修复无效 → 回滚
- 第一次：让 AI 修复
- 第二次：让 AI 再试
- 还不行：回滚代码，人工介入

---

## 验证流程

### 每步验收标准
- Step 0: 判断任务类型 → 明确流程
- Step 1: 读手册 → 输出具体页码和内容
- Step 2: 生成代码 → 编译 0 errors
- Step 3: 用户验证 → 寄存器值正确
- Step 4: 修复（如果需要）→ 重新验证通过
- Step 5: 继续下一步 → 上一步全部通过

### 闭环处理
- 失败必须回到修复
- 修复后必须重新验证
- 循环最多 3 次
- 超过 3 次 → 停止，报告问题

---

## 信息源

### 必须读取
1. **数据手册** — 寄存器定义、配置流程、时序要求
2. **HAL 库手册** — 函数签名、参数说明、已知问题
3. **项目文档** — 设计文档、调试日志、检查清单
4. **约束文档** — 硬件约束、软件约束、流程约束

### 可选读取
5. **网络资源** — 已知问题、已验证配置
6. **社区讨论** — GitHub Issues、官方论坛

---

## 防骗机制

### AI 不能说"我验证了"
- AI 只能输出代码和预期值
- AI 必须输出验证代码让用户运行
- AI 必须输出检查清单

### 用户必须确认
- 关键步骤必须用户确认
- 用户说"确认"才能继续
- 用户说"不对"必须回退

### 工具必须验证
- 编译器验证（0 errors, 0 warnings）
- 静态分析验证（cppcheck）
- 串口验证（读寄存器值）
- 万用表/示波器验证（硬件测量）

---

## 相关技能

- **embedded-workflow** — 通用嵌入式开发工作流
- **embedded-verification-methodology** — 验证驱动的嵌入式开发方法论
- **stm32-verifier** — 自动化验证工具（62 项检查）

---

## 工作流

每次开始任务时：
1. 读取本文档了解项目规则
2. 读取相关技能 SKILL.md 了解工作流
3. 按照技能方法论执行任务
4. 完成后更新文档

---

**记住：一切代码服务硬件。先读手册，再写代码。每写 20 行就验证。**
```

- [ ] **Step 2: Commit**

```bash
git add ~/.agents/skills/embedded-workflow/references/templates/
git commit -m "feat: add CLAUDE.md template"
```

---

### Task 5: 测试技能

**Files:**
- 无新文件

**Interfaces:**
- Consumes: 所有工具
- Produces: 测试结果

- [ ] **Step 1: 测试手册读取器**

```bash
python ~/.agents/skills/embedded-workflow/tools/manual_reader.py "d:/stm32 _project_hal/scope-siggen/docs/RM0090.pdf" "ADC"
```

Expected: 输出找到的页数和内容

- [ ] **Step 2: 测试约束检查器**

```bash
python ~/.agents/skills/embedded-workflow/tools/constraint_checker.py "d:/stm32 _project_hal/scope-siggen"
```

Expected: 输出约束检查结果

- [ ] **Step 3: 测试验证代码生成器**

```bash
python ~/.agents/skills/embedded-workflow/tools/verification_generator.py "d:/stm32 _project_hal/scope-siggen" "adc"
```

Expected: 输出 ADC 验证代码和检查清单

- [ ] **Step 4: Commit**

```bash
git add ~/.agents/skills/embedded-workflow/
git commit -m "feat: embedded-workflow skill complete"
```

---

## 任务依赖

```
Task 1 (SKILL.md)
    ↓
Task 2 (参考文档)
    ↓
Task 3 (工具脚本)
    ↓
Task 4 (CLAUDE.md 模板)
    ↓
Task 5 (测试)
```

## 验收标准

1. SKILL.md 完整且 < 500 行
2. 参考文档完整（RM0090、UM1725、约束）
3. 工具脚本正常工作
4. CLAUDE.md 模板完整
5. 测试通过
