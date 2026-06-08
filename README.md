# FileTransfer - Rust版本

跨平台文件传输系统的Rust实现版本。

## 项目结构

```
RustVersion/
├── Cargo.toml              # 项目配置文件
├── src/
│   ├── main.rs             # 主入口文件
│   ├── lib.rs              # 库入口文件
│   ├── common/             # 公共模块
│   │   ├── mod.rs
│   │   ├── constants.rs    # 常量定义
│   │   └── utils.rs        # 工具函数
│   ├── logger/             # 日志系统
│   │   ├── mod.rs
│   │   └── logger.rs
│   ├── network/            # 网络模块
│   │   ├── mod.rs
│   │   ├── protocol.rs     # 网络协议
│   │   ├── device_discoverer.rs    # 设备发现服务
│   │   └── file_transfer_service.rs # 文件传输服务
│   ├── device/             # 设备管理
│   │   ├── mod.rs
│   │   └── device_manager.rs
│   ├── filesystem/         # 文件系统
│   │   ├── mod.rs
│   │   └── filesystem_model.rs
│   └── messagebus/         # 消息总线
│       ├── mod.rs
│       └── messagebus.rs
```

## 功能特性

- ✅ 跨平台支持(Windows/macOS/Linux)
- ✅ 设备自动发现(UDP广播)
- ✅ 文件传输服务(TCP)
- ✅ 日志系统(彩色输出)
- ✅ 消息总线(发布-订阅模式)
- ✅ 设备管理
- ✅ 文件系统浏览

## 技术栈

- **语言**: Rust 2021 Edition
- **异步运行时**: Tokio
- **网络**: std::net + tokio
- **日志**: log + env_logger + colored
- **序列化**: serde + serde_json
- **时间**: chrono
- **错误处理**: anyhow + thiserror

## 编译和运行

### 编译项目

```bash
cd RustVersion
cargo build --release
```

### 运行项目

```bash
cargo run --release
```

### 带参数运行

```bash
cargo run --release -- --log-level debug --discover true --transfer true
```

## 命令行参数

- `--log-level`: 日志级别(debug/info/warn/error)
- `--discover`: 是否启用设备发现(true/false)
- `--transfer`: 是否启用文件传输服务(true/false)

## 网络端口

- **设备发现**: UDP 38888
- **文件传输**: TCP 38889

## 与C++版本对比

### 优势
- ✅ 内存安全保证
- ✅ 更好的并发性能
- ✅ 更现代的错误处理
- ✅ 更小的运行时依赖
- ✅ 更好的测试支持

### 待完善
- ⚠️ GUI界面(需要集成GTK-rs或Iced)
- ⚠️ 完整的文件传输实现
- ⚠️ 配置文件管理
- ⚠️ 防火墙管理

## 开发计划

1. **Phase 1**: 核心网络功能 ✅
2. **Phase 2**: 文件传输完善 🚧
3. **Phase 3**: GUI界面开发 📋
4. **Phase 4**: 性能优化 📋
5. **Phase 5**: 测试和文档 📋

## 注意事项

- 项目使用单例模式,需要注意线程安全
- 网络模块使用异步IO,需要理解Tokio运行时
- 文件系统操作需要处理所有权和生命周期
- 跨平台代码需要测试不同操作系统

## 许可证

MIT License

## 作者

FileTransfer Team