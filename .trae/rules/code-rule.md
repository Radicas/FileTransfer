1. 使用 `log` 或 `tracing` crate 实现日志系统，日志输出使用中文。
2. 公共API（函数、结构体、枚举、trait）需添加 `///` 文档注释，模块文件开头使用 `//!` 说明模块作用、作者、版本。
3. 遵循 Rust 官方风格指南，使用 `rustfmt` 格式化。命名规则：函数/变量用 snake_case，类型用 PascalCase，常量用 SCREAMING_SNAKE_CASE。
4. 源文件和模块目录名使用 snake_case，如 `function_data.rs`。
5. 结构体字段需添加 `///` 注释说明用途。
6. 跨平台代码使用 `#[cfg(target_os = "...")]` 条件编译处理平台差异。
7. 代码需通过 `cargo clippy -- -D warnings` 和 `cargo fmt -- --check` 检查，CI 中运行 `cargo test`。
8. 使用 trait 定义接口，采用组合而非继承，适当使用 Builder、Strategy 等设计模式。
9. 模块导入使用绝对路径 `use crate::module::item;`，避免相对路径。
10. 错误处理使用 `Result<T, E>` 和 `Option<T>`，自定义错误类型实现 `std::error::Error` 和 `Display` trait。
11. unsafe 代码块必须添加 `// SAFETY:` 注释说明安全条件和理由。
12. 单元测试用 `#[test]`，文档测试用 `///` 代码块，集成测试放在 `tests/` 目录。
13. 依赖管理通过 `Cargo.toml`，开发依赖放 `[dev-dependencies]`，构建依赖放 `[build-dependencies]`。
14. 公共 API 明确标注 `pub`，内部实现保持私有，避免过度暴露。
15. 类型别名用 `type` 定义，命名使用 PascalCase。