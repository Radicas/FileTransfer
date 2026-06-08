"""
@file firewall_manager.py
@brief 防火墙管理脚本
@date 2026-06-01
@version 1.0

@details 自动检测和配置防火墙规则，
支持Windows和macOS系统。
"""

import os
import sys
import platform
import subprocess
import shutil


class FirewallManager:
    """防火墙管理类"""
    
    UDP_PORT = 38888
    TCP_PORT = 38889
    APP_NAME = "FileTransfer"
    
    def __init__(self):
        self.system = platform.system()
        self.is_admin = self._check_admin()
    
    def _check_admin(self):
        """检查是否有管理员权限"""
        try:
            if self.system == "Windows":
                import ctypes
                return ctypes.windll.shell32.IsUserAnAdmin() != 0
            else:
                return os.geteuid() == 0
        except:
            return False
    
    def _run_command(self, cmd, check=True):
        """运行命令"""
        try:
            result = subprocess.run(
                cmd,
                shell=True,
                capture_output=True,
                text=True,
                check=check
            )
            return result.returncode == 0, result.stdout, result.stderr
        except subprocess.CalledProcessError as e:
            return False, "", str(e)
        except Exception as e:
            return False, "", str(e)
    
    def check_port_open(self, port, protocol="tcp"):
        """检查端口是否已开放"""
        if self.system == "Windows":
            cmd = f'netsh advfirewall firewall show rule name="{self.APP_NAME} {protocol.upper()} In"'
            success, stdout, stderr = self._run_command(cmd, check=False)
            return port in stdout
        else:
            cmd = f"pfctl -s rules 2>/dev/null | grep 'port {port}'"
            success, stdout, stderr = self._run_command(cmd, check=False)
            if success and stdout.strip() != "":
                return True
            pf_conf = "/etc/pf.conf.filetransfer"
            if os.path.exists(pf_conf):
                return True
            return False
    
    def add_rules_windows(self):
        """添加Windows防火墙规则"""
        if not self.is_admin:
            print("需要管理员权限才能配置防火墙")
            return False
        
        print("正在添加Windows防火墙规则...")
        
        rules = [
            (f'netsh advfirewall firewall add rule name="{self.APP_NAME} UDP In (设备发现)" dir=in action=allow protocol=UDP localport={self.UDP_PORT}', "UDP入站"),
            (f'netsh advfirewall firewall add rule name="{self.APP_NAME} UDP Out (设备发现)" dir=out action=allow protocol=UDP localport={self.UDP_PORT}', "UDP出站"),
            (f'netsh advfirewall firewall add rule name="{self.APP_NAME} TCP In (文件传输)" dir=in action=allow protocol=TCP localport={self.TCP_PORT}', "TCP入站"),
            (f'netsh advfirewall firewall add rule name="{self.APP_NAME} TCP Out (文件传输)" dir=out action=allow protocol=TCP localport={self.TCP_PORT}', "TCP出站"),
        ]
        
        all_success = True
        for cmd, desc in rules:
            success, stdout, stderr = self._run_command(cmd)
            if success:
                print(f"  ✓ {desc}规则添加成功")
            else:
                print(f"  ✗ {desc}规则添加失败: {stderr}")
                all_success = False
        
        return all_success
    
    def add_rules_macos(self):
        """添加macOS防火墙规则"""
        if not self.is_admin:
            print("需要root权限才能配置防火墙")
            return False
        
        print("正在添加macOS防火墙规则...")
        
        pf_conf = "/etc/pf.conf.filetransfer"
        
        try:
            with open(pf_conf, 'w') as f:
                f.write(f"# {self.APP_NAME} 端口规则\n")
                f.write(f"# UDP {self.UDP_PORT} - 设备发现广播\n")
                f.write(f"pass in proto udp from any to any port {self.UDP_PORT}\n")
                f.write(f"pass out proto udp from any to any port {self.UDP_PORT}\n")
                f.write(f"# TCP {self.TCP_PORT} - 文件传输服务\n")
                f.write(f"pass in proto tcp from any to any port {self.TCP_PORT}\n")
                f.write(f"pass out proto tcp from any to any port {self.TCP_PORT}\n")
            
            print(f"  ✓ 配置文件已创建: {pf_conf}")
            
            success, stdout, stderr = self._run_command(f"pfctl -ef {pf_conf}")
            if success:
                print("  ✓ pf规则已加载")
                return True
            else:
                print(f"  ✗ pf规则加载失败: {stderr}")
                return False
                
        except Exception as e:
            print(f"  ✗ 配置失败: {e}")
            return False
    
    def remove_rules_windows(self):
        """删除Windows防火墙规则"""
        if not self.is_admin:
            print("需要管理员权限才能删除防火墙规则")
            return False
        
        print("正在删除Windows防火墙规则...")
        
        rules = [
            f'netsh advfirewall firewall delete rule name="{self.APP_NAME} UDP In (设备发现)"',
            f'netsh advfirewall firewall delete rule name="{self.APP_NAME} UDP Out (设备发现)"',
            f'netsh advfirewall firewall delete rule name="{self.APP_NAME} TCP In (文件传输)"',
            f'netsh advfirewall firewall delete rule name="{self.APP_NAME} TCP Out (文件传输)"',
        ]
        
        for cmd in rules:
            self._run_command(cmd, check=False)
        
        print("  ✓ 规则已删除")
        return True
    
    def remove_rules_macos(self):
        """删除macOS防火墙规则"""
        if not self.is_admin:
            print("需要root权限才能删除防火墙规则")
            return False
        
        print("正在删除macOS防火墙规则...")
        
        pf_conf = "/etc/pf.conf.filetransfer"
        
        if os.path.exists(pf_conf):
            os.remove(pf_conf)
            print(f"  ✓ 配置文件已删除: {pf_conf}")
        
        self._run_command("pfctl -F all", check=False)
        print("  ✓ pf规则已重置")
        
        return True
    
    def add_rules(self):
        """添加防火墙规则"""
        if self.system == "Windows":
            return self.add_rules_windows()
        elif self.system == "Darwin":
            return self.add_rules_macos()
        else:
            print(f"不支持的系统: {self.system}")
            return False
    
    def remove_rules(self):
        """删除防火墙规则"""
        if self.system == "Windows":
            return self.remove_rules_windows()
        elif self.system == "Darwin":
            return self.remove_rules_macos()
        else:
            print(f"不支持的系统: {self.system}")
            return False
    
    def check_and_configure(self):
        """检查并自动配置防火墙"""
        print(f"系统: {self.system}")
        print(f"管理员权限: {'有' if self.is_admin else '无'}")
        print()
        
        udp_open = self.check_port_open(self.UDP_PORT, "udp")
        tcp_open = self.check_port_open(self.TCP_PORT, "tcp")
        
        print(f"UDP {self.UDP_PORT}: {'已开放' if udp_open else '未开放'}")
        print(f"TCP {self.TCP_PORT}: {'已开放' if tcp_open else '未开放'}")
        print()
        
        if udp_open and tcp_open:
            print("防火墙规则已配置，无需操作")
            return True
        
        if not self.is_admin:
            print("需要管理员/root权限才能配置防火墙")
            print()
            if self.system == "Windows":
                print("请以管理员身份重新运行此脚本")
                print("或手动运行: scripts\\add_firewall_rules.bat")
            else:
                print("请使用 sudo 运行此脚本")
                print("或手动运行: sudo bash scripts/add_firewall_rules_macos.sh")
            return False
        
        print("正在自动配置防火墙规则...")
        return self.add_rules()


def main():
    """主函数"""
    print("=" * 50)
    print("  FileTransfer 防火墙配置工具")
    print("=" * 50)
    print()
    
    manager = FirewallManager()
    
    if len(sys.argv) > 1:
        action = sys.argv[1]
        if action == "add":
            manager.add_rules()
        elif action == "remove":
            manager.remove_rules()
        elif action == "check":
            manager.check_and_configure()
        else:
            print(f"未知操作: {action}")
            print("用法: python firewall_manager.py [add|remove|check]")
    else:
        manager.check_and_configure()
    
    print()
    print("=" * 50)


if __name__ == "__main__":
    main()