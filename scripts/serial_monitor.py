#!/usr/bin/env python3
"""
串口监视器 - 捕获 STM32 输出
使用方法: python serial_monitor.py [COM端口] [波特率]
"""

import serial
import sys
import time
from datetime import datetime

def main():
    # 默认参数
    port = sys.argv[1] if len(sys.argv) > 1 else "COM3"
    baudrate = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    print(f"串口监视器启动")
    print(f"端口: {port}")
    print(f"波特率: {baudrate}")
    print("-" * 50)

    try:
        # 打开串口
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"已连接到 {port}")
        print("等待数据...\n")

        # 记录开始时间
        start_time = time.time()

        while True:
            # 读取数据
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                try:
                    # 尝试解码为 UTF-8
                    text = data.decode('utf-8', errors='ignore')
                    # 添加时间戳
                    elapsed = time.time() - start_time
                    timestamp = f"[{elapsed:.3f}s]"
                    print(f"{timestamp} {text}", end='')
                except:
                    # 如果解码失败，显示十六进制
                    print(f"HEX: {data.hex()}")

            time.sleep(0.01)

    except serial.SerialException as e:
        print(f"串口错误: {e}")
        print("请检查:")
        print("1. 串口是否被占用")
        print("2. COM 端口号是否正确")
        print("3. 串口驱动是否安装")
    except KeyboardInterrupt:
        print("\n\n监视已停止")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("串口已关闭")

if __name__ == "__main__":
    main()
