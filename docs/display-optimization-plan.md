# 显示模块优化方案 ✅ 已完成

**完成日期**: 2026-06-27

## 实现内容

### 1. Display_UpdateScopeEx 扩展参数 ✅
- 新增 `trigger_level_mv`, `trigger_edge`, `sample_rate`, `trigger_mode` 参数
- 参数使用 `const uint16_t *data` 确保只读语义

### 2. 触发电平指示线 ✅
- `Draw_TriggerLevel()` 在波形区域画水平虚线
- 判断条件：`trigger_level_mv == 0` 时不显示

### 3. 中心十字线 ✅
- `Draw_Grid()` 画 y=31 水平实线 + x=64 垂直虚线
- 边框标记点每 16 像素

### 4. 信息栏 ✅
- `Draw_InfoBar()` 在 Page 6-7 显示：
  - Page 6 左侧：触发模式标记 (A/N/S)
  - Page 6 右侧：时基 T:xxxus
  - Page 7 左侧：触发电平 Tr/x.xV
  - Page 7 右侧：采样率 xxxK

### 5. 局部清屏 ✅
- `Clear_WaveformArea()` 只清除 y=10~47 的 GRAM
- 保留 Page 0（测量值）和 Page 6-7（信息栏）
- 页面切换时强制全屏清零

### 6. EMA 平滑缩放 ✅
- 均值±3σ 裁剪杂波尖峰
- EMA 跨帧平滑（alpha=0.3）
- 跳变检测：range 暴增 3x 时冻结缩放
- 宏定义：EMA_JUMP_THRESHOLD=3, EMA_MIN_RANGE=20

---

## 代码审查修复

| # | 严重度 | 问题 | 修复 |
|---|--------|------|------|
| 1 | Critical | trigger_enabled 导致触发电平线不显示 | 删除字段，改用 trigger_level_mv == 0 |
| 2 | High | EMA 每帧重置，平滑效果失效 | 只在 Display_Clear 时重置 |
| 3 | High | switch 用硬编码数字 | 改用 TriggerMode_t 枚举类型 |
| 4 | High | 触发居中可能越界 | 添加 remaining 裁剪 |
| 5 | Medium | "T" 标签混淆 | 触发电平改用 "Tr" 前缀 |
| 6 | Medium | 魔法数字 3, 20, 10 | 提取为宏定义 |
| 7 | Medium | const 指针被强转非 const | 参数改为 const uint16_t * |
| 8 | Low | 冗余 #ifndef ADC_REF_VOLTAGE | 删除 |

---

## 涉及文件

| 文件 | 改动 |
|------|------|
| Core/Inc/display.h | 新增 Display_UpdateScopeEx 声明，参数改为 const |
| Core/Src/display.c | 扩展 scope_state、新增 5 个函数、修改 Render_Scope/Draw_Grid |

---

**文档完成**
