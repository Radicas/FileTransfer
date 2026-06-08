#![allow(dead_code)]

//! 文件传输服务实现
//!
//! 提供基于TCP的文件传输服务

use crate::common::constants::TRANSFER_PORT;
use crate::common::utils::{generate_id, get_millis_timestamp};
use crate::filesystem::{FileItem, FileSystemBrowser, FileSystemType};
use crate::logger::Logger;
use crate::network::protocol::{FileInfoData, NetworkMessage, NetworkMessageType};
use once_cell::sync::Lazy;
use std::collections::HashMap;
use std::net::SocketAddr;
use std::path::Path;
use std::sync::Arc;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::{TcpListener, TcpStream};
use tokio::sync::RwLock;

/// 传输方向
#[derive(Debug, Clone, PartialEq)]
pub enum TransferDirection {
    Upload,
    Download,
}

/// 传输状态
#[derive(Debug, Clone, PartialEq)]
pub enum TransferState {
    Pending,
    Preparing,
    Running,
    Paused,
    Completed,
    Failed,
    Cancelled,
}

/// 传输任务信息
#[derive(Debug, Clone)]
pub struct TransferTaskInfo {
    pub task_id: String,
    pub source_path: String,
    pub target_path: String,
    pub target_device_id: String,
    pub direction: TransferDirection,
    pub state: TransferState,
    pub total_size: u64,
    pub transferred_size: u64,
    pub progress: u32,
    pub error_message: Option<String>,
    pub start_time: u64,
    pub end_time: Option<u64>,
}

impl TransferTaskInfo {
    pub fn new() -> Self {
        Self {
            task_id: generate_id("task"),
            source_path: String::new(),
            target_path: String::new(),
            target_device_id: String::new(),
            direction: TransferDirection::Download,
            state: TransferState::Pending,
            total_size: 0,
            transferred_size: 0,
            progress: 0,
            error_message: None,
            start_time: get_millis_timestamp(),
            end_time: None,
        }
    }
}

impl Default for TransferTaskInfo {
    fn default() -> Self {
        Self::new()
    }
}

/// 文件传输服务
pub struct FileTransferService {
    /// 传输端口
    transfer_port: u16,
    /// 传输任务列表
    transfer_tasks: Arc<RwLock<HashMap<String, TransferTaskInfo>>>,
    /// 运行状态标志
    running: Arc<RwLock<bool>>,
    /// TCP listener
    listener: Arc<RwLock<Option<TcpListener>>>,
}

/// 全局文件传输实例
static INSTANCE: Lazy<FileTransferService> = Lazy::new(|| {
    FileTransferService {
        transfer_port: TRANSFER_PORT,
        transfer_tasks: Arc::new(RwLock::new(HashMap::new())),
        running: Arc::new(RwLock::new(false)),
        listener: Arc::new(RwLock::new(None)),
    }
});

