#![allow(dead_code)]

//! 文件系统模型实现
//!
//! 提供本地和远程文件系统的浏览模型

use crate::common::utils::{format_size, get_file_type};
use crate::network::protocol::FileInfoData;
use std::path::Path;

/// 文件系统类型枚举
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum FileSystemType {
    Local,
    Remote,
}

/// 文件项数据结构
#[derive(Debug, Clone)]
pub struct FileItem {
    pub name: String,
    pub path: String,
    pub is_directory: bool,
    pub size: u64,
    pub size_display: String,
    pub file_type: String,
    pub is_readable: bool,
    pub is_writable: bool,
}

impl FileItem {
    /// 从FileInfoData创建FileItem
    pub fn from_data(data: FileInfoData) -> Self {
        let name_clone = data.name.clone();
        Self {
            name: name_clone.clone(),
            path: data.path,
            is_directory: data.is_directory,
            size: data.size,
            size_display: if data.is_directory {
                String::new()
            } else {
                format_size(data.size)
            },
            file_type: if data.is_directory {
                "文件夹".to_string()
            } else {
                get_file_type(&name_clone)
            },
            is_readable: data.is_readable,
            is_writable: data.is_writable,
        }
    }

    /// 转换为FileInfoData
    pub fn to_data(&self) -> FileInfoData {
        FileInfoData {
            name: self.name.clone(),
            path: self.path.clone(),
            is_directory: self.is_directory,
            size: self.size,
            last_modified: String::new(),
            is_readable: self.is_readable,
            is_writable: self.is_writable,
        }
    }
}

/// 文件系统浏览器
pub struct FileSystemBrowser {
    /// 文件系统类型
    file_system_type: FileSystemType,
    /// 当前路径
    current_path: String,
    /// 文件列表
    files: Vec<FileItem>,
}

impl FileSystemBrowser {
    /// 创建新的文件系统浏览器
    pub fn new(file_system_type: FileSystemType) -> Self {
        let current_path = if file_system_type == FileSystemType::Local {
            // 从系统根目录开始
            #[cfg(target_os = "windows")]
            {
                // Windows: 使用当前驱动器的根目录
                std::env::current_dir()
                    .map(|p| {
                        let path = p.to_string_lossy().to_string();
                        // 提取驱动器根目录 (如 C:\)
                        // Windows路径格式通常是 "C:\xxx"，取前3个字符
                        if path.len() >= 3 {
                            path[0..3].to_string()  // 例如 "C:\"
                        } else {
                            "C:\\".to_string()
                        }
                    })
                    .unwrap_or("C:\\".to_string())
            }

            #[cfg(not(target_os = "windows"))]
            {
                // Unix: 使用根目录
                "/".to_string()
            }
        } else {
            String::new()
        };

        let mut browser = Self {
            file_system_type,
            current_path,
            files: Vec::new(),
        };

        // 初始化时加载文件列表
        if file_system_type == FileSystemType::Local {
            if let Err(e) = browser.refresh() {
                eprintln!("初始化文件列表失败: {}", e);
            }
        }

        browser
    }

    /// 获取当前路径
    pub fn get_current_path(&self) -> &str {
        &self.current_path
    }

    /// 设置当前路径
    pub fn set_current_path(&mut self, path: String) {
        self.current_path = path;
    }

    /// 获取文件系统类型
    pub fn get_file_system_type(&self) -> FileSystemType {
        self.file_system_type
    }

    /// 刷新文件列表
    pub fn refresh(&mut self) -> anyhow::Result<()> {
        if self.file_system_type == FileSystemType::Local {
            self.load_local_files()?;
        }
        Ok(())
    }

    /// 加载本地文件
    fn load_local_files(&mut self) -> anyhow::Result<()> {
        self.files.clear();

        let path = Path::new(&self.current_path);
        if !path.exists() || !path.is_dir() {
            return Ok(());
        }

        for entry in std::fs::read_dir(path)? {
            let entry = entry?;
            let metadata = entry.metadata()?;

            let file_item = FileItem {
                name: entry.file_name().to_string_lossy().to_string(),
                path: entry.path().to_string_lossy().to_string(),
                is_directory: metadata.is_dir(),
                size: metadata.len(),
                size_display: if metadata.is_dir() {
                    String::new()
                } else {
                    format_size(metadata.len())
                },
                file_type: if metadata.is_dir() {
                    "文件夹".to_string()
                } else {
                    get_file_type(&entry.file_name().to_string_lossy())
                },
                is_readable: true,
                is_writable: true,
            };

            self.files.push(file_item);
        }

        Ok(())
    }

    /// 获取文件列表
    pub fn get_files(&self) -> &[FileItem] {
        &self.files
    }

    /// 返回上一级目录
    pub fn go_up(&mut self) -> anyhow::Result<()> {
        let path = Path::new(&self.current_path);
        if let Some(parent) = path.parent() {
            if parent != path {
                self.current_path = parent.to_string_lossy().to_string();
                self.refresh()?;
            }
        }
        Ok(())
    }

    /// 返回根目录
    pub fn go_root(&mut self) -> anyhow::Result<()> {
        #[cfg(target_os = "windows")]
        {
            // Windows: 返回到驱动器根目录
            let path = Path::new(&self.current_path);
            // 获取驱动器前缀（如 "C:"）
            let root = path.components().next();
            if let Some(component) = root {
                let prefix = component.as_os_str().to_string_lossy().to_string();
                // 确保路径格式正确：C: -> C:\ 或 C:\ -> C:\ 或 C:\\ -> C:\
                let root_path = if prefix.ends_with('\\') {
                    prefix
                } else {
                    format!("{}\\", prefix)
                };
                self.current_path = root_path;
                self.refresh()?;
            }
        }

        #[cfg(not(target_os = "windows"))]
        {
            // Unix: 返回到 /
            self.current_path = "/".to_string();
            self.refresh()?;
        }

        Ok(())
    }
}