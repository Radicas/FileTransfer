#![allow(dead_code)]

//! 主应用程序界面
//!
//! 实现文件传输系统的主窗口和交互逻辑
//! 左侧：设备树状列表
//! 右侧：本机文件系统 + 目标机器文件系统

use crate::filesystem::{FileItem, FileSystemBrowser, FileSystemType};
use crate::logger::Logger;
use crate::network::DeviceInfoData;
use crate::network::file_transfer_service::TransferTaskInfo;
use iced::widget::{button, column, container, row, text, Column, Scrollable, Space};
use iced::{Alignment, Application, Command, Element, Length, Settings, Subscription, Theme};
use iced::time;

/// 应用消息
#[derive(Debug, Clone)]
pub enum Message {
    /// 刷新设备列表
    RefreshDevices,
    /// 定时检查设备列表
    DevicesCheck,
    /// 设备列表更新
    DevicesUpdated(Vec<DeviceInfoData>),
    /// 选择设备
    DeviceSelected(String),
    /// 刷新本机文件列表
    RefreshLocalFiles,
    /// 刷新远程文件列表
    RefreshRemoteFiles,
    /// 本机文件列表更新
    LocalFilesUpdated(Vec<FileItem>),
    /// 远程文件列表更新
    RemoteFilesUpdated(Vec<FileItem>),
    /// 选择本机文件
    LocalFileSelected(String),
    /// 选择远程文件
    RemoteFileSelected(String),
    /// 本机目录导航
    LocalNavigateTo(String),
    /// 远程目录导航
    RemoteNavigateTo(String),
    /// 本机返回上一级
    LocalGoUp,
    /// 本机返回根目录
    LocalGoRoot,
    /// 远程返回上一级
    RemoteGoUp,
    /// 远程返回根目录
    RemoteGoRoot,
    /// 开始上传（本机 -> 远程）
    StartUpload,
    /// 开始下载（远程 -> 本机）
    StartDownload,
    /// 传输任务更新
    TransfersUpdated(Vec<TransferTaskInfo>),
    /// 取消传输
    CancelTransfer(String),
    /// 无操作
    Nothing,
}

/// 文件传输应用
pub struct FileTransferApp {
    /// 设备列表
    devices: Vec<DeviceInfoData>,
    /// 当前选中的设备
    selected_device: Option<String>,
    /// 本机文件浏览器
    local_browser: FileSystemBrowser,
    /// 本机文件列表
    local_files: Vec<FileItem>,
    /// 当前选中的本机文件
    selected_local_file: Option<String>,
    /// 远程文件浏览器（模拟）
    remote_browser: FileSystemBrowser,
    /// 远程文件列表
    remote_files: Vec<FileItem>,
    /// 当前选中的远程文件
    selected_remote_file: Option<String>,
    /// 传输任务列表
    transfers: Vec<TransferTaskInfo>,
    /// 状态消息
    status_message: String,
}

impl Application for FileTransferApp {
    type Executor = iced::executor::Default;
    type Message = Message;
    type Theme = Theme;
    type Flags = ();

    fn new(_flags: Self::Flags) -> (Self, Command<Self::Message>) {
        // 初始化设备发现服务
        tokio::spawn(async {
            if let Err(e) = crate::network::DeviceDiscoverer::instance().initialize().await {
                crate::logger::Logger::error(&format!("设备发现服务初始化失败: {}", e));
                return;
            }

            if let Err(e) = crate::network::DeviceDiscoverer::instance().start().await {
                crate::logger::Logger::error(&format!("设备发现服务启动失败: {}", e));
                return;
            }

            crate::logger::Logger::info("设备发现服务已启动");
        });

        let local_browser = FileSystemBrowser::new(FileSystemType::Local);
        let local_files = local_browser.get_files().to_vec();

        let remote_browser = FileSystemBrowser::new(FileSystemType::Remote);

        (
            Self {
                devices: Vec::new(),
                selected_device: None,
                local_browser,
                local_files,
                selected_local_file: None,
                remote_browser,
                remote_files: Vec::new(),
                selected_remote_file: None,
                transfers: Vec::new(),
                status_message: "应用程序已启动".to_string(),
            },
            Command::none(),
        )
    }

    fn title(&self) -> String {
        "文件传输系统".to_string()
    }

