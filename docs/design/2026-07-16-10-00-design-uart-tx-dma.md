# UART TX DMA 日志系统设计方案

**日期：** 2026-07-16
**版本：** v1.0
**状态：** 设计阶段

---

## 1. 目标

在现有 `LOG_*` 宏基础上，额外让 `printf()` 能输出到 UART，实现非阻塞、DMA 驱动的日志传输。

## 2. 现状分析

### 2.1 硬件平台
- MCU：STM32F407VETx（Cortex-M4，168MHz，192KB SRAM）
- UART：USART1（PA9_TX，PA10_RX），115200 8N1
- DMA：未配置（待启用）

### 2.2 软件现状
- 已有 `debug.c` 日志系统：5 级日志（DEBUG/INFO/WARNING/ERROR/FATAL）
- `LOG_*` 宏内部用 `snprintf()` + `HAL_UART_Transmit()` 阻塞发送
- `printf()` 未重定向，无法直接输出到 UART
- FreeRTOS 环境，Keil MDK-ARM（ARMCC V5），MicroLIB 关闭

### 2.3 现有任务栈使用

| 任务 | 栈大小 | 优先级 |
|------|--------|--------|
| defaultTask（喂狗） | 512 字节 | Normal |
| oscilloscope | 待确认 | 待确认 |
| signalgen | 待确认 | 待确认 |
| uart_protocol | 待确认 | 待确认 |
| display | 待确认 | 待确认 |
| key_handler | 待确认 | 待确认 |

## 3. 设计决策记录

| # | 决策 | 选项 | 最终选择 | 理由 |
|---|------|------|----------|------|
| D1 | LOG_* 与 printf 关系 | A:替换 B:共存 C:重做 | **B:共存** | 保留现有 LOG_* 体系，额外加 printf |
| D2 | printf 实现方式 | B1:_write B2:MicroLIB B3:ARMCLANG | **B1:_write** | 不改 Keil 配置，标准库完整 |
| D3 | 缓冲策略 | B1-a:阻塞 B1-b:ring+DMA B1-c:ring+轮询 | **B1-b:ring+DMA** | 非阻塞，适合多任务环境 |
| D4 | TX ring buffer 大小 | 256/512/1024 | **1024 字节** | 安全余量大 |
| D5 | DMA 映射 | Stream7/Stream6 | **DMA2 Stream7 CH4** | 标准映射，不冲突 |
| D6 | 发送任务触发方式 | i:_write启动 ii:idle中断 iii:发送任务 | **iii:发送任务** | 逻辑清晰，易维护 |
| D7 | 事件机制 | a:事件标志 b:信号量 c:轮询 | **a:事件标志组** | 零延迟，最精确 |
| D8 | 事件标志定义 | DATA_READY + DMA_DONE | **两个标志** | 流水线支持 |
| D9 | 取数据方式 | 连续块/tail为止 | **tail 为止** | 一次 DMA 搬完 |
| D10 | DMA 单次长度 | 不限制/256/128 | **不限制** | 日志量小，一次搬完 |
| D11 | DMA 中断类型 | TC only/TC+Half/TC+Error | **TC+Error** | 安全网 |
| D12 | _write() 线程安全 | 互斥量/关中断/不保护 | **互斥量** | 标准做法 |
| D13 | 环形回绕处理 | 分两段/复制/只搬一端 | **只搬一端** | 最简单 |
| D14 | LOG_* 调用路径 | 调 printf/直接塞 buffer/调 _write | **调 printf** | 统一走 _write() |
| D15 | _write() 接口 | fd 处理/返回值 | stdout+stderr，返回 len 或 0 | 标准接口 |
| D16 | DMA Error 处理 | 中断全做/中断停+任务重启 | **中断停+任务重启** | 中断短，安全 |
| D17 | 文件结构 | 改 debug.c/新建 uart_tx | **新建 uart_tx.c/h** | 解耦，可复用 |
| D18 | HAL 回调位置 | uart_tx.c/stm32f4xx_it.c | **uart_tx.c** | 内聚性好 |
| D19 | DMA 初始化 | CubeMX/手写 | **CubeMX** | 标准，可靠 |
| D20 | Buffer 满策略 | 丢弃/覆盖/阻塞/丢弃+记录 | **丢弃+计数器** | 知道丢了什么 |
| D21 | _write() mutex 超时 | 无超时/100ms | **100ms** | 防死锁 |
| D22 | ISR 保护 | 检查 ISR 上下文 | **xPortIsInsideInterrupt()** | 防 Hard Fault |
| D23 | _write() 约束 | 只做三件事 | **确认** | 防意外阻塞 |
| D24 | DMA 超时 | 100ms + AbortTransmit + break | **确认** | 防卡死 |
| D25 | 发送任务心跳 | 递增计数器 | **确认** | 喂狗前检查存活 |
| D26 | FPU | 启用 | **configENABLE_FPU = 1** | 防浮点数据损坏 |
| D27 | 堆大小 | 15KB → 24KB | **24576 字节** | 留足余量 |
| D28 | 发送任务栈 | 512 → 1024 | **1024 字节** | printf 调用链安全 |
| D29 | DMA 缓冲区对齐 | __attribute__((aligned(4))) | **确认** | DMA 要求 |
| D30 | FreeRTOS 栈溢出检查 | configCHECK_FOR_STACK_OVERFLOW = 2 | **确认** | 检测栈溢出 |
| D31 | Hard Fault 处理 | 不喂狗，让看门狗复位 | **确认** | 自然复位 |

