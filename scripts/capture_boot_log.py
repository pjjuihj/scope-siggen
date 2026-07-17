#!/usr/bin/env python3
"""
捕获 STM32 启动日志 - 复位后立即监视
"""

import serial
import time
import sys

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "COM3"
    baudrate = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    print("=" * 50)
    print("STM32 启动日志捕获")
    print("=" * 50)
    print(f"端口: {port}")
    print(f"波特率: {baudrate}")
    print("-" * 50)

    try:
        # 打开串口
        ser = serial.Serial(port, baudrate, timeout=1)
        print("[OK] 串口已打开")

        # 清空缓冲区
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        # 复位 STM32
        print("\n[1] 复位 STM32...")
        ser.dtr = False
        ser.rts = True
        time.sleep(0.05)
        ser.rts = False
        time.sleep(0.05)
        ser.dtr = True
        time.sleep(0.05)
        ser.dtr = False
        print("[OK] 复位完成")

        # 立即开始监视
        print("\n[2] 捕获启动日志...")
        print("-" * 50)

        start_time = time.time()
        buffer = ""
        line_count = 0

        # 监视 5 秒，捕获所有启动日志
        while time.time() - start_time < 5:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                try:
                    text = data.decode('utf-8', errors='ignore')
                    for char in text:
                        if char == '\n':
                            elapsed = time.time() - start_time
                            print(f"[{elapsed:7.3f}s] {buffer}")
                            buffer = ""
                            line_count += 1
                        elif char != '\r':
                            buffer += char
                except:
                    pass
            time.sleep(0.01)

        # 输出剩余的缓冲区
        if buffer:
            elapsed = time.time() - start_time
            print(f"[{elapsed:7.3f}s] {buffer}")
            line_count += 1

        print("-" * 50)
        print(f"\n[完成] 捕获了 {line_count} 行日志")
        print(f"总耗时: {time.time() - start_time:.3f} 秒")

        ser.close()
        print("[OK] 串口已关闭")

    except serial.SerialException as e:
        print(f"[ERROR] 串口错误: {e}")
    except Exception as e:
        print(f"[ERROR] 错误: {e}")

if __name__ == "__main__":
    main()
