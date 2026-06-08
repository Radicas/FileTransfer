#![allow(dead_code)]

//! 设备发现服务实现
//!
//! 使用UDP广播机制在局域网中发现其他设备

use crate::common::constants::{DISCOVER_INTERVAL, DISCOVER_PORT, HEARTBEAT_TIMEOUT};
use crate::logger::Logger;
use crate::network::protocol::{DeviceInfoData, NetworkMessage, NetworkMessageType};
use once_cell::sync::Lazy;
use std::collections::HashMap;
use std::net::{Ipv4Addr, SocketAddr};
use std::sync::Arc;
use std::time::{Duration, Instant};
use tokio::net::UdpSocket;
use tokio::sync::RwLock;
use tokio::time::{interval, timeout};

/// 设备发现服务
pub struct DeviceDiscoverer {
    /// 在线设备列表
    devices: Arc<RwLock<HashMap<String, DeviceInfoData>>>,
    /// 设备最后活动时间
    last_activity: Arc<RwLock<HashMap<String, Instant>>>,
    /// 本地设备信息
    local_device_info: Arc<RwLock<Option<DeviceInfoData>>>,
    /// 运行状态标志
    running: Arc<RwLock<bool>>,
    /// UDP socket
    socket: Arc<RwLock<Option<Arc<UdpSocket>>>>,
}

/// 全局设备发现实例
static INSTANCE: Lazy<DeviceDiscoverer> = Lazy::new(|| {
    DeviceDiscoverer {
        devices: Arc::new(RwLock::new(HashMap::new())),
        last_activity: Arc::new(RwLock::new(HashMap::new())),
        local_device_info: Arc::new(RwLock::new(None)),
        running: Arc::new(RwLock::new(false)),
        socket: Arc::new(RwLock::new(None)),
    }
});

