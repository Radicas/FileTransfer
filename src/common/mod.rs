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
    // 使用 socket 连接来获取本地 IP 地址
    // 创建一个 UDP socket，连接到一个公共地址（不需要真正发送数据）
    use std::net::{UdpSocket, SocketAddr};

    // 尝试连接到一个公共地址来获取本地 IP
    // 这里使用 Google 的公共 DNS 服务器地址作为目标
    let socket = UdpSocket::bind("0.0.0.0:0").ok()?;
    let dest_addr: SocketAddr = "8.8.8.8:53".parse().ok()?;

    // 连接到目标地址（不会真正发送数据）
    socket.connect(dest_addr).ok()?;

    // 获取本地地址
    let local_addr = socket.local_addr().ok()?;
    Some(local_addr.ip().to_string())
}