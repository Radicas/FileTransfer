/**
 * @file filebrowserpanel.cpp
 * @brief 文件浏览器面板控件实现
 */

#include "ui/widgets/filebrowserpanel.h"
#include "core/network/filetransferservice.h"
#include "core/device/devicemanager.h"
#include "common/logger/logger.h"
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QStyle>
#include <QMessageBox>
#include <QInputDialog>
#include <QClipboard>
#include <QHeaderView>

FileBrowserPanel::FileBrowserPanel(FileSystemType type, QWidget* parent)
    : QWidget(parent)
    , m_fileSystemType(type)
    , m_targetDeviceId("")
    , m_currentPath("")
    , m_titleLabel(nullptr)
    , m_pathEdit(nullptr)
    , m_fileTableView(nullptr)
    , m_toolBar(nullptr)
    , m_mainLayout(nullptr)
    , m_pathLayout(nullptr)
    , m_fileModel(nullptr)
    , m_upButton(nullptr)
    , m_refreshButton(nullptr)
    , m_homeButton(nullptr)
    , m_contextMenu(nullptr)
    , m_copyAction(nullptr)
    , m_pasteAction(nullptr)
    , m_deleteAction(nullptr)
    , m_newFolderAction(nullptr)
    , m_refreshAction(nullptr)
{
    initUI();
    initModel();
    initToolBar();

    connect(&FileTransferService::instance(), &FileTransferService::fileListResponse,
            this, &FileBrowserPanel::onFileListResponse);

    if (m_fileSystemType == FileSystemType::Local)
    {
        m_currentPath = QDir::homePath();
        loadLocalFiles(m_currentPath);
    }

    setAcceptDrops(true);
    updateTitle();

    Logger::instance().info("文件浏览器面板初始化完成");
}

FileBrowserPanel::~FileBrowserPanel()
{
}

void FileBrowserPanel::setFileSystemType(FileSystemType type)
{
    m_fileSystemType = type;
    updateTitle();
    refresh();
}

FileSystemType FileBrowserPanel::getFileSystemType() const
{
    return m_fileSystemType;
}

void FileBrowserPanel::setTargetDevice(const QString& deviceId)
{
    m_targetDeviceId = deviceId;
    updateTitle();
    
    if (m_fileSystemType == FileSystemType::Remote && !deviceId.isEmpty())
    {
        m_currentPath = "/";
        loadRemoteFiles(m_currentPath);
    }
}

QString FileBrowserPanel::getTargetDevice() const
{
    return m_targetDeviceId;
}

QString FileBrowserPanel::getCurrentPath() const
{
    return m_currentPath;
}

void FileBrowserPanel::setCurrentPath(const QString& path)
{
    navigateTo(path);
}

QList<FileItem> FileBrowserPanel::getSelectedFiles() const
{
    QList<FileItem> files;
    QItemSelectionModel* selectionModel = m_fileTableView->selectionModel();
    
    if (selectionModel)
    {
        QModelIndexList indexes = selectionModel->selectedRows();
        for (const QModelIndex& index : indexes)
        {
            FileItem file = m_fileModel->getFile(index.row());
            if (!file.name.isEmpty() && file.name != "..")
            {
                files.append(file);
            }
        }
    }
    
    return files;
}

void FileBrowserPanel::refresh()
{
    if (m_fileSystemType == FileSystemType::Local)
    {
        loadLocalFiles(m_currentPath);
    }
    else if (!m_targetDeviceId.isEmpty())
    {
        loadRemoteFiles(m_currentPath);
    }
}

QString FileBrowserPanel::getTitle() const
{
    return m_titleLabel ? m_titleLabel->text() : "";
}

void FileBrowserPanel::setTitle(const QString& title)
{
    if (m_titleLabel)
    {
        m_titleLabel->setText(title);
    }
}

void FileBrowserPanel::navigateUp()
{
    QDir dir(m_currentPath);
    if (dir.cdUp())
    {
        navigateTo(dir.absolutePath());
    }
}

