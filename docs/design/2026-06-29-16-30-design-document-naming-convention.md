# Document Naming Convention Design

**日期**: 2026-06-29
**状态**: 设计完成，待实现
**版本**: 1.0.0

---

## 1. 概述

### 1.1 目标

为嵌入式项目创建统一的文档命名规范，便于搜索、分类和管理。

### 1.2 核心方案

使用 `YYYY-MM-DD-HH-MM-type-topic.md` 格式命名所有文档。

---

## 2. 命名规范

### 2.1 基本格式

```
YYYY-MM-DD-HH-MM-type-topic.md
```

### 2.2 示例

```
2026-06-29-14-30-design-stm32-verifier.md
2026-06-29-15-00-plan-commit-convention.md
2026-06-29-15-30-report-code-review.md
2026-06-29-16-00-debug-adc-issue.md
2026-06-29-16-30-reflection-day3.md
2026-06-29-17-00-test-adc-verification.md
2026-06-29-17-30-config-hal-settings.md
2026-06-29-18-00-data-adc-samples.md
```

### 2.3 类型（type）

| 类型 | 说明 | 示例 |
|------|------|------|
| `design` | 设计文档 | `design-stm32-verifier.md` |
| `plan` | 实现计划 | `plan-commit-convention.md` |
| `report` | 报告 | `report-code-review.md` |
| `debug` | 调试日志 | `debug-adc-issue.md` |
| `reflection` | 反思 | `reflection-day3.md` |
| `guide` | 使用指南 | `guide-serial-debug.md` |
| `checklist` | 检查清单 | `checklist-compilation.md` |
| `spec` | 规格说明 | `spec-verification-flow.md` |
| `test` | 测试文档 | `test-adc-verification.md` |
| `config` | 配置文档 | `config-hal-settings.md` |
| `data` | 数据文档 | `data-adc-samples.md` |

### 2.4 主题（topic）

- 使用英文小写
- 用连字符分隔单词
- 简洁明了

---

## 3. 目录结构

```
docs/
├── design/           # 设计文档
├── plan/             # 实现计划
├── reports/          # 报告
├── debug_logs/       # 调试日志
├── guides/           # 使用指南
├── checklists/       # 检查清单
├── specs/            # 规格说明
├── tests/            # 测试文档
├── configs/          # 配置文档
├── data/             # 数据文档
└── hal-docs/         # HAL 库文档
```

---

## 4. 迁移计划

### 4.1 现有文档重命名

| 现有名称 | 新名称 |
|---------|--------|
| `2026-06-26-scope-siggen-design.md` | `2026-06-26-00-00-design-scope-siggen.md` |
| `2026-06-26-implementation-plan.md` | `2026-06-26-00-00-plan-implementation.md` |
| `2026-06-26-code-review.md` | `2026-06-26-00-00-report-code-review.md` |
| `2026-06-27-reflection.md` | `2026-06-27-00-00-reflection-day1.md` |
| `2026-06-27-dma-buffer-review.md` | `2026-06-27-00-00-report-dma-buffer-review.md` |

### 4.2 新文档

所有新文档必须遵循新命名规范。

---

**设计完成。**