impl FileTransferService {
    /// 获取单例实例
    pub fn instance() -> &'static FileTransferService {
        &INSTANCE
    }

    /// 初始化文件传输服务
    pub fn initialize(&self) -> anyhow::Result<()> {
        Logger::info("文件传输服务初始化");
        Ok(())
    }

    /// 启动传输服务监听
    pub async fn start(&self) -> anyhow::Result<()> {
        if *self.running.read().await {
            return Ok(());
        }

        // 创建 TCP listener
        let listener = TcpListener::bind(("0.0.0.0", self.transfer_port)).await?;

        *self.running.write().await = true;

        Logger::info(&format!(
            "文件传输服务已启动，监听端口: {}",
            self.transfer_port
        ));

        // 启动 TCP 服务处理任务
        self.start_tcp_server(listener);

        Logger::info("文件传输服务运行中...");

        Ok(())
    }

    /// 启动 TCP 服务器
    fn start_tcp_server(&self, listener: TcpListener) {
        let running = self.running.clone();
        let transfer_tasks = self.transfer_tasks.clone();

        tokio::spawn(async move {
            while *running.read().await {
                // 接受新的连接
                let result = listener.accept().await;

                if let Ok((stream, addr)) = result {
                    Logger::info(&format!("接受新的文件传输连接: {}", addr));

                    // 处理连接
                    let running_clone = running.clone();
                    let transfer_tasks_clone = transfer_tasks.clone();

                    tokio::spawn(async move {
                        Self::handle_connection(stream, addr, running_clone, transfer_tasks_clone)
                            .await;
                    });
                }
            }

            Logger::info("TCP 服务器已停止");
        });
    }

    /// 处理 TCP 连接
    async fn handle_connection(
        mut stream: TcpStream,
        addr: SocketAddr,
        running: Arc<RwLock<bool>>,
        transfer_tasks: Arc<RwLock<HashMap<String, TransferTaskInfo>>>,
    ) {
        Logger::info(&format!("开始处理连接: {}", addr));

        // 读取消息头（前4字节表示消息长度）
        let mut len_buf = [0u8; 4];

        while *running.read().await {
            // 读取消息长度
            if let Err(e) = stream.read_exact(&mut len_buf).await {
                if e.kind() == std::io::ErrorKind::UnexpectedEof {
                    Logger::info(&format!("连接 {} 已关闭", addr));
                    break;
                }
                Logger::error(&format!("读取消息长度失败: {}", e));
                break;
            }

            let msg_len = u32::from_be_bytes(len_buf) as usize;

            // 读取消息内容
            let mut msg_buf = vec![0u8; msg_len];
            if let Err(e) = stream.read_exact(&mut msg_buf).await {
                Logger::error(&format!("读取消息内容失败: {}", e));
                break;
            }

            // 解析消息
            if let Some(message) = NetworkMessage::deserialize(&msg_buf) {
                Logger::debug(&format!(
                    "收到文件传输消息来自 {} - 类型: {:?}",
                    addr, message.message_type
                ));

                // 处理不同类型的消息
                match message.message_type {
                    NetworkMessageType::FileListRequest => {
                        // 文件列表请求
                        if let Ok(path) = serde_json::from_value::<String>(message.data) {
                            Self::handle_file_list_request(&mut stream, &path).await;
                        }
                    }

                    NetworkMessageType::FileDownloadRequest => {
                        // 文件下载请求
                        if let Ok(file_path) = serde_json::from_value::<String>(message.data) {
                            Self::handle_file_download_request(&mut stream, &file_path).await;
                        }
                    }

                    NetworkMessageType::FileUploadRequest => {
                        // 文件上传请求
                        if let Ok(upload_info) =
                            serde_json::from_value::<(String, u64)>(message.data)
                        {
                            let (target_path, file_size) = upload_info;
                            Self::handle_file_upload_request(
                                &mut stream,
                                &target_path,
                                file_size,
                                &transfer_tasks,
                            )
                            .await;
                        }
                    }

                    _ => {
                        Logger::warning(&format!(
                            "未处理的文件传输消息类型: {:?}",
                            message.message_type
                        ));
                    }
                }
            }
        }

        Logger::info(&format!("连接 {} 处理完成", addr));
    }

    /// 处理文件列表请求
    async fn handle_file_list_request(stream: &mut TcpStream, path: &str) {
        Logger::info(&format!("处理文件列表请求: {}", path));

        // 创建文件浏览器
        let mut browser = FileSystemBrowser::new(FileSystemType::Local);
        browser.set_current_path(path.to_string());

        // 刷新文件列表
        if let Err(e) = browser.refresh() {
            Logger::error(&format!("刷新文件列表失败: {}", e));

            // 发送错误响应
            let error_msg = NetworkMessage::new(
                NetworkMessageType::FileListResponse,
                serde_json::json!({ "error": e.to_string() }),
                "local".to_string(),
            );

            let _ = Self::send_message(stream, &error_msg).await;
            return;
        }

        // 获取文件列表
        let files = browser.get_files();
        let file_infos: Vec<FileInfoData> = files
            .iter()
            .map(|f| FileInfoData {
                name: f.name.clone(),
                path: f.path.clone(),
                is_directory: f.is_directory,
                size: f.size,
                last_modified: String::new(),
                is_readable: f.is_readable,
                is_writable: f.is_writable,
            })
            .collect();

        // 发送响应
        let response_msg = NetworkMessage::new(
            NetworkMessageType::FileListResponse,
            serde_json::to_value(&file_infos).unwrap_or_default(),
            "local".to_string(),
        );

        let _ = Self::send_message(stream, &response_msg).await;

        Logger::info(&format!("已发送文件列表响应，包含 {} 个文件", file_infos.len()));
    }

    /// 处理文件下载请求
    async fn handle_file_download_request(stream: &mut TcpStream, file_path: &str) {
        Logger::info(&format!("处理文件下载请求: {}", file_path));

        let path = Path::new(file_path);

        // 检查文件是否存在
        if !path.exists() {
            Logger::error(&format!("文件不存在: {}", file_path));

            let error_msg = NetworkMessage::new(
                NetworkMessageType::FileDownloadResponse,
                serde_json::json!({ "error": "文件不存在" }),
                "local".to_string(),
            );

            let _ = Self::send_message(stream, &error_msg).await;
            return;
        }

        // 检查是否是文件
        if !path.is_file() {
            Logger::error(&format!("不是文件: {}", file_path));

            let error_msg = NetworkMessage::new(
                NetworkMessageType::FileDownloadResponse,
                serde_json::json!({ "error": "不是文件" }),
                "local".to_string(),
            );

            let _ = Self::send_message(stream, &error_msg).await;
            return;
        }

        // 获取文件大小
        let file_size = match path.metadata() {
            Ok(meta) => meta.len(),
            Err(e) => {
                Logger::error(&format!("获取文件信息失败: {}", e));

                let error_msg = NetworkMessage::new(
                    NetworkMessageType::FileDownloadResponse,
                    serde_json::json!({ "error": e.to_string() }),
                    "local".to_string(),
                );

                let _ = Self::send_message(stream, &error_msg).await;
                return;
            }
        };

        // 发送准备响应
        let ready_msg = NetworkMessage::new(
            NetworkMessageType::FileDownloadResponse,
            serde_json::json!({ "size": file_size, "ready": true }),
            "local".to_string(),
        );

        let _ = Self::send_message(stream, &ready_msg).await;

        // 打开文件并发送数据
        if let Err(e) = Self::send_file_data(stream, file_path, file_size).await {
            Logger::error(&format!("发送文件数据失败: {}", e));
        } else {
            Logger::info(&format!("文件 {} 发送完成", file_path));
        }
    }

    /// 发送文件数据
    async fn send_file_data(stream: &mut TcpStream, file_path: &str, file_size: u64) -> anyhow::Result<()> {
        use tokio::fs::File;
        use tokio::io::BufReader;

        let file = File::open(file_path).await?;
        let mut reader = BufReader::new(file);

        let chunk_size = crate::common::constants::TRANSFER_CHUNK_SIZE;
        let mut sent = 0u64;

        while sent < file_size {
            let mut buffer = vec![0u8; chunk_size];
            let bytes_read = reader.read(&mut buffer).await?;

            if bytes_read == 0 {
                break;
            }

            // 发送数据块消息
            let data_msg = NetworkMessage::new(
                NetworkMessageType::TransferData,
                serde_json::json!({
                    "data": buffer[..bytes_read].to_vec(),
                    "offset": sent,
                    "size": bytes_read as u64,
                }),
                "local".to_string(),
            );

            Self::send_message(stream, &data_msg).await?;

            sent += bytes_read as u64;

            Logger::debug(&format!(
                "已发送 {} / {} 字节",
                sent, file_size
            ));
        }

        // 发送完成消息
        let complete_msg = NetworkMessage::new(
            NetworkMessageType::TransferComplete,
            serde_json::json!({ "size": file_size }),
            "local".to_string(),
        );

        Self::send_message(stream, &complete_msg).await?;

        Ok(())
    }

    /// 处理文件上传请求
    async fn handle_file_upload_request(
        stream: &mut TcpStream,
        target_path: &str,
        file_size: u64,
        _transfer_tasks: &Arc<RwLock<HashMap<String, TransferTaskInfo>>>,
    ) {
        Logger::info(&format!("处理文件上传请求: {}, 大小: {}", target_path, file_size));

        // 发送准备接收响应
        let ready_msg = NetworkMessage::new(
            NetworkMessageType::FileUploadResponse,
            serde_json::json!({ "ready": true }),
            "local".to_string(),
        );

        let _ = Self::send_message(stream, &ready_msg).await;

        // 接收文件数据
        if let Err(e) = Self::receive_file_data(stream, target_path, file_size).await {
            Logger::error(&format!("接收文件数据失败: {}", e));

            let error_msg = NetworkMessage::new(
                NetworkMessageType::TransferError,
                serde_json::json!({ "error": e.to_string() }),
                "local".to_string(),
            );

            let _ = Self::send_message(stream, &error_msg).await;
        } else {
            Logger::info(&format!("文件 {} 接收完成", target_path));
        }
    }

    /// 接收文件数据
    async fn receive_file_data(stream: &mut TcpStream, target_path: &str, file_size: u64) -> anyhow::Result<()> {
        use tokio::fs::File;
        use tokio::io::BufWriter;

        // 创建文件
        let file = File::create(target_path).await?;
        let mut writer = BufWriter::new(file);

        let mut received = 0u64;
        let mut len_buf = [0u8; 4];

        while received < file_size {
            // 读取消息长度
            stream.read_exact(&mut len_buf).await?;
            let msg_len = u32::from_be_bytes(len_buf) as usize;

            // 读取消息内容
            let mut msg_buf = vec![0u8; msg_len];
            stream.read_exact(&mut msg_buf).await?;

            // 解析消息
            if let Some(message) = NetworkMessage::deserialize(&msg_buf) {
                match message.message_type {
                    NetworkMessageType::TransferData => {
                        // 接收数据块
                        if let Ok(data_info) =
                            serde_json::from_value::<serde_json::Value>(message.data)
                        {
                            if let Some(data) = data_info.get("data").and_then(|d| d.as_array()) {
                                let bytes: Vec<u8> = data
                                    .iter()
                                    .filter_map(|b| b.as_u64().map(|v| v as u8))
                                    .collect();

                                writer.write_all(&bytes).await?;
                                received += bytes.len() as u64;

                                Logger::debug(&format!(
                                    "已接收 {} / {} 字节",
                                    received, file_size
                                ));
                            }
                        }
                    }

                    NetworkMessageType::TransferComplete => {
                        Logger::info("收到传输完成消息");
                        break;
                    }

                    NetworkMessageType::TransferError => {
                        Logger::error("收到传输错误消息");
                        break;
                    }

                    _ => {}
                }
            }
        }

        writer.flush().await?;

        Ok(())
    }

    /// 发送消息
    async fn send_message(stream: &mut TcpStream, message: &NetworkMessage) -> anyhow::Result<()> {
        let data = message.serialize();
        let len = data.len() as u32;

        // 发送消息长度
        stream.write_all(&len.to_be_bytes()).await?;

        // 发送消息内容
        stream.write_all(&data).await?;

        Ok(())
    }

    /// 停止传输服务
    pub async fn stop(&self) {
        *self.running.write().await = false;
        self.transfer_tasks.write().await.clear();
        Logger::info("文件传输服务已停止");
    }

    /// 获取传输端口
    pub fn get_transfer_port(&self) -> u16 {
        self.transfer_port
    }

    /// 请求远程文件列表
    pub async fn request_file_list(
        &self,
        device_ip: &str,
        path: &str,
    ) -> anyhow::Result<Vec<FileItem>> {
        Logger::info(&format!("请求远程文件列表: {} 来自 {}", path, device_ip));

        // 连接到远程设备
        let addr = SocketAddr::new(
            device_ip.parse()?,
            self.transfer_port,
        );

        let mut stream = TcpStream::connect(addr).await?;
        Logger::info(&format!("已连接到远程设备: {}", addr));

        // 发送文件列表请求
        let request_msg = NetworkMessage::new(
            NetworkMessageType::FileListRequest,
            serde_json::to_value(path).unwrap_or_default(),
            "local".to_string(),
        );

        Self::send_message(&mut stream, &request_msg).await?;

        // 接收响应
        let mut len_buf = [0u8; 4];
        stream.read_exact(&mut len_buf).await?;
        let msg_len = u32::from_be_bytes(len_buf) as usize;

        let mut msg_buf = vec![0u8; msg_len];
        stream.read_exact(&mut msg_buf).await?;

        // 解析响应
        if let Some(response) = NetworkMessage::deserialize(&msg_buf) {
            if response.message_type == NetworkMessageType::FileListResponse {
                if let Ok(file_infos) =
                    serde_json::from_value::<Vec<FileInfoData>>(response.data)
                {
                    // 转换为 FileItem
                    let file_items: Vec<FileItem> = file_infos
                        .iter()
                        .map(|f| FileItem {
                            name: f.name.clone(),
                            path: f.path.clone(),
                            is_directory: f.is_directory,
                            size: f.size,
                            size_display: if f.is_directory {
                                "[文件夹]".to_string()
                            } else {
                                Self::format_file_size(f.size)
                            },
                            file_type: if f.is_directory {
                                "文件夹".to_string()
                            } else {
                                "文件".to_string()
                            },
                            is_readable: f.is_readable,
                            is_writable: f.is_writable,
                        })
                        .collect();

                    Logger::info(&format!("收到文件列表，包含 {} 个文件", file_items.len()));
                    return Ok(file_items);
                }
            }
        }

        Logger::error("无法解析文件列表响应");
        Ok(Vec::new())
    }

    /// 格式化文件大小
    fn format_file_size(size: u64) -> String {
        const KB: u64 = 1024;
        const MB: u64 = KB * 1024;
        const GB: u64 = MB * 1024;

        if size >= GB {
            format!("{:.2} GB", size as f64 / GB as f64)
        } else if size >= MB {
            format!("{:.2} MB", size as f64 / MB as f64)
        } else if size >= KB {
            format!("{:.2} KB", size as f64 / KB as f64)
        } else {
            format!("{} B", size)
        }
    }

    /// 请求下载文件
    pub async fn request_download(
        &self,
        _device_id: &str,
        remote_path: &str,
        local_path: &str,
    ) -> anyhow::Result<String> {
        let task = TransferTaskInfo::new();
        let task_id = task.task_id.clone();
        Logger::info(&format!(
            "请求下载文件: {} 到 {}",
            remote_path, local_path
        ));
        self.transfer_tasks.write().await.insert(task_id.clone(), task);
        Ok(task_id)
    }

    /// 请求上传文件
    pub async fn request_upload(
        &self,
        _device_id: &str,
        local_path: &str,
        remote_path: &str,
    ) -> anyhow::Result<String> {
        let task = TransferTaskInfo::new();
        let task_id = task.task_id.clone();
        let path = Path::new(local_path);
        if !path.exists() || !path.is_file() {
            Logger::error(&format!("上传文件不存在或不可读: {}", local_path));
            return Ok(task_id);
        }
        Logger::info(&format!(
            "请求上传文件: {} 到 {}",
            local_path, remote_path
        ));
        self.transfer_tasks.write().await.insert(task_id.clone(), task);
        Ok(task_id)
    }

    /// 取消传输任务
    pub async fn cancel_transfer(&self, task_id: &str) {
        if let Some(task) = self.transfer_tasks.write().await.get_mut(task_id) {
            task.state = TransferState::Cancelled;
            Logger::info(&format!("取消传输任务: {}", task_id));
        }
    }

    /// 获取传输任务信息
    pub async fn get_transfer_info(&self, task_id: &str) -> Option<TransferTaskInfo> {
        self.transfer_tasks.read().await.get(task_id).cloned()
    }

    /// 获取所有传输任务
    pub async fn get_all_transfers(&self) -> Vec<TransferTaskInfo> {
        self.transfer_tasks.read().await.values().cloned().collect()
    }

    /// 获取正在进行的传输任务数量
    pub async fn get_active_transfer_count(&self) -> usize {
        self.transfer_tasks
            .read()
            .await
            .values()
            .filter(|t| t.state == TransferState::Running || t.state == TransferState::Preparing)
            .count()
    }
}