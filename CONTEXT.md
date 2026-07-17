# CONTEXT.md — scope-siggen

## 项目概述

STM32F407 示波器 + 信号发生器固件项目。

## 核心模块

| 模块 | 文件 | 功能 |
|------|------|------|
| 示波器 | `oscilloscope.c` | ADC + DMA 采集，波形显示 |
| 信号发生器 | `signal_gen.c` | DAC + TIM 波形输出 |
| 显示 | `display.c` | OLED 渲染，菜单系统 |
| 按键 | `key_handler.c` | GPIO 扫描，消抖 |
| 串口协议 | `uart_protocol.c` | 命令解析，响应格式化 |
| 上位机 | `upper_computer.c` | 波形/状态流式传输 |
| 配置 | `config.c` | Flash 持久化存储 |
| 错误追踪 | `error_tracker.c` | 环形错误记录 |
| 版本 | `version.c` | 版本信息管理 |
| 环形缓冲 | `ring_buffer.c` | 生产者-消费者数据缓冲 |

## 硬件约束

- MCU: STM32F407
- ADC: 12-bit, 1MHz 采样率
- DAC: 12-bit, TIM5 触发
- OLED: I2C, 128x64
- 串口: USART1, 115200 baud
- FreeRTOS: 多任务架构

## 开发规则

详见 `CLAUDE.md` — 每次开始任务前必须读取。