## 4. 架构设计

### 4.1 数据流

```
任务A: printf("Hello %d", 42)
         ↓
     _write(1, buf, len) 被调用
         ↓
     检查 ISR 上下文（是 → 返回 0）
         ↓
     osMutexAcquire(tx_mutex, 100ms)
         ↓
     memcpy → TX ring buffer (1024 bytes)
         ↓
     osEventFlagsSet(TX_EVT_DATA_READY)
         ↓
     osMutexRelease(tx_mutex)
         
发送任务: (等待 TX_EVT_DATA_READY | TX_EVT_DMA_DONE)
         ↓
     唤醒 → 检查两个标志
         ↓
     检查 DMA 状态（忙 → 跳过）
         ↓
     从 ring buffer 取连续数据块（head→tail 或 head→末尾）
         ↓
     HAL_UART_Transmit_DMA(&huart1, data, len)
         ↓
     DMA2 Stream7 搬运数据到 USART1
         ↓
     USART1 TX 引脚输出
         ↓
     DMA 搬完 → TC 中断
         ↓
     osEventFlagsSet(TX_EVT_DMA_DONE)
         ↓
     发送任务再次唤醒
```

### 4.2 模块划分

| 模块 | 文件 | 职责 |
|------|------|------|
| 日志格式化 | `debug.c`（改） | `LOG_*` 宏 → `printf` → `_write()` |
| UART TX 传输 | `uart_tx.c`（新建） | TX ring buffer + 发送任务 + DMA 回调 |
| UART TX 接口 | `uart_tx.h`（新建） | API 定义 |
| 应用初始化 | `app_init.c`（改） | 调用 `UART_TX_Init()` |
| DMA 初始化 | `main.c`（CubeMX 生成） | USART1_TX DMA 配置 |

### 4.3 API 设计

```c
// uart_tx.h

#ifndef UART_TX_H
#define UART_TX_H

#include <stdint.h>

// 初始化 — 创建 mutex、event flags、启动发送任务
void UART_TX_Init(void);

// 供 _write() 调用 — 往 ring buffer 塞数据并唤醒发送任务
// 返回: 实际写入的字节数（buffer 满时返回 0）
int UART_TX_Enqueue(const uint8_t *data, uint16_t len);

// 供 HAL 回调调用 — DMA TC 和 Error
void UART_TX_DmaTxCpltCallback(void);
void UART_TX_DmaTxErrorCallback(void);

// 获取丢弃计数（供调试用）
uint32_t UART_TX_GetDropCount(void);

#endif // UART_TX_H
```

## 5. 详细设计

### 5.1 TX Ring Buffer

```c
// uart_tx.c 内部

#define TX_BUF_SIZE  1024

typedef struct {
    uint8_t  data[TX_BUF_SIZE];  // 数据区，4字节对齐
    volatile uint16_t head;       // 写入位置（_write() 修改）
    volatile uint16_t tail;       // 读取位置（发送任务修改）
    volatile uint16_t count;      // 当前数据量
} TX_RingBuffer_t;

__attribute__((aligned(4)))
static TX_RingBuffer_t tx_ring;
```

