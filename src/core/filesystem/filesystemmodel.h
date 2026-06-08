/**
 * @file filesystemmodel.h
 * @brief 文件系统模型类定义
 * @date 2026-06-01
 * @version 1.0
 * 
 * @details 提供本地和远程文件系统的浏览模型，支持：
 * - 本地文件系统浏览
 * - 远程文件系统浏览
 * - 文件/目录信息展示
 * - 路径导航
 */

#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QObject>
#include <QAbstractListModel>
#include <QAbstractTableModel>
#include <QDir>
#include <QFileIconProvider>
#include <QIcon>
#include <QMap>
#include "core/network/networkprotocol.h"

/**
 * @brief 文件系统类型枚举
 */
enum class FileSystemType
{
    Local,      ///< 本地文件系统
    Remote      ///< 远程文件系统
};

/**
 * @class FileItem
 * @brief 文件项数据结构
 * 
 * @details 存储单个文件或目录的完整信息。
 */
class FileItem
{
public:
    QString name;           ///< 文件/目录名称
    QString path;           ///< 完整路径
    QString displayPath;    ///< 显示路径（相对路径）
    bool isDirectory;       ///< 是否为目录
    qint64 size;            ///< 文件大小（字节）
    QString sizeDisplay;    ///< 显示大小（格式化）
    QString lastModified;   ///< 最后修改时间
    QString type;           ///< 文件类型描述
    QIcon icon;             ///< 文件图标
    bool isReadable;        ///< 是否可读
    bool isWritable;        ///< 是否可写
    bool isSelected;        ///< 是否选中

    FileItem() : isDirectory(false), size(0), isReadable(false), isWritable(false), isSelected(false) {}

    FileItem(const FileInfoData& data)
        : name(data.name)
        , path(data.path)
        , isDirectory(data.isDirectory)
        , size(data.size)
        , lastModified(data.lastModified)
        , isReadable(data.isReadable)
        , isWritable(data.isWritable)
        , isSelected(false)
    {
        updateDisplayInfo();
    }

    void updateDisplayInfo()
    {
        if (isDirectory)
        {
            sizeDisplay = "";
            type = "文件夹";
        }
        else
        {
            sizeDisplay = formatSize(size);
            type = getFileType(name);
        }
    }

    static QString formatSize(qint64 bytes)
    {
        if (bytes < 1024)
        {
            return QString::number(bytes) + " B";
        }
        else if (bytes < 1024 * 1024)
        {
            return QString::number(bytes / 1024.0, 'f', 1) + " KB";
        }
        else if (bytes < 1024 * 1024 * 1024)
        {
            return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
        }
        else
        {
            return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 1) + " GB";
        }
    }

    static QString getFileType(const QString& fileName)
    {
        if (fileName.contains('.'))
        {
            QString ext = fileName.split('.').last().toLower();
            
            if (ext == "txt" || ext == "doc" || ext == "docx" || ext == "pdf")
            {
                return "文档";
            }
            else if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp")
            {
                return "图片";
            }
            else if (ext == "mp3" || ext == "wav" || ext == "flac" || ext == "aac")
            {
                return "音频";
            }
            else if (ext == "mp4" || ext == "avi" || ext == "mkv" || ext == "mov")
            {
                return "视频";
            }
            else if (ext == "zip" || ext == "rar" || ext == "7z" || ext == "tar" || ext == "gz")
            {
                return "压缩文件";
            }
            else if (ext == "exe" || ext == "msi" || ext == "app" || ext == "dmg")
            {
                return "可执行文件";
            }
            else if (ext == "cpp" || ext == "h" || ext == "py" || ext == "js" || ext == "java")
            {
                return "源代码";
            }
            else
            {
                return ext.toUpper() + " 文件";
            }
        }
        
        return "文件";
    }

    FileInfoData toFileInfoData() const
    {
        FileInfoData data;
        data.name = name;
        data.path = path;
        data.isDirectory = isDirectory;
        data.size = size;
        data.lastModified = lastModified;
        data.isReadable = isReadable;
        data.isWritable = isWritable;
        return data;
    }
};

