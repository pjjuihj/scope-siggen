# 调试流程图

**遇到问题时按此流程执行，不要随机尝试。**

---

## Step 1: 收集数据

不要猜原因，先看数据。

```bash
# 串口输出
python -c "
import serial, time
ser = serial.Serial('COM3', 115200, timeout=2)
time.sleep(2)
ser.write(b'status\n')
time.sleep(1)
print(ser.read(ser.in_waiting or 1000).decode())
ser.close()
"
```

**记录**:
- 具体现象是什么？
- 数据值是多少？
- 什么时候发生？

---

## Step 2: 分类问题

| 现象 | 可能原因 | 验证方法 |
|------|---------|---------|
| 编译错误 | 语法/链接 | 看 build.log |
| 烧录失败 | 连接/配置 | 检查 ST-Link |
| 串口无输出 | UART 配置 | 示波器量 TX 引脚 |
| 数据全 0 | 外设未启动 | 读寄存器 CR bit0 |
| 数据不变 | DMA 未循环 | 检查 DMAContinuousRequests |
| 数据范围小 | 频率/采样问题 | 计算 min/max 范围 |
| 波形错误 | 配置/算法 | 方波测试 |

---

## Step 3: 方波测试

**最快验证 DAC/ADC 链路的方法。**

```c
// 临时替换波形生成函数
for (uint32_t i = 0; i < size; i++) {
    buffer[i] = (i < size/2) ? amplitude : 0;
}
```

**预期结果**:
- ADC min ≈ 116（低电平）
- ADC max ≈ 2041（高电平）
- 两个电平清晰分离

**如果方波正常**: 问题在波形生成算法
**如果方波异常**: 问题在硬件配置

---

## Step 4: 检查公式

### 定时器频率

```
TRGO = timer_clk / (PSC+1) / (ARR+1)
```

**常见错误**: 忘记 ARR+1

```python
# 验证计算
timer_clk = 84_000_000  # APB1 Timer Clock
psc = 327
arr = 255
trgo = timer_clk / (psc + 1) / (arr + 1)
print(f"TRGO = {trgo:.1f} Hz")  # 应该是 1001.2 Hz
```

### ADC 缓冲区范围

```
期望范围 = 2048 (12-bit 满量程)
实际范围 = max - min

如果 < 1000: DAC 频率太低
如果 ≈ 0: DAC 未输出或 ADC 未采样
```

---

## Step 5: 读寄存器

```c
// 串口输出寄存器值
char dbg[64];
snprintf(dbg, sizeof(dbg), "PSC=%lu ARR=%lu CNT=%lu\r\n",
         TIM5->PSC, TIM5->ARR, TIM5->CNT);
HAL_UART_Transmit(&huart1, (uint8_t*)dbg, strlen(dbg), 100);
```

**检查项**:
- PSC 和 ARR 是否正确？
- CNT 在变吗？（定时器在跑吗？）
- DMA CR 寄存器 bit 0 = 1？（DMA 启动了吗？）

---

## Step 6: 三次失败 → 停下来

```
尝试 1: 失败 → 分析原因
尝试 2: 失败 → 换方向
尝试 3: 失败 → 停下来，回滚代码，人工介入
```

**不要继续尝试同一个方向。**

---

## 常见问题速查

| 问题 | 检查 | 修复 |
|------|------|------|
| DAC 输出 0 | TIM5 启动了吗？ | HAL_TIM_Base_Start() |
| ADC 读数不变 | DMA 循环模式？ | DMAContinuousRequests=ENABLE |
| 频率不对 | PSC 公式对吗？ | 考虑 ARR+1 |
| 波形锯齿 | sinf() 工作吗？ | 用查表法 |
| 串口乱码 | 波特率对吗？ | 115200 |
| 命令不响应 | \r\n 处理了吗？ | 消耗剩余换行符 |
