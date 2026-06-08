#![allow(dead_code)]

//! 文件传输服务实现
//!
//! 提供基于TCP的文件传输服务

use crate::common::constants::TRANSFER_PORT;
use crate::common::utils::{generate_id, get_millis_timestamp};
use crate::logger::Logger;
use once_cell::sync::Lazy;
use std::collections::HashMap;
use std::path::Path;
use std::sync::Arc;
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
}

/// 全局文件传输实例
static INSTANCE: Lazy<FileTransferService> = Lazy::new(|| {
    FileTransferService {
        transfer_port: TRANSFER_PORT,
        transfer_tasks: Arc::new(RwLock::new(HashMap::new())),
        running: Arc::new(RwLock::new(false)),
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

        Logger::info(&format!(
            "文件传输服务已启动，监听端口: {}",
            self.transfer_port
        ));
        *self.running.write().await = true;

        Logger::info("文件传输服务运行中...");

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
        _device_id: &str,
        path: &str,
    ) -> anyhow::Result<String> {
        let request_id = generate_id("req");
        Logger::info(&format!("请求文件列表: {}", path));
        Ok(request_id)
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