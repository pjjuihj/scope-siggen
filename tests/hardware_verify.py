#!/usr/bin/env python3
"""
STM32 硬件验证测试
验证所有修复是否在硬件上正常工作
"""

import serial
import time
import sys

class HardwareVerifier:
    def __init__(self, port='COM3', baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        self.results = []

    def connect(self):
        """连接串口"""
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=2)
            print(f"[OK] 已连接到 {self.port}")
            return True
        except Exception as e:
            print(f"[FAIL] 连接失败: {e}")
            return False

    def disconnect(self):
        """断开串口"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("[OK] 串口已关闭")

    def reset_stm32(self):
        """复位 STM32 - 使用 reset 命令"""
        try:
            # 清空缓冲区
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()

            # 发送 reset 命令
            self.ser.write(b'reset\r\n')
            time.sleep(1)  # 等待复位完成

            # 清空缓冲区（清除 reset 命令的响应）
            self.ser.reset_input_buffer()

            return True
        except Exception as e:
            print(f"  [DEBUG] 复位异常: {e}")
            return False

    def send_command(self, cmd, wait_time=1):
        """发送命令并等待响应"""
        try:
            self.ser.reset_input_buffer()
            self.ser.write((cmd + '\r\n').encode())
            time.sleep(wait_time)

            if self.ser.in_waiting > 0:
                data = self.ser.read(self.ser.in_waiting)
                return data.decode('utf-8', errors='ignore')
            return ""
        except:
            return ""

    def capture_boot_log(self, duration=5):
        """捕获启动日志"""
        try:
            # 清空缓冲区
            self.ser.reset_input_buffer()

            # 发送 reset 命令
            self.ser.write(b'reset\r\n')

            # 等待启动日志
            start_time = time.time()
            log = ""

            while time.time() - start_time < duration:
                if self.ser.in_waiting > 0:
                    data = self.ser.read(self.ser.in_waiting)
                    log += data.decode('utf-8', errors='ignore')
                time.sleep(0.01)

            # 调试信息
            if not log:
                print("  [DEBUG] 未捕获到启动日志")
            else:
                print(f"  [DEBUG] 捕获到 {len(log)} 字节日志")

            return log
        except Exception as e:
            print(f"  [DEBUG] 捕获日志异常: {e}")
            return ""

    def verify_boot_log(self, log):
        """验证启动日志"""
        tests = []

        # 打印捕获到的日志（调试用）
        if log:
            print(f"  [DEBUG] 日志内容:")
            for line in log.split('\n')[:10]:  # 只打印前 10 行
                print(f"    {line}")

        # 测试 1: 配置模块初始化
        result = "Initializing config module" in log
        tests.append(("配置模块初始化", result))
        if not result:
            print(f"  [DEBUG] 未找到: 'Initializing config module'")

        # 测试 2: Flash 配置加载（首次启动可能失败，这是正常的）
        flash_load_ok = ("Config loaded from Flash" in log) or ("Failed to load config from flash" in log)
        tests.append(("Flash 配置加载", flash_load_ok))
        if not flash_load_ok:
            print(f"  [DEBUG] Flash 配置加载异常")

        # 测试 3: 配置从 Flash 加载
        result = "Config loaded from Flash" in log
        tests.append(("配置从 Flash 加载", result))
        if not result:
            print(f"  [DEBUG] 未找到: 'Config loaded from Flash'")

        # 测试 4: 自检通过
        result = "Self test passed" in log
        tests.append(("自检通过", result))
        if not result:
            print(f"  [DEBUG] 未找到: 'Self test passed'")

        # 测试 5: 所有模块启动
        result = "All modules started" in log
        tests.append(("所有模块启动", result))
        if not result:
            print(f"  [DEBUG] 未找到: 'All modules started'")

        # 测试 6: 应用就绪
        result = "Application Ready" in log
        tests.append(("应用就绪", result))
        if not result:
            print(f"  [DEBUG] 未找到: 'Application Ready'")

        # 测试 7: 按键处理只初始化一次
        key_init_count = log.count("Key handler module initialized")
        result = key_init_count == 1
        tests.append(("按键处理只初始化一次", result))
        if not result:
            print(f"  [DEBUG] 按键处理初始化次数: {key_init_count}")

        return tests

    def verify_uart_commands(self):
        """验证 UART 命令"""
        tests = []

        # 测试 1: help 命令
        response = self.send_command("help")
        tests.append(("help 命令", "=== Commands ===" in response))

        # 测试 2: status 命令
        response = self.send_command("status")
        tests.append(("status 命令", "=== Status ===" in response))

        # 测试 3: version 命令
        response = self.send_command("version")
        tests.append(("version 命令", "=== Version ===" in response))

        # 测试 4: tasks 命令
        response = self.send_command("tasks")
        tests.append(("tasks 命令", "=== Task Status ===" in response))

        # 测试 5: memory 命令
        response = self.send_command("memory")
        tests.append(("memory 命令", "=== Memory Status ===" in response))

        # 测试 6: 未知命令
        response = self.send_command("xyz")
        tests.append(("未知命令处理", "Unknown command" in response))

        return tests

    def verify_uart_tx_mark_ready(self):
        """验证 UART_TX_MarkReady 修复"""
        tests = []

        # 发送 help 命令，验证 _write 正常工作
        response = self.send_command("help")
        tests.append(("_write 函数正常", "=== Commands ===" in response))

        # 发送多个命令，验证无数据丢失
        for i in range(5):
            response = self.send_command("version")
            if "=== Version ===" not in response:
                tests.append(("数据传输无丢失", False))
                return tests

        tests.append(("数据传输无丢失", True))

        return tests

    def run_all_tests(self):
        """运行所有测试"""
        print("=" * 60)
        print("STM32 硬件验证测试")
        print("=" * 60)

        all_tests = []

        # 连接串口
        if not self.connect():
            return False

        try:
            # 复位 STM32
            print("\n[1] 复位 STM32...")
            self.reset_stm32()

            # 捕获启动日志
            print("[2] 捕获启动日志...")
            log = self.capture_boot_log(3)

            # 验证启动日志
            print("[3] 验证启动日志...")
            boot_tests = self.verify_boot_log(log)
            all_tests.extend(boot_tests)

            # 验证 UART 命令
            print("[4] 验证 UART 命令...")
            cmd_tests = self.verify_uart_commands()
            all_tests.extend(cmd_tests)

            # 验证 UART_TX_MarkReady 修复
            print("[5] 验证 UART_TX_MarkReady 修复...")
            uart_tests = self.verify_uart_tx_mark_ready()
            all_tests.extend(uart_tests)

        finally:
            self.disconnect()

        # 打印测试结果
        print("\n" + "=" * 60)
        print("测试结果")
        print("=" * 60)

        passed = 0
        failed = 0

        for name, result in all_tests:
            status = "[PASS]" if result else "[FAIL]"
            print(f"{status} {name}")
            if result:
                passed += 1
            else:
                failed += 1

        print("-" * 60)
        print(f"总计: {len(all_tests)} 项测试")
        print(f"通过: {passed} 项")
        print(f"失败: {failed} 项")
        print("=" * 60)

        return failed == 0

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "COM3"
    baudrate = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    verifier = HardwareVerifier(port, baudrate)
    success = verifier.run_all_tests()

    if success:
        print("\n[SUCCESS] 所有硬件验证测试通过！")
        return 0
    else:
        print("\n[ERROR] 部分测试失败！")
        return 1

if __name__ == "__main__":
    sys.exit(main())
