/**
 * @file mod.rs
 * @brief 日志系统模块入口
 * @details 提供跨平台彩色日志输出功能
 */

mod logger;

pub use logger::Logger;
pub use logger::init_logger;