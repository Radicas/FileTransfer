#![allow(dead_code)]

//! 通用工具函数
//!
//! 提供文件大小格式化、时间戳生成等工具函数

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
        let ext = filename.split('.').next_back().unwrap_or("").to_lowercase();

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

#[cfg(test)]
mod tests {
    use super::*;

    /// 测试文件大小格式化
    #[test]
    fn test_format_size() {
        assert_eq!(format_size(0), "0 B");
        assert_eq!(format_size(512), "512 B");
        assert_eq!(format_size(1024), "1.0 KB");
        assert_eq!(format_size(1024 * 1024), "1.0 MB");
        assert_eq!(format_size(1024 * 1024 * 1024), "1.0 GB");
    }

    /// 测试文件类型识别
    #[test]
    fn test_get_file_type() {
        assert_eq!(get_file_type("document.txt"), "文档");
        assert_eq!(get_file_type("image.jpg"), "图片");
        assert_eq!(get_file_type("music.mp3"), "音频");
        assert_eq!(get_file_type("video.mp4"), "视频");
        assert_eq!(get_file_type("archive.zip"), "压缩文件");
        assert_eq!(get_file_type("program.exe"), "可执行文件");
        assert_eq!(get_file_type("code.rs"), "源代码");
        assert_eq!(get_file_type("folder"), "文件");
    }

    /// 测试ID生成
    #[test]
    fn test_generate_id() {
        let id1 = generate_id("test");
        let id2 = generate_id("test");
        
        // ID应该以指定前缀开头
        assert!(id1.starts_with("test_"));
        assert!(id2.starts_with("test_"));
        
        // 两个ID应该不同（包含随机数）
        assert_ne!(id1, id2);
    }
}