void FileBrowserPanel::navigateToRoot()
{
    if (m_fileSystemType == FileSystemType::Local)
    {
        navigateTo(QDir::rootPath());
    }
    else
    {
        navigateTo("/");
    }
}

void FileBrowserPanel::navigateTo(const QString& path)
{
    m_currentPath = path;
    updatePathDisplay();
    refresh();
    emit pathChanged(path);
}

void FileBrowserPanel::initUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(5);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet("font-weight: bold; padding: 5px; background-color: #e0e0e0;");
    m_mainLayout->addWidget(m_titleLabel);

    m_pathLayout = new QHBoxLayout();
    m_pathLayout->setSpacing(5);

    m_upButton = new QPushButton("..", this);
    m_upButton->setFixedSize(40, 25);
    m_upButton->setToolTip("上级目录");
    m_pathLayout->addWidget(m_upButton);

    m_homeButton = new QPushButton("主目录", this);
    m_homeButton->setFixedSize(60, 25);
    m_homeButton->setToolTip("用户主目录");
    m_pathLayout->addWidget(m_homeButton);

    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setPlaceholderText("输入路径...");
    m_pathLayout->addWidget(m_pathEdit);

    m_refreshButton = new QPushButton("刷新", this);
    m_refreshButton->setFixedSize(50, 25);
    m_pathLayout->addWidget(m_refreshButton);

    m_mainLayout->addLayout(m_pathLayout);

    m_fileTableView = new QTableView(this);
    m_fileTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileTableView->setAlternatingRowColors(true);
    m_fileTableView->setSortingEnabled(true);
    m_fileTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_fileTableView->horizontalHeader()->setStretchLastSection(true);
    m_fileTableView->verticalHeader()->setVisible(false);
    m_mainLayout->addWidget(m_fileTableView);

    connect(m_upButton, &QPushButton::clicked, this, &FileBrowserPanel::navigateUp);
    connect(m_homeButton, &QPushButton::clicked, [this]() {
        if (m_fileSystemType == FileSystemType::Local)
        {
            navigateTo(QDir::homePath());
        }
    });
    connect(m_pathEdit, &QLineEdit::returnPressed, this, &FileBrowserPanel::onPathEditReturnPressed);
    connect(m_refreshButton, &QPushButton::clicked, this, &FileBrowserPanel::refresh);
    connect(m_fileTableView, &QTableView::doubleClicked, this, &FileBrowserPanel::onTableViewDoubleClicked);
    connect(m_fileTableView, &QTableView::customContextMenuRequested, this, &FileBrowserPanel::onCustomContextMenu);
}

void FileBrowserPanel::initModel()
{
    m_fileModel = new FileListModel(this);
    m_fileTableView->setModel(m_fileModel);

    m_fileTableView->setColumnWidth(0, 200);
    m_fileTableView->setColumnWidth(1, 100);
    m_fileTableView->setColumnWidth(2, 100);
    m_fileTableView->setColumnWidth(3, 150);

    connect(m_fileTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &FileBrowserPanel::onSelectionChanged);
}

void FileBrowserPanel::initToolBar()
{
    m_contextMenu = new QMenu(this);

    m_copyAction = m_contextMenu->addAction("复制");
    m_pasteAction = m_contextMenu->addAction("粘贴");
    m_deleteAction = m_contextMenu->addAction("删除");
    m_contextMenu->addSeparator();
    m_newFolderAction = m_contextMenu->addAction("新建文件夹");
    m_contextMenu->addSeparator();
    m_refreshAction = m_contextMenu->addAction("刷新");

    connect(m_copyAction, &QAction::triggered, this, &FileBrowserPanel::onCopyAction);
    connect(m_pasteAction, &QAction::triggered, this, &FileBrowserPanel::onPasteAction);
    connect(m_deleteAction, &QAction::triggered, this, &FileBrowserPanel::onDeleteAction);
    connect(m_newFolderAction, &QAction::triggered, this, &FileBrowserPanel::onNewFolderAction);
    connect(m_refreshAction, &QAction::triggered, this, &FileBrowserPanel::refresh);
}

