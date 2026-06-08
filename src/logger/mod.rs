//! 日志系统模块入口
//!
//! 提供跨平台彩色日志输出功能

mod logger_impl;

pub use logger_impl::Logger;
pub use logger_impl::init_logger;