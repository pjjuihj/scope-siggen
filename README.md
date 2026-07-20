# Scope-SigGen

**STM32F4 示波器 + 信号发生器** — 基于 STM32F407 的便携式测量工具

[![Version](https://img.shields.io/badge/version-v1.4.4-blue)](docs/CHANGELOG.md)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)

## 功能特性

### 示波器
- 12-bit ADC 采样，最高 1 MSPS
- 实时波形显示（128x64 OLED）
- 自动测量：频率、周期、Vpp、Vrms、占空比
- 触发电平指示
- 时间轴缩放（1x/2x/4x/8x）

### 信号发生器
- 12-bit DAC 输出
- 支持波形：正弦波、方波、三角波、锯齿波
- 可调频率（1Hz ~ 100kHz）
- 可调幅值（0 ~ 3.3V）
- 可调占空比

### 系统功能
- 多页面显示：示波器、信号发生器、系统信息、菜单、消息、OTA
- 菜单导航系统
- 操作记录日志
- 配置保存到 Flash
- OTA 固件更新支持
- UART 调试接口

## 硬件要求

- **MCU**: STM32F411CEU6
- **显示屏**: 0.96" OLED (SSD1306, I2C, 128x64)
- **输入**: 2 个按键（PA0 页面切换，PA1 菜单选择）
- **输出**: DAC 输出 (PA4)
- **调试**: UART (PA9/PA10)

## 按键操作

| 按键 | 菜单页面 | 其他页面 |
|------|----------|----------|
| PA0 短按 | 切换页面 | 切换页面 |
| PA0 长按 | 确认选择 | 保存配置 |
| PA1 短按 | 移动选择 | 无功能 |

详细操作说明请参考 [按键操作指南](docs/BUTTON_GUIDE.md)

## 页面循环

```
OSCOPE → SIGGEN → SYSINFO → MENU → MESSAGE → OTA → OSCOPE
```

## 菜单选项

| 选项 | 功能 |
|------|------|
| Oscilloscope | 切换到示波器页面 |
| Signal Gen | 切换到信号发生器页面 |
| System Info | 查看系统信息 |
| About | 显示版本信息 |

## 项目结构

```
scope-siggen/
├── Core/
│   ├── Inc/              # 头文件
│   │   ├── display.h     # 显示模块接口
│   │   ├── key_handler.h # 按键处理接口
│   │   ├── version.h     # 版本信息
│   │   └── ...
│   └── Src/              # 源文件
│       ├── display.c     # 显示模块实现
│       ├── key_handler.c # 按键处理实现
│       ├── app_init.c    # 应用初始化
│       └── ...
├── docs/                 # 项目文档
│   ├── CHANGELOG.md      # 版本变更记录
│   ├── BUTTON_GUIDE.md   # 按键操作指南
│   └── ...
├── MDK-ARM/              # Keil 工程文件
└── tests/                # 测试代码
```

## 编译说明

### 环境要求
- Keil MDK-ARM 5.x
- STM32F4xx_DFP 包
- CMSIS 包

### 编译步骤
1. 打开 `MDK-ARM/scope-siggen.uvprojx`
2. 选择目标芯片：STM32F411CEUx
3. 点击 Build (F7)
4. 编译成功后生成 `.hex` 文件

## 烧录说明

### 使用 ST-Link
1. 连接 ST-Link 到 SWD 接口（SWDIO/SWCLK）
2. 在 Keil 中点击 Download (F8)
3. 或使用 STM32CubeProgrammer 烧录

### 使用串口烧录
1. 进入 Boot 模式（复位时按住 BOOT0）
2. 使用 STM32CubeProgrammer 通过 UART 烧录

## 调试接口

### UART 命令
连接 UART (115200 baud) 后可使用以下命令：

| 命令 | 说明 |
|------|------|
| `help` | 显示帮助信息 |
| `version` | 显示版本信息 |
| `status` | 显示系统状态 |
| `page next` | 切换到下一页 |
| `page prev` | 切换到上一页 |
| `zoom 1/2/4/8` | 设置时间轴缩放 |
| `measure` | 获取测量结果 |
| `config save` | 保存配置到 Flash |
| `config load` | 从 Flash 加载配置 |
| `reset` | 系统复位 |
| `tasks` | 显示任务状态 |
| `memory` | 显示内存状态 |

详细调试说明请参考 [串口调试指南](docs/guides/serial-debug-guide.md)

## 版本历史

查看 [CHANGELOG.md](docs/CHANGELOG.md) 了解完整版本历史。

### 最新版本 v1.4.4 (2026-07-19)
- 页面循环功能
- 菜单导航系统
- 操作记录功能
- 双按键支持

## 相关文档

- [按键操作指南](docs/BUTTON_GUIDE.md)
- [串口调试指南](docs/guides/serial-debug-guide.md)
- [系统设计文档](docs/design/2026-06-26-scope-siggen-design.md)
- [版本变更记录](docs/CHANGELOG.md)

## 许可证

MIT License

## 作者

cmj156