impl DeviceDiscoverer {
    /// 获取单例实例
    pub fn instance() -> &'static DeviceDiscoverer {
        &INSTANCE
    }

    /// 初始化设备发现服务
    pub async fn initialize(&self) -> anyhow::Result<()> {
        // 创建本地设备信息
        let local_device = DeviceInfoData::new();
        *self.local_device_info.write().await = Some(local_device.clone());

        Logger::info(&format!(
            "设备发现服务初始化完成 - 设备ID: {}, IP: {}",
            local_device.device_id, local_device.ip_address
        ));

        Ok(())
    }

    /// 启动设备发现
    pub async fn start(&self) -> anyhow::Result<()> {
        if *self.running.read().await {
            return Ok(());
        }

        // 创建 UDP socket
        let socket = self.create_socket().await?;
        let socket_arc = Arc::new(socket);
        *self.socket.write().await = Some(socket_arc.clone());

        *self.running.write().await = true;

        Logger::info(&format!("设备发现服务已启动，监听端口: {}", DISCOVER_PORT));

        // 启动广播任务
        self.start_broadcaster(socket_arc.clone());

        // 启动接收任务
        self.start_receiver(socket_arc);

        // 启动心跳检测任务
        self.start_heartbeat_checker();

        Logger::info("设备发现服务运行中...");

        Ok(())
    }

    /// 创建 UDP socket
    async fn create_socket(&self) -> anyhow::Result<UdpSocket> {
        // 绑定到所有接口的指定端口
        let socket = UdpSocket::bind(("0.0.0.0", DISCOVER_PORT)).await?;

        // 设置广播选项
        socket.set_broadcast(true)?;

        Logger::info(&format!("UDP socket 已创建并绑定到端口 {}", DISCOVER_PORT));

        Ok(socket)
    }

    /// 启动广播发送任务
    fn start_broadcaster(&self, socket: Arc<UdpSocket>) {
        let local_device_info = self.local_device_info.clone();
        let running = self.running.clone();
        let devices = self.devices.clone();

        tokio::spawn(async move {
            let mut broadcast_interval = interval(Duration::from_millis(DISCOVER_INTERVAL));

            while *running.read().await {
                broadcast_interval.tick().await;

                let local_device = local_device_info.read().await.clone();
                if let Some(device) = &local_device {
                    // 创建广播消息
                    let message = NetworkMessage::new(
                        NetworkMessageType::DiscoverBroadcast,
                        serde_json::to_value(device).unwrap_or_default(),
                        device.device_id.clone(),
                    );

                    // 序列化消息
                    let data = message.serialize();

                    // 发送广播到局域网
                    let broadcast_addr = SocketAddr::new(
                        Ipv4Addr::new(255, 255, 255, 255).into(),
                        DISCOVER_PORT,
                    );

                    if let Err(e) = socket.send_to(&data, broadcast_addr).await {
                        Logger::error(&format!("发送广播失败: {}", e));
                    } else {
                        Logger::debug(&format!(
                            "已发送设备发现广播 - 设备: {}",
                            device.device_name
                        ));
                    }

                    // 同时发送心跳消息给已发现的设备
                    let online_devices = devices.read().await;
                    for (device_id, device_info) in online_devices.iter() {
                        let heartbeat_msg = NetworkMessage::new(
                            NetworkMessageType::DiscoverHeartbeat,
                            serde_json::to_value(&local_device).unwrap_or_default(),
                            local_device.as_ref().unwrap().device_id.clone(),
                        );

                        let heartbeat_data = heartbeat_msg.serialize();
                        let ip_parts: Vec<&str> = device_info.ip_address.split('.').collect();
                        if ip_parts.len() == 4 {
                            let ip_addr = Ipv4Addr::new(
                                ip_parts[0].parse().unwrap_or(0),
                                ip_parts[1].parse().unwrap_or(0),
                                ip_parts[2].parse().unwrap_or(0),
                                ip_parts[3].parse().unwrap_or(0),
                            );
                            let target_addr = SocketAddr::new(ip_addr.into(), DISCOVER_PORT);

                            if let Err(e) = socket.send_to(&heartbeat_data, target_addr).await {
                                Logger::warning(&format!(
                                    "发送心跳到设备 {} 失败: {}",
                                    device_id, e
                                ));
                            }
                        }
                    }
                }
            }

            Logger::info("广播任务已停止");
        });
    }

    /// 启动接收任务
    fn start_receiver(&self, socket: Arc<UdpSocket>) {
        let running = self.running.clone();
        let devices = self.devices.clone();
        let last_activity = self.last_activity.clone();
        let local_device_info = self.local_device_info.clone();

        tokio::spawn(async move {
            let mut buf = [0u8; 4096];

            while *running.read().await {
                // 使用 timeout 避免阻塞太久
                let result = timeout(Duration::from_millis(100), socket.recv_from(&mut buf)).await;

                if let Ok(Ok((size, src_addr))) = result {
                    // 解析接收到的消息
                    if let Some(message) = NetworkMessage::deserialize(&buf[..size]) {
                        Logger::debug(&format!(
                            "收到消息来自 {} - 类型: {:?}",
                            src_addr, message.message_type
                        ));

                        // 处理不同类型的消息
                        match message.message_type {
                            NetworkMessageType::DiscoverBroadcast => {
                                // 收到其他设备的广播消息
                                if let Ok(device_info) =
                                    serde_json::from_value::<DeviceInfoData>(message.data)
                                {
                                    // 不添加自己的设备信息
                                    let local_id = local_device_info.read().await.clone();
                                    if let Some(local) = &local_id {
                                        if device_info.device_id != local.device_id {
                                            Self::add_device(
                                                &devices,
                                                &last_activity,
                                                device_info,
                                            )
                                            .await;
                                        }
                                    }
                                }
                            }

                            NetworkMessageType::DiscoverHeartbeat => {
                                // 收到心跳消息，更新设备活动时间
                                if let Ok(device_info) =
                                    serde_json::from_value::<DeviceInfoData>(message.data)
                                {
                                    Self::update_device_activity(
                                        &devices,
                                        &last_activity,
                                        &device_info.device_id,
                                    )
                                    .await;
                                }
                            }

                            NetworkMessageType::DiscoverLeave => {
                                // 设备离线消息
                                if let Ok(device_info) =
                                    serde_json::from_value::<DeviceInfoData>(message.data)
                                {
                                    Self::remove_device(&devices, &device_info.device_id).await;
                                    Logger::info(&format!(
                                        "设备 {} 已离线",
                                        device_info.device_name
                                    ));
                                }
                            }

                            _ => {
                                // 其他消息类型暂不处理
                            }
                        }
                    }
                }
            }

            Logger::info("接收任务已停止");
        });
    }

    /// 启动心跳检测任务
    fn start_heartbeat_checker(&self) {
        let running = self.running.clone();
        let devices = self.devices.clone();
        let last_activity = self.last_activity.clone();

        tokio::spawn(async move {
            let mut check_interval = interval(Duration::from_millis(HEARTBEAT_TIMEOUT));

            while *running.read().await {
                check_interval.tick().await;

                // 检查设备是否超时
                let now = Instant::now();
                let mut devices_to_remove = Vec::new();

                {
                    let activity = last_activity.read().await;
                    let online_devices = devices.read().await;

                    for (device_id, last_time) in activity.iter() {
                        if now.duration_since(*last_time) > Duration::from_millis(HEARTBEAT_TIMEOUT)
                        {
                            if let Some(device_info) = online_devices.get(device_id) {
                                Logger::warning(&format!(
                                    "设备 {} 心跳超时，标记为离线",
                                    device_info.device_name
                                ));
                                devices_to_remove.push(device_id.clone());
                            }
                        }
                    }
                }

                // 移除超时设备
                for device_id in devices_to_remove {
                    Self::remove_device(&devices, &device_id).await;
                }
            }

            Logger::info("心跳检测任务已停止");
        });
    }

    /// 添加设备到列表
    async fn add_device(
        devices: &Arc<RwLock<HashMap<String, DeviceInfoData>>>,
        last_activity: &Arc<RwLock<HashMap<String, Instant>>>,
        device_info: DeviceInfoData,
    ) {
        let device_id = device_info.device_id.clone();

        // 检查是否是新设备
        let is_new = !devices.read().await.contains_key(&device_id);

        // 添加设备
        devices.write().await.insert(device_id.clone(), device_info.clone());

        // 更新活动时间
        last_activity.write().await.insert(device_id, Instant::now());

        if is_new {
            Logger::info(&format!(
                "发现新设备: {} ({})",
                device_info.device_name, device_info.ip_address
            ));
        } else {
            Logger::debug(&format!(
                "更新设备信息: {} ({})",
                device_info.device_name, device_info.ip_address
            ));
        }
    }

    /// 更新设备活动时间
    async fn update_device_activity(
        devices: &Arc<RwLock<HashMap<String, DeviceInfoData>>>,
        last_activity: &Arc<RwLock<HashMap<String, Instant>>>,
        device_id: &str,
    ) {
        // 只有在线设备才更新活动时间
        if devices.read().await.contains_key(device_id) {
            last_activity.write().await.insert(device_id.to_string(), Instant::now());
            Logger::debug(&format!("更新设备 {} 的活动时间", device_id));
        }
    }

    /// 移除设备
    async fn remove_device(devices: &Arc<RwLock<HashMap<String, DeviceInfoData>>>, device_id: &str) {
        if let Some(device_info) = devices.write().await.remove(device_id) {
            Logger::info(&format!(
                "设备 {} ({}) 已移除",
                device_info.device_name, device_info.ip_address
            ));
        }
    }

    /// 停止设备发现
    pub async fn stop(&self) {
        // 发送离线消息
        let socket_opt = self.socket.read().await.clone();
        if let Some(socket) = socket_opt {
            let local_device_opt = self.local_device_info.read().await.clone();
            if let Some(local_device) = local_device_opt {
                let leave_msg = NetworkMessage::new(
                    NetworkMessageType::DiscoverLeave,
                    serde_json::to_value(&local_device).unwrap_or_default(),
                    local_device.device_id.clone(),
                );

                let leave_data = leave_msg.serialize();
                let broadcast_addr = SocketAddr::new(
                    Ipv4Addr::new(255, 255, 255, 255).into(),
                    DISCOVER_PORT,
                );

                if let Err(e) = socket.send_to(&leave_data, broadcast_addr).await {
                    Logger::error(&format!("发送离线消息失败: {}", e));
                }
            }
        }

        *self.running.write().await = false;
        Logger::info("设备发现服务已停止");
    }

    /// 获取当前在线设备列表
    pub async fn get_online_devices(&self) -> Vec<DeviceInfoData> {
        self.devices.read().await.values().cloned().collect()
    }

    /// 获取指定设备信息
    pub async fn get_device_info(&self, device_id: &str) -> Option<DeviceInfoData> {
        self.devices.read().await.get(device_id).cloned()
    }

    /// 检查设备是否在线
    pub async fn is_device_online(&self, device_id: &str) -> bool {
        self.devices.read().await.contains_key(device_id)
    }
}