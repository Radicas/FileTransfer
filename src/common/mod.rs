#![allow(dead_code)]

//! 公共模块入口
//!
//! 提供跨平台兼容性和通用工具函数

pub mod utils;
pub mod constants;

/// 获取系统信息
pub fn system_info() -> String {
    #[cfg(target_os = "windows")]
    {
        format!("Windows {}", os_info::get().version())
    }
    #[cfg(target_os = "macos")]
    {
        format!("macOS {}", os_info::get().version())
    }
    #[cfg(target_os = "linux")]
    {
        format!("Linux {}", os_info::get().version())
    }
    #[cfg(not(any(target_os = "windows", target_os = "macos", target_os = "linux")))]
    {
        "Unknown OS".to_string()
    }
}

/// 获取主机名
pub fn hostname() -> String {
    hostname::get()
        .map(|h| h.to_string_lossy().into_owned())
        .unwrap_or_else(|_| "Unknown".to_string())
}

/// 获取用户名
pub fn username() -> String {
    whoami::username()
}

/// 获取本地IP地址
pub fn local_ip_address() -> Option<String> {
    // TODO: 实现获取本地IP地址的逻辑
    None
}