void FileBrowserPanel::loadLocalFiles(const QString& path)
{
    QDir dir(path);
    if (!dir.exists())
    {
        Logger::instance().warning("目录不存在: " + path);
        return;
    }

    QList<FileItem> files;
    
    if (dir.absolutePath() != QDir::rootPath())
    {
        FileItem parentItem;
        parentItem.name = "..";
        parentItem.path = dir.absolutePath();
        parentItem.isDirectory = true;
        parentItem.icon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
        files.append(parentItem);
    }

    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name | QDir::DirsFirst);
    
    QFileIconProvider iconProvider;
    for (const QFileInfo& entry : entries)
    {
        FileItem item;
        item.name = entry.fileName();
        item.path = entry.absoluteFilePath();
        item.isDirectory = entry.isDir();
        item.size = entry.size();
        item.lastModified = entry.lastModified().toString("yyyy-MM-dd hh:mm:ss");
        item.isReadable = entry.isReadable();
        item.isWritable = entry.isWritable();
        item.icon = iconProvider.icon(entry);
        item.updateDisplayInfo();
        files.append(item);
    }

    m_fileModel->setFiles(files);
    Logger::instance().info("加载本地文件列表: " + path + " (" + QString::number(files.size()) + " 个文件)");
}

void FileBrowserPanel::loadRemoteFiles(const QString& path)
{
    if (m_targetDeviceId.isEmpty())
    {
        Logger::instance().warning("未设置目标设备");
        return;
    }

    m_pendingRequestId = FileTransferService::instance().requestFileList(m_targetDeviceId, path);
    Logger::instance().info("请求远程文件列表: " + path + " 从设备 " + m_targetDeviceId);
}

void FileBrowserPanel::onFileListResponse(const QString& requestId, const QList<FileInfoData>& files, bool success, const QString& errorMessage)
{
    if (requestId != m_pendingRequestId)
    {
        return;
    }

    if (!success)
    {
        Logger::instance().error("获取远程文件列表失败: " + errorMessage);
        QMessageBox::warning(this, "错误", "获取文件列表失败: " + errorMessage);
        return;
    }

    QList<FileItem> fileItems;
    
    if (m_currentPath != "/")
    {
        FileItem parentItem;
        parentItem.name = "..";
        parentItem.path = m_currentPath;
        parentItem.isDirectory = true;
        parentItem.icon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
        fileItems.append(parentItem);
    }

    QFileIconProvider iconProvider;
    for (const FileInfoData& data : files)
    {
        if (data.name.isEmpty() || data.name == "." || data.name == "..")
        {
            continue;
        }

        FileItem item(data);
        if (item.isDirectory)
        {
            item.icon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
        }
        else
        {
            item.icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
        }
        fileItems.append(item);
    }

    m_fileModel->setFiles(fileItems);
    Logger::instance().info("加载远程文件列表: " + m_currentPath + " (" + QString::number(fileItems.size()) + " 个文件)");
}

void FileBrowserPanel::onPathEditReturnPressed()
{
    QString path = m_pathEdit->text();
    if (!path.isEmpty())
    {
        navigateTo(path);
    }
}

void FileBrowserPanel::onTableViewDoubleClicked(const QModelIndex& index)
{
    FileItem file = m_fileModel->getFile(index.row());
    
    if (file.isDirectory)
    {
        if (file.name == "..")
        {
            navigateUp();
        }
        else
        {
            navigateTo(file.path);
        }
    }
    else
    {
        emit fileDoubleClicked(file);
    }
}

void FileBrowserPanel::onSelectionChanged()
{
    QList<FileItem> files = getSelectedFiles();
    emit selectionChanged(files);
}

void FileBrowserPanel::onCustomContextMenu(const QPoint& point)
{
    QModelIndex index = m_fileTableView->indexAt(point);
    
    m_copyAction->setEnabled(index.isValid());
    m_pasteAction->setEnabled(!m_clipboardFiles.isEmpty());
    m_deleteAction->setEnabled(index.isValid() && m_fileSystemType == FileSystemType::Local);
    m_newFolderAction->setEnabled(m_fileSystemType == FileSystemType::Local);

    m_contextMenu->exec(m_fileTableView->mapToGlobal(point));
}

