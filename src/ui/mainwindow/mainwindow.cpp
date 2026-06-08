/**
 * @file mainwindow.cpp
 * @brief 主窗口类实现
 */

#include "ui/mainwindow/mainwindow.h"
#include "common/logger/logger.h"
#include "common/theme/thememanager.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QSysInfo>
#include <QStandardPaths>
#include <QNetworkInterface>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_mainToolBar(nullptr)
    , m_mainSplitter(nullptr)
    , m_fileSplitter(nullptr)
    , m_deviceTreeView(nullptr)
    , m_localFilePanel(nullptr)
    , m_remoteFilePanel(nullptr)
    , m_transferPanel(nullptr)
    , m_statusLabel(nullptr)
    , m_deviceCountLabel(nullptr)
    , m_transferCountLabel(nullptr)
    , m_currentRemoteDeviceId("")
{
    setWindowTitle("文件传输系统");
    resize(1200, 800);

    initCoreServices();
    initMenuBar();
    initToolBar();
    initStatusBar();
    initCentralWidget();
    connectSignals();

    Logger::instance().info("主窗口初始化完成");
}

MainWindow::~MainWindow()
{
    DeviceDiscoverer::instance().stop();
    FileTransferService::instance().stop();
    Logger::instance().info("应用程序退出");
}

void MainWindow::initCoreServices()
{
    DeviceInfoData localInfo = getLocalDeviceInfoData();
    
    if (!DeviceDiscoverer::instance().initialize(localInfo))
    {
        Logger::instance().error("设备发现服务初始化失败");
        QMessageBox::warning(this, "警告", "设备发现服务初始化失败，可能无法发现其他设备。");
    }
    
    DeviceDiscoverer::instance().start();
    
    if (!FileTransferService::instance().initialize())
    {
        Logger::instance().error("文件传输服务初始化失败");
        QMessageBox::warning(this, "警告", "文件传输服务初始化失败，文件传输功能可能不可用。");
    }
    
    FileTransferService::instance().start();
    
    DeviceManager::instance().initialize();
    
    Logger::instance().info("核心服务初始化完成");
}

void MainWindow::initMenuBar()
{
    QMenuBar* menuBar = this->menuBar();

    QMenu* fileMenu = menuBar->addMenu("文件(&F)");
    
    QAction* refreshAction = fileMenu->addAction("刷新设备(&R)");
    refreshAction->setShortcut(QKeySequence(Qt::Key_F5));
    connect(refreshAction, &QAction::triggered, this, &MainWindow::onRefreshDevices);

    fileMenu->addSeparator();
    
    QAction* settingsAction = fileMenu->addAction("设置(&S)");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettings);

    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction("退出(&X)");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    QMenu* editMenu = menuBar->addMenu("编辑(&E)");
    editMenu->addAction("复制(&C)")->setShortcut(QKeySequence::Copy);
    editMenu->addAction("粘贴(&V)")->setShortcut(QKeySequence::Paste);
    editMenu->addAction("删除(&D)")->setShortcut(QKeySequence::Delete);

    QMenu* viewMenu = menuBar->addMenu("视图(&V)");
    
    QAction* themeModernAction = viewMenu->addAction("现代主题");
    connect(themeModernAction, &QAction::triggered, []() {
        ThemeManager::instance().setTheme(ThemeType::Modern);
    });
    
    QAction* themeIndustrialAction = viewMenu->addAction("工业主题");
    connect(themeIndustrialAction, &QAction::triggered, []() {
        ThemeManager::instance().setTheme(ThemeType::Industrial);
    });
    
    QAction* themeCyberpunkAction = viewMenu->addAction("赛博朋克主题");
    connect(themeCyberpunkAction, &QAction::triggered, []() {
        ThemeManager::instance().setTheme(ThemeType::Cyberpunk);
    });

    QMenu* helpMenu = menuBar->addMenu("帮助(&H)");
    QAction* aboutAction = helpMenu->addAction("关于(&A)");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::initToolBar()
{
    m_mainToolBar = addToolBar("主工具栏");
    
    QAction* refreshAction = m_mainToolBar->addAction("刷新设备");
    connect(refreshAction, &QAction::triggered, this, &MainWindow::onRefreshDevices);
    
    m_mainToolBar->addSeparator();
    
    QAction* uploadAction = m_mainToolBar->addAction("上传文件");
    connect(uploadAction, &QAction::triggered, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, "选择文件", "", "所有文件 (*.*)");
        if (!fileName.isEmpty() && !m_currentRemoteDeviceId.isEmpty())
        {
            FileItem file;
            file.name = QFileInfo(fileName).fileName();
            file.path = fileName;
            file.isDirectory = false;
            file.size = QFileInfo(fileName).size();
            
            QList<FileItem> files;
            files.append(file);
            
            startFileTransfer(files, m_localFilePanel, m_remoteFilePanel);
        }
    });
    
    QAction* downloadAction = m_mainToolBar->addAction("下载文件");
    connect(downloadAction, &QAction::triggered, [this]() {
        QList<FileItem> files = m_remoteFilePanel->getSelectedFiles();
        if (!files.isEmpty() && !m_currentRemoteDeviceId.isEmpty())
        {
            startFileTransfer(files, m_remoteFilePanel, m_localFilePanel);
        }
    });
}

