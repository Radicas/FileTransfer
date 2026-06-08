#![allow(dead_code)]

/**
 * @file utils.rs
 * @brief 通用工具函数
 * @details 提供文件大小格式化、时间戳生成等工具函数
 */

use chrono::{DateTime, Local};
use std::time::{SystemTime, UNIX_EPOCH};

/// 格式化文件大小
pub fn format_size(bytes: u64) -> String {
    if bytes < 1024 {
        format!("{} B", bytes)
    } else if bytes < 1024 * 1024 {
        format!("{:.1} KB", bytes as f64 / 1024.0)
    } else if bytes < 1024 * 1024 * 1024 {
        format!("{:.1} MB", bytes as f64 / (1024.0 * 1024.0))
    } else {
        format!("{:.1} GB", bytes as f64 / (1024.0 * 1024.0 * 1024.0))
    }
}

/// 获取当前时间戳字符串
pub fn get_timestamp() -> String {
    let now: DateTime<Local> = Local::now();
    now.format("%Y-%m-%d %H:%M:%S").to_string()
}

/// 获取当前毫秒时间戳
pub fn get_millis_timestamp() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap()
        .as_millis() as u64
}

/// 生成唯一ID
pub fn generate_id(prefix: &str) -> String {
    let timestamp = get_millis_timestamp();
    let random = rand::random::<u32>();
    format!("{}_{}_{}", prefix, timestamp, random)
}

/// 获取文件类型描述
pub fn get_file_type(filename: &str) -> String {
    if filename.contains('.') {
        let ext = filename.split('.').last().unwrap_or("").to_lowercase();

        match ext.as_str() {
            "txt" | "doc" | "docx" | "pdf" => "文档".to_string(),
            "jpg" | "jpeg" | "png" | "gif" | "bmp" => "图片".to_string(),
            "mp3" | "wav" | "flac" | "aac" => "音频".to_string(),
            "mp4" | "avi" | "mkv" | "mov" => "视频".to_string(),
            "zip" | "rar" | "7z" | "tar" | "gz" => "压缩文件".to_string(),
            "exe" | "msi" | "app" | "dmg" => "可执行文件".to_string(),
            "cpp" | "h" | "py" | "js" | "java" | "rs" => "源代码".to_string(),
            ext => format!("{} 文件", ext.to_uppercase()),
        }
    } else {
        "文件".to_string()
    }
}