/**
 * @class FileListModel
 * @brief 文件列表模型类
 * 
 * @details 为Qt视图组件提供文件列表数据的模型。
 */
class FileListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum FileRoles
    {
        NameRole = Qt::UserRole + 1,
        PathRole,
        IsDirectoryRole,
        SizeRole,
        SizeDisplayRole,
        LastModifiedRole,
        TypeRole,
        IconRole,
        IsReadableRole,
        IsWritableRole,
        IsSelectedRole
    };

    enum Columns
    {
        ColName = 0,
        ColSize,
        ColType,
        ColModified,
        ColCount
    };

    explicit FileListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setFiles(const QList<FileItem>& files);
    void clearFiles();
    FileItem getFile(int index) const;
    FileItem getFile(const QString& path) const;
    int findFileIndex(const QString& path) const;
    QList<FileItem> getSelectedFiles() const;
    void setSelected(int index, bool selected);
    void clearSelection();
    void selectAll();

private:
    QList<FileItem> m_files;
    QMap<QString, int> m_fileIndexMap;
    QFileIconProvider m_iconProvider;
};

/**
 * @class FileSystemBrowser
 * @brief 文件系统浏览器类
 * 
 * @details 提供文件系统浏览功能，支持本地和远程文件系统。
 */
class FileSystemBrowser : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param type 文件系统类型
     * @param parent 父对象
     */
    explicit FileSystemBrowser(FileSystemType type, QObject* parent = nullptr);

    /**
     * @brief 获取文件列表模型
     * @return 文件列表模型指针
     */
    FileListModel* getFileListModel();

    /**
     * @brief 获取当前路径
     * @return 当前路径
     */
    QString getCurrentPath() const;

    /**
     * @brief 设置当前路径
     * @param path 新路径
     */
    void setCurrentPath(const QString& path);

    /**
     * @brief 获取文件系统类型
     * @return 文件系统类型
     */
    FileSystemType getType() const;

    /**
     * @brief 设置目标设备ID（仅远程文件系统）
     * @param deviceId 设备ID
     */
    void setTargetDevice(const QString& deviceId);

    /**
     * @brief 获取目标设备ID
     * @return 设备ID
     */
    QString getTargetDevice() const;

    /**
     * @brief 刷新当前目录
     */
    void refresh();

    /**
     * @brief 导航到上级目录
     */
    void navigateUp();

    /**
     * @brief 导航到根目录
     */
    void navigateToRoot();

    /**
     * @brief 导航到指定目录
     * @param path 目录路径
     */
    void navigateTo(const QString& path);

    /**
     * @brief 获取路径历史
     * @return 路径历史列表
     */
    QList<QString> getPathHistory() const;

    /**
     * @brief 获取是否可以向上导航
     * @return 是否可以向上
     */
    bool canNavigateUp() const;

signals:
    /**
     * @brief 当前路径变化信号
     * @param path 新路径
     */
    void currentPathChanged(const QString& path);

    /**
     * @brief 文件列表更新信号
     * @param files 文件列表
     */
    void fileListUpdated(const QList<FileItem>& files);

    /**
     * @brief 错误发生信号
     * @param errorMessage 错误消息
     */
    void errorOccurred(const QString& errorMessage);

    /**
     * @brief 加载开始信号
     */
    void loadingStarted();

    /**
     * @brief 加载完成信号
     */
    void loadingFinished();

private slots:
    void onFileListResponse(const QString& requestId, const QList<FileInfoData>& files, bool success, const QString& errorMessage);

private:
    void loadLocalFiles();
    void loadRemoteFiles();
    QString getRootPath() const;

    FileSystemType m_type;              ///< 文件系统类型
    FileListModel* m_fileListModel;    ///< 文件列表模型
    QString m_currentPath;             ///< 当前路径
    QString m_targetDeviceId;          ///< 目标设备ID（远程）
    QList<QString> m_pathHistory;      ///< 路径历史
    QString m_pendingRequestId;        ///< 待处理的请求ID
};

#endif  // FILESYSTEMMODEL_H