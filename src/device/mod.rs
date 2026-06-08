//! 设备管理模块入口
//!
//! 提供设备状态跟踪和设备信息查询功能

mod device_manager;

#[allow(unused_imports)]
pub use device_manager::{DeviceManager, DeviceInfo, DeviceStatus};