void MainWindow::initStatusBar()
{
    QWidget* statusWidget = new QWidget(this);
    QHBoxLayout* statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(5, 0, 5, 0);
    statusLayout->setSpacing(20);

    m_statusLabel = new QLabel("就绪", statusWidget);
    statusLayout->addWidget(m_statusLabel);

    m_deviceCountLabel = new QLabel("设备: 0", statusWidget);
    statusLayout->addWidget(m_deviceCountLabel);

    m_transferCountLabel = new QLabel("传输: 0", statusWidget);
    statusLayout->addWidget(m_transferCountLabel);

    statusLayout->addStretch();

    statusBar()->addWidget(statusWidget);
}

void MainWindow::initCentralWidget()
{
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    m_deviceTreeView = new DeviceTreeView(this);
    m_deviceTreeView->setMinimumWidth(150);
    m_deviceTreeView->setMaximumWidth(300);
    m_mainSplitter->addWidget(m_deviceTreeView);

    m_fileSplitter = new QSplitter(Qt::Horizontal, this);
    
    m_localFilePanel = new FileBrowserPanel(FileSystemType::Local, this);
    m_fileSplitter->addWidget(m_localFilePanel);
    
    m_remoteFilePanel = new FileBrowserPanel(FileSystemType::Remote, this);
    m_fileSplitter->addWidget(m_remoteFilePanel);
    
    m_fileSplitter->setSizes({500, 500});
    m_mainSplitter->addWidget(m_fileSplitter);
    
    m_mainSplitter->setSizes({200, 800});
    mainLayout->addWidget(m_mainSplitter);

    m_transferPanel = new TransferProgressPanel(this);
    m_transferPanel->setMinimumHeight(100);
    m_transferPanel->setMaximumHeight(200);
    mainLayout->addWidget(m_transferPanel);

    setCentralWidget(centralWidget);
}

