#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
C++桌面应用程序模板 - 构建工具 (Python 版本)
支持 Windows (MSVC, MinGW) 和 macOS (Clang)
"""

import os
import sys
import subprocess
import argparse
import shutil
import platform
from pathlib import Path

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

def get_default_compiler():
    """根据操作系统获取默认编译器"""
    system = platform.system().lower()
    if system == 'darwin':
        return 'clang'
    elif system == 'windows':
        return 'msvc'
    elif system == 'linux':
        return 'gcc'
    return 'clang'

def check_compiler_available(compiler):
    """检查编译器是否可用"""
    if compiler == 'msvc':
        try:
            result = subprocess.run(
                ['cl.exe'],
                capture_output=True,
                text=True,
                encoding='utf-8',
                errors='replace'
            )
            output = (result.stdout or '') + (result.stderr or '')
            if 'Microsoft (R) C/C++' in output:
                return True
        except FileNotFoundError:
            pass
        
        default_paths = [
            r'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC',
            r'C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Tools\MSVC',
            r'C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Tools\MSVC',
            r'C:\Program Files\Microsoft Visual Studio\2026\Community\VC\Tools\MSVC',
            r'C:\Program Files\Microsoft Visual Studio\2026\Professional\VC\Tools\MSVC',
            r'C:\Program Files\Microsoft Visual Studio\2026\Enterprise\VC\Tools\MSVC',
            r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC',
            r'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC',
            r'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC'
        ]
        
        for base_path in default_paths:
            if os.path.exists(base_path):
                try:
                    versions = os.listdir(base_path)
                    versions.sort(reverse=True)
                    for version in versions:
                        cl_path = os.path.join(base_path, version, 'bin', 'Hostx64', 'x64', 'cl.exe')
                        if os.path.exists(cl_path):
                            result = subprocess.run(
                                [cl_path],
                                capture_output=True,
                                text=True,
                                encoding='utf-8',
                                errors='replace'
                            )
                            output = (result.stdout or '') + (result.stderr or '')
                            if 'Microsoft (R) C/C++' in output:
                                return True
                except Exception:
                    pass
        return False
    
    elif compiler == 'mingw':
        try:
            result = subprocess.run(
                ['g++', '--version'],
                capture_output=True,
                text=True,
                encoding='utf-8',
                errors='replace',
                check=True
            )
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            pass
        
        default_paths = [
            r'C:\MinGW\bin',
            r'C:\msys64\mingw64\bin',
            r'C:\Program Files\MinGW\bin'
        ]
        
        for path in default_paths:
            gpp_path = os.path.join(path, 'g++.exe')
            if os.path.exists(gpp_path):
                try:
                    result = subprocess.run(
                        [gpp_path, '--version'],
                        capture_output=True,
                        text=True,
                        encoding='utf-8',
                        errors='replace',
                        check=True
                    )
                    return True
                except (subprocess.CalledProcessError, FileNotFoundError):
                    pass
        return False
    
    elif compiler == 'clang':
        try:
            result = subprocess.run(
                ['clang++', '--version'],
                capture_output=True,
                text=True,
                encoding='utf-8',
                errors='replace',
                check=True
            )
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            pass
        return False
    
    elif compiler == 'gcc':
        try:
            result = subprocess.run(
                ['g++', '--version'],
                capture_output=True,
                text=True,
                encoding='utf-8',
                errors='replace',
                check=True
            )
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            pass
        return False
    
    return False

def find_msvc_path():
    """查找 MSVC 编译器路径"""
    default_paths = [
        r'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC',
        r'C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Tools\MSVC',
        r'C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Tools\MSVC',
        r'C:\Program Files\Microsoft Visual Studio\2026\Community\VC\Tools\MSVC',
        r'C:\Program Files\Microsoft Visual Studio\2026\Professional\VC\Tools\MSVC',
        r'C:\Program Files\Microsoft Visual Studio\2026\Enterprise\VC\Tools\MSVC',
        r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC',
        r'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC',
        r'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC'
    ]
    
    for base_path in default_paths:
        if os.path.exists(base_path):
            try:
                versions = os.listdir(base_path)
                versions.sort(reverse=True)
                for version in versions:
                    cl_path = os.path.join(base_path, version, 'bin', 'Hostx64', 'x64', 'cl.exe')
                    if os.path.exists(cl_path):
                        return os.path.dirname(cl_path)
            except Exception:
                pass
    return None

def find_mingw_path():
    """查找 MingW 编译器路径"""
    default_paths = [
        r'C:\MinGW\bin',
        r'C:\msys64\mingw64\bin',
        r'C:\Program Files\MinGW\bin'
    ]
    
    for path in default_paths:
        gpp_path = os.path.join(path, 'g++.exe')
        if os.path.exists(gpp_path):
            return path
    return None

def find_cmake_path():
    """查找 CMake 路径"""
    system = platform.system().lower()
    
    if system == 'windows':
        default_paths = [
            r'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin',
            r'C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin',
            r'C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin',
            r'C:\Program Files\Microsoft Visual Studio\2026\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin',
            r'C:\Program Files\Microsoft Visual Studio\2026\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin',
            r'C:\Program Files\Microsoft Visual Studio\2026\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin',
            r'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin',
            r'C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin',
            r'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin',
            r'C:\Program Files\CMake\bin',
            r'C:\cmake\bin'
        ]
        
        for path in default_paths:
            cmake_path = os.path.join(path, 'cmake.exe')
            if os.path.exists(cmake_path):
                return path
    elif system == 'darwin':
        default_paths = [
            '/usr/local/bin',
            '/opt/homebrew/bin',
            '/Applications/CMake.app/Contents/bin'
        ]
        
        for path in default_paths:
            cmake_path = os.path.join(path, 'cmake')
            if os.path.exists(cmake_path):
                return path
    elif system == 'linux':
        default_paths = [
            '/usr/bin',
            '/usr/local/bin',
            '/opt/cmake/bin'
        ]
        
        for path in default_paths:
            cmake_path = os.path.join(path, 'cmake')
            if os.path.exists(cmake_path):
                return path
    
    return None

def get_cmake_generator(compiler):
    """根据编译器获取 CMake 生成器"""
    system = platform.system().lower()
    if compiler == 'msvc':
        vs_versions = [
            ('18', 'Visual Studio 18 2026'),
            ('2026', 'Visual Studio 18 2026'),
            ('17', 'Visual Studio 17 2022'),
            ('2022', 'Visual Studio 17 2022'),
            ('16', 'Visual Studio 16 2019'),
            ('2019', 'Visual Studio 16 2019'),
            ('15', 'Visual Studio 15 2017'),
            ('2017', 'Visual Studio 15 2017'),
        ]
        
        for vs_num, generator in vs_versions:
            vs_paths = [
                f'C:\\Program Files\\Microsoft Visual Studio\\{vs_num}\\Community',
                f'C:\\Program Files\\Microsoft Visual Studio\\{vs_num}\\Professional',
                f'C:\\Program Files\\Microsoft Visual Studio\\{vs_num}\\Enterprise',
            ]
            for path in vs_paths:
                if os.path.exists(path):
                    return generator
        
        return 'Visual Studio 17 2022'
    elif compiler == 'mingw':
        return 'MinGW Makefiles'
    elif compiler == 'clang':
        if system == 'darwin':
            return 'Unix Makefiles'
        return 'Unix Makefiles'
    elif compiler == 'gcc':
        return 'Unix Makefiles'
    return ''

def main():
    """主函数"""
    default_compiler = get_default_compiler()
    
    parser = argparse.ArgumentParser(
        description='C++桌面应用程序模板构建工具',
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        '--compiler', '-c',
        choices=['msvc', 'mingw', 'clang', 'gcc'],
        default=default_compiler,
        help=f'选择编译器 (默认: {default_compiler})'
    )
    parser.add_argument(
        '--build-type', '-b',
        choices=['Debug', 'Release'],
        default='Debug',
        help='构建类型 (默认: Debug)'
    )
    parser.add_argument(
        '--clean', '-C',
        action='store_true',
        help='清理构建目录'
    )
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='显示详细信息'
    )
    
    args = parser.parse_args()
    
    print(f"{Colors.BLUE}=== C++桌面应用程序模板构建工具 ==={Colors.NC}")
    
    script_path = Path(__file__).resolve()
    project_root = script_path.parent.parent
    build_dir = project_root / 'build'
    
    print(f"{Colors.YELLOW}项目根目录:{Colors.NC} {project_root}")
    print(f"{Colors.YELLOW}构建目录:{Colors.NC} {build_dir}")
    
    if args.clean:
        print(f"{Colors.YELLOW}清理构建目录...{Colors.NC}")
        if build_dir.exists():
            try:
                shutil.rmtree(build_dir)
                print(f"{Colors.GREEN}✅ 构建目录已清理{Colors.NC}")
            except Exception as e:
                print(f"{Colors.RED}错误: 清理构建目录失败: {e}{Colors.NC}")
                return 1
        else:
            print(f"{Colors.YELLOW}构建目录不存在，无需清理{Colors.NC}")
        return 0
    
    if not check_compiler_available(args.compiler):
        print(f"{Colors.RED}错误: {args.compiler.upper()} 编译器未找到{Colors.NC}")
        if args.compiler == 'msvc':
            print(f"{Colors.YELLOW}请确保已安装 Visual Studio 并在正确的环境中运行此脚本{Colors.NC}")
        elif args.compiler == 'mingw':
            print(f"{Colors.YELLOW}请确保已安装 MingW 并将其添加到系统 PATH 中{Colors.NC}")
        elif args.compiler == 'clang':
            print(f"{Colors.YELLOW}请确保已安装 Xcode Command Line Tools (macOS) 或 Clang{Colors.NC}")
            if platform.system().lower() == 'darwin':
                print(f"{Colors.YELLOW}可以运行: xcode-select --install{Colors.NC}")
        elif args.compiler == 'gcc':
            print(f"{Colors.YELLOW}请确保已安装 GCC 并将其添加到系统 PATH 中{Colors.NC}")
        return 1
    
    if args.compiler == 'msvc':
        msvc_path = find_msvc_path()
        if msvc_path:
            if msvc_path not in os.environ.get('PATH', ''):
                os.environ['PATH'] = f"{msvc_path};{os.environ.get('PATH', '')}"
                print(f"{Colors.GREEN}已将 MSVC 路径添加到环境变量: {msvc_path}{Colors.NC}")
    elif args.compiler == 'mingw':
        mingw_path = find_mingw_path()
        if mingw_path:
            if mingw_path not in os.environ.get('PATH', ''):
                os.environ['PATH'] = f"{mingw_path};{os.environ.get('PATH', '')}"
                print(f"{Colors.GREEN}已将 MingW 路径添加到环境变量: {mingw_path}{Colors.NC}")
    
    cmake_path = find_cmake_path()
    if cmake_path:
        if cmake_path not in os.environ.get('PATH', ''):
            os.environ['PATH'] = f"{cmake_path};{os.environ.get('PATH', '')}"
            print(f"{Colors.GREEN}已将 CMake 路径添加到环境变量: {cmake_path}{Colors.NC}")
    else:
        print(f"{Colors.RED}错误: CMake 未找到{Colors.NC}")
        print(f"{Colors.YELLOW}请确保已安装 CMake 并将其添加到系统 PATH 中{Colors.NC}")
        return 1
    
    cmake_generator = get_cmake_generator(args.compiler)
    if not cmake_generator:
        print(f"{Colors.RED}错误: 不支持的编译器: {args.compiler}{Colors.NC}")
        return 1
    
    print(f"{Colors.CYAN}使用编译器:{Colors.NC} {args.compiler.upper()}")
    print(f"{Colors.CYAN}CMake 生成器:{Colors.NC} {cmake_generator}")
    print(f"{Colors.CYAN}构建类型:{Colors.NC} {args.build_type}")
    
    if build_dir.exists():
        print(f"{Colors.YELLOW}构建目录已存在，尝试增量编译...{Colors.NC}")
        try:
            test_build_cmd = [
                'cmake',
                '--build', '.',
                '--config', args.build_type,
                '--parallel'
            ]
            print(f"\n{Colors.YELLOW}开始增量构建...{Colors.NC}")
            result = subprocess.run(
                test_build_cmd,
                cwd=build_dir,
                capture_output=False,
                text=True,
                encoding='utf-8',
                errors='replace',
                check=True
            )
            print(f"\n{Colors.GREEN}✅ 增量编译成功！{Colors.NC}")
            return 0
        except subprocess.CalledProcessError:
            print(f"{Colors.YELLOW}增量编译失败，删除构建目录并重新编译...{Colors.NC}")
            try:
                shutil.rmtree(build_dir)
                print(f"{Colors.GREEN}✅ 构建目录已删除{Colors.NC}")
            except Exception as e:
                print(f"{Colors.RED}错误: 删除构建目录失败: {e}{Colors.NC}")
                return 1
    
    print(f"{Colors.YELLOW}创建构建目录...{Colors.NC}")
    try:
        build_dir.mkdir(parents=True)
        print(f"{Colors.GREEN}✅ 构建目录已创建{Colors.NC}")
    except Exception as e:
        print(f"{Colors.RED}错误: 创建构建目录失败: {e}{Colors.NC}")
        return 1
    
    print(f"\n{Colors.YELLOW}执行 CMake 配置...{Colors.NC}")
    cmake_config_cmd = [
        'cmake',
        '-G', cmake_generator,
        f'-DCMAKE_BUILD_TYPE={args.build_type}',
        str(project_root)
    ]
    
    if args.verbose:
        print(f"{Colors.CYAN}执行命令:{Colors.NC} {' '.join(cmake_config_cmd)}")
    
    try:
        result = subprocess.run(
            cmake_config_cmd,
            cwd=build_dir,
            capture_output=not args.verbose,
            text=True,
            encoding='utf-8',
            errors='replace',
            check=True
        )
        if args.verbose:
            print(result.stdout)
        print(f"{Colors.GREEN}✅ CMake 配置成功{Colors.NC}")
    except subprocess.CalledProcessError as e:
        print(f"{Colors.RED}错误: CMake 配置失败{Colors.NC}")
        if not args.verbose:
            if hasattr(e, 'stdout') and e.stdout:
                print(e.stdout)
            if hasattr(e, 'stderr') and e.stderr:
                print(e.stderr)
        return 1
    
    print(f"\n{Colors.YELLOW}开始构建项目...{Colors.NC}")
    cmake_build_cmd = [
        'cmake',
        '--build', '.',
        '--config', args.build_type,
        '--parallel'
    ]
    
    if args.verbose:
        print(f"{Colors.CYAN}执行命令:{Colors.NC} {' '.join(cmake_build_cmd)}")
    
    try:
        result = subprocess.run(
            cmake_build_cmd,
            cwd=build_dir,
            capture_output=False,
            text=True,
            encoding='utf-8',
            errors='replace',
            check=True
        )
        print(f"\n{Colors.GREEN}✅ 项目构建成功！{Colors.NC}")
    except subprocess.CalledProcessError as e:
        print(f"{Colors.RED}错误: 项目构建失败{Colors.NC}")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
