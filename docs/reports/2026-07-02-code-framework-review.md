# 代码框架评审报告

**评审日期**: 2026-07-02
**评审范围**: 模块架构、接口设计、依赖管理、代码质量
**代码规模**: 18 .c + 16 .h = 6923 行

---

## 一、模块架构

### 1.1 分层结构

```
┌─────────────────────────────────────────────────────────────────┐
│                         应用层 (Application)                      │
│  oscilloscope.c   signal_gen.c   display.c   uart_protocol.c    │
│  key_handler.c   app_init.c                                     │
├─────────────────────────────────────────────────────────────────┤
│                         服务层 (Service)                          │
│  config.c   error_tracker.c   ring_buffer.c   debug.c           │
│  version.c   code_reviewer.c                                     │
├─────────────────────────────────────────────────────────────────┤
│                         驱动层 (Driver)                           │
│  CubeMX 生成: main.c, stm32f4xx_it.c, stm32f4xx_hal_msp.c      │
│  OLED 驱动: Drivers/OLED/oled.c                                  │
└─────────────────────────────────────────────────────────────────┘
```

**评分**: 8/10

**优点**:
- CubeMX 生成代码与应用代码分离
- 模块职责单一，每个文件一个功能域
- 服务层提供通用能力（配置、错误追踪、调试）

**问题**:
- `main.c` 混合了 CubeMX 生成代码和用户初始化代码（751 行）
- `app_init.c` 与 `main.c` 的 USER CODE 区域职责重叠

---

## 二、接口设计

### 2.1 模块接口规范

所有模块遵循统一接口模式：

```c
ErrorCode_t Module_Init(void);          // 初始化
ErrorCode_t Module_Start(void);         // 启动
ErrorCode_t Module_Stop(void);          // 停止
ErrorCode_t Module_GetStatus(Status_t *status);  // 获取状态
ErrorCode_t Module_SetConfig(const Config_t *config);  // 设置配置
ErrorCode_t Module_GetConfig(Config_t *config);  // 获取配置
ErrorCode_t Module_SelfTest(void);      // 自检
void Module_Task(void *argument);       // FreeRTOS 任务
```

**评分**: 9/10

**优点**:
- 接口一致，易于理解和使用
- 错误码统一（ErrorCode_t）
- 配置与状态分离

### 2.2 接口统计

| 模块 | 导出函数 | 配置结构 | 状态结构 |
|------|---------|---------|---------|
| oscilloscope | 14 | OscConfig_t | OscStatus_t |
| signal_gen | 14 | SigGenConfig_t | SigGenStatus_t |
| display | 21 | - | - |
| uart_protocol | 14 | - | - |
| config | 12 | AppConfig_t | - |
| debug | 13 | - | - |
| error_tracker | 12 | ErrorRecord_t | - |

**问题**:
- `display.c` 导出 21 个函数，接口过多
- `uart_protocol.c` 直接调用其他模块内部函数

---

## 三、依赖管理

### 3.1 依赖关系图

```
uart_protocol ──→ oscilloscope
      │               │
      │               ├──→ display ──→ oled
      │               │
      ├──→ signal_gen ─┤
      │               │
      ├──→ config ────┘
      │
      ├──→ version
      │
      └──→ debug ──→ error_tracker

oscilloscope ──→ config
      │
      └──→ display

signal_gen ──→ config

key_handler ──→ (独立)

app_init ──→ 所有模块
```

**评分**: 6/10

**问题**:
1. **uart_protocol 耦合度高** — 直接调用 oscilloscope、signal_gen、display 的内部函数
2. **循环依赖风险** — oscilloscope 依赖 display，display 依赖 oled
3. **缺乏抽象层** — 模块间直接调用，没有接口抽象

**建议**:
```c
// 当前: 直接调用
Oscilloscope_Start();
SignalGen_SetFrequency(1000);

// 改进: 通过消息队列
App_PostMessage(MSG_START_OSC, NULL, 0);
App_PostMessage(MSG_SET_FREQ, &freq, sizeof(freq));
```

### 3.2 依赖统计

| 模块 | 被依赖次数 | 依赖其他模块数 |
|------|-----------|--------------|
| debug | 10 | 2 |
| config | 4 | 2 |
| error_tracker | 3 | 1 |
| oscilloscope | 3 | 4 |
| signal_gen | 2 | 2 |
| display | 2 | 2 |
| uart_protocol | 1 | 7 |

**关键发现**: `debug` 模块被 10 个模块依赖，是核心基础设施。`uart_protocol` 依赖 7 个模块，耦合度最高。

---

## 四、并发模型

### 4.1 FreeRTOS 任务

