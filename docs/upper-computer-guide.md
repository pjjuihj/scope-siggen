# 上位机使用指南

## 概述

本项目已集成上位机通信模块，支持将示波器数据实时发送到 PC 端上位机进行显示和分析。

## 文件结构

### 新增文件

```
Core/
├── Inc/
│   └── upper_computer.h      # 上位机通信模块头文件
└── Src/
    └── upper_computer.c      # 上位机通信模块实现

upper-computer/                # PC端上位机
├── server.js                  # Node.js 后端服务器
├── package.json               # 项目配置
├── README.md                  # 使用说明
├── start.bat                  # Windows 启动脚本
└── public/                    # 前端界面
    ├── index.html             # 主页面
    ├── style.css              # 样式文件
    └── app.js                 # 前端逻辑
```

### 修改文件

```
Core/Src/
├── oscilloscope.c             # 添加上位机数据发送
├── debug.c                    # 添加上位机控制命令
└── app_init.c                 # 添加上位机模块初始化
```

## 串口数据格式

上位机模块使用以下数据格式：

### 波形数据
```
WAVE:0A1B0C1D0E1F...（16进制ADC采样值）
```

### 频率数据
```
FREQ:1234.56（频率值，单位Hz）
```

### 电压数据
```
VOLT:0.123,3.210,3.087（最小电压,最大电压,峰峰值，单位V）
```

### 状态响应
```
OK:消息内容
ERROR:错误信息
INFO:信息内容
```

## 串口命令

### 上位机控制命令

| 命令 | 说明 |
|------|------|
| `uc status` | 显示上位机通信状态 |
| `uc stream on` | 启用自动流式传输 |
| `uc stream off` | 禁用自动流式传输 |
| `uc interval N` | 设置传输间隔（毫秒） |
| `uc send` | 手动发送一次波形数据 |
| `uc test` | 测试上位机通信 |

### 原有命令

| 命令 | 说明 |
|------|------|
| `help` | 显示帮助信息 |
| `status` | 显示系统状态 |
| `tasks` | 显示任务列表 |
| `memory` | 显示内存信息 |
| `version` | 显示版本信息 |
| `errors` | 显示错误历史 |
| `log N` | 设置日志级别 |
| `reset` | 系统复位 |

## 使用步骤

### 1. 编译固件

使用 Keil MDK 编译项目：

1. 打开 `MDK-ARM/scope-siggen.uvprojx`
2. 添加 `upper_computer.c` 到项目
3. 编译项目

### 2. 烧录固件

```bash
# 使用 ST-LINK 烧录
ST-LINK_CLI -c SWD -p scope-siggen.hex -V -Rst
```

### 3. 启动上位机

```bash
# 进入上位机目录
cd upper-computer

# 安装依赖（首次运行）
npm install

# 启动服务器
npm start
# 或者双击 start.bat
```

### 4. 连接设备

1. 打开浏览器访问 http://localhost:3000
2. 选择串口（通常是 COM3 或 COM4）
3. 选择波特率 115200
4. 点击"连接"按钮

### 5. 使用示波器

1. 点击"开始采集"按钮
2. 波形将实时显示在图表中
3. 测量数据（频率、电压）会自动更新

### 6. 使用信号发生器

1. 选择波形类型（正弦、方波、三角、锯齿、直流）
2. 设置频率和幅度
3. 点击"启动信号源"

## 代码集成说明

### 1. 添加上位机模块

将以下文件添加到项目：

- `Core/Inc/upper_computer.h`
- `Core/Src/upper_computer.c`

### 2. 包含头文件

在需要使用的文件中添加：

```c
#include "upper_computer.h"
```

### 3. 初始化模块

在 `app_init.c` 的 `App_StartModules` 函数中添加：

```c
/* 启动上位机通信模块 */
err = UpperComputer_Init();
if (err != ERR_OK) {
    LOG_ERROR("Upper computer init failed");
    return err;
}
```

### 4. 发送数据

#### 手动发送波形数据

```c
/* 获取ADC缓冲区 */
extern uint16_t adc_buffer[];
extern volatile uint32_t osc_config_buffer_size;

/* 发送波形数据 */
UC_SendWaveform(adc_buffer, osc_config_buffer_size);
```

#### 发送完整数据包

```c
/* 发送波形+测量数据 */
UC_SendCompletePacket(
    waveform_buffer,
    buffer_length,
    frequency_hz,
    min_voltage_mv,
    max_voltage_mv
);
```

#### 启用自动流式传输

```c
/* 启用流式传输 */
UC_SetStreamMode(true);

/* 设置传输间隔（可选，默认100ms） */
UC_SetStreamInterval(50);  /* 50ms间隔 */
```

## 自定义扩展

### 添加新的数据类型

1. 在 `upper_computer.h` 中定义新的前缀：

```c
#define UC_PREFIX_NEW     "NEW:"
```

2. 在 `upper_computer.c` 中添加发送函数：

```c
ErrorCode_t UC_SendNewData(uint32_t value)
{
    int offset = snprintf(tx_buffer, UC_MAX_PACKET_SIZE,
                          UC_PREFIX_NEW "%lu\r\n", value);
    return UC_SendData((uint8_t*)tx_buffer, offset);
}
```

3. 在 `upper_computer.h` 中声明函数：

```c
ErrorCode_t UC_SendNewData(uint32_t value);
```

### 修改数据格式

如果需要使用不同的数据格式（如JSON），修改相应的发送函数：

```c
ErrorCode_t UC_SendWaveformJSON(const uint16_t *data, uint16_t len)
{
    int offset = snprintf(tx_buffer, UC_MAX_PACKET_SIZE, "{\"waveform\":[");
    for (uint16_t i = 0; i < len; i++) {
        offset += snprintf(tx_buffer + offset, UC_MAX_PACKET_SIZE - offset,
                           "%s%u", (i > 0 ? "," : ""), data[i]);
    }
    offset += snprintf(tx_buffer + offset, UC_MAX_PACKET_SIZE - offset, "]}\r\n");
    return UC_SendData((uint8_t*)tx_buffer, offset);
}
```

## 故障排除

### 串口无法连接

1. 检查串口是否被其他程序占用（如串口调试助手）
2. 确认波特率设置正确（默认115200）
3. 检查USB转串口驱动是否安装

### 波形不显示

1. 确认设备已连接并开始采集
2. 检查浏览器控制台是否有错误
3. 确认WebSocket连接正常（端口3001）

### 数据不同步

1. 点击"清除"按钮清除旧数据
2. 重新开始采集
3. 检查采样率设置

### 命令无响应

1. 确认串口连接正常
2. 检查命令格式是否正确
3. 查看日志输出是否有错误信息

## 性能优化

### 减少数据量

1. 降低采样率
2. 减小缓冲区大小
3. 增加传输间隔

### 提高传输效率

1. 使用更高的波特率（如460800或921600）
2. 减少不必要的日志输出
3. 使用二进制格式代替文本格式

## 下一步开发

1. **添加数据记录功能**：将波形数据保存到文件
2. **添加FFT分析**：对波形进行频谱分析
3. **添加触发控制**：通过上位机设置触发电平和触发模式
4. **添加远程控制**：通过网络远程控制设备
5. **添加多设备支持**：同时连接多个设备

## 技术支持

如有问题，请查看：

1. 项目文档：`docs/` 目录
2. 调试日志：串口输出
3. 浏览器控制台：F12 开发者工具

## 许可证

MIT License
