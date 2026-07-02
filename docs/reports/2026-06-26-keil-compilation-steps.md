# Keil MDK-ARM 编译验证步骤

**项目**: SCOPE-SIGGEN  
**日期**: 2026-06-26  
**工具**: Keil MDK-ARM V5.32+

---

## 步骤1：打开Keil工程

1. 启动Keil MDK-ARM
2. 打开工程文件：
   ```
   文件 → 打开工程 → 选择 MDK-ARM/scope-siggen.uvprojx
   ```

---

## 步骤2：添加新源文件到工程

### 2.1 添加源文件

1. 在左侧"Project"面板中，展开"Target scope-siggen"
2. 右键点击"Source Group 1"
3. 选择"Add Files to Group..."
4. 浏览到 `Core/Src` 目录
5. 添加以下文件：
   - `version.c`
   - `error_tracker.c`
   - `ring_buffer.c`
   - `debug.c`
6. 点击"Add"，然后"Close"

### 2.2 验证文件已添加

在"Source Group 1"中应该看到：
```
Source Group 1
├── main.c
├── freertos.c
├── stm32f4xx_it.c
├── stm32f4xx_hal_msp.c
├── system_stm32f4xx.c
├── version.c          ← 新增
├── error_tracker.c    ← 新增
├── ring_buffer.c      ← 新增
└── debug.c            ← 新增
```

---

## 步骤3：配置头文件路径

### 3.1 打开项目选项

1. 菜单：Project → Options for Target 'scope-siggen'
2. 或者快捷键：Alt+F7

### 3.2 配置Include Paths

1. 选择"C/C++"选项卡
2. 在"Include Paths"字段中，添加以下路径：
   ```
   ../Core/Inc
   ../Drivers/STM32F4xx_HAL_Driver/Inc
   ../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
   ../Middlewares/Third_Party/FreeRTOS/Source/include
   ../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
   ../Drivers/CMSIS/Device/ST/STM32F4xx/Include
   ../Drivers/CMSIS/Include
   ```
3. 点击"OK"

### 3.3 验证配置

确保"Include Paths"中包含 `../Core/Inc`

---

## 步骤4：编译项目

### 4.1 清理旧的编译产物（可选）

1. 菜单：Project → Clean Targets
2. 或者快捷键：无

### 4.2 编译项目

1. 菜单：Project → Build Target
2. 或者快捷键：F7
3. 或者点击工具栏上的"Build"按钮

### 4.3 观察编译输出

在"Build Output"窗口中观察编译过程：

```
Build target 'scope-siggen'
compiling main.c...
compiling version.c...
compiling error_tracker.c...
compiling ring_buffer.c...
compiling debug.c...
compiling freertos.c...
compiling stm32f4xx_it.c...
compiling stm32f4xx_hal_msp.c...
compiling system_stm32f4xx.c...
linking...
Program Size: Code=XXXXX  RO-data=XXXX  RW-data=XXX  ZI-data=XXXX
".\scope-siggen\scope-siggen.axf" - 0 Error(s), 0 Warning(s).
Build Time Elapsed: 00:00:XX
```

---

## 步骤5：验证编译结果

### 5.1 检查编译状态

**成功标志**：
```
0 Error(s), 0 Warning(s)
```

**失败标志**：
```
X Error(s), X Warning(s)
```

### 5.2 检查生成文件

编译成功后，应该生成以下文件：

```
MDK-ARM/scope-siggen/
├── scope-siggen.axf    ← 可执行文件（ELF格式）
├── scope-siggen.hex    ← HEX文件（用于烧录）
├── scope-siggen.map    ← 映射文件（内存分配）
├── scope-siggen.htm    ← 链接报告
├── scope-siggen.lnp    ← 链接器输入文件
├── scope-siggen.sct    ← 分散加载文件
└── Objects/            ← 目标文件目录
    ├── main.o
    ├── version.o
    ├── error_tracker.o
    ├── ring_buffer.o
    ├── debug.o
    └── ...
```

### 5.3 检查内存使用

查看编译输出的内存信息：

```
Program Size: Code=XXXXX  RO-data=XXXX  RW-data=XXX  ZI-data=XXXX
```