void MainWindow::connectSignals()
{
    connect(m_deviceTreeView, &DeviceTreeView::deviceSelected,
            this, &MainWindow::onDeviceSelected);
    connect(m_deviceTreeView, &DeviceTreeView::deviceDoubleClicked,
            this, &MainWindow::onDeviceDoubleClicked);

    connect(m_localFilePanel, &FileBrowserPanel::selectionChanged,
            this, &MainWindow::onLocalFileSelected);
    connect(m_remoteFilePanel, &FileBrowserPanel::selectionChanged,
            this, &MainWindow::onRemoteFileSelected);

    connect(m_remoteFilePanel, &FileBrowserPanel::filesDropped,
            this, &MainWindow::onFilesDroppedToRemote);
    connect(m_localFilePanel, &FileBrowserPanel::filesDropped,
            this, &MainWindow::onFilesDroppedToLocal);

    connect(m_transferPanel, &TransferProgressPanel::transferCountChanged,
            this, &MainWindow::onTransferCountChanged);

    connect(&DeviceDiscoverer::instance(), &DeviceDiscoverer::deviceDiscovered,
            this, &MainWindow::onDeviceDiscovered);
    connect(&DeviceDiscoverer::instance(), &DeviceDiscoverer::deviceOffline,
            this, &MainWindow::onDeviceOffline);
}

void MainWindow::onDeviceSelected(const QString& deviceId)
{
    if (deviceId.isEmpty())
    {
        m_currentRemoteDeviceId.clear();
        m_remoteFilePanel->setTargetDevice("");
        m_remoteFilePanel->setTitle("远程文件系统 (未连接)");
        m_statusLabel->setText("选择本地文件系统");
    }
    else
    {
        m_currentRemoteDeviceId = deviceId;
        m_remoteFilePanel->setTargetDevice(deviceId);
        m_remoteFilePanel->refresh();
        
        DeviceInfo device = DeviceManager::instance().getDevice(deviceId);
        m_statusLabel->setText("已连接: " + device.deviceName + " (" + device.ipAddress + ")");
        
        Logger::instance().info("选择设备: " + device.deviceName);
    }
}

void MainWindow::onDeviceDoubleClicked(const QString& deviceId)
{
    onDeviceSelected(deviceId);
}

void MainWindow::onLocalFileSelected(const QList<FileItem>& files)
{
    if (!files.isEmpty())
    {
        m_statusLabel->setText("已选择 " + QString::number(files.size()) + " 个本地文件");
    }
}

void MainWindow::onRemoteFileSelected(const QList<FileItem>& files)
{
    if (!files.isEmpty())
    {
        m_statusLabel->setText("已选择 " + QString::number(files.size()) + " 个远程文件");
    }
}

void MainWindow::onFilesDroppedToRemote(const QList<FileItem>& files, const QString& targetPath)
{
    if (m_currentRemoteDeviceId.isEmpty())
    {
        QMessageBox::warning(this, "警告", "请先选择一个远程设备");
        return;
    }
    
    startFileTransfer(files, m_localFilePanel, m_remoteFilePanel);
    Logger::instance().info("拖拽文件到远程: " + QString::number(files.size()) + " 个文件");
}

void MainWindow::onFilesDroppedToLocal(const QList<FileItem>& files, const QString& targetPath)
{
    if (m_currentRemoteDeviceId.isEmpty())
    {
        QMessageBox::warning(this, "警告", "请先选择一个远程设备");
        return;
    }
    
    startFileTransfer(files, m_remoteFilePanel, m_localFilePanel);
    Logger::instance().info("拖拽文件到本地: " + QString::number(files.size()) + " 个文件");
}

void MainWindow::onTransferCountChanged(int count)
{
    m_transferCountLabel->setText("传输: " + QString::number(count));
    updateStatusBar();
}

void MainWindow::onDeviceDiscovered(const DeviceInfoData& deviceInfo)
{
    m_deviceCountLabel->setText("设备: " + QString::number(DeviceDiscoverer::instance().getOnlineDevices().size()));
    m_statusLabel->setText("发现设备: " + deviceInfo.deviceName);
    
    m_deviceTreeView->refreshDeviceList();
}

void MainWindow::onDeviceOffline(const QString& deviceId)
{
    m_deviceCountLabel->setText("设备: " + QString::number(DeviceDiscoverer::instance().getOnlineDevices().size()));
    
    if (m_currentRemoteDeviceId == deviceId)
    {
        m_currentRemoteDeviceId.clear();
        m_remoteFilePanel->setTargetDevice("");
        m_remoteFilePanel->setTitle("远程文件系统 (设备已离线)");
        m_statusLabel->setText("设备已离线");
    }
    
    m_deviceTreeView->refreshDeviceList();
}