### 5.2 事件标志

```c
#define TX_EVT_DATA_READY   (1 << 0)  // ring buffer 有数据
#define TX_EVT_DMA_DONE     (1 << 1)  // DMA 传输完成
```

### 5.3 发送任务

```c
static void UART_TX_Task(void *arg) {
    uint32_t last_alive = 0;
    
    while (1) {
        // 心跳计数
        tx_task_alive_counter++;
        
        // 等待事件
        osEventFlagsWait(tx_flags, 
            TX_EVT_DATA_READY | TX_EVT_DMA_DONE,
            osFlagsWaitAny, osWaitForever);
        
        // 检查 DMA 是否忙
        if (HAL_DMA_GetState(&hdma_usart1_tx) != HAL_DMA_STATE_READY) {
            continue;  // 等下次唤醒
        }
        
        // 检查 ring buffer
        uint16_t count = tx_ring.count;
        if (count == 0) {
            osEventFlagsClear(tx_flags, TX_EVT_DATA_READY);
            continue;
        }
        
        // 取连续数据块
        uint16_t head = tx_ring.head;
        uint16_t tail = tx_ring.tail;
        uint16_t contiguous;
        if (head >= tail) {
            contiguous = TX_BUF_SIZE - head;
        } else {
            contiguous = tail - head;
        }
        uint16_t to_send = (count < contiguous) ? count : contiguous;
        
        // 启动 DMA
        HAL_UART_Transmit_DMA(&huart1, tx_ring.data + head, to_send);
        uint32_t start = osKernelGetTickCount();
        
        // 等待 DMA 完成或超时
        while (HAL_DMA_GetState(&hdma_usart1_tx) != HAL_DMA_STATE_READY) {
            if (osKernelGetTickCount() - start > 100) {
                HAL_UART_AbortTransmit(&huart1);
                tx_drop_count++;
                break;
            }
            osDelay(1);
        }
        
        // 更新 head
        tx_ring.head = (tx_ring.head + to_send) % TX_BUF_SIZE;
        
        // 更新 count（临界区）
        taskENTER_CRITICAL();
        tx_ring.count -= to_send;
        taskEXIT_CRITICAL();
    }
}
```

### 5.4 _write() 实现

```c
// debug.c 或 uart_tx.c

static volatile uint32_t tx_drop_count = 0;

int _write(int fd, const char *buf, int len) {
    // 1. ISR 检查
    if (xPortIsInsideInterrupt()) {
        return 0;
    }
    
    // 2. 只处理 stdout 和 stderr
    if (fd != 1 && fd != 2) {
        return -1;
    }
    
    // 3. 获取 mutex（100ms 超时）
    osStatus_t status = osMutexAcquire(tx_mutex, osMsToTicks(100));
    if (status != osOK) {
        tx_drop_count++;
        return 0;
    }
    
    // 4. 塞入 ring buffer
    uint16_t free_space = TX_BUF_SIZE - tx_ring.count;
    if (free_space < len) {
        tx_drop_count++;
        osMutexRelease(tx_mutex);
        return 0;
    }
    
    // 分段写入（处理回绕）
    uint16_t first = len;
    if (tx_ring.head + len > TX_BUF_SIZE) {
        first = TX_BUF_SIZE - tx_ring.head;
    }
    memcpy(tx_ring.data + tx_ring.head, buf, first);
    if (first < len) {
        memcpy(tx_ring.data, buf + first, len - first);
    }
    tx_ring.head = (tx_ring.head + len) % TX_BUF_SIZE;
    
    // 更新 count
    taskENTER_CRITICAL();
    tx_ring.count += len;
    taskEXIT_CRITICAL();
    
    // 5. 设置事件标志
    osEventFlagsSet(tx_flags, TX_EVT_DATA_READY);
    
    // 6. 释放 mutex
    osMutexRelease(tx_mutex);
    
    return len;
}
```

### 5.5 DMA 回调

