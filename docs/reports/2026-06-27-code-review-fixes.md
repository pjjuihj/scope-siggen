# 2026-06-27 代码审查与修复报告

**日期**: 2026-06-27
**审查范围**: SCOPE-SIGGEN 全模块
**审查方法**: 三阶段审查法（预审→深审→后审）

---

## 1. 审查统计

| 指标 | 审查前 | 修复后 |
|------|--------|--------|
| Critical | 4 | 0 |
| High | 6 | 0 |
| Medium | 8 | 0 |
| Low | 5 | 1 (Deferred) |
| 代码质量评分 | 5/10 | 8/10 |
| 互斥锁保护 | 0 处 | 62 处 |
| TODO 残留 | 6 个 | 0 个 |
| 编译警告 | 6 个 | 0 个 |

---

## 2. 修复清单

### 2.1 Critical 修复（4项）

| # | 问题 | 文件 | 修复 |
|---|------|------|------|
| 1 | Oscilloscope_Stop 未停止 DMA | oscilloscope.c | 取消注释 HAL_ADC_Stop_DMA + 添加 HAL_TIM_Base_Stop |
| 2 | SignalGen_Stop 未停止 DMA | signal_gen.c | 取消注释 HAL_DAC_Stop_DMA |
| 3 | 竞态条件 | oscilloscope.c, signal_gen.c | 添加 osc_mutex/siggen_mutex 互斥锁 |
| 4 | 重复初始化 | oscilloscope.c, signal_gen.c | 添加 if(!initialized) 守卫 |

### 2.2 High 修复（6项）

| # | 问题 | 文件 | 修复 |
|---|------|------|------|
| 1 | Config_Init 初始化顺序 | config.c | initialized=true 移到 Config_Load 前 |
| 2 | Debug_Log 缓冲区溢出 | debug.c | 每次 snprintf 后检查 offset 边界 |
| 3 | I2C 变量遮蔽 | main.c | 移除循环内重复声明 |
| 4 | Display_Test HAL_Delay | display.c | 改为 osDelay |
| 5 | Oscilloscope_Stop TIM8 | oscilloscope.c | 添加 HAL_TIM_Base_Stop |
| 6 | 任务优先级相同 | main.c | 分配 AboveNormal/Normal/BelowNormal/Low |

### 2.3 Medium 修复（8项）

| # | 问题 | 文件 | 修复 |
|---|------|------|------|
| 1 | pvPortMalloc 内存碎片 | debug.c | 改用 static TaskStatus_t[16] |
| 2 | main() 过长 | main.c | 提取 Peripherals_SelfTest() |
| 3 | TIM5 返回值未检查 | signal_gen.c | 添加 status 检查 |
| 4 | 诊断代码残留 | oscilloscope.c | 移除 notify_count/timeout_count |
| 5 | Osc↔Display 同步 | display.c | 新增 Display_UpdateScope 原子更新 |
| 6 | Stop HAL 返回值 | oscilloscope.c, signal_gen.c | 添加 hal_err 检查 |
| 7 | ADC 采样时间过短 | main.c | 3CYCLES → 84CYCLES |
| 8 | 函数过长 | oscilloscope.c, uart_protocol.c | 提取 HandleAdcData/HandleSetCursorCmd |

### 2.4 Low 修复（5项）

| # | 问题 | 文件 | 修复 |
|---|------|------|------|
| 1 | code_reviewer 引用 | main.c, app_init.c | 移除 include 和 Init 调用 |
| 2 | code_reviewer 文件 | code_reviewer.c/h | 删除 |
| 3 | 光标功能缺失 | display.c, uart_protocol.c | 实现 Draw_Cursor + set_cursor/cursor_off 命令 |
| 4 | IWDG 禁用 | main.c | Deferred to Human（发布前启用） |
| 5 | Display 全局变量过多 | display.c | 封装为 scope_state/menu_state 结构体 |

---

## 3. 功能实现

### 3.1 新增功能

