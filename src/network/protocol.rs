#![allow(dead_code)]

//! 网络通信协议定义
//!
//! 定义设备发现和文件传输的通信协议

use serde::{Deserialize, Serialize};
use uuid::Uuid;

/// 网络消息类型枚举
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub enum NetworkMessageType {
    None = 0,

    // 设备发现消息
    DiscoverStart = 100,
    DiscoverBroadcast,      // 设备发现广播消息
    DiscoverResponse,       // 设备发现响应消息
    DiscoverHeartbeat,      // 心跳消息
    DiscoverLeave,          // 设备离线消息

    // 文件请求消息
    FileRequestStart = 200,
    FileListRequest,        // 文件列表请求
    FileListResponse,       // 文件列表响应
    FileDownloadRequest,    // 文件下载请求
    FileDownloadResponse,   // 文件下载响应
    FileUploadRequest,      // 文件上传请求
    FileUploadResponse,     // 文件上传响应
    FileDeleteRequest,      // 文件删除请求
    FileDeleteResponse,     // 文件删除响应
    FileCreateDirRequest,   // 创建目录请求
    FileCreateDirResponse,  // 创建目录响应

    // 文件传输消息
    TransferStart = 300,
    TransferData,           // 文件数据块
    TransferComplete,       // 传输完成
    TransferError,          // 传输错误
    TransferCancel,         // 取消传输

    // 传输请求消息
    TransferRequestStart = 400,
    TransferPullRequest,    // 拉取文件请求(从对方下载)
    TransferPushRequest,    // 推送文件请求(向对方上传)
    TransferReady,          // 准备接收/发送
}

/// 设备信息数据结构
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DeviceInfoData {
    /// 设备唯一标识
    pub device_id: String,
    /// 设备名称
    pub device_name: String,
    /// IP地址
    pub ip_address: String,
    /// 文件传输端口
    pub transfer_port: u16,
    /// 操作系统类型
    pub os_type: String,
    /// 用户名
    pub user_name: String,
}

impl DeviceInfoData {
    /// 创建新的设备信息
    pub fn new() -> Self {
        Self {
            device_id: Uuid::new_v4().to_string(),
            device_name: crate::common::hostname(),
            ip_address: crate::common::local_ip_address().unwrap_or_default(),
            transfer_port: crate::common::constants::TRANSFER_PORT,
            os_type: crate::common::system_info(),
            user_name: crate::common::username(),
        }
    }
}

impl Default for DeviceInfoData {
    fn default() -> Self {
        Self::new()
    }
}

/// 文件信息数据结构
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FileInfoData {
    /// 文件/目录名称
    pub name: String,
    /// 完整路径
    pub path: String,
    /// 是否为目录
    pub is_directory: bool,
    /// 文件大小(字节)
    pub size: u64,
    /// 最后修改时间
    pub last_modified: String,
    /// 是否可读
    pub is_readable: bool,
    /// 是否可写
    pub is_writable: bool,
}

/// 网络消息封装类
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct NetworkMessage {
    /// 消息类型
    pub message_type: NetworkMessageType,
    /// 消息唯一标识
    pub message_id: String,
    /// 消息数据
    pub data: serde_json::Value,
    /// 发送者设备ID
    pub sender_id: String,
}

impl NetworkMessage {
    /// 创建新的网络消息
    pub fn new(message_type: NetworkMessageType, data: serde_json::Value, sender_id: String) -> Self {
        Self {
            message_type,
            message_id: crate::common::utils::generate_id("msg"),
            data,
            sender_id,
        }
    }

    /// 序列化消息为JSON字节
    pub fn serialize(&self) -> Vec<u8> {
        serde_json::to_vec(self).unwrap_or_default()
    }

    /// 从JSON字节反序列化消息
    pub fn deserialize(bytes: &[u8]) -> Option<Self> {
        serde_json::from_slice(bytes).ok()
    }
}