```c
// uart_tx.c

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        UART_TX_DmaTxCpltCallback();
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        UART_TX_DmaTxErrorCallback();
    }
}

void UART_TX_DmaTxCpltCallback(void) {
    osEventFlagsSet(tx_flags, TX_EVT_DMA_DONE);
}

void UART_TX_DmaTxErrorCallback(void) {
    // 停止 DMA
    HAL_UART_DMAStop(&huart1);
    // 记录错误（用计数器，不用 RECORD_ERROR 防死锁）
    tx_error_count++;
    // 重置 DMA 状态
    HAL_DMA_Abort(&hdma_usart1_tx);
    // 通知发送任务
    osEventFlagsSet(tx_flags, TX_EVT_DMA_DONE);
}
```

## 6. 死锁/阻塞风险及对策

| # | 风险 | 对策 |
|---|------|------|
| 1 | `_write()` 里调 `RECORD_ERROR` → 递归死锁 | 用 `tx_drop_count` 计数器替代 |
| 2 | DMA 卡死不触发 TC 中断 | 发送任务 100ms 超时 + `HAL_UART_AbortTransmit` |
| 3 | 优先级反转 | `osMutexNew` 自动优先级继承 |
| 4 | `_write()` 被 ISR 调用 | `xPortIsInsideInterrupt()` 检查 |
| 5 | `printf` 内部状态不保护 | `printf` 用栈上 buffer，各任务独立 |
| 6 | USART TX 线卡住 | `HAL_UART_AbortTransmit` 释放总线 |
| 7 | Ring buffer 空满判断 | `count` 变量追踪实际数据量 |
| 8 | DMA 缓冲区对齐 | `__attribute__((aligned(4)))` |
| 9 | 任务删除时 mutex 未释放 | 发送任务永不退出 |
| 10 | 发送任务栈溢出 | 栈 1024 字节 + FreeRTOS 栈溢出检查 |

## 7. 看门狗保护

| # | 方案 | 实现 |
|---|------|------|
| 1 | 喂狗任务不调 `printf` | `StartDefaultTask` 只喂狗 |
| 2 | 发送任务心跳监控 | 喂狗前检查 `tx_task_alive_counter` |
| 3 | FreeRTOS 栈溢出检查 | `configCHECK_FOR_STACK_OVERFLOW = 2` |
| 4 | Hard Fault 不喂狗 | 让看门狗自然复位 |

## 8. 堆栈配置改动

| # | 改动 | 原值 | 新值 |
|---|------|------|------|
| 1 | `configENABLE_FPU` | 0 | **1** |
| 2 | `configTOTAL_HEAP_SIZE` | 15360 | **24576** |
| 3 | 发送任务栈 | - | **1024 字节** |

## 9. 文件改动清单

| # | 文件 | 改动 | 顺序 |
|---|------|------|------|
| 1 | `scope-siggen.ioc` | CubeMX 启用 USART1_TX DMA | 1 |
| 2 | 重新生成代码 | 更新 `main.c`、`stm32f4xx_hal_msp.c` | 2 |
| 3 | `FreeRTOSConfig.h` | 启用 FPU、扩大堆、启用栈溢出检查 | 3 |
| 4 | `Core/Inc/uart_tx.h` | **新建** — 接口定义 | 4 |
| 5 | `Core/Src/uart_tx.c` | **新建** — TX ring buffer + 发送任务 + DMA 回调 | 5 |
| 6 | `Core/Src/debug.c` | **改** — `LOG_*` 内部调 `printf` | 6 |
| 7 | `Core/Src/debug.c` | **改** — `_write()` 实现 | 7 |
| 8 | `Core/Src/app_init.c` | **改** — 调用 `UART_TX_Init()` | 8 |

## 10. 验证计划

| # | 验证项 | 方法 |
|---|--------|------|
| 1 | printf 输出到 UART | `printf("Hello\n")` → 串口助手显示 |
| 2 | LOG_* 宏正常工作 | `LOG_INFO("test")` → 串口助手显示 |
| 3 | 非阻塞验证 | 多任务同时 printf，观察不卡死 |
| 4 | DMA 传输验证 | 示波器观察 TX 引脚波形 |
| 5 | 看门狗不复位 | 运行 10 分钟，系统稳定 |
| 6 | 栈溢出检查 | 故意溢出栈，观察是否触发 hook |
| 7 | Buffer 满测试 | 高频 printf，观察 drop_count |

## 11. 待确认项

- [ ] oscilloscope/signalgen/uart_protocol/display/key_handler 各任务栈大小
- [ ] 各任务优先级
- [ ] upper_computer.c 是否后续迁移到 uart_tx 模块

---

**文档结束**
