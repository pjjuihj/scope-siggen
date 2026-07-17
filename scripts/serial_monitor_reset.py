#!/usr/bin/env python3
"""
串口监视器 - 自动复位 STM32 并监视输出
"""

import serial
import time
import sys

def reset_stm32(ser):
    """通过 DTR/RTS 信号复位 STM32"""
    try:
        # CH340 复位序列
        ser.dtr = False
        ser.rts = True
        time.sleep(0.1)
        ser.rts = False
        time.sleep(0.1)
        ser.dtr = True
        time.sleep(0.1)
        ser.dtr = False
        print("[OK] STM32 已复位")
        return True
    except Exception as e:
        print(f"[WARN] 复位失败: {e}")
        return False

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "COM3"
    baudrate = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    print("=" * 50)
    print("STM32 串口监视器 (自动复位)")
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
        print("\n正在复位 STM32...")
        reset_stm32(ser)

        # 等待启动
        print("等待 STM32 启动...")
        print("-" * 50)

        # 监视串口
        start_time = time.time()
        last_data_time = time.time()
        buffer = ""

        while True:
            # 读取数据
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                try:
                    text = data.decode('utf-8', errors='ignore')
                    # 添加时间戳
                    elapsed = time.time() - start_time
                    timestamp = f"[{elapsed:7.3f}s]"

                    # 逐行输出
                    for char in text:
                        if char == '\n':
                            print(f"{timestamp} {buffer}")
                            buffer = ""
                        elif char != '\r':
                            buffer += char

                    last_data_time = time.time()
                except:
                    print(f"HEX: {data.hex()}")
            else:
                # 如果超过 5 秒没有数据，显示提示
                if time.time() - last_data_time > 5:
                    print(f"\n[INFO] 等待数据中... (已等待 {time.time() - last_data_time:.1f} 秒)")
                    last_data_time = time.time() + 4  # 5秒后再提示

            time.sleep(0.01)

    except serial.SerialException as e:
        print(f"[ERROR] 串口错误: {e}")
        print("\n请检查:")
        print("1. 串口是否被占用")
        print("2. COM 端口号是否正确")
        print("3. 串口驱动是否安装")
    except KeyboardInterrupt:
        print("\n\n" + "=" * 50)
        print("监视已停止")
        print("=" * 50)
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("[OK] 串口已关闭")

if __name__ == "__main__":
    main()
