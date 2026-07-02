# 代码前检查清单

**每次写外设代码前必须完成以下检查。**

---

## 定时器 (TIM)

### 必读
- [ ] RM0090 定时器章节（Page 515+）
- [ ] 时钟树：APB1 Timer Clock = ?

### 必查
- [ ] TRGO 公式: `TRGO = timer_clk / (PSC+1) / (ARR+1)`
- [ ] 反推 PSC: `PSC = timer_clk / (target_trgo * (ARR+1)) - 1`
- [ ] ARR 值是多少？不能忽略！
- [ ] MasterOutputTrigger 设为什么？（默认 RESET 不输出）

### 常见错误
- ❌ `PSC = timer_clk / target_trgo - 1`（忘记 ARR）
- ❌ TRGO 设为 RESET（不输出触发信号）
- ❌ 运行时修改 PSC/ARR 不加 EGR=UG

---

## ADC

### 必读
- [ ] RM0090 ADC 章节（Page 390+）
- [ ] 采样时间 vs 信号源阻抗

### 必查
- [ ] ExternalTrigConv 设为什么？（TIMx_TRGO？）
- [ ] ExternalTrigConvEdge 设为什么？（RISING？）
- [ ] ContinuousConvMode = DISABLE（外部触发模式）
- [ ] DMAContinuousRequests = ENABLE（循环采集）
- [ ] 采样时间：高阻信号源用 480 cycles

### 常见错误
- ❌ ContinuousConvMode = ENABLE（覆盖外部触发）
- ❌ DMAContinuousRequests = DISABLE（只采一次）
- ❌ 采样时间太短（高阻信号源读数偏小）

---

## DAC

### 必读
- [ ] RM0090 DAC 章节
- [ ] DAC 输出缓冲器阻抗

### 必查
- [ ] 触发源设为什么？（TIMx_TRGO？）
- [ ] DMA 使能了吗？
- [ ] 缓冲区大小和定时器频率匹配吗？

---

## DMA

### 必读
- [ ] RM0090 DMA 章节（Page 304+）

### 必查
- [ ] Direction: Peripheral → Memory 还是 Memory → Peripheral？
- [ ] Mode: Normal 还是 Circular？
- [ ] Data Width 匹配吗？（16-bit for ADC/DAC）
- [ ] __HAL_LINKDMA 把 DMA 链接到外设了吗？

---

## 启动顺序

```
1. HAL_xxx_Start_DMA()    ← 先启动 DMA
2. HAL_TIM_Base_Start()   ← 后启动定时器
```

**Why**: 定时器先启动可能在 DMA 就绪前就触发，导致数据丢失。

---

## 验证方法

烧录后用 `verify_config.py` 自动检查：

```bash
python scripts/verify_config.py
```

或手动检查：
1. 串口输出版本号
2. ADC 缓冲区范围 > 1000
3. DAC 输出电压用万用表量