| 任务 | 优先级 | 栈大小 | 周期 | 触发方式 |
|------|--------|--------|------|---------|
| Oscilloscope | AboveNormal | 4KB | 事件驱动 | DMA 半/全传输中断 |
| SignalGen | Normal | 2KB | 1s | 定时器 |
| UART | Normal | 4KB | 事件驱动 | UART 中断 |
| Display | BelowNormal | 2KB | 100ms | 定时器 |
| Key | Low | 1KB | 10ms | 定时器 |

**评分**: 7/10

**优点**:
- 优先级合理：示波器 > 信号发生器 > 显示 > 按键
- 事件驱动减少 CPU 占用

**问题**:
- `SignalGen_Task` 栈只有 2KB，波形生成可能溢出
- `Display_Task` 周期 100ms，刷新率只有 10Hz

### 4.2 同步机制

| 资源 | 保护方式 | 位置 |
|------|---------|------|
| OLED I2C | 互斥锁 | display.c |
| ADC 缓冲区 | 半传输中断 + 信号量 | oscilloscope.c |
| DAC 缓冲区 | 互斥锁 | signal_gen.c |
| 配置结构体 | 互斥锁 | config.c |
| UART 接收 | 环形缓冲区 + 中断 | uart_protocol.c |

**问题**:
- 62 处互斥锁操作，部分可以优化为无锁设计
- 缺乏死锁检测机制

---

## 五、代码质量

### 5.1 代码规范

| 指标 | 评分 | 说明 |
|------|------|------|
| 命名规范 | 8/10 | 函数/变量命名清晰，遵循驼峰命名 |
| 注释密度 | 7/10 | 关键函数有注释，部分缺少 |
| 代码结构 | 8/10 | 模块划分清晰 |
| 错误处理 | 6/10 | 部分函数缺少错误检查 |
| 内存管理 | 7/10 | 无动态分配，静态分配 |

### 5.2 代码统计

```
文件                    行数    说明
─────────────────────────────────────
main.c                  751     CubeMX + 用户代码混合
system_stm32f4xx.c      749     CubeMX 生成
uart_protocol.c         729     命令处理
display.c               691     OLED 显示
oscilloscope.c          615     ADC 采集
signal_gen.c            604     DAC 输出
stm32f4xx_hal_msp.c     497     CubeMX 生成
stm32f4xx_it.c          350     中断处理
app_init.c              346     应用初始化
config.c                323     配置管理
debug.c                 294     调试工具
error_tracker.c         235     错误追踪
code_reviewer.c         211     代码审查
ring_buffer.c           198     环形缓冲
key_handler.c           183     按键处理
version.c               135     版本管理
─────────────────────────────────────
总计                    6923
```

### 5.3 代码异味 (Code Smells)

| # | 问题 | 位置 | 影响 |
|---|------|------|------|
| 1 | 函数过长 | Osc_HandleAdcData (80行) | 可读性差 |
| 2 | 魔数 | ADC_RESOLUTION=4096 | 维护困难 |
| 3 | 全局变量 | 多处 | 线程安全风险 |
| 4 | 重复代码 | 波形生成函数 | 维护成本高 |
| 5 | 缺少断言 | 关键函数 | 调试困难 |

---

## 六、架构改进建议

### 6.1 短期改进（1 周）

1. **提取 main.c 用户代码** — 将 CubeMX 生成代码与用户代码分离
2. **统一错误处理** — 添加断言宏和错误日志
3. **减少魔数** — 用宏定义替代硬编码值
4. **增加注释** — 关键函数添加注释

### 6.2 中期改进（1 个月）

1. **解耦 uart_protocol** — 通过消息队列通信
2. **抽象硬件层** — 创建 HAL 抽象层
3. **优化互斥锁** — 减少锁的使用，优化为无锁设计
4. **增加单元测试** — Unity 框架

### 6.3 长期改进（3 个月）

1. **重构 display 模块** — 减少接口数量
2. **添加配置验证** — 运行时配置校验
3. **性能优化** — DMA 优化、中断优化
4. **文档完善** — API 文档、架构图

---

## 七、总结

| 维度 | 评分 | 说明 |
|------|------|------|
| 模块架构 | 8/10 | 分层清晰，职责单一 |
| 接口设计 | 9/10 | 接口一致，易于使用 |
| 依赖管理 | 6/10 | uart_protocol 耦合度高 |
| 并发模型 | 7/10 | 优先级合理，但有优化空间 |
| 代码质量 | 7/10 | 命名规范，但有代码异味 |

**总体评价**: 代码框架设计良好，模块划分清晰，接口规范统一。主要问题是 `uart_protocol.c` 耦合度高，需要通过消息队列解耦。建议优先优化依赖关系和错误处理。
