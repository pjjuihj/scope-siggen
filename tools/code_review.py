#!/usr/bin/env python3
"""
代码审查自动化工具
检查 C 代码质量、命名规范、错误处理等
"""

import os
import re
import sys
import json
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Dict, Optional
from enum import Enum

class Severity(Enum):
    ERROR = "error"
    WARNING = "warning"
    INFO = "info"

@dataclass
class Issue:
    file: str
    line: int
    severity: Severity
    category: str
    message: str
    suggestion: str = ""

@dataclass
class ReviewResult:
    issues: List[Issue] = field(default_factory=list)
    stats: Dict[str, int] = field(default_factory=dict)

class CodeReviewer:
    """C 代码审查器"""

    def __init__(self, src_dir: str):
        self.src_dir = Path(src_dir)
        self.result = ReviewResult()
        self.files_checked = 0

    def review(self) -> ReviewResult:
        """执行代码审查"""
        print(f"=== 代码审查开始 ===")
        print(f"目录: {self.src_dir}")
        print()

        # 检查所有 .c 文件
        for c_file in self.src_dir.glob("**/*.c"):
            if "MDK-ARM" in str(c_file) or "Drivers" in str(c_file):
                continue
            self._review_file(c_file)

        # 检查所有 .h 文件
        for h_file in self.src_dir.glob("**/*.h"):
            if "MDK-ARM" in str(h_file) or "Drivers" in str(h_file):
                continue
            self._review_header(h_file)

        self.result.stats["files_checked"] = self.files_checked
        self.result.stats["total_issues"] = len(self.result.issues)
        self.result.stats["errors"] = sum(1 for i in self.result.issues if i.severity == Severity.ERROR)
        self.result.stats["warnings"] = sum(1 for i in self.result.issues if i.severity == Severity.WARNING)
        self.result.stats["info"] = sum(1 for i in self.result.issues if i.severity == Severity.INFO)

        return self.result

    def _review_file(self, filepath: Path):
        """审查 .c 文件"""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            print(f"无法读取文件: {filepath} - {e}")
            return

        self.files_checked += 1
        rel_path = str(filepath.relative_to(self.src_dir))

        # 检查命名规范
        self._check_naming(rel_path, lines)

        # 检查错误处理
        self._check_error_handling(rel_path, lines)

        # 检查魔法数字
        self._check_magic_numbers(rel_path, lines)

        # 检查函数长度
        self._check_function_length(rel_path, lines)

        # 检查注释密度
        self._check_comments(rel_path, lines, content)

        # 检查 include 顺序
        self._check_includes(rel_path, lines)

        # 检查 static 使用
        self._check_static_usage(rel_path, lines)

    def _review_header(self, filepath: Path):
        """审查 .h 文件"""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception as e:
            return

        self.files_checked += 1
        rel_path = str(filepath.relative_to(self.src_dir))

        # 检查 include guard
        self._check_include_guard(rel_path, lines)

    def _check_naming(self, filepath: str, lines: List[str]):
        """检查命名规范"""
        for i, line in enumerate(lines, 1):
            # 函数定义检查 (小写+下划线)
            if re.match(r'^(static\s+)?\w+\s+\w+\s*\(', line):
                func_match = re.search(r'(\w+)\s*\(', line)
                if func_match:
                    func_name = func_match.group(1)
                    if func_name[0].isupper() and not func_name.startswith('HAL_'):
                        # 允许 HAL_ 开头的函数
                        pass
                    elif not re.match(r'^[a-z][a-z0-9_]*$', func_name) and not func_name.startswith('_'):
                        if func_name not in ['main', 'Error_Handler']:
                            self.result.issues.append(Issue(
                                file=filepath,
                                line=i,
                                severity=Severity.WARNING,
                                category="命名规范",
                                message=f"函数名 '{func_name}' 应使用小写+下划线",
                                suggestion=f"建议改为: {func_name.lower()}"
                            ))

            # 宏定义检查 (大写+下划线)
            if line.strip().startswith('#define'):
                macro_match = re.search(r'#define\s+(\w+)', line)
                if macro_match:
                    macro_name = macro_match.group(1)
                    if not re.match(r'^[A-Z][A-Z0-9_]*$', macro_name) and not macro_name.startswith('_'):
                        self.result.issues.append(Issue(
                            file=filepath,
                            line=i,
                            severity=Severity.WARNING,
                            category="命名规范",
                            message=f"宏名 '{macro_name}' 应使用大写+下划线"
                        ))

    def _check_error_handling(self, filepath: str, lines: List[str]):
        """检查错误处理"""
        for i, line in enumerate(lines, 1):
            # HAL 函数调用未检查返回值
            if 'HAL_' in line and '(' in line and '=' not in line and 'void' not in line:
                if 'LOG_' not in line and '//' not in line.split('HAL_')[0]:
                    self.result.issues.append(Issue(
                        file=filepath,
                        line=i,
                        severity=Severity.WARNING,
                        category="错误处理",
                        message="HAL 函数调用未检查返回值",
                        suggestion="添加返回值检查: if (HAL_...() != HAL_OK) { ... }"
                    ))

    def _check_magic_numbers(self, filepath: str, lines: List[str]):
        """检查魔法数字"""
        # 忽略的数字
        ignore_numbers = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 16, 32, 64, 128, 256, 512, 1024}

        for i, line in enumerate(lines, 1):
            # 跳过注释和宏定义
            stripped = line.strip()
            if stripped.startswith('//') or stripped.startswith('#') or stripped.startswith('*'):
                continue

            # 查找数字
            numbers = re.findall(r'\b(\d+)\b', line)
            for num_str in numbers:
                num = int(num_str)
                if num > 10 and num not in ignore_numbers:
                    # 检查是否是已定义的常量
                    try:
                        parts = line.split(str(num))[0].split()
                        if parts:
                            last_part = parts[-1]
                            if any(c.isupper() for c in last_part if c.isalpha()):
                                continue
                    except (IndexError, ValueError):
                        pass

                    self.result.issues.append(Issue(
                        file=filepath,
                        line=i,
                        severity=Severity.INFO,
                        category="魔法数字",
                        message=f"数字 {num} 可能是魔法数字",
                        suggestion="考虑定义为宏或常量"
                    ))

    def _check_function_length(self, filepath: str, lines: List[str]):
        """检查函数长度"""
        in_function = False
        brace_count = 0
        func_start = 0
        func_name = ""

        for i, line in enumerate(lines, 1):
            # 检测函数开始
            if re.match(r'^(static\s+)?\w+\s+\w+\s*\(', line) and '{' in line:
                in_function = True
                brace_count = line.count('{') - line.count('}')
                func_start = i
                func_match = re.search(r'(\w+)\s*\(', line)
                if func_match:
                    func_name = func_match.group(1)
            elif in_function:
                brace_count += line.count('{') - line.count('}')
                if brace_count <= 0:
                    func_length = i - func_start
                    if func_length > 100:
                        self.result.issues.append(Issue(
                            file=filepath,
                            line=func_start,
                            severity=Severity.WARNING,
                            category="函数长度",
                            message=f"函数 '{func_name}' 长度 {func_length} 行，超过 100 行",
                            suggestion="考虑拆分为更小的函数"
                        ))
                    in_function = False

    def _check_comments(self, filepath: str, lines: List[str], content: str):
        """检查注释密度"""
        code_lines = sum(1 for line in lines if line.strip() and not line.strip().startswith('//') and not line.strip().startswith('*'))
        comment_lines = sum(1 for line in lines if '//' in line or '/*' in line or line.strip().startswith('*'))

        if code_lines > 0:
            comment_ratio = comment_lines / code_lines
            if comment_ratio < 0.1:
                self.result.issues.append(Issue(
                    file=filepath,
                    line=0,
                    severity=Severity.INFO,
                    category="注释密度",
                    message=f"注释比例 {comment_ratio:.1%}，建议增加注释",
                    suggestion="关键函数和复杂逻辑应添加注释"
                ))

    def _check_includes(self, filepath: str, lines: List[str]):
        """检查 include 顺序"""
        includes = []
        for i, line in enumerate(lines, 1):
            if line.strip().startswith('#include'):
                includes.append((i, line.strip()))

        # 检查顺序: 本项目头文件 -> 标准库
        project_includes = []
        system_includes = []
        for line_num, inc in includes:
            if '"' in inc:
                project_includes.append((line_num, inc))
            else:
                system_includes.append((line_num, inc))

        if project_includes and system_includes:
            if project_includes[-1][0] > system_includes[0][0]:
                self.result.issues.append(Issue(
                    file=filepath,
                    line=system_includes[0][0],
                    severity=Severity.INFO,
                    category="Include 顺序",
                    message="系统头文件应在项目头文件之后",
                    suggestion="调整 include 顺序: 项目头文件在前，系统头文件在后"
                ))

    def _check_static_usage(self, filepath: str, lines: List[str]):
        """检查 static 使用"""
        for i, line in enumerate(lines, 1):
            # 全局变量未使用 static
            if re.match(r'^(volatile\s+)?\w+\s+\w+\s*[=;]', line):
                if not line.strip().startswith('static') and not line.strip().startswith('extern'):
                    # 检查是否在函数内
                    if not self._is_in_function(lines, i):
                        self.result.issues.append(Issue(
                            file=filepath,
                            line=i,
                            severity=Severity.WARNING,
                            category="变量作用域",
                            message="全局变量未使用 static 限制作用域",
                            suggestion="如果只在本文件使用，应添加 static"
                        ))

    def _is_in_function(self, lines: List[str], line_num: int) -> bool:
        """检查某行是否在函数内"""
        brace_count = 0
        for i in range(line_num - 1, -1, -1):
            brace_count += lines[i].count('}') - lines[i].count('{')
            if brace_count > 0:
                return True
        return False

    def _check_include_guard(self, filepath: str, lines: List[str]):
        """检查 include guard"""
        has_ifndef = False
        has_endif = False

        for line in lines:
            if '#ifndef' in line:
                has_ifndef = True
            if '#endif' in line:
                has_endif = True

        if not has_ifndef or not has_endif:
            self.result.issues.append(Issue(
                file=filepath,
                line=0,
                severity=Severity.WARNING,
                category="Include Guard",
                message="头文件缺少 include guard",
                suggestion="添加 #ifndef __FILENAME_H / #define __FILENAME_H / #endif"
            ))

    def print_report(self):
        """打印审查报告"""
        print(f"=== 代码审查报告 ===")
        print(f"检查文件数: {self.result.stats.get('files_checked', 0)}")
        print(f"发现问题数: {self.result.stats.get('total_issues', 0)}")
        print(f"  - 错误: {self.result.stats.get('errors', 0)}")
        print(f"  - 警告: {self.result.stats.get('warnings', 0)}")
        print(f"  - 信息: {self.result.stats.get('info', 0)}")
        print()

        if not self.result.issues:
            print("[OK] 未发现问题")
            return

        # 按类别分组
        categories = {}
        for issue in self.result.issues:
            if issue.category not in categories:
                categories[issue.category] = []
            categories[issue.category].append(issue)

        for category, issues in categories.items():
            print(f"### {category}")
            for issue in issues[:10]:  # 每类最多显示 10 个
                severity_icon = {"error": "[ERR]", "warning": "[WARN]", "info": "[INFO]"}
                print(f"  {severity_icon.get(issue.severity.value, '?')} {issue.file}:{issue.line}")
                print(f"    {issue.message}")
                if issue.suggestion:
                    print(f"    建议: {issue.suggestion}")
            if len(issues) > 10:
                print(f"  ... 还有 {len(issues) - 10} 个问题")
            print()

    def export_json(self, output_path: str):
        """导出 JSON 报告"""
        report = {
            "stats": self.result.stats,
            "issues": [
                {
                    "file": i.file,
                    "line": i.line,
                    "severity": i.severity.value,
                    "category": i.category,
                    "message": i.message,
                    "suggestion": i.suggestion
                }
                for i in self.result.issues
            ]
        }

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(report, f, ensure_ascii=False, indent=2)

        print(f"报告已导出到: {output_path}")

