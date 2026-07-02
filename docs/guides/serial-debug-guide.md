# 串口调试指南

**项目**: SCOPE-SIGGEN  
**日期**: 2026-06-26

---

## 1. 串口配置

### 1.1 UART配置参数

| 参数 | 值 | 说明 |
|------|-----|------|
| 波特率 | 115200 | 通信速度 |
| 数据位 | 8 | 每个字节8位 |
| 停止位 | 1 | 1位停止位 |
| 校验位 | 无 | 无奇偶校验 |
| 流控制 | 无 | 无硬件流控 |

### 1.2 引脚配置

| 功能 | 引脚 | 说明 |
|------|------|------|
| USART1_TX | PA9 | 发送数据 |
| USART1_RX | PA10 | 接收数据 |

---

## 2. 硬件连接检查

### 2.1 连接示意图

```
STM32F407VETx          CH340 USB转串口模块
┌─────────┐            ┌─────────┐
│ PA9 (TX)├───────────►│ RXD     │
│ PA10(RX)│◄───────────┤ TXD     │
│ GND     ├────────────┤ GND     │
└─────────┘            └─────────┘
```

### 2.2 检查清单

- [ ] CH340模块已插入USB口
- [ ] 设备管理器中识别到COM3
- [ ] STM32的PA9连接到CH340的RXD
- [ ] STM32的PA10连接到CH340的TXD
- [ ] GND已连接
- [ ] STM32已供电（3.3V或5V）
- [ ] ST-LINK已连接（用于烧录）

### 2.3 常见连接错误

**错误1：TX/RX接反**
- 症状：发送数据正常，但收不到响应
- 解决：交换TX和RX连接

**错误2：GND未连接**
- 症状：通信不稳定或无响应
- 解决：连接GND

**错误3：波特率不匹配**
- 症状：收到乱码
- 解决：确保波特率为115200

---

## 3. 软件调试步骤

### 3.1 检查串口设备

```bash
python serial_monitor.py --list
```

应该看到：
```
可用串口:
  COM3: USB-SERIAL CH340 (COM3) (USB VID:PID=1A86:7523 SER= LOCATION=1-2.2)
```

### 3.2 监听串口输出

```bash
python serial_debug.py --port COM3 --proto printf --listen 10 --baud 115200
```

**预期输出**：
```
========================================
SCOPE-SIGGEN Firmware
========================================
Version: 1.0.0 (build 42)
Built: Jun 26 2026 12:39:48
Git: main@abc1234 (clean)
Compiler: Keil MDK-ARM
Target: STM32F407VETx
========================================
System initialized
```

**实际输出**：
```
（无输出）
```

### 3.3 发送测试命令

```bash
python serial_debug.py --port COM3 --proto text --send "help" --baud 115200
```

**预期响应**：
```
=== Debug Commands ===
help           - Show this help
status         - Show system status
tasks          - Show task list
memory         - Show memory info
version        - Show version info
errors         - Show error history
log <level>    - Set log level (0-4)
reset          - System reset
```

**实际响应**：
```
（无响应）
```

---

## 4. 可能的问题及解决方案

### 问题1：无串口输出

**可能原因**：
1. 串口接线错误
2. 芯片未正确运行
3. UART初始化失败
4. 版本信息输出代码未执行

**解决方案**：

1. **检查接线**
   - 确认TX/RX连接正确
   - 确认GND已连接

2. **检查芯片运行**
   - 观察LED是否闪烁（PB2）
   - 使用ST-LINK调试器单步调试

3. **检查UART初始化**
   - 在main.c中添加调试代码：
   ```c
   /* USER CODE BEGIN 2 */
   /* 测试UART输出 */
   char test_msg[] = "UART Test\r\n";
   HAL_UART_Transmit(&huart1, (uint8_t*)test_msg, strlen(test_msg), 100);
   /* USER CODE END 2 */
   ```

4. **检查版本信息输出**
   - 确认Version_Print()函数被调用
   - 检查Debug_Init()是否正确初始化

### 问题2：收到乱码

**可能原因**：
1. 波特率不匹配
2. 时钟配置错误

**解决方案**：

1. **检查波特率**
   - 确保使用115200波特率
   - 尝试其他波特率（9600, 38400, 57600）

2. **检查时钟配置**
   - 确认HSE配置正确
   - 确认PLL配置正确

### 问题3：发送命令无响应

**可能原因**：
1. 串口接收功能未正常工作
2. 命令解析代码未执行
3. 芯片未正确运行

**解决方案**：

1. **检查串口接收**
   - 在main.c中添加接收测试代码：
   ```c
   /* USER CODE BEGIN 2 */
   /* 测试UART接收 */
   uint8_t rx_data;
   if (HAL_UART_Receive(&huart1, &rx_data, 1, 100) == HAL_OK) {
       HAL_UART_Transmit(&huart1, &rx_data, 1, 100);  // 回显
   }
   /* USER CODE END 2 */
   ```

2. **检查命令解析**
   - 确认Debug_ProcessCommand()函数被调用
   - 检查命令格式是否正确

---

## 5. 调试技巧

### 5.1 使用LED指示

在main.c中添加LED闪烁代码，确认程序运行：

```c
/* USER CODE BEGIN 2 */
/* LED闪烁测试 */
HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
HAL_Delay(500);
HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
HAL_Delay(500);
/* USER CODE END 2 */
```

### 5.2 使用ST-LINK调试

1. 连接ST-LINK到STM32
2. 在Keil中启动调试会话
3. 设置断点在Version_Print()函数
4. 单步执行，观察变量值

### 5.3 使用逻辑分析仪

如果有逻辑分析仪，可以：
1. 连接到PA9引脚
2. 捕获UART波形
3. 验证波特率和数据格式

---

## 6. 测试命令列表

### 6.1 基本命令

| 命令 | 功能 | 预期响应 |
|------|------|---------|
| help | 显示帮助 | 命令列表 |
| status | 系统状态 | 状态信息 |
| version | 版本信息 | 版本详情 |
| memory | 内存信息 | 内存使用情况 |

### 6.2 调试命令

| 命令 | 功能 | 预期响应 |
|------|------|---------|
| errors | 错误历史 | 错误记录 |
| tasks | 任务列表 | 任务状态 |
| log 0 | 设置日志级别 | 确认信息 |
| reset | 系统复位 | 复位确认 |

---

## 7. 下一步行动

### 如果串口正常工作

1. **记录测试结果**
2. **继续开发阶段2**
3. **使用串口进行功能测试**

### 如果串口仍有问题

1. **检查硬件连接**
2. **使用ST-LINK调试**
3. **检查代码逻辑**

---

## 8. 快速参考

### 串口测试命令

```bash
# 列出串口
python serial_monitor.py --list

# 监听串口
python serial_debug.py --port COM3 --proto printf --listen 10 --baud 115200

# 发送命令
python serial_debug.py --port COM3 --proto text --send "help" --baud 115200

# 交互模式
python serial_monitor.py --port COM3 --mode interactive --baud 115200
```

---

**文档创建时间**: 2026-06-26
