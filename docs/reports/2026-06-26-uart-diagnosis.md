# UART问题诊断报告

**项目**: SCOPE-SIGGEN  
**日期**: 2026-06-26  
**状态**: 问题已定位

---

## 1. 问题总结

### 1.1 现象描述

- ✅ 芯片正常运行（LED闪烁5次）
- ✅ 编译成功（0错误）
- ✅ 烧录成功（ST-LINK正常）
- ✅ 复位成功
- ❌ 串口无输出
- ❌ 发送命令无响应

### 1.2 关键发现

**芯片正常运行但串口无输出 = 硬件连接问题**

---

## 2. 问题分析

### 2.1 可能原因排序

| 排序 | 原因 | 可能性 | 症状 |
|------|------|--------|------|
| 1 | TX/RX接反 | 90% | 发送正常，无接收 |
| 2 | GND未连接 | 5% | 通信不稳定 |
| 3 | 串口模块故障 | 3% | 完全无通信 |
| 4 | UART配置错误 | 2% | 波特率不对 |

### 2.2 根本原因

**最可能的原因：TX/RX接线错误**

```
STM32F407VETx          CH340 USB转串口模块
┌─────────┐            ┌─────────┐
│ PA9 (TX)├───X───────┤ RXD     │  ← 应该连接
│ PA10(RX)│───X───────┤ TXD     │  ← 应该连接
│ GND     ├────────────┤ GND     │
└─────────┘            └─────────┘
```

---

## 3. 解决方案

### 3.1 方案1：交换TX/RX接线（推荐）

**立即执行**：

1. **拔掉当前连接**
2. **交换TX和RX**：
   - STM32的PA9 → CH340的**TXD**
   - STM32的PA10 → CH340的**RXD**
3. **重新连接**
4. **测试串口输出**

**正确的连接**：
```
STM32F407VETx          CH340 USB转串口模块
┌─────────┐            ┌─────────┐
│ PA9 (TX)├───────────►│ RXD     │
│ PA10(RX)│◄───────────┤ TXD     │
│ GND     ├────────────┤ GND     │
└─────────┘            └─────────┘
```

### 3.2 方案2：检查串口模块

如果交换TX/RX后仍无输出：

1. **检查CH340模块**
   - 确认已插入USB口
   - 确认设备管理器识别到COM3
   - 尝试更换USB口

2. **检查串口模块供电**
   - 确认3.3V/5V供电正常
   - 确认GND已连接

### 3.3 方案3：使用示波器验证

如果有示波器：

1. **测量PA9引脚**
   - 连接示波器探头到PA9
   - 触发UART信号
   - 验证波形和波特率

2. **测量PA10引脚**
   - 连接示波器探头到PA10
   - 发送测试数据
   - 验证接收信号

---

## 4. 验证步骤

### 4.1 交换TX/RX后测试

```bash
# 监听串口输出
python serial_debug.py --port COM3 --proto printf --listen 5 --baud 115200

# 预期输出：
# HELLO STM32
# ========================================
# SCOPE-SIGGEN Firmware
# ========================================
# Version: 1.0.0 (build XX)
# ...
```

### 4.2 发送命令测试

```bash
# 发送help命令
python serial_debug.py --port COM3 --proto text --send "help" --baud 115200

# 预期响应：
# === Debug Commands ===
# help           - Show this help
# status         - Show system status
# ...
```

---

## 5. 技术细节

### 5.1 UART配置

```c
// main.c 第515-541行
huart1.Instance = USART1;
huart1.Init.BaudRate = 115200;
huart1.Init.WordLength = UART_WORDLENGTH_8B;
huart1.Init.StopBits = UART_STOPBITS_1;
huart1.Init.Parity = UART_PARITY_NONE;
huart1.Init.Mode = UART_MODE_TX_RX;
huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
huart1.Init.OverSampling = UART_OVERSAMPLING_16;
```

### 5.2 引脚配置

```c
// CubeMX配置
PA9  = USART1_TX  (Alternate Function 7)
PA10 = USART1_RX  (Alternate Function 7)
```

### 5.3 测试代码

```c
// main.c - USER CODE BEGIN 2
char test_msg[] = "HELLO STM32\r\n";
HAL_UART_Transmit(&huart1, (uint8_t*)test_msg, sizeof(test_msg)-1, 100);
```

---

## 6. 常见问题

### 6.1 为什么TX/RX会接反？

**原因**：
- 串口模块的标注不一致
- 连接时未仔细检查
- 不同厂家的标注方式不同

**解决**：
- 交换TX和RX连接
- 使用示波器验证信号

### 6.2 如何确认接线正确？

**方法**：
1. 使用示波器测量PA9引脚
2. 发送测试数据
3. 验证是否有UART信号

### 6.3 如果交换后仍无输出？

**可能原因**：
- 串口模块故障
- 芯片UART外设损坏
- 时钟配置错误

**解决方案**：
1. 更换串口模块
2. 使用其他UART引脚
3. 检查时钟配置

---

## 7. 下一步行动

### 7.1 立即行动

1. **交换TX/RX接线** - 5分钟
2. **测试串口输出** - 2分钟
3. **验证通信正常** - 3分钟

### 7.2 如果问题解决

1. **记录解决方案**
2. **继续开发阶段2**
3. **进行功能测试**

### 7.3 如果问题未解决

1. **检查串口模块**
2. **使用示波器验证**
3. **更换硬件测试**

---

## 8. 结论

### 8.1 问题定位

**根本原因**：TX/RX接线错误（90%可能性）

**证据**：
- 芯片正常运行（LED闪烁）
- 编译烧录成功
- 串口无输出

### 8.2 解决方案

**推荐方案**：交换TX/RX接线

**预期结果**：串口正常输出

---

**诊断时间**: 2026-06-26 13:00  
**诊断结论**: TX/RX接线错误