    fn update(&mut self, message: Self::Message) -> Command<Self::Message> {
        match message {
            Message::RefreshDevices => {
                self.status_message = "正在刷新设备列表...".to_string();
                Logger::info("GUI: 刷新设备列表");
                // 清空设备列表，等待定时器更新
                self.devices.clear();
                self.status_message = "设备列表已刷新，等待设备上线...".to_string();
                Command::none()
            }

            Message::DevicesCheck => {
                // 定时检查设备列表（每3秒）
                // 使用 Command 异步获取设备列表
                Command::perform(
                    async {
                        // 从设备发现服务获取在线设备列表
                        crate::network::DeviceDiscoverer::instance()
                            .get_online_devices()
                            .await
                    },
                    Message::DevicesUpdated,
                )
            }

            Message::DevicesUpdated(devices) => {
                self.devices = devices;
                if self.devices.is_empty() {
                    self.status_message = "暂无在线设备".to_string();
                } else {
                    self.status_message = format!("发现 {} 个在线设备", self.devices.len());
                }
                Command::none()
            }

            Message::DeviceSelected(device_id) => {
                self.selected_device = Some(device_id.clone());
                self.status_message = format!("已连接设备: {}", device_id);
                Logger::info(&format!("GUI: 选中设备 {}", device_id));
                // 选择设备后刷新远程文件列表
                Command::none()
            }

            Message::RefreshLocalFiles => {
                self.status_message = "正在刷新本机文件列表...".to_string();
                Logger::info("GUI: 刷新本机文件列表");
                if let Ok(()) = self.local_browser.refresh() {
                    self.local_files = self.local_browser.get_files().to_vec();
                    self.status_message = format!("本机文件已加载: {} 个", self.local_files.len());
                }
                Command::none()
            }

            Message::RefreshRemoteFiles => {
                if self.selected_device.is_none() {
                    self.status_message = "请先选择目标设备".to_string();
                    return Command::none();
                }
                self.status_message = "正在刷新远程文件列表...".to_string();
                Logger::info("GUI: 刷新远程文件列表");
                // TODO: 实现实际的远程文件刷新
                Command::none()
            }

            Message::LocalFilesUpdated(files) => {
                self.local_files = files;
                Command::none()
            }

            Message::RemoteFilesUpdated(files) => {
                self.remote_files = files;
                Command::none()
            }

            Message::LocalFileSelected(file_path) => {
                self.selected_local_file = Some(file_path.clone());
                self.status_message = format!("选中本机文件: {}", file_path);
                Command::none()
            }

            Message::RemoteFileSelected(file_path) => {
                self.selected_remote_file = Some(file_path.clone());
                self.status_message = format!("选中远程文件: {}", file_path);
                Command::none()
            }

            Message::LocalNavigateTo(path) => {
                self.local_browser.set_current_path(path.clone());
                if let Ok(()) = self.local_browser.refresh() {
                    self.local_files = self.local_browser.get_files().to_vec();
                }
                self.status_message = format!("导航到: {}", path);
                Command::none()
            }

            Message::RemoteNavigateTo(path) => {
                self.remote_browser.set_current_path(path);
                self.status_message = "远程导航".to_string();
                Command::none()
            }

            Message::LocalGoUp => {
                if let Ok(()) = self.local_browser.go_up() {
                    self.local_files = self.local_browser.get_files().to_vec();
                    self.status_message = format!("返回上一级: {}", self.local_browser.get_current_path());
                } else {
                    self.status_message = "已经是根目录".to_string();
                }
                Command::none()
            }

            Message::LocalGoRoot => {
                if let Ok(()) = self.local_browser.go_root() {
                    self.local_files = self.local_browser.get_files().to_vec();
                    self.status_message = format!("返回根目录: {}", self.local_browser.get_current_path());
                }
                Command::none()
            }

            Message::RemoteGoUp => {
                if let Ok(()) = self.remote_browser.go_up() {
                    self.remote_files = self.remote_browser.get_files().to_vec();
                    self.status_message = format!("远程返回上一级: {}", self.remote_browser.get_current_path());
                } else {
                    self.status_message = "已经是根目录".to_string();
                }
                Command::none()
            }

            Message::RemoteGoRoot => {
                if let Ok(()) = self.remote_browser.go_root() {
                    self.remote_files = self.remote_browser.get_files().to_vec();
                    self.status_message = format!("远程返回根目录: {}", self.remote_browser.get_current_path());
                }
                Command::none()
            }

            Message::StartUpload => {
                if self.selected_local_file.is_none() || self.selected_device.is_none() {
                    self.status_message = "请先选择本机文件和目标设备".to_string();
                    return Command::none();
                }
                self.status_message = "开始上传文件...".to_string();
                Logger::info("GUI: 开始上传");
                Command::none()
            }

            Message::StartDownload => {
                if self.selected_remote_file.is_none() || self.selected_device.is_none() {
                    self.status_message = "请先选择远程文件和目标设备".to_string();
                    return Command::none();
                }
                self.status_message = "开始下载文件...".to_string();
                Logger::info("GUI: 开始下载");
                Command::none()
            }

            Message::TransfersUpdated(transfers) => {
                self.transfers = transfers;
                Command::none()
            }

            Message::CancelTransfer(task_id) => {
                self.status_message = format!("取消传输: {}", task_id);
                Logger::info(&format!("GUI: 取消传输 {}", task_id));
                Command::none()
            }

            Message::Nothing => Command::none(),
        }
    }

