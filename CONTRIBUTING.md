# 贡献指南

感谢您对FileTransfer项目的关注！我们欢迎任何形式的贡献。

## 如何贡献

### 报告问题

如果您发现了bug或有功能建议，请通过[GitHub Issues](https://github.com/yourusername/FileTransfer/issues)提交。

提交Issue时，请包含以下信息：

- **问题描述**：清晰描述遇到的问题
- **复现步骤**：如何复现该问题
- **预期行为**：您期望发生什么
- **实际行为**：实际发生了什么
- **环境信息**：操作系统、编译器版本、Qt版本等
- **截图/日志**：如果有相关的截图或日志，请附上

### 提交代码

1. **Fork仓库**
   - 点击GitHub页面上的"Fork"按钮
   - 将您的fork克隆到本地

2. **创建分支**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **编写代码**
   - 遵循项目的代码规范
   - 添加必要的注释和文档
   - 确保代码能正常编译

4. **提交更改**
   ```bash
   git add .
   git commit -m "feat: 添加新功能描述"
   ```

5. **推送分支**
   ```bash
   git push origin feature/your-feature-name
   ```

6. **创建Pull Request**
   - 在GitHub上创建Pull Request
   - 描述您的更改内容和原因

## 代码规范

### 编码风格

- 遵循[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- 使用4空格缩进，不使用Tab
- 每行最大长度120字符

### 注释规范

- 使用Doxygen风格的中文注释
- 所有公共接口都需要注释
- 文件头部需要添加文件说明

```cpp
/**
 * @file filename.h
 * @brief 文件简要描述
 * @date 2026-06-01
 * @version 1.0
 * 
 * @details 详细描述文件内容和用途。
 */

/**
 * @brief 函数简要描述
 * 
 * @details 函数详细描述
 * 
 * @param param1 参数1描述
 * @param param2 参数2描述
 * @return 返回值描述
 * @throws ExceptionType 异常描述
 */
```

### 命名规范

- **类名**：大驼峰命名，如 `DeviceManager`
- **函数名**：大驼峰命名，如 `getDeviceInfo`
- **变量名**：小驼峰命名，如 `deviceList`
- **常量名**：全大写加下划线，如 `MAX_RETRY_COUNT`
- **文件名**：全小写，如 `devicemanager.cpp`

### 头文件引用

- 使用绝对路径引用，不使用相对路径
- 引用顺序：系统头文件 → Qt头文件 → 项目头文件

```cpp
#include <iostream>
#include <QString>

#include "core/network/devicediscoverer.h"
#include "common/logger/logger.h"
```

## 提交信息规范

使用[语义化提交](https://www.conventionalcommits.org/)格式：

```
<type>(<scope>): <subject>

<body>

<footer>
```

### 提交类型

- `feat`: 新功能
- `fix`: Bug修复
- `docs`: 文档更新
- `style`: 代码格式调整（不影响功能）
- `refactor`: 代码重构
- `test`: 测试相关
- `chore`: 构建/工具相关

### 示例

```
feat(network): 添加设备心跳检测功能

- 实现UDP心跳广播
- 添加超时检测机制
- 更新设备状态管理

Closes #123
```

## 开发环境设置

### 前置要求

- CMake 3.16+
- Qt6 6.0+
- C++17兼容编译器

### 编译项目

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Debug
```

### 代码格式化

```bash
python scripts/format.py
```

## 代码审查标准

Pull Request需要满足以下条件：

1. 代码能正常编译
2. 遵循代码规范
3. 有适当的注释和文档
4. 不引入新的警告
5. 功能经过测试

## 许可证

贡献的代码将采用MIT许可证，与项目主许可证一致。

---

再次感谢您的贡献！