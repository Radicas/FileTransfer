#![allow(dead_code)]

//! 设备管理器实现
//!
//! 管理所有发现的设备,提供设备状态跟踪和设备信息查询功能

use crate::logger::Logger;
use crate::network::protocol::DeviceInfoData;
use once_cell::sync::Lazy;
use std::collections::HashMap;
use std::sync::Arc;
use tokio::sync::RwLock;

/// 设备状态枚举
#[derive(Debug, Clone, PartialEq)]
pub enum DeviceStatus {
    Offline,
    Online,
    Busy,
    Error,
}

/// 设备信息类
#[derive(Debug, Clone)]
pub struct DeviceInfo {
    pub device_id: String,
    pub device_name: String,
    pub ip_address: String,
    pub transfer_port: u16,
    pub os_type: String,
    pub user_name: String,
    pub status: DeviceStatus,
    pub active_transfers: u32,
    pub total_transferred: u64,
}

impl DeviceInfo {
    /// 从DeviceInfoData创建DeviceInfo
    pub fn from_data(data: DeviceInfoData) -> Self {
        Self {
            device_id: data.device_id,
            device_name: data.device_name,
            ip_address: data.ip_address,
            transfer_port: data.transfer_port,
            os_type: data.os_type,
            user_name: data.user_name,
            status: DeviceStatus::Online,
            active_transfers: 0,
            total_transferred: 0,
        }
    }

    /// 转换为DeviceInfoData
    pub fn to_data(&self) -> DeviceInfoData {
        DeviceInfoData {
            device_id: self.device_id.clone(),
            device_name: self.device_name.clone(),
            ip_address: self.ip_address.clone(),
            transfer_port: self.transfer_port,
            os_type: self.os_type.clone(),
            user_name: self.user_name.clone(),
        }
    }
}

/// 设备管理器
pub struct DeviceManager {
    /// 设备列表
    devices: Arc<RwLock<HashMap<String, DeviceInfo>>>,
    /// 本地设备信息
    local_device: Arc<RwLock<Option<DeviceInfo>>>,
}

/// 全局设备管理实例
static INSTANCE: Lazy<DeviceManager> = Lazy::new(|| {
    DeviceManager {
        devices: Arc::new(RwLock::new(HashMap::new())),
        local_device: Arc::new(RwLock::new(None)),
    }
});

impl DeviceManager {
    /// 获取单例实例
    pub fn instance() -> &'static DeviceManager {
        &INSTANCE
    }

    /// 初始化设备管理器
    pub fn initialize(&self) -> anyhow::Result<()> {
        Logger::info("设备管理器初始化完成");
        Ok(())
    }

    /// 获取所有在线设备
    pub async fn get_online_devices(&self) -> Vec<DeviceInfo> {
        self.devices
            .read()
            .await
            .values()
            .filter(|d| d.status == DeviceStatus::Online || d.status == DeviceStatus::Busy)
            .cloned()
            .collect()
    }

    /// 获取指定设备信息
    pub async fn get_device(&self, device_id: &str) -> Option<DeviceInfo> {
        self.devices.read().await.get(device_id).cloned()
    }

    /// 检查设备是否在线
    pub async fn is_device_online(&self, device_id: &str) -> bool {
        self.devices
            .read()
            .await
            .get(device_id)
            .map(|d| d.status == DeviceStatus::Online || d.status == DeviceStatus::Busy)
            .unwrap_or(false)
    }

    /// 获取本地设备信息
    pub async fn get_local_device(&self) -> Option<DeviceInfo> {
        self.local_device.read().await.clone()
    }

    /// 添加设备
    pub async fn add_device(&self, device_info: DeviceInfo) {
        self.devices
            .write()
            .await
            .insert(device_info.device_id.clone(), device_info.clone());
        Logger::info(&format!("设备管理器添加设备: {}", device_info.device_name));
    }

    /// 移除设备
    pub async fn remove_device(&self, device_id: &str) {
        self.devices.write().await.remove(device_id);
        Logger::info(&format!("设备管理器移除设备: {}", device_id));
    }
}