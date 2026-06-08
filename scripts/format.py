#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
项目 - 代码格式化工具 (Python 版本)
使用 clang-format 自动格式化项目代码
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path

# 定义颜色输出
class Colors:
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    CYAN = '\033[0;36m'
    NC = '\033[0m'  # No Color

    @classmethod
    def disable(cls):
        """禁用颜色输出"""
        cls.RED = ''
        cls.GREEN = ''
        cls.YELLOW = ''
        cls.BLUE = ''
        cls.CYAN = ''
        cls.NC = ''

# 检查是否在 Windows 平台
if os.name == 'nt':
    Colors.disable()

def main():
    """主函数"""
    # 解析命令行参数
    parser = argparse.ArgumentParser(
        description='RedCGL 代码格式化工具',
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        '--dry-run', '-n',
        action='store_true',
        help='只显示会被格式化的文件，不实际修改'
    )
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='显示详细信息'
    )

    
    args = parser.parse_args()
    
    print(f"{Colors.BLUE}=== RedCGL 代码格式化工具 ==={Colors.NC}")
    
    # 项目根目录
    script_path = Path(__file__)
    project_root = script_path.parent.parent
    print(f"{Colors.YELLOW}项目根目录:{Colors.NC} {project_root}")
    
    # 检查 clang-format 是否存在
    try:
        result = subprocess.run(
            ['clang-format', '--version'],
            capture_output=True,
            text=True,
            check=True
        )
        clang_format_version = result.stdout.strip()
        print(f"{Colors.CYAN}使用工具:{Colors.NC} {clang_format_version}")
    except (subprocess.CalledProcessError, FileNotFoundError):
        print(f"{Colors.RED}错误: clang-format 未安装{Colors.NC}")
        print(f"{Colors.YELLOW}请先安装 clang-format:{Colors.NC}")
        print(f"  {Colors.CYAN}# macOS{Colors.NC}")
        print("  brew install clang-format")
        print(f"  {Colors.CYAN}# Ubuntu/Debian{Colors.NC}")
        print("  sudo apt-get install clang-format")
        print(f"  {Colors.CYAN}# CentOS/RHEL{Colors.NC}")
        print("  sudo yum install clang-tools-extra")
        print(f"  {Colors.CYAN}# Windows{Colors.NC}")
        print("  从 LLVM 官网下载并安装")
        return 1
    
    # 要格式化的目录
    directories = [
        project_root / "src",
        project_root / "inc",
        project_root / "demos",
        project_root / "test_googletest",
        project_root / "test_benchmark",
        project_root / "backend"
    ]
    
    # 支持的文件扩展名
    extensions = ["cpp", "c", "h", "hpp", "cc", "cxx"]
    
    # 需要排除的目录
    exclude_dirs = [
        "build",
        "install",
        "submodule",
        "libs",
        ".git",
        "CMakeFiles"
    ]
    
    # 统计变量
    total_files = 0
    formatted_files = 0
    skipped_files = 0
    
    print()
    if args.dry_run:
        print(f"{Colors.YELLOW}🔍 预览模式 - 显示会被格式化的文件{Colors.NC}")
    else:
        print(f"{Colors.YELLOW}🚀 开始格式化代码文件{Colors.NC}")
    
    # 遍历每个目录并格式化文件
    for directory in directories:
        if not directory.exists():
            if args.verbose:
                print(f"{Colors.YELLOW}跳过不存在的目录: {directory}{Colors.NC}")
            continue
        
        if args.verbose:
            print(f"{Colors.CYAN}扫描目录: {directory}{Colors.NC}")
        
        for ext in extensions:
            # 使用 glob 查找文件
            pattern = f"**/*.{ext}"
            for file_path in directory.glob(pattern):
                total_files += 1
                
                # 检查是否应该排除此文件
                should_exclude = False
                for exclude in exclude_dirs:
                    if exclude in str(file_path):
                        should_exclude = True
                        break
                
                if should_exclude:
                    skipped_files += 1
                    if args.verbose:
                        print(f"{Colors.YELLOW}跳过: {file_path}{Colors.NC}")
                    continue
                
                if args.dry_run:
                    print(f"{Colors.GREEN}预览: {file_path}{Colors.NC}")
                else:
                    if args.verbose:
                        print(f"{Colors.GREEN}格式化: {file_path}{Colors.NC}")
                    else:
                        print(f"{Colors.GREEN}✓{Colors.NC} {file_path.name}")
                    
                    # 执行格式化
                    try:
                        subprocess.run(
                            ['clang-format', '-i', str(file_path)],
                            capture_output=True,
                            check=True
                        )
                        formatted_files += 1
                    except subprocess.CalledProcessError:
                        print(f"{Colors.RED}格式化失败: {file_path}{Colors.NC}")
    
    print()
    print(f"{Colors.BLUE}=== 格式化统计 ==={Colors.NC}")
    print(f"{Colors.CYAN}总文件数:{Colors.NC} {total_files}")
    if args.dry_run:
        print(f"{Colors.GREEN}预览文件数:{Colors.NC} {total_files - skipped_files}")
    else:
        print(f"{Colors.GREEN}已格式化:{Colors.NC} {formatted_files}")
    print(f"{Colors.YELLOW}跳过文件数:{Colors.NC} {skipped_files}")
    
    if args.dry_run:
        print()
        print(f"{Colors.YELLOW}提示: 使用不带 --dry-run 参数的命令来实际执行格式化{Colors.NC}")
    else:
        print()
        print(f"{Colors.GREEN}✅ 代码格式化完成！{Colors.NC}")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