    fn view(&self) -> Element<'_, Self::Message> {
        // 左侧设备面板
        let device_panel = self.view_device_panel();

        // 右侧工作区（左右两个文件系统）
        let workspace = self.view_workspace();

        // 底部状态栏
        let status_bar = container(text(&self.status_message).size(12))
            .padding(5)
            .style(iced::theme::Container::Box);

        // 主布局
        let main_layout = row![
            container(device_panel)
                .width(Length::Fixed(200.0))
                .height(Length::Fill)
                .padding(10)
                .style(iced::theme::Container::Box),
            container(workspace)
                .width(Length::Fill)
                .height(Length::Fill)
                .padding(10)
                .style(iced::theme::Container::Box),
        ]
        .spacing(10)
        .height(Length::Fill)
        .padding(10);

        column![main_layout, status_bar]
            .spacing(5)
            .height(Length::Fill)
            .into()
    }

    fn subscription(&self) -> Subscription<Self::Message> {
        // 每3秒检查一次设备列表
        time::every(std::time::Duration::from_secs(3)).map(|_| Message::DevicesCheck)
    }

    fn theme(&self) -> Self::Theme {
        Theme::Light
    }
}

impl FileTransferApp {
    /// 设备面板视图
    fn view_device_panel(&self) -> Element<'_, Message> {
        let header = row![
            text("设备列表").size(16),
            button("刷新").on_press(Message::RefreshDevices),
        ]
        .spacing(10)
        .align_items(Alignment::Center);

        let device_tree: Element<Message> = if self.devices.is_empty() {
            text("暂无在线设备\n点击刷新搜索设备").into()
        } else {
            let tree: Column<Message> = self
                .devices
                .iter()
                .fold(column![], |col, device| {
                    let is_selected = self.selected_device.as_ref() == Some(&device.device_id);

                    col.push(
                        button(
                            column![
                                row![
                                    text(if is_selected { "✓ " } else { "  " }),
                                    text(&device.device_name).size(14),
                                ]
                                .spacing(5)
                                .align_items(Alignment::Center),
                                row![
                                    text("  ").width(Length::Fixed(20.0)),
                                    text(&device.ip_address).size(12),
                                ]
                                .spacing(5),
                                row![
                                    text("  ").width(Length::Fixed(20.0)),
                                    text(&device.os_type).size(10),
                                ]
                                .spacing(5),
                            ]
                            .spacing(2)
                        )
                        .padding(5)
                        .style(if is_selected {
                            iced::theme::Button::Primary
                        } else {
                            iced::theme::Button::Text
                        })
                        .on_press(Message::DeviceSelected(device.device_id.clone()))
                    )
                });
            Scrollable::new(tree.spacing(5)).into()
        };