void MainWindow::startFileTransfer(const QList<FileItem>& files, FileBrowserPanel* sourcePanel, FileBrowserPanel* targetPanel)
{
    if (files.isEmpty())
    {
        return;
    }

    QString targetDeviceId = targetPanel->getFileSystemType() == FileSystemType::Remote ? 
                             targetPanel->getTargetDevice() : 
                             DeviceManager::instance().getLocalDevice().deviceId;

    QString targetPath = targetPanel->getCurrentPath();

    for (const FileItem& file : files)
    {
        QString targetFilePath = targetPath;
        if (!targetFilePath.endsWith("/") && !targetFilePath.endsWith("\\"))
        {
            targetFilePath += "/";
        }
        targetFilePath += file.name;

        if (sourcePanel->getFileSystemType() == FileSystemType::Local)
        {
            FileTransferService::instance().requestUpload(targetDeviceId, file.path, targetFilePath);
            Logger::instance().info("开始上传: " + file.name + " 到 " + targetFilePath);
        }
        else
        {
            FileTransferService::instance().requestDownload(targetDeviceId, file.path, targetFilePath);
            Logger::instance().info("开始下载: " + file.name + " 到 " + targetFilePath);
        }
    }

    m_statusLabel->setText("开始传输 " + QString::number(files.size()) + " 个文件");
}

void MainWindow::updateStatusBar()
{
    int deviceCount = DeviceDiscoverer::instance().getOnlineDevices().size();
    int transferCount = FileTransferService::instance().getActiveTransferCount();
    
    m_deviceCountLabel->setText("设备: " + QString::number(deviceCount));
    m_transferCountLabel->setText("传输: " + QString::number(transferCount));
}

void MainWindow::onNewFile()
{
    Logger::instance().info("新建文件");
    statusBar()->showMessage("新建文件", 2000);
}

void MainWindow::onOpenFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "打开文件", "", "所有文件 (*.*)");
    if (!fileName.isEmpty())
    {
        Logger::instance().info("打开文件: " + fileName);
        statusBar()->showMessage("打开文件: " + fileName, 2000);
    }
}

void MainWindow::onSaveFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, "保存文件", "", "所有文件 (*.*)");
    if (!fileName.isEmpty())
    {
        Logger::instance().info("保存文件: " + fileName);
        statusBar()->showMessage("保存文件: " + fileName, 2000);
    }
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "关于", 
        "文件传输系统\n"
        "版本 1.0.0\n\n"
        "跨平台文件传输软件\n"
        "支持Windows和MacOS系统\n\n"
        "功能特性:\n"
        "- 自动发现局域网设备\n"
        "- 双面板文件浏览\n"
        "- 拖拽文件传输\n"
        "- 传输进度显示");
}

void MainWindow::onSettings()
{
    QMessageBox::information(this, "设置", "设置功能开发中...");
}

void MainWindow::onRefreshDevices()
{
    m_deviceTreeView->refreshDeviceList();
    m_statusLabel->setText("刷新设备列表");
    Logger::instance().info("手动刷新设备列表");
}

DeviceInfoData MainWindow::getLocalDeviceInfoData() const
{
    DeviceInfoData data;
    data.deviceId = "";
    data.deviceName = QSysInfo::machineHostName();
    data.osType = QSysInfo::prettyProductName();
    data.transferPort = NetworkConstants::TRANSFER_PORT;
    
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    data.userName = homePath.split("/").last();
#ifdef Q_OS_WIN
    data.userName = homePath.split("\\").last();
#endif

    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : addresses)
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback())
        {
            data.ipAddress = address.toString();
            break;
        }
    }

    return data;
}