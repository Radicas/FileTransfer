#![allow(dead_code)]

/**
 * @file mod.rs
 * @brief GUI模块入口
 * @details 提供基于Iced的图形用户界面
 */

mod app;
mod style;

pub use app::run_gui;