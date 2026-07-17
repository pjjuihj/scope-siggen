#!/usr/bin/env python3
"""
简单的串口测试脚本
"""

import serial
import time

print("串口测试")
print("=" * 40)

try:
    # 打开串口
    ser = serial.Serial('COM3', 115200, timeout=2)
    print("[OK] 已连接到 COM3")

    # 清空缓冲区
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    # 等待 STM32 启动
    print("等待 STM32 启动 (3秒)...")
    time.sleep(3)

    # 检查是否有数据
    print("\n检查接收数据:")
    if ser.in_waiting > 0:
        data = ser.read(ser.in_waiting)
        text = data.decode('utf-8', errors='ignore')
        print(f"收到 {len(data)} 字节:")
        print(text)
    else:
        print("[FAIL] 没有收到数据")

    # 尝试发送数据
    print("\n发送测试数据:")
    test_msg = "TEST\r\n"
    bytes_written = ser.write(test_msg.encode())
    print(f"发送了 {bytes_written} 字节: {test_msg.strip()}")

    # 等待响应
    print("\n等待响应 (2秒)...")
    time.sleep(2)

    if ser.in_waiting > 0:
        data = ser.read(ser.in_waiting)
        text = data.decode('utf-8', errors='ignore')
        print(f"收到响应 {len(data)} 字节:")
        print(text)
    else:
        print("[FAIL] 没有收到响应")

    ser.close()
    print("\n[OK] 串口已关闭")

except serial.SerialException as e:
    print(f"[ERROR] 串口错误: {e}")
except Exception as e:
    print(f"[ERROR] 错误: {e}")
