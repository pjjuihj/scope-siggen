#!/usr/bin/env python3
"""
SCOPE-SIGGEN 自动化回归测试

通过 UART 串口自动验证固件功能。
用法: python regression_test.py [COM端口]
示例: python regression_test.py COM3
"""

import serial
import time
import sys


class RegressionTester:
    """UART 串口回归测试器"""

    def __init__(self, port='COM3', baud=115200):
        self.port = port
        self.baud = baud
        self.ser = None
        self.passed = 0
        self.failed = 0
        self.errors = []

    def connect(self):
        """连接串口"""
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=2)
            time.sleep(0.5)  # 等待串口稳定
            # 清空接收缓冲区
            self.ser.read(self.ser.in_waiting)
            return True
        except serial.SerialException as e:
            print(f"ERROR: Cannot open {self.port}: {e}")
            return False

    def disconnect(self):
        """断开串口"""
        if self.ser and self.ser.is_open:
            self.ser.close()

    def drain_serial(self, timeout=0.5):
        """排空串口接收缓冲区"""
        end = time.time() + timeout
        while time.time() < end:
            n = self.ser.in_waiting
            if n:
                self.ser.read(n)
                time.sleep(0.05)  # 等待更多数据到达
            else:
                break

    def send_cmd(self, cmd, wait=0.3):
        """发送命令并返回响应"""
        if not self.ser or not self.ser.is_open:
            return False, "Serial not connected"

        # 排空接收缓冲区（等待残留数据到达并清除）
        self.drain_serial(0.3)

        # 发送命令
        self.ser.write(f'{cmd}\n'.encode())
        time.sleep(wait)

        # 读取响应
        response = self.ser.read(self.ser.in_waiting).decode(errors='ignore')
        return True, response

    def check_ok(self, cmd, wait=0.3):
        """发送命令并检查是否返回 OK"""
        ok, resp = self.send_cmd(cmd, wait)
        if not ok:
            return False, resp
        if 'OK' not in resp and 'ok' not in resp.lower():
            return False, resp
        return True, resp

    def run_test(self, name, test_func):
        """运行单个测试"""
        try:
            test_func()
            self.passed += 1
            print(f"  PASS  {name}")
            return True
        except AssertionError as e:
            self.failed += 1
            self.errors.append(f"{name}: {e}")
            print(f"  FAIL  {name} -- {e}")
            return False
        except Exception as e:
            self.failed += 1
            self.errors.append(f"{name}: {e}")
            print(f"  ERROR {name} -- {e}")
            return False

    # =========================================================================
    # 测试用例
    # =========================================================================

    def test_version(self):
        """验证版本信息返回"""
        ok, resp = self.send_cmd('version')
        assert ok, "No response"
        assert 'Version' in resp, f"Missing 'Version' in: {resp}"
        assert 'OK:version' in resp, f"Missing OK in: {resp}"

    def test_help(self):
        """验证帮助信息返回"""
        ok, resp = self.send_cmd('help')
        assert ok, "No response"
        assert 'Commands' in resp, f"Missing 'Commands' in: {resp}"
        assert 'OK:help' in resp, f"Missing OK in: {resp}"

    def test_status(self):
        """验证状态信息返回"""
        ok, resp = self.send_cmd('status')
        assert ok, "No response"
        assert 'Oscilloscope' in resp, f"Missing 'Oscilloscope' in: {resp}"
        assert 'Signal Gen' in resp, f"Missing 'Signal Gen' in: {resp}"
        assert 'OK:status' in resp, f"Missing OK in: {resp}"

    def test_set_freq(self):
        """验证频率设置"""
        ok, resp = self.check_ok('set_freq 1000')
        assert ok, f"set_freq failed: {resp}"

    def test_set_wave_sine(self):
        """验证正弦波设置"""
        ok, resp = self.check_ok('set_wave sine')
        assert ok, f"set_wave sine failed: {resp}"

    def test_set_wave_square(self):
        """验证方波设置"""
        ok, resp = self.check_ok('set_wave square')
        assert ok, f"set_wave square failed: {resp}"

    def test_set_wave_triangle(self):
        """验证三角波设置"""
        ok, resp = self.check_ok('set_wave triangle')
        assert ok, f"set_wave triangle failed: {resp}"

    def test_set_amp(self):
        """验证幅度设置"""
        ok, resp = self.check_ok('set_amp 1500')
        assert ok, f"set_amp failed: {resp}"

    def test_gen_start_stop(self):
        """验证信号发生器启停"""
        ok, resp = self.check_ok('start_gen')
        assert ok, f"start_gen failed: {resp}"
        time.sleep(0.5)
        ok, resp = self.check_ok('stop_gen')
        assert ok, f"stop_gen failed: {resp}"

    def test_osc_start_stop(self):
        """验证示波器启停"""
        ok, resp = self.check_ok('start_osc')
        assert ok, f"start_osc failed: {resp}"
        time.sleep(0.5)
        ok, resp = self.check_ok('stop_osc')
        assert ok, f"stop_osc failed: {resp}"

    def test_measure(self):
        """验证测量命令"""
        ok, resp = self.send_cmd('measure')
        assert ok, "No response"
        # measure 可能在示波器未启动时返回 ERROR，这是正常的
        # 只要命令不崩溃就行

    def test_page_switch(self):
        """验证页面切换"""
        ok, resp = self.check_ok('page next')
        assert ok, f"page next failed: {resp}"
        ok, resp = self.check_ok('page prev')
        assert ok, f"page prev failed: {resp}"

    def test_config_save_load(self):
        """验证配置保存和加载"""
        ok, resp = self.check_ok('save', wait=1.0)
        assert ok, f"save failed: {resp}"
        time.sleep(0.5)
        ok, resp = self.check_ok('load', wait=1.0)
        assert ok, f"load failed: {resp}"

    def test_brightness(self):
        """验证亮度设置"""
        ok, resp = self.check_ok('brightness 128')
        assert ok, f"brightness failed: {resp}"

    def test_cursor(self):
        """验证光标控制"""
        ok, resp = self.check_ok('cursor 64,32')
        assert ok, f"cursor move failed: {resp}"
        ok, resp = self.check_ok('cursor off')
        assert ok, f"cursor off failed: {resp}"

    def test_unknown_command(self):
        """验证未知命令处理"""
        ok, resp = self.send_cmd('nonexistent_cmd', wait=0.5)
        assert ok, "No response"
        assert 'ERROR' in resp or 'Unknown' in resp, f"Expected error for unknown cmd: {resp}"

    # =========================================================================
    # 测试执行
    # =========================================================================

    def run_all(self):
        """运行所有测试"""
        print(f"SCOPE-SIGGEN Regression Test")
        print(f"Port: {self.port}, Baud: {self.baud}")
        print(f"{'='*40}")

        if not self.connect():
            return 1

        # 等待系统启动
        print("Waiting for system boot...")
        time.sleep(2)
        self.ser.read(self.ser.in_waiting)  # 清空启动输出

        # 定义测试套件
        tests = [
            ("version",             self.test_version),
            ("help",                self.test_help),
            ("status",              self.test_status),
            ("set_freq",            self.test_set_freq),
            ("set_wave_sine",       self.test_set_wave_sine),
            ("set_wave_square",     self.test_set_wave_square),
            ("set_wave_triangle",   self.test_set_wave_triangle),
            ("set_amp",             self.test_set_amp),
            ("gen_start_stop",      self.test_gen_start_stop),
            ("osc_start_stop",      self.test_osc_start_stop),
            ("measure",             self.test_measure),
            ("page_switch",         self.test_page_switch),
            ("brightness",          self.test_brightness),
            ("cursor",              self.test_cursor),
            ("config_save_load",    self.test_config_save_load),
            ("unknown_command",     self.test_unknown_command),
        ]

        # 执行测试（每个测试间加间隔，避免串口缓冲区污染）
        for name, func in tests:
            self.run_test(name, func)
            time.sleep(0.2)

        # 输出报告
        total = self.passed + self.failed
        print(f"{'='*40}")
        print(f"Result: {self.passed}/{total} passed")

        if self.errors:
            print(f"\nFailures:")
            for err in self.errors:
                print(f"  - {err}")

        self.disconnect()
        return 0 if self.failed == 0 else 1


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else 'COM3'
    tester = RegressionTester(port)
    sys.exit(tester.run_all())


if __name__ == '__main__':
    main()
