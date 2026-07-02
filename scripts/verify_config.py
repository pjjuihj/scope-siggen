#!/usr/bin/env python3
"""STM32 配置验证脚本 - 烧录后自动检查关键寄存器"""

import serial
import time
import sys

def check_serial(port='COM3', baud=115200):
    """检查串口通信"""
    try:
        ser = serial.Serial(port, baud, timeout=2)
        time.sleep(0.5)
        ser.read(ser.in_waiting or 1000)
        ser.write(b'version\n')
        time.sleep(1)
        out = ser.read(ser.in_waiting or 500).decode('utf-8', errors='replace')
        ser.close()
        return 'Version' in out
    except:
        return False

def check_adc_range(port='COM3', baud=115200, samples=5):
    """检查 ADC 缓冲区范围（验证 DAC 频率）"""
    try:
        ser = serial.Serial(port, baud, timeout=2)
        time.sleep(2)
        ser.read(ser.in_waiting or 5000)

        ranges = []
        for _ in range(samples):
            ser.write(b'status\n')
            time.sleep(1)
            out = ser.read(ser.in_waiting or 1000).decode('utf-8', errors='replace')
            # 解析 min/max
            for line in out.split('\n'):
                if 'min=' in line and 'max=' in line:
                    try:
                        parts = line.split('min=')[1].split('max=')
                        mn = int(parts[0].strip())
                        mx = int(parts[1].split('v=')[0].strip())
                        ranges.append(mx - mn)
                    except:
                        pass

        ser.close()

        if not ranges:
            return None, "No data"

        avg_range = sum(ranges) / len(ranges)
        return avg_range, None
    except Exception as e:
        return None, str(e)

def main():
    print("=== STM32 配置验证 ===\n")

    # 1. 串口通信
    print("[1] 串口通信...", end=' ')
    if check_serial():
        print("✅")
    else:
        print("❌ 串口不通")
        return 1

    # 2. ADC 范围
    print("[2] ADC 缓冲区范围...", end=' ')
    avg_range, err = check_adc_range()
    if err:
        print(f"❌ {err}")
    elif avg_range > 1000:
        print(f"✅ 范围={avg_range:.0f}")
    else:
        print(f"❌ 范围={avg_range:.0f} (期望>1000)")
        print("   可能原因: TIM 预分频公式错误，DAC 频率太低")
        return 1

    print("\n=== 验证通过 ===")
    return 0

if __name__ == '__main__':
    sys.exit(main())
