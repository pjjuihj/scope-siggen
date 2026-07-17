#!/usr/bin/env python3
"""
TDD 自动化闭环：编译 -> 烧录 -> 串口抓取 -> 分析结果
"""

import subprocess
import serial
import time
import sys
import os

# 配置
KEIL_PATH = r"D:\k5\UV4\UV4.exe"
PROJECT_PATH = r"d:\stm32 _project_hal\scope-siggen\MDK-ARM\scope-siggen.uvprojx"
COM_PORT = "COM3"
BAUD_RATE = 115200
TIMEOUT_SEC = 8

def build():
    """Keil 编译"""
    print("[1/4] Build...")
    result = subprocess.run(
        [KEIL_PATH, "-b", PROJECT_PATH, "-o", "-", "-j0"],
        capture_output=True, text=True, timeout=60
    )
    output = result.stdout + result.stderr
    if "0 Error(s)" in output:
        print("  OK: Build succeeded")
        return True
    else:
        print(f"  FAIL: Build failed:\n{output}")
        return False

def flash():
    """Keil 烧录"""
    print("[2/4] Flash...")
    result = subprocess.run(
        [KEIL_PATH, "-f", PROJECT_PATH, "-o", "-", "-j0"],
        capture_output=True, text=True, timeout=60
    )
    output = result.stdout + result.stderr
    if "Verify OK" in output or "Application running" in output:
        print("  OK: Flash succeeded")
        return True
    else:
        print(f"  FAIL: Flash failed:\n{output}")
        return False

def reset_board():
    """RTS 信号复位板子"""
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=0.1)
    ser.rts = False; time.sleep(0.05)
    ser.rts = True; time.sleep(0.05)
    ser.rts = False
    ser.close()
    time.sleep(3)

def capture_serial():
    """串口抓取测试输出"""
    print(f"[3/4] Serial capture ({COM_PORT}, {BAUD_RATE}baud, {TIMEOUT_SEC}s)...")
    try:
        reset_board()
        ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)

        lines = []
        start = time.time()
        while time.time() - start < TIMEOUT_SEC:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    lines.append(line)
                    print(f"    {line}")
            else:
                time.sleep(0.05)
        ser.close()

        if not lines:
            print("  WARN: No data received")
            return None
        return lines
    except serial.SerialException as e:
        print(f"  FAIL: Serial error: {e}")
        return None

def analyze(lines):
    """分析测试结果"""
    print("[4/4] Analyze...")
    if not lines:
        print("  FAIL: No data to analyze")
        return False

    for line in lines:
        # 解析结构化结果: RESULT:total,passed,failed
        if line.startswith("RESULT:"):
            parts = line.split(":")[1].split(",")
            if len(parts) == 3:
                total, passed, failed = int(parts[0]), int(parts[1]), int(parts[2])
                print(f"  Total: {total}, Passed: {passed}, Failed: {failed}")
                if failed == 0:
                    print("  ALL PASSED!")
                    return True
                else:
                    print(f"  {failed} FAILED")
                    return False

    print("  WARN: No RESULT line found")
    return False

def main():
    print("=" * 50)
    print("TDD Automation Loop")
    print("=" * 50)

    if not build():
        sys.exit(1)
    if not flash():
        sys.exit(1)

    lines = capture_serial()
    success = analyze(lines)

    print("=" * 50)
    if success:
        print("TDD loop PASSED")
    else:
        print("TDD loop FAILED - needs fix")
    print("=" * 50)

    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