void FileBrowserPanel::onCopyAction()
{
    m_clipboardFiles = getSelectedFiles();
    m_clipboardSourcePath = m_currentPath;
    
    if (!m_clipboardFiles.isEmpty())
    {
        QApplication::clipboard()->setText(m_clipboardFiles.first().path);
        Logger::instance().info("复制文件到剪贴板: " + QString::number(m_clipboardFiles.size()) + " 个文件");
    }
}

void FileBrowserPanel::onPasteAction()
{
    if (m_clipboardFiles.isEmpty())
    {
        return;
    }

    Logger::instance().info("粘贴文件: " + QString::number(m_clipboardFiles.size()) + " 个文件");
}

void FileBrowserPanel::onDeleteAction()
{
    QList<FileItem> files = getSelectedFiles();
    if (files.isEmpty())
    {
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认删除", 
        "确定要删除选中的 " + QString::number(files.size()) + " 个文件吗？",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes)
    {
        for (const FileItem& file : files)
        {
            if (file.isDirectory)
            {
                QDir dir(file.path);
                dir.removeRecursively();
            }
            else
            {
                QFile::remove(file.path);
            }
            Logger::instance().info("删除文件: " + file.path);
        }
        refresh();
    }
}

void FileBrowserPanel::onNewFolderAction()
{
    QString folderName = QInputDialog::getText(this, "新建文件夹", "文件夹名称:");
    if (folderName.isEmpty())
    {
        return;
    }

    QDir dir(m_currentPath);
    if (dir.mkdir(folderName))
    {
        Logger::instance().info("创建文件夹: " + folderName);
        refresh();
    }
    else
    {
        QMessageBox::warning(this, "错误", "无法创建文件夹");
    }
}

void FileBrowserPanel::onRefreshAction()
{
    refresh();
    Logger::instance().info("刷新文件列表");
}

void FileBrowserPanel::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-fileitem-list"))
    {
        event->acceptProposedAction();
    }
}

void FileBrowserPanel::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-fileitem-list"))
    {
        event->acceptProposedAction();
    }
}

void FileBrowserPanel::dropEvent(QDropEvent* event)
{
    QByteArray data = event->mimeData()->data("application/x-fileitem-list");
    QDataStream stream(&data, QIODevice::ReadOnly);
    
    QList<FileItem> files;
    int count;
    stream >> count;
    for (int i = 0; i < count; ++i)
    {
        FileItem file;
        stream >> file.name >> file.path >> file.isDirectory >> file.size;
        files.append(file);
    }

    emit filesDropped(files, m_currentPath);
    event->acceptProposedAction();
}

void FileBrowserPanel::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        QList<FileItem> files = getSelectedFiles();
        if (!files.isEmpty())
        {
            startDrag(files);
        }
    }
    QWidget::mousePressEvent(event);
}

void FileBrowserPanel::startDrag(const QList<FileItem>& files)
{
    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData();

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << files.size();
    for (const FileItem& file : files)
    {
        stream << file.name << file.path << file.isDirectory << file.size;
    }

    mimeData->setData("application/x-fileitem-list", data);
    drag->setMimeData(mimeData);

    emit dragStarted(files, this);
    drag->exec(Qt::CopyAction);
}

void FileBrowserPanel::updatePathDisplay()
{
    m_pathEdit->setText(m_currentPath);
}

void FileBrowserPanel::updateTitle()
{
    QString title;
    if (m_fileSystemType == FileSystemType::Local)
    {
        title = "本地文件系统";
    }
    else
    {
        if (m_targetDeviceId.isEmpty())
        {
            title = "远程文件系统 (未连接)";
        }
        else
        {
            DeviceInfo device = DeviceManager::instance().getDevice(m_targetDeviceId);
            title = "远程: " + (device.deviceName.isEmpty() ? m_targetDeviceId : device.deviceName);
        }
    }
    setTitle(title);
}