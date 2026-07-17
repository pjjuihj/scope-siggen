#!/usr/bin/env python3
"""
上位机通信测试脚本
用于测试STM32与上位机之间的通信
"""

import serial
import time
import sys
import json

def test_upper_computer(port, baudrate=115200):
    """测试上位机通信"""

    print(f"=== 上位机通信测试 ===")
    print(f"串口: {port}")
    print(f"波特率: {baudrate}")
    print()

    try:
        # 打开串口
        ser = serial.Serial(port, baudrate, timeout=1)
        print("[OK] 串口已打开")
        print()

        # 测试命令列表
        test_commands = [
            ("help", "显示帮助"),
            ("uc status", "查看上位机状态"),
            ("uc test", "测试通信"),
            ("uc send", "发送波形数据"),
            ("status", "系统状态"),
            ("version", "版本信息"),
        ]

        print("=== 测试命令 ===")
        for cmd, desc in test_commands:
            print(f"\n>>> 发送命令: {cmd} ({desc})")
            ser.write((cmd + "\r\n").encode())
            time.sleep(0.5)

            # 读取响应
            response = ""
            while ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    response += line + "\n"
                    print(f"    {line}")

            if not response:
                print("    [无响应]")

        print("\n=== 测试流式传输 ===")
        print("启用流式传输...")
        ser.write(b"uc stream on\r\n")
        time.sleep(1)

        print("等待数据...")
        data_count = 0
        start_time = time.time()

        while time.time() - start_time < 5:  # 测试5秒
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    if line.startswith("WAVE:"):
                        data_count += 1
                        print(f"    [波形数据 #{data_count}] 长度: {len(line)-5} 字符")
                    elif line.startswith("FREQ:"):
                        print(f"    [频率] {line}")
                    elif line.startswith("VOLT:"):
                        print(f"    [电压] {line}")
                    elif line.startswith("OK:"):
                        print(f"    [OK] {line}")
                    elif line.startswith("ERROR:"):
                        print(f"    [错误] {line}")
                    elif line.startswith("INFO:"):
                        print(f"    [信息] {line}")
                    else:
                        print(f"    {line}")

        print("\n禁用流式传输...")
        ser.write(b"uc stream off\r\n")
        time.sleep(0.5)

        print("\n=== 测试结果 ===")
        print(f"接收到 {data_count} 个波形数据包")

        if data_count > 0:
            print("[PASS] 上位机通信正常")
        else:
            print("[FAIL] 未接收到波形数据")

        # 关闭串口
        ser.close()
        print("\n串口已关闭")

        return data_count > 0

    except serial.SerialException as e:
        print(f"[ERROR] 串口错误: {e}")
        return False
    except Exception as e:
        print(f"[ERROR] 错误: {e}")
        return False

def main():
    """主函数"""

    if len(sys.argv) < 2:
        print("用法: python test_upper_computer.py <串口> [波特率]")
        print("示例: python test_upper_computer.py COM3 115200")
        sys.exit(1)

    port = sys.argv[1]
    baudrate = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    success = test_upper_computer(port, baudrate)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
