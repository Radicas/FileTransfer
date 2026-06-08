#![allow(dead_code)]

/**
 * @file style.rs
 * @brief GUI样式定义
 * @details 定义应用程序的视觉样式和主题
 */

use iced::Color;

/// 自定义主题颜色
pub struct CustomTheme;

impl CustomTheme {
    /// 主色调
    pub const PRIMARY: Color = Color::from_rgb(0.2, 0.5, 0.8);
    /// 次色调
    pub const SECONDARY: Color = Color::from_rgb(0.6, 0.6, 0.6);
    /// 成功色
    pub const SUCCESS: Color = Color::from_rgb(0.2, 0.7, 0.3);
    /// 警告色
    pub const WARNING: Color = Color::from_rgb(0.9, 0.6, 0.1);
    /// 错误色
    pub const ERROR: Color = Color::from_rgb(0.8, 0.2, 0.2);
    /// 背景色
    pub const BACKGROUND: Color = Color::from_rgb(0.95, 0.95, 0.95);
    /// 文本色
    pub const TEXT: Color = Color::from_rgb(0.2, 0.2, 0.2);
}