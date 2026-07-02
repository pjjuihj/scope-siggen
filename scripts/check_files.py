#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
文件完整性检查脚本

功能：
1. 检查必需的文件是否存在
2. 检查文件内容是否正确
3. 生成检查报告
"""

import os
import sys
import io

# 设置标准输出编码为UTF-8
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

# 必需的文件列表
REQUIRED_FILES = [
    # 版本管理模块
    'Core/Inc/version.h',
    'Core/Src/version.c',

    # 错误追踪模块
    'Core/Inc/error_tracker.h',
    'Core/Src/error_tracker.c',

    # 环形缓冲模块
    'Core/Inc/ring_buffer.h',
    'Core/Src/ring_buffer.c',

    # 调试工具模块
    'Core/Inc/debug.h',
    'Core/Src/debug.c',

    # 版本管理脚本
    'scripts/build_version.py',

    # CubeMX配置
    'scope-siggen.ioc',

    # Keil工程
    'MDK-ARM/scope-siggen.uvprojx',
]

# main.c中需要的修改
MAIN_C_MODIFICATIONS = [
    '#include "version.h"',
    '#include "debug.h"',
    '#include "error_tracker.h"',
    'Debug_Init();',
    'ErrorTracker_Init();',
    'Version_Print();',
]

def check_file_exists(project_dir, file_path):
    """检查文件是否存在"""
    full_path = os.path.join(project_dir, file_path)
    return os.path.exists(full_path)

def check_main_c_modifications(project_dir):
    """检查main.c是否已修改"""
    main_c_path = os.path.join(project_dir, 'Core/Src/main.c')

    if not os.path.exists(main_c_path):
        return False, "main.c not found"

    with open(main_c_path, 'r', encoding='utf-8') as f:
        content = f.read()

    missing_modifications = []
    for mod in MAIN_C_MODIFICATIONS:
        if mod not in content:
            missing_modifications.append(mod)

    if missing_modifications:
        return False, f"Missing modifications: {missing_modifications}"

    return True, "All modifications found"

def check_project_dir(project_dir):
    """检查项目目录"""
    if not os.path.exists(project_dir):
        return False, f"Project directory not found: {project_dir}"

    return True, "Project directory exists"

def main():
    """主函数"""
    import argparse

    parser = argparse.ArgumentParser(description='File Integrity Checker')
    parser.add_argument('--project', default='.', help='Project directory')

    args = parser.parse_args()

    project_dir = os.path.abspath(args.project)

    print(f"Checking project: {project_dir}")
    print("=" * 60)

    # 检查项目目录
    success, message = check_project_dir(project_dir)
    if not success:
        print(f"❌ {message}")
        return 1

    print(f"✅ {message}")

    # 检查必需文件
    print("\nChecking required files:")
    print("-" * 60)

    all_files_ok = True
    for file_path in REQUIRED_FILES:
        if check_file_exists(project_dir, file_path):
            print(f"✅ {file_path}")
        else:
            print(f"❌ {file_path} - MISSING")
            all_files_ok = False

    # 检查main.c修改
    print("\nChecking main.c modifications:")
    print("-" * 60)

    success, message = check_main_c_modifications(project_dir)
    if success:
        print(f"✅ {message}")
    else:
        print(f"❌ {message}")
        all_files_ok = False

    # 总结
    print("\n" + "=" * 60)
    if all_files_ok:
        print("✅ All checks passed!")
        print("\nYou can now compile the project in Keil MDK-ARM.")
        return 0
    else:
        print("❌ Some checks failed!")
        print("\nPlease fix the issues before compiling.")
        return 1

if __name__ == '__main__':
    exit(main())
