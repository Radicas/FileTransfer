/**
 * @file mod.rs
 * @brief 文件系统模块入口
 * @details 提供本地和远程文件系统的浏览模型
 */

mod filesystem_model;

#[allow(unused_imports)]
pub use filesystem_model::{FileSystemBrowser, FileItem, FileSystemType};