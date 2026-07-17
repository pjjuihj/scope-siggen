"""
串口波形数据捕获和分析工具
使用方法：python scripts/capture_waveform.py
"""

import serial
import time
import sys
import re
from collections import defaultdict

class WaveformAnalyzer:
    def __init__(self, port='COM3', baudrate=115200):
        self.ser = serial.Serial(port, baudrate, timeout=2)
        self.waveform_data = []
        self.analysis_results = {}

    def reset_stm32(self):
        """复位 STM32"""
        print("[1] 复位 STM32...")
        self.ser.write(b'reset\r\n')
        time.sleep(2)
        # 读取启动日志
        log = self.ser.read(self.ser.in_waiting).decode('utf-8', errors='ignore')
        print(f"  启动日志长度: {len(log)} 字节")
        return log

    def send_command(self, cmd, wait_time=0.5):
        """发送命令并等待响应"""
        print(f"  发送命令: {cmd}")
        self.ser.reset_input_buffer()
        self.ser.write((cmd + '\r\n').encode())
        time.sleep(wait_time)
        if self.ser.in_waiting > 0:
            response = self.ser.read(self.ser.in_waiting).decode('utf-8', errors='ignore')
            print(f"  响应长度: {len(response)} 字节")
            return response
        print(f"  无响应")
        return ""

    def capture_waveform_data(self, duration=3):
        """捕获波形数据"""
        print(f"\n[2] 捕获波形数据 ({duration}秒)...")

        # 启动示波器和信号发生器
        self.send_command("start_osc")
        time.sleep(0.5)
        self.send_command("start_gen")
        time.sleep(0.5)

        # 发送 stream_dac 命令
        print("  发送 stream_dac 命令...")
        self.ser.write(b'stream_dac\r\n')
        time.sleep(1)

        # 捕获数据
        start_time = time.time()
        data_lines = []
        bytes_read = 0

        print(f"  开始捕获 ({duration}秒)...")
        while time.time() - start_time < duration:
            if self.ser.in_waiting > 0:
                data = self.ser.read(self.ser.in_waiting)
                bytes_read += len(data)
                text = data.decode('utf-8', errors='ignore')
                lines = text.split('\n')
                for line in lines:
                    line = line.strip()
                    if line:
                        data_lines.append(line)
            time.sleep(0.01)

        print(f"  读取字节: {bytes_read}")
        print(f"  捕获行数: {len(data_lines)}")

        # 停止示波器和信号发生器
        self.send_command("stop_osc")
        self.send_command("stop_gen")

        return data_lines

    def parse_waveform_data(self, data_lines):
        """解析波形数据"""
        print("\n[3] 解析波形数据...")

        waveform = []
        dac_data = []
        adc_data = []

        for line in data_lines:
            # 跳过 STREAM: 头
            if line.startswith("STREAM:"):
                print(f"  帧头: {line}")
                continue

            # 解析 DAC,ADC 格式
            if ',' in line:
                try:
                    parts = line.split(',')
                    dac_val = int(parts[0].strip())
                    adc_val = int(parts[1].strip())
                    dac_data.append(dac_val)
                    adc_data.append(adc_val)
                    waveform.append(adc_val)  # 主要分析 ADC 数据
                except (ValueError, IndexError):
                    pass
            else:
                # 尝试解析单个数字
                try:
                    value = int(line.strip())
                    waveform.append(value)
                except ValueError:
                    pass

        print(f"  解析到 {len(waveform)} 个采样点")
        if dac_data:
            print(f"  DAC 数据: {len(dac_data)} 个点")
        if adc_data:
            print(f"  ADC 数据: {len(adc_data)} 个点")

        return waveform, dac_data, adc_data

    def analyze_waveform(self, waveform):
        """分析波形"""
        print("\n[4] 分析波形...")

        if len(waveform) < 2:
            print("  数据不足，无法分析")
            return

        # 基本统计
        min_val = min(waveform)
        max_val = max(waveform)
        avg_val = sum(waveform) / len(waveform)
        range_val = max_val - min_val

        print(f"  采样点数: {len(waveform)}")
        print(f"  最小值: {min_val} (ADC)")
        print(f"  最大值: {max_val} (ADC)")
        print(f"  平均值: {avg_val:.1f} (ADC)")
        print(f"  范围: {range_val} (ADC)")

        # 转换为电压 (mV)
        min_mv = min_val * 3300 // 4096
        max_mv = max_val * 3300 // 4096
        avg_mv = avg_val * 3300 / 4096
        vpp_mv = range_val * 3300 // 4096

        print(f"\n  电压分析:")
        print(f"    最小电压: {min_mv} mV")
        print(f"    最大电压: {max_mv} mV")
        print(f"    平均电压: {avg_mv:.1f} mV")
        print(f"    峰峰值 (Vpp): {vpp_mv} mV")

        # 频率分析（过零检测）
        threshold = (min_val + max_val) // 2
        zero_crossings = []
        for i in range(1, len(waveform)):
            if (waveform[i-1] < threshold and waveform[i] >= threshold):
                zero_crossings.append(i)

        if len(zero_crossings) >= 2:
            # 计算周期
            periods = []
            for i in range(1, len(zero_crossings)):
                periods.append(zero_crossings[i] - zero_crossings[i-1])

            avg_period = sum(periods) / len(periods)
            # 假设采样率 1MHz
            freq_hz = 1000000 / avg_period

            print(f"\n  频率分析:")
            print(f"    过零点数: {len(zero_crossings)}")
            print(f"    平均周期: {avg_period:.1f} 样本")
            print(f"    频率: {freq_hz:.1f} Hz")
        else:
            print(f"\n  频率分析: 无法检测（过零点不足）")

        # 波形质量分析
        print(f"\n  波形质量:")
        # 检查是否有噪声（相邻点差异）
        noise_count = 0
        for i in range(1, len(waveform)):
            diff = abs(waveform[i] - waveform[i-1])
            if diff > 50:  # 阈值
                noise_count += 1

        noise_ratio = noise_count / len(waveform) * 100
        print(f"    噪声点数: {noise_count} ({noise_ratio:.1f}%)")

        # 检查是否稳定
        if len(waveform) > 100:
            first_half = waveform[:len(waveform)//2]
            second_half = waveform[len(waveform)//2:]
            first_avg = sum(first_half) / len(first_half)
            second_avg = sum(second_half) / len(second_half)
            stability = abs(first_avg - second_avg) / max_val * 100
            print(f"    稳定性: {100-stability:.1f}%")

        self.analysis_results = {
            'samples': len(waveform),
            'min_val': min_val,
            'max_val': max_val,
            'avg_val': avg_val,
            'vpp_mv': vpp_mv,
            'min_mv': min_mv,
            'max_mv': max_mv,
            'noise_ratio': noise_ratio
        }

    def plot_waveform(self, waveform):
        """绘制波形（ASCII）"""
        print("\n[5] 波形预览 (ASCII):")

        if len(waveform) == 0:
            print("  无数据")
            return

        min_val = min(waveform)
        max_val = max(waveform)
        range_val = max_val - min_val

        if range_val == 0:
            print("  波形为直流")
            return

        # 显示前 64 个点
        display_len = min(64, len(waveform))
        height = 10

        print(f"  Max: {max_val}")
        for row in range(height, -1, -1):
            threshold = min_val + (range_val * row // height)
            line = "  "
            for i in range(display_len):
                if waveform[i] >= threshold:
                    line += "*"
                else:
                    line += " "
            if row == height:
                line += f"  {max_val}"
            elif row == 0:
                line += f"  {min_val}"
            print(line)

        print(f"  Min: {min_val}")
        print(f"  显示前 {display_len} 个采样点 (共 {len(waveform)} 个)")

    def run(self):
        """运行完整分析"""
        print("=" * 60)
        print("串口波形数据分析工具")
        print("=" * 60)

        try:
            # 1. 复位 STM32
            log = self.reset_stm32()

            # 2. 捕获波形数据
            data_lines = self.capture_waveform_data(duration=3)

            # 3. 解析波形数据
            waveform, dac_data, adc_data = self.parse_waveform_data(data_lines)

            # 4. 分析波形
            self.analyze_waveform(waveform)

            # 5. 绘制波形
            self.plot_waveform(waveform)

            # 6. 绘制 DAC 波形
            if dac_data:
                print("\n[6] DAC 波形预览:")
                self.plot_waveform(dac_data)

            # 6. 输出分析报告
            print("\n" + "=" * 60)
            print("分析报告")
            print("=" * 60)

            if self.analysis_results:
                r = self.analysis_results
                print(f"采样点数: {r['samples']}")
                print(f"电压范围: {r['min_mv']} ~ {r['max_mv']} mV")
                print(f"峰峰值: {r['vpp_mv']} mV")
                print(f"噪声比例: {r['noise_ratio']:.1f}%")

                # 评估
                print("\n评估:")
                if r['vpp_mv'] > 100:
                    print("  [OK] 信号幅度正常")
                else:
                    print("  [WARNING] 信号幅度过小")

                if r['noise_ratio'] < 5:
                    print("  [OK] 噪声水平正常")
                else:
                    print("  [WARNING] 噪声偏大")

            print("=" * 60)

        except Exception as e:
            print(f"错误: {e}")
        finally:
            self.ser.close()

if __name__ == "__main__":
    analyzer = WaveformAnalyzer()
    analyzer.run()
