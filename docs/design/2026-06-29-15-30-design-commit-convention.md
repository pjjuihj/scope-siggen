# Commit Convention Design

**日期**: 2026-06-29
**状态**: 设计完成，待实现
**版本**: 1.0.0

---

## 1. 概述

### 1.1 目标

为嵌入式项目创建自定义提交规范，包含硬件信息、测试结果和外设配置。

### 1.2 核心方案

扩展 Conventional Commits 格式，添加硬件相关元数据。

---

## 2. 提交格式

### 2.1 基本格式

```
type(scope): description

[hardware]
mcu: STM32F407VET6
clock: 168MHz
peripherals:
  - ADC1: PA6, TIM8 trigger, DMA circular, 10kHz
  - DAC1: PA4, TIM5 trigger, DMA circular

[test]
result: pass
checks: 62/62
errors: 0

[breaking]
none
```

### 2.2 类型（type）

| 类型 | 说明 |
|------|------|
| `feat` | 新功能 |
| `fix` | 修复 |
| `refactor` | 重构 |
| `test` | 测试 |
| `docs` | 文档 |
| `chore` | 杂项 |
| `perf` | 性能优化 |
| `style` | 代码风格 |
| `ci` | CI/CD |
| `build` | 构建系统 |
| `revert` | 回滚 |

### 2.3 范围（scope）

| 范围 | 说明 |
|------|------|
| `adc` | ADC 相关 |
| `dac` | DAC 相关 |
| `tim` | 定时器相关 |
| `dma` | DMA 相关 |
| `i2c` | I2C 相关 |
| `spi` | SPI 相关 |
| `uart` | UART 相关 |
| `gpio` | GPIO 相关 |
| `display` | 显示相关 |
| `config` | 配置相关 |
| `rtos` | RTOS 相关 |
| `oled` | OLED 显示 |
| `key` | 按键输入 |
| `flash` | Flash 存储 |
| `power` | 电源管理 |
| `debug` | 调试相关 |
| `system` | 系统级 |

### 2.4 硬件信息（hardware）

```yaml
[hardware]
mcu: STM32F407VET6
clock: 168MHz
voltage: 3.3V
peripherals:
  - ADC1: PA6, TIM8 trigger, DMA circular, 10kHz
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

### 2.5 测试结果（test）

```yaml
[test]
result: pass|fail
checks: 62/62
warnings: 0
errors: 0
```

### 2.6 破坏性变更（breaking）

```yaml
[breaking]
description: "Changed ADC sampling time from 84 to 480 cycles"
migration: "Update your configuration to use ADC_SAMPLETIME_480CYCLES"
```

---

## 3. 执行机制

### 3.1 自动填充流程

```
1. 自动读取项目配置
   ├─ 从 .ioc 文件读取 MCU 型号
   ├─ 从 main.c 读取时钟配置
   └─ 从 CubeMX 配置读取外设列表

2. 自动检测硬件信息
   ├─ 扫描代码中的外设使用
   ├─ 扫描 GPIO 配置
   └─ 扫描 DMA 配置

3. 自动填充提交信息
   ├─ 填充 [hardware] 部分
   ├─ 填充 [test] 部分（如果测试结果可用）
   └─ 提示用户确认

4. 用户确认
   ├─ 显示自动生成的提交信息
   ├─ 用户可以修改
   └─ 确认后提交
```

### 3.2 检查流程

```
提交前检查：
├─ 检查提交信息格式
├─ 检查硬件信息完整性
├─ 检查测试结果
└─ 检查破坏性变更
```

---

## 4. 文件结构

```
commit-convention/
├── SKILL.md                    (技能定义)
├── templates/
│   ├── commit-template.txt     (提交信息模板)
│   ├── hardware-template.yaml  (硬件信息模板)
│   └── release-template.md     (发布说明模板)
├── hooks/
│   ├── pre-commit              (Git Hook)
│   └── check_commit.py         (Claude Code Hook)
├── tools/
│   ├── project_scanner.py      (项目扫描器)
│   ├── hardware_detector.py    (硬件检测器)
│   ├── commit_generator.py     (提交生成器)
│   ├── version_bumper.py       (版本号管理)
│   └── changelog_generator.py  (变更日志生成)
└── config/
    └── commit-convention.json  (配置文件)
```

---

## 5. 集成方式

| 集成点 | 功能 |
|--------|------|
| **Git Hook** | 提交前检查格式，自动填充硬件信息 |
| **Claude Code Hook** | AI 生成提交信息时检查格式 |
| **verification-enforcement** | 提交前必须通过验证 |
| **embedded-verification-methodology** | 提交规范遵循方法论 |
| **CI/CD** | 提交后自动检查格式，生成变更日志 |
| **GitHub Actions** | 自动发布、自动变更日志 |

---

## 6. 示例

### 6.1 新功能提交

```
feat(adc): add DMA circular mode

[hardware]
mcu: STM32F407VET6
clock: 168MHz
peripherals:
  - ADC1: PA6, TIM8 trigger, DMA circular, 10kHz

[test]
result: pass
checks: 62/62
errors: 0
```

### 6.2 Bug 修复提交

```
fix(dma): fix startup order

[hardware]
mcu: STM32F407VET6
clock: 168MHz
peripherals:
  - ADC1: PA6, TIM8 trigger, DMA circular
  - TIM8: 10kHz trigger

[test]
result: pass
checks: 62/62
errors: 0

[breaking]
description: "Changed startup order: ADC DMA before TIM8"
migration: "Update Oscilloscope_Start() to call HAL_ADC_Start_DMA() before HAL_TIM_Base_Start()"
```

### 6.3 性能优化提交

```
perf(display): optimize waveform rendering

[hardware]
mcu: STM32F407VET6
clock: 168MHz
peripherals:
  - OLED: I2C1, 128x64

[test]
result: pass
checks: 62/62
errors: 0
```

---

**设计完成。**
