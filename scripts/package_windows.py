#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
C++桌面应用程序模板 Windows 打包工具
"""

import os
import sys
import subprocess
import argparse
import shutil
import zipfile
from pathlib import Path
from datetime import datetime

class Colors:
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    CYAN = '\033[0;36m'
    NC = '\033[0m'

    @classmethod
    def disable(cls):
        cls.RED = ''
        cls.GREEN = ''
        cls.YELLOW = ''
        cls.BLUE = ''
        cls.CYAN = ''
        cls.NC = ''

if os.name == 'nt':
    Colors.disable()

def find_qt_path():
    """查找 Qt 安装路径"""
    qt_path = os.environ.get('Qt6_DIR')
    if qt_path:
        for i in range(1, 5):
            test_path = Path(qt_path)
            for _ in range(i):
                test_path = test_path.parent
            if test_path.exists() and (test_path / 'bin' / 'windeployqt.exe').exists():
                return test_path
    
    default_paths = [
        r'C:\Qt\6.10.2\msvc2022_64',
        r'C:\Qt\6.9.0\msvc2022_64',
        r'C:\Qt\6.8.1\msvc2022_64',
        r'C:\Qt\6.8.0\msvc2022_64',
        r'C:\Qt\6.7.2\msvc2019_64',
        r'C:\Qt\6.7.1\msvc2019_64',
        r'C:\Qt\6.6.3\msvc2019_64',
    ]
    
    for path in default_paths:
        qt_path = Path(path)
        if qt_path.exists() and (qt_path / 'bin' / 'windeployqt.exe').exists():
            return qt_path
    
    return None

def get_timestamp():
    """获取时间戳"""
    now = datetime.now()
    return now.strftime('%Y%m%d_%H%M%S')

def create_zip_archive(source_dir, output_file):
    """创建 ZIP 归档文件"""
    with zipfile.ZipFile(output_file, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for root, dirs, files in os.walk(source_dir):
            for file in files:
                file_path = os.path.join(root, file)
                arcname = os.path.relpath(file_path, source_dir)
                zipf.write(file_path, arcname)

def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='C++桌面应用程序模板 Windows 打包工具',
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        '--config', '-c',
        choices=['Debug', 'Release'],
        default='Debug',
        help='构建配置 (默认: Debug)'
    )
    parser.add_argument(
        '--qt-path', '-q',
        help='Qt 安装路径'
    )
    parser.add_argument(
        '--output', '-o',
        help='输出目录 (默认: dist)'
    )
    parser.add_argument(
        '--app-name', '-n',
        default='CppAppTemplate',
        help='应用程序名称 (默认: CppAppTemplate)'
    )
    
    args = parser.parse_args()
    
    print(f"{Colors.BLUE}========================================{Colors.NC}")
    print(f"{Colors.BLUE}  C++桌面应用程序模板 Windows 打包工具{Colors.NC}")
    print(f"{Colors.BLUE}========================================{Colors.NC}")
    print()
    
    script_path = Path(__file__)
    project_root = script_path.parent.parent
    
    build_config = args.config
    app_name = args.app_name
    
    build_dir = project_root / 'build'
    exe_file = build_dir / 'src' / 'app' / build_config / f'{app_name}.exe'
    
    print(f"{Colors.YELLOW}构建配置:{Colors.NC} {build_config}")
    print(f"{Colors.YELLOW}可执行文件路径:{Colors.NC} {exe_file}")
    
    if not exe_file.exists():
        print(f"{Colors.RED}错误: 可执行文件未找到: {exe_file}{Colors.NC}")
        print()
        print(f"{Colors.YELLOW}请先构建项目:{Colors.NC}")
        print(f"  python scripts/build.py --build-type {build_config}")
        return 1
    
    print(f"{Colors.GREEN}✅ 找到可执行文件: {exe_file}{Colors.NC}")
    
    qt_path = args.qt_path
    if qt_path:
        qt_path = Path(qt_path)
    else:
        qt_path = find_qt_path()
    
    valid_qt_path = None
    if qt_path and qt_path.exists():
        windeployqt = qt_path / 'bin' / 'windeployqt.exe'
        if windeployqt.exists():
            valid_qt_path = qt_path
        else:
            for i in range(1, 5):
                test_path = qt_path
                for _ in range(i):
                    test_path = test_path.parent
                if test_path.exists() and (test_path / 'bin' / 'windeployqt.exe').exists():
                    valid_qt_path = test_path
                    break
    
    if not valid_qt_path:
        print(f"{Colors.RED}错误: 未找到有效的 Qt 安装{Colors.NC}")
        print()
        print(f"{Colors.YELLOW}请使用以下命令指定正确的 Qt 路径:{Colors.NC}")
        print(f"  python scripts/package_windows.py -q 'C:/Qt/6.x.x/msvc2022_64'")
        return 1
    
    qt_path = valid_qt_path
    print(f"{Colors.GREEN}✅ Qt 路径: {qt_path}{Colors.NC}")
    
    windeployqt = qt_path / 'bin' / 'windeployqt.exe'
    if not windeployqt.exists():
        print(f"{Colors.RED}错误: windeployqt 未找到: {windeployqt}{Colors.NC}")
        return 1
    
    print(f"{Colors.GREEN}✅ windeployqt: {windeployqt}{Colors.NC}")
    
    os.environ['PATH'] = f"{qt_path / 'bin'};{os.environ.get('PATH', '')}"
    
    output_dir = args.output
    if output_dir:
        output_dir = Path(output_dir)
    else:
        output_dir = project_root / 'dist'
    
    if not output_dir.exists():
        output_dir.mkdir(parents=True)
    
    timestamp = get_timestamp()
    package_dir = output_dir / f"{app_name}_{build_config}_{timestamp}"
    
    print(f"{Colors.YELLOW}创建打包目录: {package_dir}{Colors.NC}")
    if not package_dir.exists():
        package_dir.mkdir(parents=True)
    
    print(f"{Colors.YELLOW}复制可执行文件...{Colors.NC}")
    try:
        shutil.copy2(exe_file, package_dir)
        print(f"{Colors.GREEN}✅ 可执行文件复制成功{Colors.NC}")
    except Exception as e:
        print(f"{Colors.RED}错误: 复制可执行文件失败: {e}{Colors.NC}")
        return 1
    
    print(f"{Colors.YELLOW}收集 Qt 依赖...{Colors.NC}")
    windeployqt_args = [
        str(windeployqt),
        '--no-translations',
        '--no-system-d3d-compiler',
        '--no-opengl-sw',
        str(package_dir / f'{app_name}.exe')
    ]
    
    if build_config == 'Release':
        windeployqt_args.insert(1, '--release')
    else:
        windeployqt_args.insert(1, '--debug')
    
    try:
        result = subprocess.run(
            windeployqt_args,
            cwd=package_dir,
            capture_output=True,
            text=True,
            encoding='utf-8',
            errors='replace'
        )
        if result.returncode != 0:
            print(f"{Colors.YELLOW}警告: windeployqt 可能有问题，继续执行...{Colors.NC}")
        else:
            print(f"{Colors.GREEN}✅ Qt 依赖收集成功{Colors.NC}")
    except Exception as e:
        print(f"{Colors.YELLOW}警告: 运行 windeployqt 失败: {e}，继续执行...{Colors.NC}")
    
    print(f"{Colors.YELLOW}创建启动脚本...{Colors.NC}")
    launch_bat = package_dir / 'Launch.bat'
    try:
        with open(launch_bat, 'w', encoding='utf-8') as f:
            f.write('@echo off\n')
            f.write(f'start "" "{app_name}.exe"\n')
        print(f"{Colors.GREEN}✅ 启动脚本创建成功{Colors.NC}")
    except Exception as e:
        print(f"{Colors.RED}错误: 创建启动脚本失败: {e}{Colors.NC}")
        return 1
    
    print(f"{Colors.YELLOW}创建 ZIP 包...{Colors.NC}")
    zip_file = output_dir / f"{app_name}_{build_config}_{timestamp}.zip"
    try:
        create_zip_archive(package_dir, zip_file)
        print(f"{Colors.GREEN}✅ ZIP 包创建成功{Colors.NC}")
    except Exception as e:
        print(f"{Colors.RED}错误: 创建 ZIP 包失败: {e}{Colors.NC}")
        return 1
    
    zip_size = zip_file.stat().st_size
    zip_size_mb = zip_size / (1024 * 1024)
    
    print()
    print(f"{Colors.BLUE}========================================{Colors.NC}")
    print(f"{Colors.BLUE}  打包完成！{Colors.NC}")
    print(f"{Colors.BLUE}========================================{Colors.NC}")
    print()
    print(f"{Colors.YELLOW}打包详情:{Colors.NC}")
    print(f"  构建配置: {build_config}")
    print(f"  打包目录: {package_dir}")
    print(f"  ZIP 文件: {zip_file}")
    print(f"  文件大小: {zip_size_mb:.2f} MB")
    print()
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
