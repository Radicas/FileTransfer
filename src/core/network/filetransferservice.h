/**
 * @file filetransferservice.h
 * @brief 文件传输服务类定义
 * @date 2026-06-01
 * @version 1.0
 * 
 * @details 提供基于TCP的文件传输服务，支持：
 * - 文件列表查询
 * - 文件下载/上传
 * - 目录创建/删除
 * - 分块传输和进度报告
 */

#ifndef FILETRANSFERSERVICE_H
#define FILETRANSFERSERVICE_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QQueue>
#include "core/network/networkprotocol.h"

/**
 * @brief 传输方向枚举
 */
enum class TransferDirection
{
    Upload,     ///< 上传（发送文件到对方）
    Download    ///< 下载（从对方接收文件）
};

/**
 * @brief 传输状态枚举
 */
enum class TransferState
{
    Pending,    ///< 等待中
    Preparing,  ///< 准备中
    Running,    ///< 进行中
    Paused,     ///< 已暂停
    Completed,  ///< 已完成
    Failed,     ///< 已失败
    Cancelled   ///< 已取消
};

/**
 * @brief 传输任务信息结构
 */
struct TransferTaskInfo
{
    QString taskId;             ///< 任务唯一标识
    QString sourcePath;         ///< 源文件路径
    QString targetPath;         ///< 目标文件路径
    QString targetDeviceId;     ///< 目标设备ID
    TransferDirection direction;///< 传输方向
    TransferState state;        ///< 传输状态
    qint64 totalSize;           ///< 文件总大小
    qint64 transferredSize;     ///< 已传输大小
    int progress;               ///< 进度百分比
    QString errorMessage;       ///< 错误消息
    qint64 startTime;           ///< 开始时间
    qint64 endTime;             ///< 结束时间
    
    TransferTaskInfo()
        : direction(TransferDirection::Download)
        , state(TransferState::Pending)
        , totalSize(0)
        , transferredSize(0)
        , progress(0)
        , startTime(0)
        , endTime(0)
    {}
};

/**
 * @class FileTransferService
 * @brief 文件传输服务类
 * 
 * @details 该类负责：
 * - 作为服务端监听文件传输请求
 * - 作为客户端发起文件传输请求
 * - 处理文件列表查询、下载、上传等操作
 * - 提供传输进度报告
 */
class FileTransferService : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return FileTransferService单例引用
     */
    static FileTransferService& instance();

    /**
     * @brief 初始化文件传输服务
     * @return 是否初始化成功
     */
    bool initialize();

    /**
     * @brief 启动传输服务监听
     */
    void start();

    /**
     * @brief 停止传输服务
     */
    void stop();

    /**
     * @brief 获取传输端口
     * @return 传输端口
     */
    quint16 getTransferPort() const;

    /**
     * @brief 请求远程文件列表
     * @param deviceId 目标设备ID
     * @param path 目录路径
     * @return 请求ID
     */
    QString requestFileList(const QString& deviceId, const QString& path);

    /**
     * @brief 请求下载文件
     * @param deviceId 目标设备ID
     * @param remotePath 远程文件路径
     * @param localPath 本地保存路径
     * @return 任务ID
     */
    QString requestDownload(const QString& deviceId, const QString& remotePath, const QString& localPath);

    /**
     * @brief 请求上传文件
     * @param deviceId 目标设备ID
     * @param localPath 本地文件路径
     * @param remotePath 远程保存路径
     * @return 任务ID
     */
    QString requestUpload(const QString& deviceId, const QString& localPath, const QString& remotePath);

    /**
     * @brief 取消传输任务
     * @param taskId 任务ID
     */
    void cancelTransfer(const QString& taskId);

    /**
     * @brief 获取传输任务信息
     * @param taskId 任务ID
     * @return 任务信息
     */
    TransferTaskInfo getTransferInfo(const QString& taskId) const;

    /**
     * @brief 获取所有传输任务
     * @return 任务列表
     */
    QList<TransferTaskInfo> getAllTransfers() const;

    /**
     * @brief 获取正在进行的传输任务数量
     * @return 任务数量
     */
    int getActiveTransferCount() const;

signals:
    /**
     * @brief 文件列表响应信号
     * @param requestId 请求ID
     * @param files 文件信息列表
     * @param success 是否成功
     * @param errorMessage 错误消息
     */
    void fileListResponse(const QString& requestId, const QList<FileInfoData>& files, bool success, const QString& errorMessage);

    /**
     * @brief 传输进度更新信号
     * @param taskId 任务ID
     * @param transferredSize 已传输大小
     * @param totalSize 总大小
     * @param progress 进度百分比
     */
    void transferProgress(const QString& taskId, qint64 transferredSize, qint64 totalSize, int progress);

    /**
     * @brief 传输完成信号
     * @param taskId 任务ID
     * @param success 是否成功
     * @param errorMessage 错误消息
     */
    void transferCompleted(const QString& taskId, bool success, const QString& errorMessage);

    /**
     * @brief 传输状态变化信号
     * @param taskId 任务ID
     * @param state 新状态
     */
    void transferStateChanged(const QString& taskId, TransferState state);

    /**
     * @brief 服务启动信号
     */
    void serviceStarted();

    /**
     * @brief 服务停止信号
     */
    void serviceStopped();

    /**
     * @brief 错误发生信号
     * @param errorMessage 错误消息
     */
    void errorOccurred(const QString& errorMessage);