        column![header, device_tree]
            .spacing(10)
            .width(Length::Fill)
            .into()
    }

    /// 工作区视图（左右两个文件系统）
    fn view_workspace(&self) -> Element<'_, Message> {
        // 左侧：本机文件系统
        let local_panel = self.view_local_file_panel();

        // 中间：传输操作按钮
        let transfer_buttons = column![
            button("上传 →").on_press(Message::StartUpload),
            Space::with_height(Length::Fixed(10.0)),
            button("下载 ←").on_press(Message::StartDownload),
        ]
        .spacing(5)
        .align_items(Alignment::Center);

        // 右侧：远程文件系统
        let remote_panel = self.view_remote_file_panel();

        row![
            container(local_panel)
                .width(Length::FillPortion(1))
                .height(Length::Fill)
                .padding(5)
                .style(iced::theme::Container::Box),
            container(transfer_buttons)
                .width(Length::Fixed(100.0))
                .height(Length::Fill)
                .padding(10)
                .style(iced::theme::Container::Transparent),
            container(remote_panel)
                .width(Length::FillPortion(1))
                .height(Length::Fill)
                .padding(5)
                .style(iced::theme::Container::Box),
        ]
        .spacing(10)
        .width(Length::Fill)
        .height(Length::Fill)
        .into()
    }

    /// 本机文件面板视图
    fn view_local_file_panel(&self) -> Element<'_, Message> {
        let header = row![
            text("本机文件").size(16),
            button("刷新").on_press(Message::RefreshLocalFiles),
            button("上一级").on_press(Message::LocalGoUp),
            button("根目录").on_press(Message::LocalGoRoot),
        ]
        .spacing(10)
        .align_items(Alignment::Center);

        let path_display = container(text(self.local_browser.get_current_path()).size(12))
            .padding(5)
            .style(iced::theme::Container::Box);

        let file_list: Element<Message> = if self.local_files.is_empty() {
            text("暂无文件").into()
        } else {
            let files: Column<Message> = self
                .local_files
                .iter()
                .fold(column![], |col, file| {
                    let is_selected = self.selected_local_file.as_ref() == Some(&file.path);

                    col.push(
                        button(
                            row![
                                // 文件夹使用文件夹图标，文件使用文件图标
                                text(if file.is_directory { "📂" } else { "📄" }).size(18),
                                text(&file.name).width(Length::Fill),
                                // 文件夹显示 [文件夹] 标签，文件显示大小
                                if file.is_directory {
                                    text("[文件夹]").size(12)
                                } else {
                                    text(&file.size_display).size(12)
                                },
                            ]
                            .spacing(5)
                            .align_items(Alignment::Center)
                        )
                        .padding(3)
                        .style(if is_selected {
                            iced::theme::Button::Primary
                        } else {
                            iced::theme::Button::Text
                        })
                        .on_press(if file.is_directory {
                            Message::LocalNavigateTo(file.path.clone())
                        } else {
                            Message::LocalFileSelected(file.path.clone())
                        })
                    )
                });
            Scrollable::new(files.spacing(2)).into()
        };

        column![header, path_display, file_list]
            .spacing(10)
            .width(Length::Fill)
            .height(Length::Fill)
            .into()
    }

    /// 远程文件面板视图
    fn view_remote_file_panel(&self) -> Element<'_, Message> {
        let header = row![
            text(if self.selected_device.is_some() {
                format!("远程文件 - {}", self.selected_device.as_ref().unwrap())
            } else {
                "远程文件 - 未连接".to_string()
            })
            .size(16),
            button("刷新")
                .on_press_maybe(if self.selected_device.is_some() {
                    Some(Message::RefreshRemoteFiles)
                } else {
                    None
                }),
            button("上一级")
                .on_press_maybe(if self.selected_device.is_some() {
                    Some(Message::RemoteGoUp)
                } else {
                    None
                }),
            button("根目录")
                .on_press_maybe(if self.selected_device.is_some() {
                    Some(Message::RemoteGoRoot)
                } else {
                    None
                }),
        ]
        .spacing(10)
        .align_items(Alignment::Center);

        let path_display = container(text(self.remote_browser.get_current_path()).size(12))
            .padding(5)
            .style(iced::theme::Container::Box);

        let file_list: Element<Message> = if self.selected_device.is_none() {
            text("请先选择目标设备").into()
        } else if self.remote_files.is_empty() {
            text("暂无文件").into()
        } else {
            let files: Column<Message> = self
                .remote_files
                .iter()
                .fold(column![], |col, file| {
                    let is_selected = self.selected_remote_file.as_ref() == Some(&file.path);

                    col.push(
                        button(
                            row![
                                // 文件夹使用文件夹图标，文件使用文件图标
                                text(if file.is_directory { "📂" } else { "📄" }).size(18),
                                text(&file.name).width(Length::Fill),
                                // 文件夹显示 [文件夹] 标签，文件显示大小
                                if file.is_directory {
                                    text("[文件夹]").size(12)
                                } else {
                                    text(&file.size_display).size(12)
                                },
                            ]
                            .spacing(5)
                            .align_items(Alignment::Center)
                        )
                        .padding(3)
                        .style(if is_selected {
                            iced::theme::Button::Primary
                        } else {
                            iced::theme::Button::Text
                        })
                        .on_press(if file.is_directory {
                            Message::RemoteNavigateTo(file.path.clone())
                        } else {
                            Message::RemoteFileSelected(file.path.clone())
                        })
                    )
                });
            Scrollable::new(files.spacing(2)).into()
        };

        column![header, path_display, file_list]
            .spacing(10)
            .width(Length::Fill)
            .height(Length::Fill)
            .into()
    }
}

/// 启动GUI应用
pub fn run_gui() -> iced::Result {
    Logger::info("启动GUI界面");

    // 根据操作系统设置中文字体
    #[cfg(target_os = "macos")]
    let font = iced::Font::with_name("PingFang SC");

    #[cfg(target_os = "windows")]
    let font = iced::Font::with_name("Microsoft YaHei");

    #[cfg(target_os = "linux")]
    let font = iced::Font::with_name("WenQuanYi Micro Hei");

    #[cfg(not(any(target_os = "macos", target_os = "windows", target_os = "linux")))]
    let font = iced::Font::default();

    let settings = Settings {
        window: iced::window::Settings {
            size: iced::Size::new(1000.0, 700.0),
            resizable: true,
            decorations: true,
            ..iced::window::Settings::default()
        },
        default_font: font,
        ..Settings::default()
    };

    FileTransferApp::run(settings)
}