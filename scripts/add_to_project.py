#!/usr/bin/env python3
"""
将 upper_computer.c 添加到 Keil MDK 项目
"""

import re
import sys
import os

def add_file_to_project(project_path, file_path, group_name="Application/User/Core"):
    """将文件添加到 Keil 项目"""

    # 读取项目文件
    with open(project_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # 检查文件是否已存在
    if file_path in content:
        print(f"[INFO] 文件已存在: {file_path}")
        return True

    # 查找目标组
    group_pattern = rf'<Group>\s*<GroupName>{re.escape(group_name)}</GroupName>(.*?)</Group>'
    group_match = re.search(group_pattern, content, re.DOTALL)

    if not group_match:
        print(f"[ERROR] 未找到组: {group_name}")
        return False

    # 在组中添加文件
    group_content = group_match.group(1)

    # 查找最后一个文件条目
    file_pattern = r'<File>\s*<FileName>(.*?)</FileName>.*?</File>'
    files = list(re.finditer(file_pattern, group_content, re.DOTALL))

    if not files:
        print(f"[ERROR] 组中没有文件")
        return False

    last_file = files[-1]
    last_file_end = last_file.end()

    # 构建新文件条目
    file_name = os.path.basename(file_path)
    new_file = f'''
    <File>
      <FileName>{file_name}</FileName>
      <FileType>1</FileType>
      <FilePath>{file_path}</FilePath>
    </File>'''

    # 插入新文件
    new_content = content[:group_match.start() + last_file_end] + new_file + content[group_match.start() + last_file_end:]

    # 写入项目文件
    with open(project_path, 'w', encoding='utf-8') as f:
        f.write(new_content)

    print(f"[OK] 已添加文件: {file_path}")
    return True

def main():
    """主函数"""

    project_path = r"D:\stm32 _project_hal\scope-siggen\MDK-ARM\scope-siggen.uvprojx"
    file_path = r"..\..\Core\Src\upper_computer.c"
    group_name = "Application/User/Core"

    print("=== 添加文件到项目 ===")
    print(f"项目: {project_path}")
    print(f"文件: {file_path}")
    print(f"组: {group_name}")
    print()

    success = add_file_to_project(project_path, file_path, group_name)

    if success:
        print("\n[完成] 请重新编译项目")
    else:
        print("\n[失败] 添加文件失败")

    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
