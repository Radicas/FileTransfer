#![allow(dead_code)]

//! 消息总线核心实现
//!
//! 实现模块间的解耦通信,采用发布-订阅模式

use crate::logger::Logger;
use once_cell::sync::Lazy;
use std::sync::Arc;
use tokio::sync::RwLock;

/// 消息类型枚举
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum MessageType {
    DeviceAdded,
    DeviceRemoved,
    TransferStarted,
    TransferCompleted,
    SystemStarted,
    SystemStopped,
}

/// 消息结构
#[derive(Debug, Clone)]
pub struct Message {
    pub message_type: MessageType,
    pub data: serde_json::Value,
    pub sender: String,
}

impl Message {
    pub fn new(message_type: MessageType, data: serde_json::Value, sender: String) -> Self {
        Self {
            message_type,
            data,
            sender,
        }
    }
}

/// 消息总线
pub struct MessageBus {
    /// 是否启用
    enabled: Arc<RwLock<bool>>,
}

/// 全局消息总线实例
static INSTANCE: Lazy<MessageBus> = Lazy::new(|| {
    MessageBus {
        enabled: Arc::new(RwLock::new(true)),
    }
});

impl MessageBus {
    /// 获取单例实例
    pub fn instance() -> &'static MessageBus {
        &INSTANCE
    }

    /// 发布消息
    pub async fn publish(&self, message: Message) {
        if !*self.enabled.read().await {
            return;
        }

        Logger::debug(&format!(
            "发布消息 - 类型: {:?}, 发送者: {}",
            message.message_type, message.sender
        ));
    }

    /// 启用/禁用消息总线
    pub async fn set_enabled(&self, enabled: bool) {
        *self.enabled.write().await = enabled;
        Logger::info(&format!("消息总线{}", if enabled { "已启用" } else { "已禁用" }));
    }

    /// 检查消息总线是否启用
    pub async fn is_enabled(&self) -> bool {
        *self.enabled.read().await
    }
}