**解释**：
- **Code**：代码大小（存储在Flash中）
- **RO-data**：只读数据（存储在Flash中）
- **RW-data**：已初始化数据（存储在Flash中，运行时复制到RAM）
- **ZI-data**：未初始化数据（运行时在RAM中分配）

**STM32F407VETx资源**：
- Flash：512KB
- RAM：192KB（128KB + 64KB CCM）

**检查**：
- Code + RO-data + RW-data < 512KB（Flash）
- RW-data + ZI-data < 192KB（RAM）

---

## 步骤6：常见编译错误及解决方案

### 错误1：头文件找不到

**错误信息**：
```
error: #5: cannot open source input file "version.h"
```

**解决方案**：
1. 检查Include Paths配置
2. 确保 `../Core/Inc` 在Include Paths中
3. 确认 `version.h` 文件存在于 `Core/Inc` 目录

### 错误2：未定义的函数

**错误信息**：
```
error: #20: identifier "Debug_Init" is undefined
```

**解决方案**：
1. 确保 `debug.c` 已添加到工程
2. 确保 `debug.h` 中声明了该函数
3. 检查函数名拼写

### 错误3：类型未定义

**错误信息**：
```
error: #20: identifier "ErrorCode_t" is undefined
```

**解决方案**：
1. 确保 `error_tracker.h` 已包含
2. 检查头文件包含顺序
3. 确认类型定义在正确的头文件中

### 错误4：函数重复定义

**错误信息**：
```
error: L6200E: Symbol xxx multiply defined
```

**解决方案**：
1. 检查是否有重复的函数定义
2. 检查是否有重复的源文件添加
3. 检查头文件中是否有函数定义（应该只有声明）

### 错误5：未使用的变量

**警告信息**：
```
warning: #177-D: variable "xxx" was declared but never referenced
```

**解决方案**：
1. 删除未使用的变量
2. 或者添加 `(void)xxx;` 来消除警告
3. 或者使用 `__attribute__((unused))` 标记

---

## 步骤7：编译验证报告

编译完成后，请填写以下报告：

```
编译验证报告
============

编译时间：YYYY-MM-DD HH:MM:SS
编译环境：Keil MDK-ARM V5.XX
编译目标：scope-siggen

编译结果：
- 错误数：X
- 警告数：X
- 编译状态：成功/失败

内存使用：
- Code：XXXXX bytes
- RO-data：XXXX bytes
- RW-data：XXX bytes
- ZI-data：XXXX bytes
- 总Flash使用：XXXXX bytes / 524288 bytes (XX%)
- 总RAM使用：XXXX bytes / 196608 bytes (XX%)

生成文件：
- scope-siggen.axf：是/否
- scope-siggen.hex：是/否
- scope-siggen.map：是/否

警告信息：
（如果有警告，列出警告信息）

错误信息：
（如果有错误，列出错误信息）

备注：
（记录任何问题或注意事项）
```

---

## 步骤8：下一步

### 如果编译成功

1. **记录编译结果** - 填写编译验证报告
2. **继续开发** - 开始阶段2：服务层实现
3. **烧录测试** - 将固件烧录到硬件测试（可选）

### 如果编译失败

1. **查看错误信息** - 仔细阅读编译输出的错误信息
2. **参考解决方案** - 查看上面的常见错误及解决方案
3. **修复问题** - 根据错误信息修复代码
4. **重新编译** - 修复后重新编译

---

## 附录：快速参考

### 快捷键

| 功能 | 快捷键 |
|------|--------|
| 编译项目 | F7 |
| 下载到芯片 | F8 |
| 调试 | Ctrl+F5 |
| 项目选项 | Alt+F7 |

### 菜单位置

| 功能 | 菜单位置 |
|------|---------|
| 编译项目 | Project → Build Target |
| 清理项目 | Project → Clean Targets |
| 项目选项 | Project → Options for Target |
| 添加文件 | 右键点击Source Group → Add Files |
| Include Paths | Project → Options → C/C++ → Include Paths |

---

**文档创建时间**: 2026-06-26