def main():
    import argparse
    import io

    # 设置 stdout 编码为 UTF-8
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

    parser = argparse.ArgumentParser(description='C 代码审查工具')
    parser.add_argument('--src', default='Core/Src', help='源代码目录')
    parser.add_argument('--output', '-o', help='输出 JSON 报告路径')
    parser.add_argument('--auto', action='store_true', help='自动检测项目目录')

    args = parser.parse_args()

    # 自动检测项目目录
    if args.auto:
        current = Path.cwd()
        if (current / 'Core' / 'Src').exists():
            src_dir = current / 'Core' / 'Src'
        elif (current / 'MDK-ARM').exists():
            src_dir = current / 'Core' / 'Src'
        else:
            src_dir = Path(args.src)
    else:
        src_dir = Path(args.src)

    if not src_dir.exists():
        print(f"错误: 目录不存在 - {src_dir}")
        sys.exit(1)

    # 执行审查
    reviewer = CodeReviewer(str(src_dir))
    result = reviewer.review()

    # 打印报告
    reviewer.print_report()

    # 导出 JSON
    if args.output:
        reviewer.export_json(args.output)

    # 返回码
    if result.stats.get('errors', 0) > 0:
        sys.exit(1)
    sys.exit(0)

if __name__ == '__main__':
    main()
