#!/usr/bin/env python3
"""波形数据分析模板 - 从串口输出分析 ADC/DAC 数据"""

import serial
import time
import sys

def collect_data(port='COM3', baud=115200, duration=5):
    """收集串口数据"""
    ser = serial.Serial(port, baud, timeout=2)
    time.sleep(2)
    ser.read(ser.in_waiting or 10000)

    # 发送 status 命令触发数据输出
    ser.write(b'status\n')
    time.sleep(duration)

    out = ser.read(ser.in_waiting or 50000).decode('utf-8', errors='replace')
    ser.close()
    return out

def parse_adc_data(raw):
    """解析 ADC 数据"""
    mins, maxs = [], []
    for line in raw.split('\n'):
        if 'W:' in line and 'min=' in line:
            try:
                parts = line.split('min=')[1].split('max=')
                mn = int(parts[0].strip())
                mx = int(parts[1].split('v=')[0].strip())
                mins.append(mn)
                maxs.append(mx)
            except:
                pass
    return mins, maxs

def analyze(mins, maxs):
    """分析波形数据"""
    if not mins:
        print("无数据")
        return

    ranges = [mx - mn for mn, mx in zip(mins, maxs)]
    avg_range = sum(ranges) / len(ranges)

    print(f"=== 波形分析 ===")
    print(f"样本数: {len(mins)}")
    print(f"Min 范围: {min(mins)} - {max(mins)}")
    print(f"Max 范围: {min(maxs)} - {max(maxs)}")
    print(f"平均范围: {avg_range:.0f}")

    # 频率诊断
    print(f"\n=== 频率诊断 ===")
    if avg_range > 1000:
        print(f"✅ 范围正常 (>1000)")
        print(f"   DAC 频率正确，每个缓冲区包含完整波形周期")
    elif avg_range > 100:
        print(f"⚠️ 范围偏小 (100-1000)")
        print(f"   DAC 频率偏低，每个缓冲区只包含部分周期")
        print(f"   检查 TIM 预分频公式是否考虑了 ARR")
    else:
        print(f"❌ 范围太小 (<100)")
        print(f"   DAC 频率严重偏低或未输出")
        print(f"   检查: TIM 启动？DMA 使能？PSC 公式？")

    # 分布分析
    print(f"\n=== 分布分析 ===")
    buckets = [0] * 10
    for v in mins:
        idx = min(v * 10 // 4096, 9)
        buckets[idx] += 1
    for i, count in enumerate(buckets):
        if count > 0:
            low = i * 409.6
            high = (i + 1) * 409.6
            bar = '#' * count
            print(f"  [{int(low):4d}-{int(high):4d}]: {count:3d} {bar}")

    # 大跳变检测
    jumps = []
    mids = [(mins[i] + maxs[i]) / 2 for i in range(len(mins))]
    for i in range(1, len(mids)):
        diff = abs(mids[i] - mids[i-1])
        if diff > 300:
            jumps.append((i, mids[i-1], mids[i], diff))

    if jumps:
        print(f"\n=== 大跳变 (>300) ===")
        print(f"数量: {len(jumps)}")
        for idx, prev, curr, diff in jumps[:5]:
            print(f"  [{idx}] {int(prev)} -> {int(curr)} (diff={int(diff)})")
    else:
        print(f"\n✅ 无大跳变，波形平滑")

def main():
    print("=== 波形数据分析 ===\n")

    raw = collect_data()
    mins, maxs = parse_adc_data(raw)
    analyze(mins, maxs)

    return 0

if __name__ == '__main__':
    sys.exit(main())
