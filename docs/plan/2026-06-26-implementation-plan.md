# SCOPE-SIGGEN 实现计划

**项目名称**: SCOPE-SIGGEN（示波器/信号发生器）  
**版本**: 1.0.0  
**日期**: 2026-06-26  
**状态**: 待执行

---

## 目录

1. [计划概述](#1-计划概述)
2. [开发阶段](#2-开发阶段)
3. [详细任务](#3-详细任务)
4. [里程碑](#4-里程碑)
5. [风险评估](#5-风险评估)
6. [依赖关系](#6-依赖关系)
7. [时间估算](#7-时间估算)

---

## 1. 计划概述

### 1.1 开发目标

按照设计文档，逐步实现SCOPE-SIGGEN项目的全部功能，确保代码质量、可维护性和可靠性。

### 1.2 开发原则

| 原则 | 描述 |
|------|------|
| **渐进式开发** | 从底层到上层，从简单到复杂 |
| **模块化实现** | 每个模块独立开发、测试 |
| **持续集成** | 每完成一个模块就进行编译验证 |
| **文档驱动** | 按照设计文档严格执行 |
| **质量优先** | 每个模块都必须通过代码审查 |

### 1.3 开发周期

**预计总时间**: 5-7天

| 阶段 | 时间 | 内容 |
|------|------|------|
| 阶段1 | 1天 | 基础框架搭建 |
| 阶段2 | 1天 | 服务层实现 |
| 阶段3 | 2天 | 应用层实现 |
| 阶段4 | 1天 | 集成测试 |
| 阶段5 | 1天 | 优化和完善 |

---

## 2. 开发阶段

### 2.1 阶段1：基础框架搭建

**目标**: 建立项目基础结构，实现最基础的模块

**任务清单**:
- [ ] 创建目录结构
- [ ] 实现版本管理模块 (version.c)
- [ ] 实现错误追踪模块 (error_tracker.c)
- [ ] 实现环形缓冲模块 (ring_buffer.c)
- [ ] 实现调试工具模块 (debug.c)
- [ ] 修改main.c添加版本信息输出
- [ ] 编译验证

**交付物**:
- 完整的目录结构
- 4个基础模块的完整实现
- 编译通过的项目

### 2.2 阶段2：服务层实现

**目标**: 实现配置管理和应用初始化框架

**任务清单**:
- [ ] 实现配置管理模块 (config.c)
- [ ] 实现应用初始化模块 (app_init.c)
- [ ] 创建FreeRTOS任务框架
- [ ] 实现消息队列
- [ ] 编译验证

**交付物**:
- 配置管理模块
- 应用初始化框架
- FreeRTOS任务结构

### 2.3 阶段3：应用层实现

**目标**: 实现核心功能模块

**任务清单**:
- [ ] 实现示波器模块 (oscilloscope.c)
- [ ] 实现信号发生器模块 (signal_gen.c)
- [ ] 实现串口协议模块 (uart_protocol.c)
- [ ] 实现显示模块 (display.c)
- [ ] 实现按键处理模块 (key_handler.c)
- [ ] 编译验证

**交付物**:
- 5个应用层模块的完整实现
- 完整的功能实现

### 2.4 阶段4：集成测试

**目标**: 模块集成和系统测试

**任务清单**:
- [ ] 模块间通信测试
- [ ] 数据流测试
- [ ] 错误处理测试
- [ ] 性能测试
- [ ] 代码审查

**交付物**:
- 集成测试报告
- 代码审查报告

### 2.5 阶段5：优化和完善

**目标**: 优化性能，完善文档

**任务清单**:
- [ ] 性能优化
- [ ] 内存优化
- [ ] 代码清理
- [ ] 文档更新
- [ ] 版本标记

**交付物**:
- 优化后的代码
- 完整的文档
- 稳定版本

---

## 3. 详细任务

### 3.1 版本管理模块 (version.c)

**优先级**: 高  
**预计时间**: 2小时  
**依赖**: 无

**实现任务**:
1. 创建 `Core/Inc/version.h`
   - 版本号定义
   - 版本信息结构体
   - 接口函数声明

2. 创建 `Core/Src/version.c`
   - 版本信息实例
   - Version_GetInfo() 实现
   - Version_Print() 实现
   - Version_IsDirty() 实现
   - Version_GetString() 实现

3. 创建 `scripts/build_version.py`
   - Git信息获取
   - 版本号自动更新
   - 自动提交功能

4. 修改 `main.c`
   - 在main()开头添加Version_Print()

**测试标准**:
- 编译通过
- 启动时显示版本信息
- 版本信息正确

### 3.2 错误追踪模块 (error_tracker.c)

**优先级**: 高  
**预计时间**: 2小时  
**依赖**: 无

**实现任务**:
1. 创建 `Core/Inc/error_tracker.h`
   - 错误码定义
   - 错误记录结构体
   - 接口函数声明

2. 创建 `Core/Src/error_tracker.c`
   - 错误记录数组
   - ErrorTracker_Init() 实现
   - ErrorTracker_Record() 实现
   - ErrorTracker_GetLastError() 实现
   - ErrorTracker_GetHistory() 实现
   - ErrorTracker_PrintHistory() 实现
   - ErrorTracker_Clear() 实现

**测试标准**:
- 编译通过
- 能记录错误
- 能查询错误历史

### 3.3 环形缓冲模块 (ring_buffer.c)

**优先级**: 高  
**预计时间**: 1.5小时  
**依赖**: 无

**实现任务**:
1. 创建 `Core/Inc/ring_buffer.h`
   - 环形缓冲结构体
   - 接口函数声明

2. 创建 `Core/Src/ring_buffer.c`
   - RingBuffer_Init() 实现
   - RingBuffer_Put() 实现
   - RingBuffer_Get() 实现
   - RingBuffer_Peek() 实现
   - RingBuffer_IsEmpty() 实现
   - RingBuffer_IsFull() 实现
   - RingBuffer_Count() 实现
   - RingBuffer_Free() 实现
   - RingBuffer_PutBlock() 实现
   - RingBuffer_GetBlock() 实现

**测试标准**:
- 编译通过
- 基本读写操作正确
- 边界条件处理正确

### 3.4 调试工具模块 (debug.c)

**优先级**: 高  
**预计时间**: 2小时  
**依赖**: version.c, error_tracker.c

**实现任务**:
1. 创建 `Core/Inc/debug.h`
   - 日志级别定义
   - 日志宏定义
   - 接口函数声明

2. 创建 `Core/Src/debug.c`
   - Debug_Init() 实现
   - Debug_Log() 实现
   - Debug_ProcessCommand() 实现
   - Debug_PrintSystemStatus() 实现
   - Debug_PrintTaskStatus() 实现
   - Debug_PrintMemoryStatus() 实现
   - Debug_GetCPUUsage() 实现

**测试标准**:
- 编译通过
- 日志输出正常
- 调试命令工作正常

### 3.5 配置管理模块 (config.c)

**优先级**: 中  
**预计时间**: 2小时  
**依赖**: error_tracker.c

**实现任务**:
1. 创建 `Core/Inc/config.h`
   - 配置结构体定义
   - 接口函数声明

2. 创建 `Core/Src/config.c`
   - Config_Init() 实现
   - Config_Load() 实现
   - Config_Save() 实现
   - Config_LoadDefaults() 实现
   - Config_Get() 实现
   - Config_SetOscConfig() 实现
   - Config_GetOscConfig() 实现
   - Config_SetSigGenConfig() 实现
   - Config_GetSigGenConfig() 实现

**测试标准**:
- 编译通过
- 能加载默认配置
- 能保存和加载配置

### 3.6 应用初始化模块 (app_init.c)

**优先级**: 中  
**预计时间**: 2小时  
**依赖**: 所有基础模块

**实现任务**:
1. 创建 `Core/Inc/app_init.h`
   - 接口函数声明

2. 创建 `Core/Src/app_init.c`
   - App_Init() 实现（统一入口）
   - 带错误恢复的启动流程
   - 启动进度显示
   - 降级模式支持

3. 修改 `main.c`
   - 在USER CODE区域调用App_Init()

**测试标准**:
- 编译通过
- 启动流程正常
- 错误恢复机制工作正常

### 3.7 示波器模块 (oscilloscope.c)

**优先级**: 中  
**预计时间**: 3小时  
**依赖**: ring_buffer.c, config.c

**实现任务**:
1. 创建 `Core/Inc/oscilloscope.h`
   - 配置和状态结构体
   - 接口函数声明

2. 创建 `Core/Src/oscilloscope.c`
   - Oscilloscope_Init() 实现
   - Oscilloscope_Start() 实现
   - Oscilloscope_Stop() 实现
   - Oscilloscope_GetStatus() 实现
   - Oscilloscope_SetConfig() 实现
   - Oscilloscope_GetConfig() 实现
   - Oscilloscope_GetVoltage() 实现
   - Oscilloscope_GetFrequency() 实现
   - Oscilloscope_SelfTest() 实现
   - Oscilloscope_Task() 实现

3. 修改 `stm32f4xx_it.c`
   - 添加ADC完成回调

**测试标准**:
- 编译通过
- ADC采集正常
- 数据处理正确
- 任务运行正常

### 3.8 信号发生器模块 (signal_gen.c)

**优先级**: 中  
**预计时间**: 3小时  
**依赖**: config.c

**实现任务**:
1. 创建 `Core/Inc/signal_gen.h`
   - 波形类型定义
   - 配置和状态结构体
   - 接口函数声明

2. 创建 `Core/Src/signal_gen.c`
   - SignalGen_Init() 实现
   - SignalGen_Start() 实现
   - SignalGen_Stop() 实现
   - SignalGen_GetStatus() 实现
   - SignalGen_SetConfig() 实现
   - SignalGen_GetConfig() 实现
   - SignalGen_SetWaveform() 实现
   - SignalGen_SetFrequency() 实现
   - SignalGen_SetAmplitude() 实现
   - SignalGen_SelfTest() 实现
   - SignalGen_Task() 实现
   - 波形生成函数（正弦、方波、三角波）

**测试标准**:
- 编译通过
- DAC输出正常
- 波形生成正确
- 频率控制正常

### 3.9 串口协议模块 (uart_protocol.c)

**优先级**: 中  
**预计时间**: 2.5小时  
**依赖**: config.c, error_tracker.c

**实现任务**:
1. 创建 `Core/Inc/uart_protocol.h`
   - 协议模式定义
   - 接口函数声明

2. 创建 `Core/Src/uart_protocol.c`
   - UART_Protocol_Init() 实现
   - UART_Protocol_Start() 实现
   - UART_Protocol_Stop() 实现
   - UART_SendText() 实现
   - UART_SendData() 实现
   - UART_SendResponse() 实现
   - UART_ReceiveCommand() 实现
   - UART_SetProtocolMode() 实现
   - UART_ParseCommand() 实现
   - 命令处理函数

3. 修改 `stm32f4xx_it.c`
   - 添加UART接收回调

**测试标准**:
- 编译通过
- 串口通信正常
- 命令解析正确
- 响应格式正确

### 3.10 显示模块 (display.c)

**优先级**: 低  
**预计时间**: 3小时  
**依赖**: config.c

**实现任务**:
1. 创建 `Core/Inc/display.h`
   - 菜单结构体
   - 接口函数声明

2. 创建 `Core/Src/display.c`
   - Display_Init() 实现
   - Display_Clear() 实现
   - Display_Update() 实现
   - Display_DrawWaveform() 实现
   - Display_DrawGrid() 实现
   - Display_ShowStatus() 实现
   - Display_ShowVoltage() 实现
   - Display_ShowFrequency() 实现
   - Display_ShowMessage() 实现
   - Display_Task() 实现

**测试标准**:
- 编译通过
- 显示初始化正常
- 波形绘制正确
- 信息显示正确

### 3.11 按键处理模块 (key_handler.c)

**优先级**: 低  
**预计时间**: 1.5小时  
**依赖**: 无

**实现任务**:
1. 创建 `Core/Inc/key_handler.h`
   - 按键码定义
   - 接口函数声明

2. 创建 `Core/Src/key_handler.c`
   - KeyHandler_Init() 实现
   - Key_Scan() 实现
   - Key_IsPressed() 实现
   - Key_WaitForKey() 实现
   - Key_RegisterCallback() 实现
   - Key_Task() 实现

**测试标准**:
- 编译通过
- 按键扫描正常
- 去抖处理正确

### 3.12 代码审查模块 (code_reviewer.c)

**优先级**: 低  
**预计时间**: 2小时  
**依赖**: debug.c, error_tracker.c

**实现任务**:
1. 创建 `Core/Inc/code_reviewer.h`
   - 检查项类型定义
   - 检查结果定义
   - 接口函数声明

2. 创建 `Core/Src/code_reviewer.c`
   - CodeReviewer_Init() 实现
   - CodeReviewer_CheckFile() 实现
   - CodeReviewer_CheckProject() 实现
   - CodeReviewer_RunFullCheck() 实现
   - CodeReviewer_PrintReport() 实现
   - CodeReviewer_CalculateScore() 实现
   - 设计规则检查函数

**测试标准**:
- 编译通过
- 检查功能正常
- 报告生成正确

---

## 4. 里程碑

### 4.1 里程碑1：基础框架完成

**时间**: 第1天结束  
**标志**:
- 版本管理模块完成
- 错误追踪模块完成
- 环形缓冲模块完成
- 调试工具模块完成
- 编译通过
- 启动时显示版本信息

### 4.2 里程碑2：服务层完成

**时间**: 第2天结束  
**标志**:
- 配置管理模块完成
- 应用初始化框架完成
- FreeRTOS任务结构完成
- 编译通过

### 4.3 里程碑3：核心功能完成

**时间**: 第4天结束  
**标志**:
- 示波器模块完成
- 信号发生器模块完成
- 串口协议模块完成
- 显示模块完成
- 按键处理模块完成
- 编译通过
- 基本功能可用

### 4.4 里程碑4：项目完成

**时间**: 第7天结束  
**标志**:
- 集成测试通过
- 代码审查通过
- 性能优化完成
- 文档完善
- 版本标记

---

## 5. 风险评估

### 5.1 技术风险

| 风险 | 概率 | 影响 | 应对措施 |
|------|------|------|---------|
| ADC/DMA配置复杂 | 中 | 高 | 参考CubeMX生成代码，逐步调试 |
| FreeRTOS任务调度问题 | 中 | 中 | 使用任务通知，减少同步复杂度 |
| 内存不足 | 低 | 高 | 优化缓冲区大小，使用静态分配 |
| 中断优先级冲突 | 中 | 中 | 仔细规划NVIC配置 |

### 5.2 进度风险

| 风险 | 概率 | 影响 | 应对措施 |
|------|------|------|---------|
| 某模块实现时间超预期 | 中 | 中 | 预留缓冲时间，必要时简化功能 |
| 调试时间过长 | 中 | 中 | 使用调试工具，记录问题 |
| 依赖硬件测试 | 低 | 高 | 先在仿真环境测试 |

### 5.3 质量风险

| 风险 | 概率 | 影响 | 应对措施 |
|------|------|------|---------|
| 代码质量不达标 | 低 | 中 | 严格执行代码审查 |
| 内存泄漏 | 低 | 高 | 使用静态分配，定期检查 |
| 线程安全问题 | 中 | 高 | 使用互斥锁，避免共享状态 |

---

## 6. 依赖关系

### 6.1 模块依赖图

```
version.c (无依赖)
    ↓
error_tracker.c (无依赖)
    ↓
ring_buffer.c (无依赖)
    ↓
debug.c (依赖: version.c, error_tracker.c)
    ↓
config.c (依赖: error_tracker.c)
    ↓
app_init.c (依赖: 所有基础模块)
    ↓
┌─────────────────────────────────────────────┐
│ 应用层模块                                    │
│ oscilloscope.c (依赖: ring_buffer.c, config.c)│
│ signal_gen.c (依赖: config.c)                │
│ uart_protocol.c (依赖: config.c, error_tracker.c)│
│ display.c (依赖: config.c)                   │
│ key_handler.c (无依赖)                       │
└─────────────────────────────────────────────┘
```

### 6.2 任务依赖

| 任务 | 依赖任务 |
|------|---------|
| version.c | 无 |
| error_tracker.c | 无 |
| ring_buffer.c | 无 |
| debug.c | version.c, error_tracker.c |
| config.c | error_tracker.c |
| app_init.c | 所有基础模块 |
| oscilloscope.c | ring_buffer.c, config.c |
| signal_gen.c | config.c |
| uart_protocol.c | config.c, error_tracker.c |
| display.c | config.c |
| key_handler.c | 无 |

---

## 7. 时间估算

### 7.1 详细时间表

| 任务 | 预计时间 | 实际时间 | 状态 |
|------|---------|---------|------|
| 版本管理模块 | 2小时 | - | 待开始 |
| 错误追踪模块 | 2小时 | - | 待开始 |
| 环形缓冲模块 | 1.5小时 | - | 待开始 |
| 调试工具模块 | 2小时 | - | 待开始 |
| 配置管理模块 | 2小时 | - | 待开始 |
| 应用初始化模块 | 2小时 | - | 待开始 |
| 示波器模块 | 3小时 | - | 待开始 |
| 信号发生器模块 | 3小时 | - | 待开始 |
| 串口协议模块 | 2.5小时 | - | 待开始 |
| 显示模块 | 3小时 | - | 待开始 |
| 按键处理模块 | 1.5小时 | - | 待开始 |
| 代码审查模块 | 2小时 | - | 待开始 |
| 集成测试 | 4小时 | - | 待开始 |
| 优化和完善 | 4小时 | - | 待开始 |
| **总计** | **33.5小时** | - | - |

### 7.2 每日计划

**第1天 (8小时)**:
- 上午: 版本管理模块、错误追踪模块
- 下午: 环形缓冲模块、调试工具模块

**第2天 (8小时)**:
- 上午: 配置管理模块
- 下午: 应用初始化模块、FreeRTOS任务框架

**第3天 (8小时)**:
- 上午: 示波器模块
- 下午: 信号发生器模块

**第4天 (8小时)**:
- 上午: 串口协议模块
- 下午: 显示模块、按键处理模块

**第5天 (8小时)**:
- 上午: 集成测试
- 下午: 代码审查、问题修复

**第6天 (8小时)**:
- 上午: 性能优化
- 下午: 内存优化、代码清理

**第7天 (8小时)**:
- 上午: 文档完善
- 下午: 版本标记、最终测试

---

## 8. 执行检查清单

### 8.1 每日检查

- [ ] 完成当天计划的任务
- [ ] 编译验证通过
- [ ] 代码审查通过
- [ ] 更新进度记录
- [ ] 记录遇到的问题

### 8.2 里程碑检查

- [ ] 所有里程碑任务完成
- [ ] 编译验证通过
- [ ] 功能测试通过
- [ ] 代码审查通过
- [ ] 文档更新完成

### 8.3 最终检查

- [ ] 所有功能实现
- [ ] 集成测试通过
- [ ] 性能测试通过
- [ ] 代码审查通过
- [ ] 文档完善
- [ ] 版本标记

---

**计划制定完成！**

下一步：开始执行阶段1 - 基础框架搭建
