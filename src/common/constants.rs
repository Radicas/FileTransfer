#![allow(dead_code)]

//! 网络常量定义
//!
//! 定义设备发现和文件传输的端口、超时等常量

/// 设备发现UDP端口
pub const DISCOVER_PORT: u16 = 38888;

/// 文件传输TCP端口
pub const TRANSFER_PORT: u16 = 38889;

/// 心跳间隔(毫秒)
pub const HEARTBEAT_INTERVAL: u64 = 5000;

/// 心跳超时(毫秒)
pub const HEARTBEAT_TIMEOUT: u64 = 15000;

/// 文件传输块大小(64KB)
pub const TRANSFER_CHUNK_SIZE: usize = 65536;

/// 发现广播间隔(毫秒)
pub const DISCOVER_INTERVAL: u64 = 3000;

/// 最大重试次数
pub const MAX_RETRY_COUNT: u32 = 3;

/// 连接超时(毫秒)
pub const CONNECTION_TIMEOUT: u64 = 5000;