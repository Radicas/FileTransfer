//! 消息总线模块入口
//!
//! 实现模块间的解耦通信

mod messagebus_impl;

#[allow(unused_imports)]
pub use messagebus_impl::{MessageBus, Message, MessageType};