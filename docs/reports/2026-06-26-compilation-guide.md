# 编译验证指南

**项目名称**: SCOPE-SIGGEN  
**日期**: 2026-06-26  
**状态**: 待验证

---

## 1. 编译前检查清单

### 1.1 文件完整性检查

请确认以下文件已创建：

| 文件 | 状态 | 说明 |
|------|------|------|
| Core/Inc/version.h | ⬜ | 版本管理模块头文件 |
| Core/Src/version.c | ⬜ | 版本管理模块实现 |
| Core/Inc/error_tracker.h | ⬜ | 错误追踪模块头文件 |
| Core/Src/error_tracker.c | ⬜ | 错误追踪模块实现 |
| Core/Inc/ring_buffer.h | ⬜ | 环形缓冲模块头文件 |
| Core/Src/ring_buffer.c | ⬜ | 环形缓冲模块实现 |
| Core/Inc/debug.h | ⬜ | 调试工具模块头文件 |
| Core/Src/debug.c | ⬜ | 调试工具模块实现 |

### 1.2 main.c修改检查

请确认main.c已修改：

| 修改项 | 状态 | 位置 |
|--------|------|------|
| 添加头文件 | ⬜ | USER CODE BEGIN Includes |
| 初始化调试模块 | ⬜ | USER CODE BEGIN 2 |
| 初始化错误追踪模块 | ⬜ | USER CODE BEGIN 2 |
| 打印版本信息 | ⬜ | USER CODE BEGIN 2 |

---

## 2. Keil编译步骤

### 2.1 打开工程

1. 打开Keil MDK-ARM
2. 打开工程文件：`MDK-ARM/scope-siggen.uvprojx`

### 2.2 添加新文件到工程

**添加头文件路径**：
1. Project → Options → C/C++ → Include Paths
2. 添加路径：`../Core/Inc`

**添加源文件**：
1. 在Project面板中，右键点击"Source Group 1"
2. 选择"Add Files to Group..."
3. 添加以下文件：
   - `Core/Src/version.c`
   - `Core/Src/error_tracker.c`
   - `Core/Src/ring_buffer.c`
   - `Core/Src/debug.c`

### 2.3 编译项目

1. 点击"Build"按钮（F7）
2. 或者使用菜单：Project → Build Target

---

## 3. 预期编译结果

### 3.1 成功标志

编译成功后，应该看到：
```
Build target 'scope-siggen'
compiling main.c...
compiling version.c...
compiling error_tracker.c...
compiling ring_buffer.c...
compiling debug.c...
linking...
Program Size: Code=XXxx RO-data=XXxx RW-data=XXxx ZI-data=XXxx
".\scope-siggen\scope-siggen.axf" - 0 Error(s), 0 Warning(s).
Build Time Elapsed: 00:00:XX
```

### 3.2 可能的编译错误

#### 错误1：头文件找不到
```
error: #5: cannot open source input file "version.h"
```
**解决方案**：检查Include Paths配置

#### 错误2：未定义的函数
```
error: #20: identifier "Debug_Init" is undefined
```
**解决方案**：确保debug.c已添加到工程

#### 错误3：类型未定义
```
error: #20: identifier "ErrorCode_t" is undefined
```
**解决方案**：检查error_tracker.h是否正确包含

#### 错误4：函数重复定义
```
error: L6200E: Symbol xxx multiply defined
```
**解决方案**：检查是否有重复的函数定义

---

## 4. 编译后验证

### 4.1 生成文件检查

编译成功后，应该生成以下文件：
- `MDK-ARM/scope-siggen/scope-siggen.axf` - 可执行文件
- `MDK-ARM/scope-siggen/scope-siggen.hex` - HEX文件
- `MDK-ARM/scope-siggen/scope-siggen.map` - 映射文件

### 4.2 内存使用检查

查看编译输出的内存信息：
```
Program Size: Code=XXxx RO-data=XXxx RW-data=XXxx ZI-data=XXxx
```

**检查项**：
- Code：代码大小（应在Flash范围内）
- RO-data：只读数据
- RW-data：已初始化数据
- ZI-data：未初始化数据（应在RAM范围内）

### 4.3 警告检查

检查是否有警告：
- 未使用的变量
- 类型转换警告
- 函数声明不匹配

---

## 5. 手动编译命令

如果使用命令行编译，可以使用以下命令：

```bash
# 使用Keil UV4.exe编译
UV4.exe -b MDK-ARM/scope-siggen.uvprojx -t scope-siggen -o build.log -j0

# 使用工作流脚本编译
python workflow.py --auto . --steps compile
```

---

## 6. 编译验证报告模板

编译完成后，请填写以下报告：

```
编译验证报告
============

编译时间：YYYY-MM-DD HH:MM:SS
编译环境：Keil MDK-ARM V5.XX

编译结果：
- 锂数：X
- 警告数：X
- 编译状态：成功/失败

内存使用：
- Code：XXXX bytes
- RO-data：XXXX bytes
- RW-data：XXXX bytes
- ZI-data：XXXX bytes

生成文件：
- scope-siggen.axf：是/否
- scope-siggen.hex：是/否
- scope-siggen.map：是/否

备注：
（记录任何问题或注意事项）
```

---

## 7. 常见问题解决

### 问题1：Keil版本不兼容

**症状**：工程文件无法打开

**解决方案**：
1. 确保Keil版本 ≥ 5.32
2. 安装STM32F4xx DFP包

### 问题2：缺少头文件

**症状**：编译错误：找不到头文件

**解决方案**：
1. 检查Include Paths配置
2. 确保所有头文件在正确位置

### 问题3：链接错误

**症状**：链接阶段报错

**解决方案**：
1. 检查所有源文件是否添加到工程
2. 检查是否有重复定义

### 问题4：内存溢出

**症状**：Flash或RAM超出范围

**解决方案**：
1. 优化代码大小
2. 检查缓冲区配置

---

## 8. 下一步

编译验证成功后：

1. **记录编译结果** - 填写编译验证报告
2. **开始阶段2** - 实现服务层模块
3. **烧录测试** - 将固件烧录到硬件测试

---

**注意**：由于当前环境没有Keil UV4.exe，无法自动编译。请在本地Keil环境中手动执行编译验证。

---

**文档创建时间**: 2026-06-26