private slots:
    /**
     * @brief 处理新连接
     */
    void onNewConnection();

    /**
     * @brief 处理客户端数据接收
     */
    void onClientReadyRead();

    /**
     * @brief 处理客户端断开连接
     */
    void onClientDisconnected();

    /**
     * @brief 处理服务端数据接收
     */
    void onServerReadyRead();

    /**
     * @brief 处理服务端断开连接
     */
    void onServerDisconnected();

    /**
     * @brief 发送数据块
     */
    void onSendDataChunk();

private:
    FileTransferService();
    ~FileTransferService();
    FileTransferService(const FileTransferService&) = delete;
    FileTransferService& operator=(const FileTransferService&) = delete;

    /**
     * @brief 连接到远程设备
     * @param deviceId 设备ID
     * @return TCP套接字
     */
    QTcpSocket* connectToDevice(const QString& deviceId);

    /**
     * @brief 发送消息到服务端
     * @param socket TCP套接字
     * @param message 网络消息
     */
    void sendMessageToServer(QTcpSocket* socket, const NetworkMessage& message);

    /**
     * @brief 发送消息到客户端
     * @param socket TCP套接字
     * @param message 网络消息
     */
    void sendMessageToClient(QTcpSocket* socket, const NetworkMessage& message);

    /**
     * @brief 处理服务端请求
     * @param socket 客户端套接字
     * @param message 网络消息
     */
    void processServerRequest(QTcpSocket* socket, const NetworkMessage& message);

    /**
     * @brief 处理文件列表请求
     * @param socket 客户端套接字
     * @param message 网络消息
     */
    void processFileListRequest(QTcpSocket* socket, const NetworkMessage& message);

    /**
     * @brief 处理下载请求
     * @param socket 客户端套接字
     * @param message 网络消息
     */
    void processDownloadRequest(QTcpSocket* socket, const NetworkMessage& message);

    /**
     * @brief 处理上传请求
     * @param socket 客户端套接字
     * @param message 网络消息
     */
    void processUploadRequest(QTcpSocket* socket, const NetworkMessage& message);

    /**
     * @brief 处理传输数据
     * @param socket 客户端套接字
     * @param message 网络消息
     */
    void processTransferData(QTcpSocket* socket, const NetworkMessage& message);

    /**
     * @brief 处理传输完成
     * @param socket 客户端套接字
     * @param message 网络消息
     */
    void processTransferComplete(QTcpSocket* socket, const NetworkMessage& message);

    /**
     * @brief 处理传输取消
     * @param socket 客户端套接字
     * @param message 网络消息
     */
    void processTransferCancel(QTcpSocket* socket, const NetworkMessage& message);

    /**
     * @brief 处理客户端响应
     * @param socket 服务端套接字
     * @param message 网络消息
     */
    void processClientResponse(QTcpSocket* socket, const NetworkMessage& message);

    /**
     * @brief 获取本地文件列表
     * @param path 目录路径
     * @return 文件信息列表
     */
    QList<FileInfoData> getLocalFileList(const QString& path);

    /**
     * @brief 生成任务ID
     * @return 任务ID
     */
    QString generateTaskId();

    /**
     * @brief 更新传输进度
     * @param taskId 任务ID
     * @param transferred 已传输大小
     */
    void updateTransferProgress(const QString& taskId, qint64 transferred);

    /**
     * @brief 完成传输任务
     * @param taskId 任务ID
     * @param success 是否成功
     * @param errorMessage 错误消息
     */
    void completeTransfer(const QString& taskId, bool success, const QString& errorMessage = QString());

    QTcpServer* m_tcpServer;                            ///< TCP服务器
    QMap<QTcpSocket*, QString> m_clientConnections;     ///< 客户端连接映射（socket -> deviceId）
    QMap<QString, QTcpSocket*> m_serverConnections;     ///< 服务端连接映射（deviceId -> socket）
    QMap<QString, TransferTaskInfo> m_transferTasks;    ///< 传输任务列表
    QMap<QString, QFile*> m_activeFiles;                ///< 正在传输的文件
    QMap<QString, QString> m_pendingRequests;           ///< 待处理的请求（requestId -> deviceId）
    QMap<QString, QList<FileInfoData>> m_pendingFileLists; ///< 待处理的文件列表响应
    quint16 m_transferPort;                             ///< 传输端口
    bool m_running;                                     ///< 运行状态标志
};

#endif  // FILETRANSFERSERVICE_H