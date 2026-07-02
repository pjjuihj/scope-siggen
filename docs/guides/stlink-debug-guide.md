# ST-LINK 调试指南

**项目**: SCOPE-SIGGEN  
**日期**: 2026-06-26

---

## 1. 调试环境准备

### 1.1 硬件准备

- ST-LINK V2调试器
- USB线缆
- STM32F407VETx开发板
- 杜邦线（连接SWD接口）

### 1.2 ST-LINK连接

```
ST-LINK V2              STM32F407VETx
┌─────────┐            ┌─────────┐
│ SWDIO   ├───────────┤ PA13    │
│ SWCLK   ├───────────┤ PA14    │
│ GND     ├───────────┤ GND     │
│ 3.3V    ├───────────┤ 3.3V    │
└─────────┘            └─────────┘
```

### 1.3 软件准备

- Keil MDK-ARM V5.32+
- ST-LINK驱动
- 固件已编译成功

---

## 2. Keil调试步骤

### 2.1 启动调试会话

1. 打开Keil工程：`MDK-ARM/scope-siggen.uvprojx`
2. 确保已编译成功（0 Error(s)）
3. 点击调试按钮：`Debug → Start/Stop Debug Session`
4. 或快捷键：`Ctrl+F5`

### 2.2 调试界面

调试启动后，应该看到：
- 代码窗口显示当前位置（通常是main函数）
- 寄存器窗口显示CPU状态
- 内存窗口可以查看内存

### 2.3 设置断点

1. 在代码行号左侧点击，设置断点
2. 断点显示为红色圆点
3. 建议设置的断点：
   - `main()` 函数入口
   - `Version_Print()` 函数
   - `HAL_UART_Transmit()` 函数

---

## 3. 调试UART问题

### 3.1 检查UART初始化

1. 在 `MX_USART1_UART_Init()` 函数设置断点
2. 运行到断点
3. 检查 `huart1` 结构体的值：
   ```c
   huart1.Instance = USART1
   huart1.Init.BaudRate = 115200
   huart1.Init.WordLength = UART_WORDLENGTH_8B
   huart1.Init.StopBits = UART_STOPBITS_1
   huart1.Init.Parity = UART_PARITY_NONE
   huart1.Init.Mode = UART_MODE_TX_RX
   ```

### 3.2 检查UART发送

1. 在 `HAL_UART_Transmit()` 调用处设置断点
2. 运行到断点
3. 检查参数：
   - `huart1`: UART句柄
   - `test_msg`: 要发送的数据
   - `sizeof(test_msg)-1`: 数据长度

### 3.3 单步执行

1. 使用 `Step Over` (F10) 单步执行
2. 观察每一步的执行结果
3. 检查是否有错误返回

---

## 4. 常见问题诊断

### 问题1：程序无法启动

**症状**：调试器无法连接或程序卡在某处

**可能原因**：
- ST-LINK连接错误
- 芯片损坏
- 时钟配置错误

**解决方案**：
1. 检查ST-LINK连接
2. 检查芯片供电
3. 检查时钟配置

### 问题2：UART发送失败

**症状**：`HAL_UART_Transmit()` 返回错误

**可能原因**：
- UART未正确初始化
- 引脚配置错误
- 时钟未使能

**解决方案**：
1. 检查UART初始化代码
2. 检查GPIO配置
3. 检查RCC时钟使能

### 问题3：程序卡死

**症状**：程序在某处停止执行

**可能原因**：
- 死循环
- 中断问题
- 硬件故障

**解决方案**：
1. 检查循环条件
2. 检查中断配置
3. 检查硬件连接

---

## 5. 调试命令

### 5.1 Keil调试命令

| 命令 | 快捷键 | 功能 |
|------|--------|------|
| Start Debug | Ctrl+F5 | 启动调试 |
| Stop Debug | Ctrl+F5 | 停止调试 |
| Step Over | F10 | 单步执行 |
| Step Into | F11 | 进入函数 |
| Step Out | Shift+F11 | 跳出函数 |
| Run to Cursor | Ctrl+F10 | 运行到光标处 |
| Set Breakpoint | F9 | 设置断点 |

### 5.2 调试窗口

| 窗口 | 功能 |
|------|------|
| Watch | 查看变量值 |
| Memory | 查看内存 |
| Registers | 查看寄存器 |
| Call Stack | 查看调用堆栈 |
| Peripherals | 查看外设状态 |

---

## 6. 检查UART外设

### 6.1 查看USART1寄存器

在调试器中查看USART1寄存器：

1. 打开 `Peripherals → USART` 窗口
2. 检查以下寄存器：
   - `SR` (Status Register): 状态寄存器
   - `DR` (Data Register): 数据寄存器
   - `BRR` (Baud Rate Register): 波特率寄存器
   - `CR1` (Control Register 1): 控制寄存器

### 6.2 检查关键位

**SR寄存器**：
- `TXE` (bit 7): 发送数据寄存器为空
- `TC` (bit 6): 发送完成
- `RXNE` (bit 5): 接收数据寄存器非空

**CR1寄存器**：
- `UE` (bit 13): USART使能
- `TE` (bit 3): 发送使能
- `RE` (bit 2): 接收使能

---

## 7. 调试示例

### 7.1 调试UART发送

```c
// 在此处设置断点
char test_msg[] = "HELLO STM32\r\n";
HAL_UART_Transmit(&huart1, (uint8_t*)test_msg, sizeof(test_msg)-1, 100);
```

**调试步骤**：
1. 设置断点在 `HAL_UART_Transmit` 行
2. 运行到断点
3. 检查 `test_msg` 的值
4. 检查 `huart1` 的状态
5. 单步执行，观察返回值

### 7.2 检查返回值

`HAL_UART_Transmit()` 返回值：
- `HAL_OK` (0): 成功
- `HAL_ERROR` (1): 错误
- `HAL_BUSY` (2): 忙
- `HAL_TIMEOUT` (3): 超时

---

## 8. 故障排除

### 8.1 调试器无法连接

**检查**：
- [ ] ST-LINK是否正确连接
- [ ] USB线缆是否正常
- [ ] 驱动是否安装
- [ ] 芯片是否供电

### 8.2 程序无法运行

**检查**：
- [ ] 是否设置了断点
- [ ] 是否点击了运行按钮
- [ ] 是否有死循环
- [ ] 是否有硬件故障

### 8.3 UART无输出

**检查**：
- [ ] UART是否正确初始化
- [ ] 引脚配置是否正确
- [ ] 串口接线是否正确
- [ ] 波特率是否匹配

---

## 9. 下一步行动

### 调试完成后

1. **记录调试结果**
2. **修复发现的问题**
3. **重新测试**

### 如果问题解决

1. **串口正常工作**
2. **继续开发阶段2**
3. **进行功能测试**

### 如果问题未解决

1. **检查硬件连接**
2. **更换调试方法**
3. **寻求专业帮助**

---

**文档创建时间**: 2026-06-26
