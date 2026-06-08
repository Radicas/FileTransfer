/**
 * @file mod.rs
 * @brief 消息总线模块入口
 * @details 实现模块间的解耦通信
 */

mod messagebus;

#[allow(unused_imports)]
pub use messagebus::{MessageBus, Message, MessageType};