# 文档目录

**详细索引**: [INDEX.md](INDEX.md)

## 命名规范

所有文档使用 `YYYY-MM-DD-HH-MM-type-topic.md` 格式命名。

### 类型（type）

| 类型 | 说明 | 目录 |
|------|------|------|
| `design` | 设计文档 | `design/` |
| `plan` | 实现计划 | `plan/` |
| `config` | 配置文档 | `config/` |
| `constraint` | 约束文档 | `config/` |
| `anti-deception` | 防骗文档 | `config/` |
| `report` | 报告 | `reports/` |
| `debug` | 调试日志 | `debug_logs/` |
| `guide` | 使用指南 | `guides/` |
| `checklist` | 检查清单 | `checklists/` |

### 示例

```
2026-06-29-14-30-design-stm32-verifier.md
2026-06-29-15-00-plan-embedded-workflow.md
2026-06-29-19-00-config-project-constraints.md
2026-06-29-15-30-report-code-review.md
```

## 目录结构

```
docs/
├── design/           # 设计文档
│   ├─ 2026-06-29-14-30-design-stm32-verifier.md
│   ├─ 2026-06-29-15-00-design-verification-enforcement.md
│   ├─ 2026-06-29-15-30-design-commit-convention.md
│   ├─ 2026-06-29-16-00-design-embedded-workflow.md
│   └─ 2026-06-29-16-30-design-document-naming-convention.md
├── plan/             # 实现计划
│   ├─ 2026-06-29-14-30-plan-stm32-verifier.md
│   └─ 2026-06-29-15-00-plan-embedded-workflow.md
├── config/           # 配置文档
│   ├─ 2026-06-29-19-00-config-project-constraints.md
│   └─ 2026-06-29-19-30-config-anti-deception.md
├── reports/          # 报告
├── debug_logs/       # 调试日志
├── guides/           # 使用指南
├── checklists/       # 检查清单
├── hal-docs/         # HAL 库文档
├── RM0090.pdf        # 参考手册
└── README.md         # 本文件
```
