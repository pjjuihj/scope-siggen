#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
UART测试工具 - 优化版

功能：
1. 数据接收（只接收模式）
2. 数据发送测试
3. 交互模式
4. 数据记录

使用方法：
  python test_uart.py --port COM3 --mode receive --duration 10
  python test_uart.py --port COM3 --mode send --data "hello"
  python test_uart.py --port COM3 --mode interactive
"""

import serial
import time
import sys
import io
import argparse
from datetime import datetime

# 设置标准输出编码为UTF-8
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

class UARTTester:
    """UART测试类"""

    def __init__(self, port='COM3', baudrate=115200, timeout=None, data_format='utf-8'):
        """初始化"""
        self.port = port
        self.baudrate = baudrate
        # 根据波特率动态计算超时（至少0.1秒，最多1秒）
        if timeout is None:
            self.timeout = max(0.1, min(1.0, 10.0 / baudrate * 10))
        else:
            self.timeout = timeout
        self.ser = None
        self.received_data = []
        self.is_running = False
        self.data_format = data_format  # 'utf-8', 'hex', 'ascii'

    def open(self):
        """打开串口"""
        try:
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout
            )
            print(f"串口已打开: {self.ser.name}")
            print(f"波特率: {self.baudrate}")
            print(f"超时: {self.timeout}秒")
            return True
        except serial.SerialException as e:
            print(f"串口错误: {e}")
            return False
        except Exception as e:
            print(f"其他错误: {e}")
            return False

    def close(self):
        """关闭串口"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("串口已关闭")

    def receive(self, duration=10, show_timestamp=True, save_to_file=None):
        """接收数据（只接收模式）"""
        print(f"\n接收数据: {self.port} @ {self.baudrate} bps")
        print(f"监听时长: {duration} 秒")
        print("=" * 60)

        if not self.open():
            return False

        self.received_data = []
        start_time = time.time()
        self.is_running = True
        buffer = b""

        try:
            print(f"\n等待接收数据...\n")

            while time.time() - start_time < duration and self.is_running:
                if self.ser.in_waiting:
                    data = self.ser.read(self.ser.in_waiting)
                    buffer += data

                    # 检查是否有完整行（以\r\n结尾）
                    while b"\r\n" in buffer:
                        line, buffer = buffer.split(b"\r\n", 1)
                        timestamp = time.time() - start_time
                        self.received_data.append({
                            'timestamp': timestamp,
                            'data': line
                        })

                        # 格式化数据显示
                        display_data = self._format_data(line)

                        if show_timestamp:
                            print(f"[{timestamp:8.3f}] {display_data}")
                        else:
                            print(f"{display_data}")

                time.sleep(0.01)

            # 处理剩余数据
            if buffer:
                timestamp = time.time() - start_time
                self.received_data.append({
                    'timestamp': timestamp,
                    'data': buffer
                })
                display_data = self._format_data(buffer)
                if show_timestamp:
                    print(f"[{timestamp:8.3f}] {display_data}")
                else:
                    print(f"{display_data}")

            # 打印统计
            self._print_statistics()

            # 保存到文件
            if save_to_file:
                self._save_to_file(save_to_file)

            return True

        except KeyboardInterrupt:
            print("\n\n用户中断")
            return False
        except Exception as e:
            print(f"\n错误: {e}")
            return False
        finally:
            self.is_running = False
            self.close()

    def send(self, data, wait_response=True, wait_time=1):
        """发送数据"""
        print(f"\n发送数据: {self.port} @ {self.baudrate} bps")
        print("=" * 60)

        if not self.open():
            return False

        try:
            # 发送数据
            if isinstance(data, str):
                data = data.encode('utf-8')

            print(f"发送: {data}")
            self.ser.write(data)

            # 等待响应
            if wait_response:
                print(f"等待响应: {wait_time}秒")
                time.sleep(wait_time)

                if self.ser.in_waiting:
                    response = self.ser.read(self.ser.in_waiting)
                    print(f"响应: {response}")
                    return True
                else:
                    print("无响应")
                    return False

            return True

        except Exception as e:
            print(f"错误: {e}")
            return False
        finally:
            self.close()

    def interactive(self):
        """交互模式"""
        print(f"\n交互模式: {self.port} @ {self.baudrate} bps")
        print("=" * 60)
        print("命令:")
        print("  /hex XX XX XX  - 发送十六进制数据")
        print("  /baud N        - 切换波特率")
        print("  /clear         - 清屏")
        print("  /quit          - 退出")
        print("  其他           - 发送文本")
        print("=" * 60)

        if not self.open():
            return False

        self.is_running = True

        try:
            # 启动接收线程
            import threading
            receive_thread = threading.Thread(target=self._receive_thread)
            receive_thread.daemon = True
            receive_thread.start()

            # 主线程处理输入
            while self.is_running:
                try:
                    cmd = input("\n> ")

                    if cmd == "/quit":
                        break
                    elif cmd == "/clear":
                        print("\033[2J\033[H")
                    elif cmd.startswith("/baud "):
                        new_baud = int(cmd.split(" ")[1])
                        self.ser.baudrate = new_baud
                        print(f"波特率已切换: {new_baud}")
                    elif cmd.startswith("/hex "):
                        hex_str = cmd.split(" ", 1)[1]
                        data = bytes.fromhex(hex_str.replace(" ", ""))
                        self.ser.write(data)
                        print(f"已发送: {data}")
                    elif cmd:
                        self.ser.write((cmd + "\r\n").encode('utf-8'))
                        print(f"已发送: {cmd}")

                except KeyboardInterrupt:
                    break
                except Exception as e:
                    print(f"错误: {e}")

            return True

        except Exception as e:
            print(f"错误: {e}")
            return False
        finally:
            self.is_running = False
            self.close()

    def _receive_thread(self):
        """接收线程"""
        while self.is_running:
            if self.ser and self.ser.is_open and self.ser.in_waiting:
                data = self.ser.read(self.ser.in_waiting)
                print(f"\n[接收] {data}")
            time.sleep(0.01)

    def _format_data(self, data):
        """格式化数据显示"""
        if self.data_format == 'hex':
            # HEX格式：显示为十六进制
            return ' '.join(f'{b:02X}' for b in data)
        elif self.data_format == 'ascii':
            # ASCII格式：只显示可打印字符
            return ''.join(chr(b) if 32 <= b < 127 else '.' for b in data)
        else:
            # UTF-8格式：解码为字符串
            return data.decode('utf-8', errors='replace')

    def _print_statistics(self):
        """打印统计信息"""
        print("\n" + "=" * 60)
        print("接收统计:")
        print(f"  接收次数: {len(self.received_data)}")

        total_bytes = sum(len(d['data']) for d in self.received_data)
        print(f"  总字节数: {total_bytes}")

        if self.received_data:
            print(f"  开始时间: {self.received_data[0]['timestamp']:.3f}s")
            print(f"  结束时间: {self.received_data[-1]['timestamp']:.3f}s")
            duration = self.received_data[-1]['timestamp'] - self.received_data[0]['timestamp']
            print(f"  持续时间: {duration:.3f}s")

            if duration > 0:
                rate = len(self.received_data) / duration
                print(f"  接收速率: {rate:.1f} 条/秒")

        print("=" * 60)

    def _save_to_file(self, filename):
        """保存数据到文件"""
        try:
            with open(filename, 'w', encoding='utf-8') as f:
                f.write(f"UART数据记录\n")
                f.write(f"端口: {self.port}\n")
                f.write(f"波特率: {self.baudrate}\n")
                f.write(f"时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
                f.write("=" * 60 + "\n")

                for item in self.received_data:
                    f.write(f"[{item['timestamp']:.3f}] {item['data']}\n")

            print(f"\n数据已保存: {filename}")
        except Exception as e:
            print(f"\n保存失败: {e}")

def list_ports():
    """列出可用串口"""
    import serial.tools.list_ports
    ports = serial.tools.list_ports.comports()

    print("可用串口:")
    print("=" * 60)
    for port in ports:
        print(f"  {port.device}: {port.description}")
        print(f"    硬件ID: {port.hwid}")
        print(f"    位置: {port.location}")
    print("=" * 60)
    print(f"共 {len(ports)} 个串口")

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='UART测试工具')
    parser.add_argument('--port', default='COM3', help='串口端口')
    parser.add_argument('--baud', type=int, default=115200, help='波特率')
    parser.add_argument('--mode', choices=['receive', 'send', 'interactive', 'list'],
                        default='receive', help='工作模式')
    parser.add_argument('--duration', type=int, default=10, help='接收时长（秒）')
    parser.add_argument('--data', help='发送数据')
    parser.add_argument('--output', help='输出文件路径')
    parser.add_argument('--no-timestamp', action='store_true', help='不显示时间戳')
    parser.add_argument('--format', choices=['utf-8', 'hex', 'ascii'],
                        default='utf-8', help='数据显示格式')

    args = parser.parse_args()

    print("UART测试工具")
    print("=" * 60)

    if args.mode == 'list':
        list_ports()
        return

    tester = UARTTester(port=args.port, baudrate=args.baud, data_format=args.format)

    if args.mode == 'receive':
        tester.receive(
            duration=args.duration,
            show_timestamp=not args.no_timestamp,
            save_to_file=args.output
        )
    elif args.mode == 'send':
        if not args.data:
            print("错误: 发送模式需要指定 --data 参数")
            return
        tester.send(args.data)
    elif args.mode == 'interactive':
        tester.interactive()

if __name__ == '__main__':
    main()