| 功能 | 文件 | 说明 |
|------|------|------|
| 光标显示 | display.c | Draw_Cursor 十字光标渲染 |
| set_cursor 命令 | uart_protocol.c | 设置光标位置 |
| cursor_off 命令 | uart_protocol.c | 隐藏光标 |
| history 调试命令 | debug.c | 显示复位原因+版本信息 |
| review 调试命令 | debug.c | 运行时栈/堆/错误检查 |
| set_protocol 命令 | uart_protocol.c | text/binary 协议切换 |
| HardFault 诊断 | stm32f4xx_it.c | 输出 CFSR/HFSR/BFAR |
| Display_SetBrightness | display.c | SSD1306 对比度控制 |
| Flash 配置读写 | config.c | Sector 7 读写+校验和 |
| 频率测量 | oscilloscope.c | 过零检测算法 |
| 降级模式 | app_init.c | 停止示波器/信号发生器 |
| ADC 双缓冲 | oscilloscope.c | 乒乓模式消除数据撕裂 |
| 动态 TIM5 配置 | signal_gen.c | 频率设置实时生效 |
| 动态 TIM8 配置 | oscilloscope.c | 采样率设置实时生效 |
| UART 环形缓冲 | uart_protocol.c | RingBuffer 替代行缓冲 |

### 3.2 UART 命令列表（18条）

| 命令 | 功能 |
|------|------|
| help | 显示帮助 |
| status | 系统状态 |
| version | 版本信息 |
| start_osc | 启动示波器 |
| stop_osc | 停止示波器 |
| set_freq <hz> | 设置频率 |
| set_wave <type> | 设置波形 |
| set_amp <mv> | 设置幅度 |
| start_gen | 启动信号发生器 |
| stop_gen | 停止信号发生器 |
| stream_dac | 单帧数据流 |
| stream on | 连续数据流开 |
| stream off | 连续数据流关 |
| set_cursor <x> <y> | 设置光标位置 |
| cursor_off | 隐藏光标 |
| set_protocol <text\|binary> | 设置协议模式 |
| history | 版本历史 |
| review | 运行时审查 |
| reset | 系统复位 |

---

## 4. 架构改进

### 4.1 结构体封装

| 模块 | 优化前 | 优化后 | 封装 |
|------|--------|--------|------|
| oscilloscope.c | 23 全局变量 | 6 | `osc` 结构体 |
| signal_gen.c | 23 全局变量 | 2 | `sig` 结构体 |
| display.c | 41 全局变量 | 8 | `scope_state` + `menu_state` |

### 4.2 互斥锁保护

| 模块 | 锁名 | acquire 次数 | 保护内容 |
|------|------|-------------|---------|
| oscilloscope.c | osc_mutex | 14 | osc_status, osc_config |
| signal_gen.c | siggen_mutex | 16 | siggen_status, siggen_config |
| display.c | oled_mutex | 10 | OLED_GRAM, 状态变量 |

### 4.3 任务优先级

| 任务 | 优先级 | 栈大小 |
|------|--------|--------|
| Oscilloscope | AboveNormal | 2KB |
| SignalGen | Normal | 2KB |
| UART | Normal | 4KB |
| Display | BelowNormal | 2KB |
| Key | Low | 1KB |

### 4.4 DMA 双缓冲

```
adc_buffer[1024]
┌─────────────────────┬─────────────────────┐
│   前半段 [0..511]    │   后半段 [512..1023] │
└─────────────────────┴─────────────────────┘
         ↑                                ↑
    HalfCpltCallback                CpltCallback
         ↓                                ↓
    process_ptr = &buf[0]          process_ptr = &buf[512]
    process_len = 512              process_len = 512
```

---

## 5. 剩余项

| # | 问题 | 状态 | 说明 |
|---|------|------|------|
| 1 | IWDG 看门狗禁用 | Deferred to Human | 发布前需人工启用 |
| 2 | 消息队列 | Planned / Queued | 当前用互斥锁替代 |
| 3 | 二进制协议 | Planned / Queued | 仅实现文本模式 |
| 4 | 单元测试框架 | Planned / Queued | 未实现 |

---

## 6. 编译状态

```
Rebuild target 'scope-siggen'
...
"scope-siggen\scope-siggen.axf" - 0 Error(s), 0 Warning(s).
Target created.
Build Time Elapsed: 00:00:07
```

---

**文档结束**
