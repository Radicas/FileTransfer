#![allow(dead_code)]

/**
 * @file logger.rs
 * @brief 日志系统核心实现
 * @details 提供跨平台彩色日志记录功能
 */

use chrono::Local;
use colored::Colorize;
use log::{Level, LevelFilter, Metadata, Record};
use once_cell::sync::Lazy;
use std::sync::Mutex;

/// 日志系统单例
pub struct Logger {
    level_filter: LevelFilter,
    console_enabled: bool,
    file_enabled: bool,
}

/// 全局日志实例
static LOGGER: Lazy<Mutex<Logger>> = Lazy::new(|| {
    Mutex::new(Logger {
        level_filter: LevelFilter::Info,
        console_enabled: true,
        file_enabled: false,
    })
});

impl Logger {
    /// 初始化日志系统
    pub fn init(level: &str) -> anyhow::Result<()> {
        let level_filter = match level.to_lowercase().as_str() {
            "debug" => LevelFilter::Debug,
            "info" => LevelFilter::Info,
            "warn" => LevelFilter::Warn,
            "error" => LevelFilter::Error,
            _ => LevelFilter::Info,
        };

        if let Ok(mut logger) = LOGGER.lock() {
            logger.level_filter = level_filter;
        }

        log::set_max_level(level_filter);

        Self::info("日志系统初始化成功");
        Ok(())
    }

    /// 设置控制台日志输出开关
    pub fn set_console_enabled(enabled: bool) {
        if let Ok(mut logger) = LOGGER.lock() {
            logger.console_enabled = enabled;
        }
    }

    /// 设置文件日志输出开关
    pub fn set_file_enabled(enabled: bool) {
        if let Ok(mut logger) = LOGGER.lock() {
            logger.file_enabled = enabled;
        }
    }

    /// 记录调试信息
    pub fn debug(message: &str) {
        log::debug!("{}", message);
    }

    /// 记录一般信息
    pub fn info(message: &str) {
        log::info!("{}", message);
    }

    /// 记录警告信息
    pub fn warning(message: &str) {
        log::warn!("{}", message);
    }

    /// 记录错误信息
    pub fn error(message: &str) {
        log::error!("{}", message);
    }

    /// 获取时间戳
    fn get_timestamp() -> String {
        Local::now().format("%Y-%m-%d %H:%M:%S").to_string()
    }

    /// 输出彩色日志到控制台
    fn print_colored_log(level: Level, message: &str) {
        let timestamp = Self::get_timestamp();
        let level_str = match level {
            Level::Debug => "DEBUG".cyan(),
            Level::Info => "INFO".green(),
            Level::Warn => "WARN".yellow(),
            Level::Error => "ERROR".red(),
            Level::Trace => "TRACE".white(),
        };

        println!(
            "[{}] [{}] {}",
            timestamp.bright_black(),
            level_str,
            message
        );
    }

    /// 获取日志器状态
    fn get_state() -> (bool, bool, LevelFilter) {
        if let Ok(logger) = LOGGER.lock() {
            (logger.console_enabled, logger.file_enabled, logger.level_filter)
        } else {
            (true, false, LevelFilter::Info)
        }
    }
}

/// 自定义日志实现
pub struct SimpleLogger;

impl log::Log for SimpleLogger {
    fn enabled(&self, metadata: &Metadata) -> bool {
        metadata.level() <= log::max_level()
    }

    fn log(&self, record: &Record) {
        if self.enabled(record.metadata()) {
            Logger::print_colored_log(record.level(), record.args().to_string().as_str());
        }
    }

    fn flush(&self) {}
}

/// 设置全局日志器
pub fn init_logger() {
    log::set_logger(&SimpleLogger).unwrap();
}