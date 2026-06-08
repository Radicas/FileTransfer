# FileTransfer

<div align="center">

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/yourusername/FileTransfer)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS-lightgrey.svg)](https://github.com/yourusername/FileTransfer)
[![C++](https://img.shields.io/badge/C++-17-orange.svg)](https://isocpp.org/)
[![Qt](https://img.shields.io/badge/Qt-6.0%2B-brightgreen.svg)](https://www.qt.io/)

**跨平台局域网文件传输系统**

[功能特性](#功能特性) • [快速开始](#快速开始) • [使用说明](#使用说明) • [项目结构](#项目结构)

</div>

---

## 项目简介

**FileTransfer** 是一个跨平台的局域网文件传输系统，支持Windows和macOS系统。无需复杂配置，只需在两台电脑上运行程序，即可自动发现对方并进行文件传输。采用双面板设计，支持拖拽操作，让文件传输变得简单高效。

### 核心价值

- 🔍 **自动发现**：UDP广播自动发现局域网中的其他设备
- 📁 **双面板设计**：左侧本地文件，右侧远程文件，直观对比
- 🖱️ **拖拽传输**：支持拖拽文件进行上传/下载
- 📊 **进度显示**：实时显示传输进度、速度和状态
- 🔒 **安全可靠**：TCP分块传输，支持大文件传输
- 🎨 **多主题支持**：内置多种UI主题，支持自定义样式

## 功能特性

### 核心功能

- ✅ **设备发现**：UDP广播自动发现局域网设备，心跳检测保持连接
- ✅ **文件浏览**：浏览本地和远程文件系统，支持目录导航
- ✅ **文件传输**：TCP分块传输，支持上传和下载
- ✅ **拖拽操作**：拖拽文件进行传输，操作简单直观
- ✅ **进度监控**：实时显示传输进度、速度和剩余时间
- ✅ **任务管理**：支持取消传输、清除已完成任务

### 技术特性

- 🏗️ **分层架构**：清晰的分层设计，遵循SOLID原则
- 🔌 **模块化设计**：高度模块化，易于扩展和复用
- 📝 **代码规范**：遵循Google C++ Style Guide，使用Doxygen风格中文注释
- 🎨 **现代UI**：基于Qt6的现代图形用户界面
- ⚡ **CMake构建**：现代化的CMake配置，支持跨平台编译

## 技术栈

### 核心技术

| 技术 | 版本 | 说明 |
|------|------|------|
| C++ | 17 | 核心编程语言 |
| Qt6 | 6.0+ | GUI框架（Core, Widgets, Network） |
| CMake | 3.16+ | 构建系统 |

### 支持平台

| 平台 | 编译器 | 状态 |
|------|--------|------|
| Windows | MSVC 2017+ / MinGW | ✅ 支持 |
| macOS | Clang / GCC | ✅ 支持 |

### 网络协议

| 协议 | 端口 | 说明 |
|------|------|------|
| UDP | 38888 | 设备发现广播 |
| TCP | 38889 | 文件传输服务 |

## 快速开始

### 前置要求

- **CMake** 3.16 或更高版本
- **Qt6** 6.0 或更高版本（需要以下模块）
  - Core
  - Widgets
  - Network
- **C++17兼容编译器**
  - Windows: MSVC 2017+ 或 MinGW
  - macOS: Xcode 10+ 或 Clang

### 安装依赖

#### Windows

1. 从 [Qt官网](https://www.qt.io/download) 下载并安装Qt6
2. 安装 Visual Studio 2017 或更高版本（推荐）或 MinGW
3. 设置环境变量 `CMAKE_PREFIX_PATH` 指向Qt6安装目录
   ```powershell
   # 示例（根据实际安装路径调整）
   set CMAKE_PREFIX_PATH=C:\Qt\6.10.2\msvc2022_64
   ```

#### macOS

```bash
# 使用 Homebrew 安装 Qt6
brew install qt@6

# 设置 Qt6 路径
export CMAKE_PREFIX_PATH="/usr/local/opt/qt@6"
```

### 编译项目

#### Windows (MSVC)

```powershell
# 克隆仓库
git clone https://github.com/yourusername/FileTransfer.git
cd FileTransfer

# 创建构建目录
mkdir build
cd build

# 配置项目
cmake .. -G "Visual Studio 17 2022" -A x64

# 编译
cmake --build . --config Release

# 运行
.\src\app\Release\app.exe
```

#### macOS

```bash
# 克隆仓库
git clone https://github.com/yourusername/FileTransfer.git
cd FileTransfer

# 创建构建目录
mkdir build
cd build

# 配置和编译
cmake ..
make -j$(sysctl -n hw.ncpu)

# 运行
./src/app/app
```

### 使用构建脚本

项目提供了Python构建脚本：

```bash
# Windows MSVC构建
python scripts/build.py -c msvc -b Release

# Windows MinGW构建
python scripts/build.py -c mingw -b Release

# macOS构建
python scripts/build.py -c clang -b Release
```

## 使用说明

### 启动程序

1. 在两台电脑上分别运行程序
2. 程序启动后会自动开始搜索局域网中的其他设备

### 界面布局

```
┌─────────────────────────────────────────────────────────────┐
│  菜单栏: 文件 | 编辑 | 视图 | 帮助                              │
├─────────────────────────────────────────────────────────────┤
│  工具栏: 刷新设备 | 上传文件 | 下载文件                          │
├──────────┬─────────────────────┬─────────────────────────────┤
│          │                     │                             │
│  设备树   │    本地文件系统      │      远程文件系统            │
│          │                     │                             │
│  ├─本机   │  ┌──────────────┐  │  ┌──────────────────────┐  │
│  ├─远程   │  │ 文件列表      │  │  │ 文件列表              │  │
│  │  ├─设备1│  │ (可拖拽)     │  │  │ (接收拖拽)           │  │
│  │  └─设备2│  └──────────────┘  │  └──────────────────────┘  │
│          │                     │                             │
├──────────┴─────────────────────┴─────────────────────────────┤
│  传输任务列表: 文件名 | 进度 | 大小 | 速度 | 状态 | 操作        │
├─────────────────────────────────────────────────────────────┤
│  状态栏: 设备数量: 2 | 传输任务: 1                              │
└─────────────────────────────────────────────────────────────┘
```

### 操作步骤

1. **选择设备**
   - 左侧设备树显示所有发现的设备
   - 点击远程设备，右侧面板将连接到该设备

2. **浏览文件**
   - 双面板设计：左侧为本地文件，右侧为远程文件
   - 双击目录进入，点击".."返回上级目录

3. **传输文件**
   - **拖拽方式**：从本地面板拖拽文件到远程面板（上传）
   - **拖拽方式**：从远程面板拖拽文件到本地面板（下载）
   - **按钮方式**：选中文件后点击工具栏的"上传"或"下载"按钮

4. **监控进度**
   - 底部面板显示所有传输任务
   - 实时进度条、传输速度、状态信息
   - 可随时取消正在进行的传输

### 防火墙设置

程序需要以下端口权限：

| 端口 | 协议 | 用途 |
|------|------|------|
| 38888 | UDP | 设备发现广播 |
| 38889 | TCP | 文件传输 |

**一键放行脚本：**

项目提供了防火墙端口放行脚本，位于 `scripts/` 目录：

```bash
# Windows（以管理员身份运行）
scripts\add_firewall_rules.bat

# macOS（使用sudo运行）
sudo bash scripts/add_firewall_rules_macos.sh
```

**Windows手动设置：**

```powershell
# 添加入站规则
netsh advfirewall firewall add rule name="FileTransfer UDP" dir=in action=allow protocol=UDP localport=38888
netsh advfirewall firewall add rule name="FileTransfer TCP" dir=in action=allow protocol=TCP localport=38889

# 添加出站规则
netsh advfirewall firewall add rule name="FileTransfer UDP Out" dir=out action=allow protocol=UDP localport=38888
netsh advfirewall firewall add rule name="FileTransfer TCP Out" dir=out action=allow protocol=TCP localport=38889
```

**macOS手动设置：**

在"系统偏好设置 > 安全性与隐私 > 防火墙"中允许FileTransfer应用。

## 项目结构

```
FileTransfer/
├── src/
│   ├── app/                        # 应用程序入口
│   │   ├── main.cpp               # 主函数
│   │   └── CMakeLists.txt
│   ├── common/                     # 公共模块
│   │   ├── logger/                # 日志系统
│   │   ├── theme/                 # 主题管理
│   │   ├── messagebus/            # 消息总线
│   │   ├── utils/                 # 工具类
│   │   └── json/                  # JSON工具
│   ├── core/                       # 核心业务模块
│   │   ├── network/               # 网络通信
│   │   │   ├── networkprotocol.h  # 通信协议定义
│   │   │   ├── devicediscoverer   # UDP设备发现
│   │   │   └── filetransferservice # TCP文件传输
│   │   ├── device/                # 设备管理
│   │   │   └ devicemanager        # 设备管理器
│   │   └── filesystem/            # 文件系统
│   │       └ filesystemmodel     # 文件列表模型
│   └── ui/                         # 用户界面
│       ├── mainwindow/            # 主窗口
│       └ widgets/                 # 自定义控件
│           ├── devicetreeview     # 设备树视图
│           ├── filebrowserpanel   # 文件浏览器面板
│           └── transferprogresspanel # 传输进度面板
├── scripts/                        # 构建脚本
│   ├── build.py                   # 构建脚本
│   ├── format.py                  # 代码格式化脚本
│   └── package_windows.py         # Windows打包脚本
├── .trae/                          # Trae配置
│   └ rules/
│       ├── code-rule.md           # 代码规范
│       └ project_rules.md         # 项目规则
├── .clang-format                  # 代码格式化配置
├── .gitignore                     # Git忽略配置
├── CMakeLists.txt                 # CMake配置
├── LICENSE                        # 许可证
└── README.md                      # 项目说明
```

### 模块说明

#### 网络模块 (Network)

设备发现和文件传输的核心实现：

```cpp
#include "core/network/devicediscoverer.h"
#include "core/network/filetransferservice.h"

// 设备发现服务
DeviceDiscoverer::instance().initialize(localDeviceInfo);
DeviceDiscoverer::instance().start();

// 文件传输服务
FileTransferService::instance().initialize();
FileTransferService::instance().start();

// 请求下载文件
QString taskId = FileTransferService::instance().requestDownload(deviceId, remotePath, localPath);
```

#### 设备管理模块 (Device)

管理在线设备列表和状态：

```cpp
#include "core/device/devicemanager.h"

// 获取在线设备列表
QList<DeviceInfo> devices = DeviceManager::instance().getOnlineDevices();

// 检查设备是否在线
bool online = DeviceManager::instance().isDeviceOnline(deviceId);
```

#### 文件系统模块 (FileSystem)

本地和远程文件浏览：

```cpp
#include "core/filesystem/filesystemmodel.h"

// 创建文件浏览器
FileSystemBrowser* browser = new FileSystemBrowser(FileSystemType::Local);
browser->setCurrentPath("/home/user");
browser->refresh();
```

## 开发指南

### 代码规范

本项目遵循以下代码规范：

- **编码风格**：Google C++ Style Guide
- **注释风格**：Doxygen风格的中文注释
- **文件命名**：源文件名称统一使用小写字母
- **头文件引用**：使用绝对路径，如 `#include "core/network/devicediscoverer.h"`
- **日志输出**：所有日志使用中文

### 添加新功能

1. 在相应模块目录下创建新文件
2. 编写代码和CMakeLists.txt
3. 在父级CMakeLists.txt中添加依赖
4. 更新相关文档

### 代码格式化

```bash
# 格式化所有文件
python scripts/format.py
```

## 常见问题

### 编译问题

**Q: 找不到Qt6模块？**

A: 确保已正确设置 `CMAKE_PREFIX_PATH` 环境变量指向Qt6安装目录。

**Q: Windows下编译报错？**

A: 确保使用正确的生成器：
- MSVC: `cmake .. -G "Visual Studio 17 2022" -A x64`
- MinGW: `cmake .. -G "MinGW Makefiles"`

### 运行问题

**Q: 无法发现其他设备？**

A: 检查防火墙设置，确保UDP端口38888未被阻止。

**Q: 文件传输失败？**

A: 检查TCP端口38889是否开放，确保目标设备在线。

**Q: 程序启动时找不到动态库？**

A: 确保Qt6的bin目录已添加到系统PATH环境变量中。

## 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

## 贡献指南

欢迎提交Issue和Pull Request！

### 提交规范

使用语义化提交信息：

- `feat`: 新功能
- `fix`: Bug修复
- `docs`: 文档更新
- `style`: 代码格式调整
- `refactor`: 代码重构
- `test`: 测试相关
- `chore`: 构建/工具相关

### 开发流程

1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'feat: add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建 Pull Request

## 路线图

### v1.1.0

- [ ] 支持文件夹批量传输
- [ ] 添加传输历史记录
- [ ] 支持断点续传
- [ ] 添加文件预览功能

### v1.2.0

- [ ] 支持Linux平台
- [ ] 添加加密传输选项
- [ ] 支持多设备并行传输
- [ ] 添加传输限速功能

## 联系方式

- **项目主页**: [https://github.com/yourusername/FileTransfer](https://github.com/yourusername/FileTransfer)
- **问题反馈**: [GitHub Issues](https://github.com/yourusername/FileTransfer/issues)

---

<div align="center">

**如果这个项目对您有帮助，请给一个 ⭐️ Star！**

Made with ❤️

</div>