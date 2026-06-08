/**
 * @file filesystemmodel.cpp
 * @brief 文件系统模型类实现
 */

#include "core/filesystem/filesystemmodel.h"
#include "core/network/filetransferservice.h"
#include "common/logger/logger.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

FileListModel::FileListModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int FileListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return m_files.size();
}

int FileListModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return ColCount;
}

QVariant FileListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_files.size())
    {
        return QVariant();
    }

    const FileItem& file = m_files.at(index.row());

    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
        case ColName:
            return file.name;
        case ColSize:
            return file.sizeDisplay;
        case ColType:
            return file.type;
        case ColModified:
            return file.lastModified;
        default:
            return QVariant();
        }
    }

    if (role == Qt::DecorationRole && index.column() == ColName)
    {
        if (file.icon.isNull())
        {
            if (file.isDirectory)
            {
                return m_iconProvider.icon(QFileIconProvider::Folder);
            }
            else
            {
                QFileInfo fileInfo(file.path);
                return m_iconProvider.icon(fileInfo);
            }
        }
        return file.icon;
    }

    switch (role)
    {
    case NameRole:
        return file.name;
    case PathRole:
        return file.path;
    case IsDirectoryRole:
        return file.isDirectory;
    case SizeRole:
        return file.size;
    case SizeDisplayRole:
        return file.sizeDisplay;
    case LastModifiedRole:
        return file.lastModified;
    case TypeRole:
        return file.type;
    case IconRole:
        return file.icon;
    case IsReadableRole:
        return file.isReadable;
    case IsWritableRole:
        return file.isWritable;
    case IsSelectedRole:
        return file.isSelected;
    default:
        return QVariant();
    }
}

QVariant FileListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    {
        return QVariant();
    }

    switch (section)
    {
    case ColName:
        return "名称";
    case ColSize:
        return "大小";
    case ColType:
        return "类型";
    case ColModified:
        return "修改时间";
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> FileListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[PathRole] = "path";
    roles[IsDirectoryRole] = "isDirectory";
    roles[SizeRole] = "size";
    roles[SizeDisplayRole] = "sizeDisplay";
    roles[LastModifiedRole] = "lastModified";
    roles[TypeRole] = "type";
    roles[IconRole] = "icon";
    roles[IsReadableRole] = "isReadable";
    roles[IsWritableRole] = "isWritable";
    roles[IsSelectedRole] = "isSelected";
    return roles;
}

void FileListModel::setFiles(const QList<FileItem>& files)
{
    beginResetModel();
    m_files = files;
    m_fileIndexMap.clear();
    
    for (int i = 0; i < m_files.size(); ++i)
    {
        m_fileIndexMap.insert(m_files[i].path, i);
    }
    
    endResetModel();
}

void FileListModel::clearFiles()
{
    beginResetModel();
    m_files.clear();
    m_fileIndexMap.clear();
    endResetModel();
}

FileItem FileListModel::getFile(int index) const
{
    if (index >= 0 && index < m_files.size())
    {
        return m_files.at(index);
    }
    return FileItem();
}

FileItem FileListModel::getFile(const QString& path) const
{
    if (m_fileIndexMap.contains(path))
    {
        return m_files.at(m_fileIndexMap.value(path));
    }
    return FileItem();
}

int FileListModel::findFileIndex(const QString& path) const
{
    return m_fileIndexMap.value(path, -1);
}

QList<FileItem> FileListModel::getSelectedFiles() const
{
    QList<FileItem> selected;
    for (const FileItem& file : m_files)
    {
        if (file.isSelected)
        {
            selected.append(file);
        }
    }
    return selected;
}

void FileListModel::setSelected(int index, bool selected)
{
    if (index >= 0 && index < m_files.size())
    {
        m_files[index].isSelected = selected;
        emit dataChanged(createIndex(index, 0), createIndex(index, ColCount - 1));
    }
}

void FileListModel::clearSelection()
{
    for (int i = 0; i < m_files.size(); ++i)
    {
        m_files[i].isSelected = false;
    }
    emit dataChanged(createIndex(0, 0), createIndex(m_files.size() - 1, ColCount - 1));
}

void FileListModel::selectAll()
{
    for (int i = 0; i < m_files.size(); ++i)
    {
        m_files[i].isSelected = true;
    }
    emit dataChanged(createIndex(0, 0), createIndex(m_files.size() - 1, ColCount - 1));
}

