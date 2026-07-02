#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
版本管理脚本

功能：
1. 从Git获取版本信息
2. 自动更新version.h文件
3. 支持自动提交版本更新
"""

import os
import re
import subprocess
import sys
from datetime import datetime

class VersionManager:
    """版本管理器"""

    def __init__(self, project_dir='.'):
        self.project_dir = project_dir
        self.version_header = os.path.join(project_dir, 'Core', 'Inc', 'version.h')

        # Git信息
        self.branch = "unknown"
        self.commit = "unknown"
        self.commit_full = "unknown"
        self.tag = None
        self.dirty = 0
        self.commit_count = 0

    def get_git_info(self):
        """获取Git信息"""
        try:
            # 保存当前目录
            original_dir = os.getcwd()
            os.chdir(self.project_dir)

            # 当前分支
            self.branch = subprocess.check_output(
                ['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
                stderr=subprocess.DEVNULL
            ).decode().strip()

            # 短提交哈希
            self.commit = subprocess.check_output(
                ['git', 'rev-parse', '--short', 'HEAD'],
                stderr=subprocess.DEVNULL
            ).decode().strip()

            # 完整提交哈希
            self.commit_full = subprocess.check_output(
                ['git', 'rev-parse', 'HEAD'],
                stderr=subprocess.DEVNULL
            ).decode().strip()

            # 当前标签
            try:
                self.tag = subprocess.check_output(
                    ['git', 'describe', '--exact-match', '--tags', 'HEAD'],
                    stderr=subprocess.DEVNULL
                ).decode().strip()
            except subprocess.CalledProcessError:
                self.tag = None

            # 未提交更改
            status = subprocess.check_output(
                ['git', 'status', '--porcelain'],
                stderr=subprocess.DEVNULL
            ).decode().strip()
            self.dirty = 1 if status else 0

            # 提交总数
            self.commit_count = int(subprocess.check_output(
                ['git', 'rev-list', '--count', 'HEAD'],
                stderr=subprocess.DEVNULL
            ).decode().strip())

            # 恢复原目录
            os.chdir(original_dir)

            return True

        except Exception as e:
            print(f"Git error: {e}")
            # 恢复原目录
            os.chdir(original_dir)
            return False

    def get_version_from_git(self):
        """从Git标签获取版本号"""
        if self.tag and self.tag.startswith('v'):
            # 解析标签 "v1.0.0" -> (1, 0, 0)
            version_str = self.tag[1:]
            parts = version_str.split('.')
            if len(parts) >= 3:
                try:
                    return int(parts[0]), int(parts[1]), int(parts[2])
                except ValueError:
                    pass
        return None

    def get_build_number(self):
        """获取构建号（基于提交总数）"""
        return self.commit_count

    def update_version_header(self, config='Debug'):
        """更新version.h文件"""
        # 获取Git信息
        if not self.get_git_info():
            print("Warning: Failed to get Git info, using defaults")

        # 确定版本号
        version = self.get_version_from_git()
        if version:
            major, minor, patch = version
        else:
            # 使用默认版本
            major, minor, patch = VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH

        build = self.get_build_number()
        build_date = datetime.now().strftime("%b %d %Y")
        build_time = datetime.now().strftime("%H:%M:%S")

        # 读取version.h
        try:
            with open(self.version_header, 'r') as f:
                content = f.read()
        except FileNotFoundError:
            print(f"Error: {self.version_header} not found")
            return False

        # 更新版本号
        content = re.sub(
            r'#define VERSION_MAJOR\s+\d+',
            f'#define VERSION_MAJOR      {major}',
            content
        )
        content = re.sub(
            r'#define VERSION_MINOR\s+\d+',
            f'#define VERSION_MINOR      {minor}',
            content
        )
        content = re.sub(
            r'#define VERSION_PATCH\s+\d+',
            f'#define VERSION_PATCH      {patch}',
            content
        )
        content = re.sub(
            r'#define VERSION_BUILD\s+\d+',
            f'#define VERSION_BUILD      {build}',
            content
        )

        # 更新Git信息
        content = re.sub(
            r'#define GIT_COMMIT\s+"[^"]*"',
            f'#define GIT_COMMIT         "{self.commit}"',
            content
        )
        content = re.sub(
            r'#define GIT_BRANCH\s+"[^"]*"',
            f'#define GIT_BRANCH         "{self.branch}"',
            content
        )
        content = re.sub(
            r'#define GIT_DIRTY\s+\d+',
            f'#define GIT_DIRTY          {self.dirty}',
            content
        )

        # 写入文件
        try:
            with open(self.version_header, 'w') as f:
                f.write(content)
        except Exception as e:
            print(f"Error writing {self.version_header}: {e}")
            return False

        print(f"Version updated: {major}.{minor}.{patch}.{build}")
        print(f"Git: {self.branch}@{self.commit}, dirty={self.dirty}")

        return True

    def auto_commit(self, message=None):
        """自动提交版本更新"""
        if not self.dirty:
            print("No changes to commit")
            return True

        # 保存当前目录
        original_dir = os.getcwd()
        os.chdir(self.project_dir)

        # 检查version.h是否有更改
        try:
            status = subprocess.check_output(
                ['git', 'status', '--porcelain', 'Core/Inc/version.h'],
                stderr=subprocess.DEVNULL
            ).decode().strip()

            if 'Core/Inc/version.h' not in status:
                print("version.h not changed, skipping commit")
                os.chdir(original_dir)
                return True

            # 提交版本更新
            if message is None:
                message = f"chore: update version to {self.commit}"

            subprocess.run(['git', 'add', 'Core/Inc/version.h'], check=True)
            subprocess.run(['git', 'commit', '-m', message], check=True)
            print(f"Committed: {message}")

        except Exception as e:
            print(f"Commit failed: {e}")
            os.chdir(original_dir)
            return False

        # 恢复原目录
        os.chdir(original_dir)
        return True


def main():
    """主函数"""
    import argparse

    parser = argparse.ArgumentParser(description='Version Manager')
    parser.add_argument('--project', default='.', help='Project directory')
    parser.add_argument('--auto-commit', action='store_true',
                        help='Auto commit version update')
    parser.add_argument('--config', default='Debug',
                        help='Build configuration (Debug/Release)')
    parser.add_argument('--message', default=None,
                        help='Commit message')

    args = parser.parse_args()

    vm = VersionManager(args.project)

    # 更新版本头文件
    if not vm.update_version_header(args.config):
        print("Failed to update version header")
        return 1

    # 自动提交
    if args.auto_commit:
        if not vm.auto_commit(args.message):
            print("Failed to auto commit")
            return 1

    return 0


if __name__ == '__main__':
    # 默认版本号（用于正则匹配）
    VERSION_MAJOR = 1
    VERSION_MINOR = 0
    VERSION_PATCH = 0

    exit(main())
