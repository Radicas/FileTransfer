//! 应用程序入口文件
//!
//! 负责应用程序初始化、核心服务启动和主循环

mod network;
mod device;
mod filesystem;
mod logger;
mod messagebus;
mod common;

#[cfg(feature = "gui")]
mod gui;

use logger::Logger;
use network::{DeviceDiscoverer, FileTransferService};
use device::DeviceManager;
use clap::Parser;

/// 文件传输系统命令行参数
#[derive(Parser, Debug)]
#[command(name = "FileTransfer")]
#[command(about = "跨平台文件传输系统", long_about = None)]
struct Args {
    /// 日志级别
    #[arg(short, long, default_value = "info")]
    log_level: String,

    /// 是否启用设备发现
    #[arg(short, long, default_value = "true")]
    discover: bool,

    /// 是否启用文件传输服务
    #[arg(short, long, default_value = "true")]
    transfer: bool,

    /// 强制使用CLI模式（仅GUI版本有效）
    #[cfg(feature = "gui")]
    #[arg(short, long, default_value = "false")]
    cli: bool,
}

/// 初始化核心服务
async fn init_services(args: &Args) -> anyhow::Result<()> {
    // 初始化日志系统
    Logger::init(&args.log_level)?;
    logger::init_logger();
    Logger::info("应用程序启动");
    Logger::info(&format!("系统: {}", common::system_info()));

    // 初始化设备管理器
    DeviceManager::instance().initialize()?;

    if args.discover {
        // 启动设备发现服务
        DeviceDiscoverer::instance().initialize()?;
        DeviceDiscoverer::instance().start().await?;
        Logger::info("设备发现服务已启动");
    }

    if args.transfer {
        // 启动文件传输服务
        FileTransferService::instance().initialize()?;
        FileTransferService::instance().start().await?;
        Logger::info("文件传输服务已启动");
    }

    Logger::info("核心服务初始化完成");
    Ok(())
}

/// 清理服务
async fn cleanup_services() {
    Logger::info("开始清理服务");

    DeviceDiscoverer::instance().stop().await;
    FileTransferService::instance().stop().await;

    Logger::info("应用程序退出");
}

/// 运行CLI模式
async fn run_cli(args: &Args) -> anyhow::Result<()> {
    // 初始化服务
    init_services(args).await?;

    // 等待用户输入退出
    Logger::info("按 Ctrl+C 退出应用程序");
    tokio::signal::ctrl_c().await?;

    // 清理服务
    cleanup_services().await;

    Ok(())
}

/// 运行GUI模式
#[cfg(feature = "gui")]
fn run_gui() -> anyhow::Result<()> {
    // 初始化日志系统
    Logger::init("info")?;
    logger::init_logger();
    Logger::info("应用程序启动(GUI模式)");
    Logger::info(&format!("系统: {}", common::system_info()));

    // 启动GUI
    gui::run_gui().map_err(|e| anyhow::anyhow!("GUI启动失败: {}", e))
}

/// 运行GUI模式（未启用gui feature时的提示）
#[cfg(not(feature = "gui"))]
#[allow(dead_code)]
fn run_gui() -> anyhow::Result<()> {
    anyhow::bail!("GUI功能未启用。请使用 --features gui 编译，或使用CLI模式运行。")
}

fn main() -> anyhow::Result<()> {
    // 解析命令行参数
    let args = Args::parse();

    // 设置控制台输出为UTF-8
    #[cfg(target_os = "windows")]
    {
        // Windows下设置控制台输出编码为UTF-8
        // SAFETY: SetConsoleOutputCP和SetConsoleCP是Windows API函数，用于设置控制台编码。
        // 这些函数是线程安全的，不会导致内存安全问题。设置编码为65001(UTF-8)是安全的操作，
        // 只影响当前进程的控制台输出，不会影响其他进程或系统状态。
        unsafe {
            // 设置控制台输出编码为UTF-8 (65001)
            winapi::um::wincon::SetConsoleOutputCP(65001);
            winapi::um::wincon::SetConsoleCP(65001);
        }
    }

    // GUI版本：默认启动GUI，加--cli参数才用CLI模式
    // CLI版本：只能用CLI模式
    #[cfg(feature = "gui")]
    {
        if args.cli {
            // CLI模式：创建tokio runtime
            let runtime = tokio::runtime::Runtime::new()?;
            runtime.block_on(run_cli(&args))
        } else {
            // GUI模式：不需要tokio runtime
            run_gui()
        }
    }

    #[cfg(not(feature = "gui"))]
    {
        // CLI模式：创建tokio runtime
        let runtime = tokio::runtime::Runtime::new()?;
        runtime.block_on(run_cli(&args))
    }
}