FileSystemBrowser::FileSystemBrowser(FileSystemType type, QObject* parent)
    : QObject(parent)
    , m_type(type)
    , m_fileListModel(nullptr)
    , m_currentPath("")
    , m_targetDeviceId("")
{
    m_fileListModel = new FileListModel(this);
    
    if (m_type == FileSystemType::Local)
    {
        m_currentPath = getRootPath();
        connect(&FileTransferService::instance(), &FileTransferService::fileListResponse,
                this, &FileSystemBrowser::onFileListResponse);
    }
}

FileListModel* FileSystemBrowser::getFileListModel()
{
    return m_fileListModel;
}

QString FileSystemBrowser::getCurrentPath() const
{
    return m_currentPath;
}

void FileSystemBrowser::setCurrentPath(const QString& path)
{
    if (m_currentPath != path)
    {
        m_currentPath = path;
        m_pathHistory.append(path);
        emit currentPathChanged(path);
        refresh();
    }
}

FileSystemType FileSystemBrowser::getType() const
{
    return m_type;
}

void FileSystemBrowser::setTargetDevice(const QString& deviceId)
{
    m_targetDeviceId = deviceId;
}

QString FileSystemBrowser::getTargetDevice() const
{
    return m_targetDeviceId;
}

void FileSystemBrowser::refresh()
{
    emit loadingStarted();
    
    if (m_type == FileSystemType::Local)
    {
        loadLocalFiles();
    }
    else
    {
        loadRemoteFiles();
    }
}

void FileSystemBrowser::navigateUp()
{
    if (!canNavigateUp())
    {
        return;
    }

    QDir dir(m_currentPath);
    if (dir.cdUp())
    {
        setCurrentPath(dir.absolutePath());
    }
}

void FileSystemBrowser::navigateToRoot()
{
    setCurrentPath(getRootPath());
}

void FileSystemBrowser::navigateTo(const QString& path)
{
    setCurrentPath(path);
}

QList<QString> FileSystemBrowser::getPathHistory() const
{
    return m_pathHistory;
}

bool FileSystemBrowser::canNavigateUp() const
{
    if (m_type == FileSystemType::Local)
    {
        QDir dir(m_currentPath);
        return dir.cdUp() && dir.absolutePath() != m_currentPath;
    }
    else
    {
        return !m_currentPath.isEmpty() && m_currentPath != "/";
    }
}

void FileSystemBrowser::onFileListResponse(const QString& requestId, const QList<FileInfoData>& files, bool success, const QString& errorMessage)
{
    if (requestId != m_pendingRequestId)
    {
        return;
    }

    m_pendingRequestId.clear();

    if (!success)
    {
        Logger::instance().error("获取远程文件列表失败: " + errorMessage);
        emit errorOccurred(errorMessage);
        emit loadingFinished();
        return;
    }

    QList<FileItem> fileItems;
    for (const FileInfoData& fileData : files)
    {
        fileItems.append(FileItem(fileData));
    }

    m_fileListModel->setFiles(fileItems);
    emit fileListUpdated(fileItems);
    emit loadingFinished();
    
    Logger::instance().info("获取远程文件列表成功: " + QString::number(files.size()) + " 个文件");
}

void FileSystemBrowser::loadLocalFiles()
{
    QDir dir(m_currentPath);
    
    if (!dir.exists())
    {
        dir.setPath(getRootPath());
        m_currentPath = dir.absolutePath();
    }

    QList<FileItem> files;

    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name | QDir::DirsFirst);
    
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
        item.updateDisplayInfo();
        
        files.append(item);
    }

    m_fileListModel->setFiles(files);
    emit fileListUpdated(files);
    emit loadingFinished();
    
    Logger::instance().info("加载本地文件列表: " + m_currentPath + " (" + QString::number(files.size()) + " 个文件)");
}

void FileSystemBrowser::loadRemoteFiles()
{
    if (m_targetDeviceId.isEmpty())
    {
        emit errorOccurred("未设置目标设备");
        emit loadingFinished();
        return;
    }

    QString path = m_currentPath.isEmpty() ? "/" : m_currentPath;
    m_pendingRequestId = FileTransferService::instance().requestFileList(m_targetDeviceId, path);
    
    Logger::instance().info("请求远程文件列表: " + path + " 从设备 " + m_targetDeviceId);
}

QString FileSystemBrowser::getRootPath() const
{
    if (m_type == FileSystemType::Local)
    {
#ifdef Q_OS_WIN
        return "C:/";
#else
        return QDir::rootPath();
#endif
    }
    else
    {
        return "/";
    }
}