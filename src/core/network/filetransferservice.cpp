/**
 * @file filetransferservice.cpp
 * @brief 文件传输服务类实现
 */

#include "core/network/filetransferservice.h"
#include "core/network/devicediscoverer.h"
#include "common/logger/logger.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QCoreApplication>

FileTransferService& FileTransferService::instance()
{
    static FileTransferService instance;
    return instance;
}

FileTransferService::FileTransferService()
    : m_tcpServer(nullptr)
    , m_transferPort(NetworkConstants::TRANSFER_PORT)
    , m_running(false)
{
    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &FileTransferService::onNewConnection);
}

FileTransferService::~FileTransferService()
{
    stop();
    
    for (auto it = m_activeFiles.begin(); it != m_activeFiles.end(); ++it)
    {
        if (it.value())
        {
            it.value()->close();
            delete it.value();
        }
    }
    m_activeFiles.clear();
}

bool FileTransferService::initialize()
{
    Logger::instance().info("文件传输服务初始化");
    return true;
}

void FileTransferService::start()
{
    if (m_running)
    {
        return;
    }

    if (!m_tcpServer->listen(QHostAddress::Any, m_transferPort))
    {
        Logger::instance().error("文件传输服务启动失败: " + m_tcpServer->errorString());
        emit errorOccurred("无法启动文件传输服务: " + m_tcpServer->errorString());
        return;
    }

    m_running = true;
    Logger::instance().info("文件传输服务已启动，监听端口: " + QString::number(m_transferPort));
    emit serviceStarted();
}

void FileTransferService::stop()
{
    if (!m_running)
    {
        return;
    }

    m_tcpServer->close();

    for (QTcpSocket* socket : m_clientConnections.keys())
    {
        socket->disconnectFromHost();
    }
    m_clientConnections.clear();

    for (QTcpSocket* socket : m_serverConnections.values())
    {
        socket->disconnectFromHost();
    }
    m_serverConnections.clear();

    for (auto it = m_transferTasks.begin(); it != m_transferTasks.end(); ++it)
    {
        if (it.value().state == TransferState::Running || it.value().state == TransferState::Preparing)
        {
            completeTransfer(it.key(), false, "服务停止");
        }
    }

    m_running = false;
    Logger::instance().info("文件传输服务已停止");
    emit serviceStopped();
}

quint16 FileTransferService::getTransferPort() const
{
    return m_transferPort;
}

QString FileTransferService::requestFileList(const QString& deviceId, const QString& path)
{
    QString requestId = generateTaskId();
    m_pendingRequests.insert(requestId, deviceId);

    QTcpSocket* socket = connectToDevice(deviceId);
    if (!socket)
    {
        m_pendingRequests.remove(requestId);
        emit fileListResponse(requestId, QList<FileInfoData>(), false, "无法连接到设备");
        return requestId;
    }

    QJsonObject data;
    data["requestId"] = requestId;
    data["path"] = path;

    NetworkMessage msg(NetworkMessageType::FileListRequest, data);
    sendMessageToServer(socket, msg);

    Logger::instance().info("请求文件列表: " + path + " 从设备 " + deviceId);
    return requestId;
}

QString FileTransferService::requestDownload(const QString& deviceId, const QString& remotePath, const QString& localPath)
{
    QString taskId = generateTaskId();
    
    TransferTaskInfo task;
    task.taskId = taskId;
    task.sourcePath = remotePath;
    task.targetPath = localPath;
    task.targetDeviceId = deviceId;
    task.direction = TransferDirection::Download;
    task.state = TransferState::Pending;
    task.startTime = QDateTime::currentMSecsSinceEpoch();
    
    m_transferTasks.insert(taskId, task);

    QTcpSocket* socket = connectToDevice(deviceId);
    if (!socket)
    {
        completeTransfer(taskId, false, "无法连接到设备");
        return taskId;
    }

    QJsonObject data;
    data["taskId"] = taskId;
    data["filePath"] = remotePath;

    NetworkMessage msg(NetworkMessageType::TransferPullRequest, data);
    sendMessageToServer(socket, msg);

    Logger::instance().info("请求下载文件: " + remotePath + " 到 " + localPath);
    emit transferStateChanged(taskId, TransferState::Preparing);
    return taskId;
}

QString FileTransferService::requestUpload(const QString& deviceId, const QString& localPath, const QString& remotePath)
{
    QString taskId = generateTaskId();
    
    QFileInfo fileInfo(localPath);
    if (!fileInfo.exists() || !fileInfo.isReadable())
    {
        Logger::instance().error("上传文件不存在或不可读: " + localPath);
        return taskId;
    }

    TransferTaskInfo task;
    task.taskId = taskId;
    task.sourcePath = localPath;
    task.targetPath = remotePath;
    task.targetDeviceId = deviceId;
    task.direction = TransferDirection::Upload;
    task.state = TransferState::Pending;
    task.totalSize = fileInfo.size();
    task.startTime = QDateTime::currentMSecsSinceEpoch();
    
    m_transferTasks.insert(taskId, task);

    QTcpSocket* socket = connectToDevice(deviceId);
    if (!socket)
    {
        completeTransfer(taskId, false, "无法连接到设备");
        return taskId;
    }

    QJsonObject data;
    data["taskId"] = taskId;
    data["filePath"] = remotePath;
    data["fileSize"] = fileInfo.size();
    data["fileName"] = fileInfo.fileName();

    NetworkMessage msg(NetworkMessageType::TransferPushRequest, data);
    sendMessageToServer(socket, msg);

    Logger::instance().info("请求上传文件: " + localPath + " 到 " + remotePath);
    emit transferStateChanged(taskId, TransferState::Preparing);
    return taskId;
}

void FileTransferService::cancelTransfer(const QString& taskId)
{
    if (!m_transferTasks.contains(taskId))
    {
        return;
    }

    TransferTaskInfo& task = m_transferTasks[taskId];
    if (task.state == TransferState::Completed || task.state == TransferState::Failed || task.state == TransferState::Cancelled)
    {
        return;
    }

    task.state = TransferState::Cancelled;
    emit transferStateChanged(taskId, TransferState::Cancelled);

    QTcpSocket* socket = m_serverConnections.value(task.targetDeviceId);
    if (socket)
    {
        QJsonObject data;
        data["taskId"] = taskId;
        NetworkMessage msg(NetworkMessageType::TransferCancel, data);
        sendMessageToServer(socket, msg);
    }

    if (m_activeFiles.contains(taskId))
    {
        QFile* file = m_activeFiles.take(taskId);
        file->close();
        delete file;
    }

    Logger::instance().info("取消传输任务: " + taskId);
    completeTransfer(taskId, false, "用户取消");
}

TransferTaskInfo FileTransferService::getTransferInfo(const QString& taskId) const
{
    return m_transferTasks.value(taskId);
}

QList<TransferTaskInfo> FileTransferService::getAllTransfers() const
{
    return m_transferTasks.values();
}

int FileTransferService::getActiveTransferCount() const
{
    int count = 0;
    for (const TransferTaskInfo& task : m_transferTasks.values())
    {
        if (task.state == TransferState::Running || task.state == TransferState::Preparing)
        {
            count++;
        }
    }
    return count;
}

void FileTransferService::onNewConnection()
{
    while (m_tcpServer->hasPendingConnections())
    {
        QTcpSocket* socket = m_tcpServer->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, &FileTransferService::onClientReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &FileTransferService::onClientDisconnected);
        
        QString clientId = "client_" + QString::number(socket->socketDescriptor());
        m_clientConnections.insert(socket, clientId);
        
        Logger::instance().info("新客户端连接: " + clientId);
    }
}

void FileTransferService::onClientReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
    {
        return;
    }

    QByteArray data = socket->readAll();
    NetworkMessage message = NetworkMessage::deserialize(data);
    processServerRequest(socket, message);
}

void FileTransferService::onClientDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
    {
        return;
    }

    QString clientId = m_clientConnections.value(socket);
    Logger::instance().info("客户端断开连接: " + clientId);
    
    m_clientConnections.remove(socket);
    socket->deleteLater();
}

void FileTransferService::onServerReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
    {
        return;
    }

    QByteArray data = socket->readAll();
    NetworkMessage message = NetworkMessage::deserialize(data);
    processClientResponse(socket, message);
}

void FileTransferService::onServerDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
    {
        return;
    }

    QString deviceId;
    for (auto it = m_serverConnections.begin(); it != m_serverConnections.end(); ++it)
    {
        if (it.value() == socket)
        {
            deviceId = it.key();
            break;
        }
    }

    Logger::instance().info("与服务端断开连接: " + deviceId);
    
    if (!deviceId.isEmpty())
    {
        m_serverConnections.remove(deviceId);
    }
    
    socket->deleteLater();
}

void FileTransferService::onSendDataChunk()
{
    QString taskId;
    for (auto it = m_transferTasks.begin(); it != m_transferTasks.end(); ++it)
    {
        if (it.value().state == TransferState::Running && it.value().direction == TransferDirection::Upload)
        {
            taskId = it.key();
            break;
        }
    }

    if (taskId.isEmpty())
    {
        return;
    }

    TransferTaskInfo& task = m_transferTasks[taskId];
    QFile* file = m_activeFiles.value(taskId);
    
    if (!file || !file->isOpen())
    {
        completeTransfer(taskId, false, "文件打开失败");
        return;
    }

    QByteArray chunk = file->read(NetworkConstants::TRANSFER_CHUNK_SIZE);
    
    if (chunk.isEmpty())
    {
        QJsonObject data;
        data["taskId"] = taskId;
        NetworkMessage msg(NetworkMessageType::TransferComplete, data);
        
        QTcpSocket* socket = m_serverConnections.value(task.targetDeviceId);
        if (socket)
        {
            sendMessageToServer(socket, msg);
        }
        
        file->close();
        m_activeFiles.remove(taskId);
        delete file;
        
        completeTransfer(taskId, true);
        return;
    }

    QJsonObject data;
    data["taskId"] = taskId;
    data["size"] = chunk.size();
    
    NetworkMessage msg(NetworkMessageType::TransferData, data);
    
    QTcpSocket* socket = m_serverConnections.value(task.targetDeviceId);
    if (socket)
    {
        QByteArray messageData = msg.serialize();
        socket->write(messageData);
        socket->write(chunk);
        socket->flush();
    }

    updateTransferProgress(taskId, task.transferredSize + chunk.size());
}

QTcpSocket* FileTransferService::connectToDevice(const QString& deviceId)
{
    if (m_serverConnections.contains(deviceId))
    {
        QTcpSocket* existingSocket = m_serverConnections[deviceId];
        if (existingSocket->state() == QAbstractSocket::ConnectedState)
        {
            return existingSocket;
        }
        m_serverConnections.remove(deviceId);
        existingSocket->deleteLater();
    }

    DeviceInfoData deviceInfo = DeviceDiscoverer::instance().getDeviceInfo(deviceId);
    if (deviceInfo.deviceId.isEmpty())
    {
        Logger::instance().error("设备不存在: " + deviceId);
        emit errorOccurred("设备不存在: " + deviceId);
        return nullptr;
    }

    Logger::instance().info("正在连接设备: " + deviceInfo.deviceName + " (" + deviceInfo.ipAddress + ")");

    QTcpSocket* socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::readyRead, this, &FileTransferService::onServerReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &FileTransferService::onServerDisconnected);
    connect(socket, &QAbstractSocket::errorOccurred, this, [this, deviceInfo](QAbstractSocket::SocketError error) {
        Logger::instance().error("连接设备错误: " + deviceInfo.deviceName + " - " + QString::number(error));
        emit errorOccurred("连接设备失败: " + deviceInfo.deviceName + " (" + deviceInfo.ipAddress + ")");
    });

    socket->connectToHost(deviceInfo.ipAddress, deviceInfo.transferPort);
    
    if (!socket->waitForConnected(5000))
    {
        Logger::instance().warning("连接设备超时: " + deviceInfo.ipAddress + ":" + QString::number(deviceInfo.transferPort) + 
                                   " - 请确认目标设备已运行FileTransfer且防火墙已开放端口");
        emit errorOccurred("连接超时: 请确认目标设备已运行程序并开放端口38889");
        socket->deleteLater();
        return nullptr;
    }

    m_serverConnections.insert(deviceId, socket);
    Logger::instance().info("已连接到设备: " + deviceId + " (" + deviceInfo.ipAddress + ")");
    return socket;
}

void FileTransferService::sendMessageToServer(QTcpSocket* socket, const NetworkMessage& message)
{
    if (socket && socket->state() == QAbstractSocket::ConnectedState)
    {
        socket->write(message.serialize());
        socket->flush();
    }
}

void FileTransferService::sendMessageToClient(QTcpSocket* socket, const NetworkMessage& message)
{
    if (socket && socket->state() == QAbstractSocket::ConnectedState)
    {
        socket->write(message.serialize());
        socket->flush();
    }
}

void FileTransferService::processServerRequest(QTcpSocket* socket, const NetworkMessage& message)
{
    switch (message.type)
    {
    case NetworkMessageType::FileListRequest:
        processFileListRequest(socket, message);
        break;
    case NetworkMessageType::TransferPullRequest:
        processDownloadRequest(socket, message);
        break;
    case NetworkMessageType::TransferPushRequest:
        processUploadRequest(socket, message);
        break;
    case NetworkMessageType::TransferData:
        processTransferData(socket, message);
        break;
    case NetworkMessageType::TransferComplete:
        processTransferComplete(socket, message);
        break;
    case NetworkMessageType::TransferCancel:
        processTransferCancel(socket, message);
        break;
    default:
        break;
    }
}

void FileTransferService::processFileListRequest(QTcpSocket* socket, const NetworkMessage& message)
{
    QString requestId = message.data["requestId"].toString();
    QString path = message.data["path"].toString();
    
    QList<FileInfoData> files = getLocalFileList(path);
    
    QJsonObject data;
    data["requestId"] = requestId;
    data["success"] = true;
    
    QJsonArray fileArray;
    for (const FileInfoData& file : files)
    {
        fileArray.append(file.toJson());
    }
    data["files"] = fileArray;
    
    NetworkMessage response(NetworkMessageType::FileListResponse, data);
    sendMessageToClient(socket, response);
    
    Logger::instance().info("响应文件列表请求: " + path + " (" + QString::number(files.size()) + " 个文件)");
}

void FileTransferService::processDownloadRequest(QTcpSocket* socket, const NetworkMessage& message)
{
    QString taskId = message.data["taskId"].toString();
    QString filePath = message.data["filePath"].toString();
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile())
    {
        QJsonObject data;
        data["taskId"] = taskId;
        data["success"] = false;
        data["error"] = "文件不存在";
        
        NetworkMessage response(NetworkMessageType::TransferReady, data);
        sendMessageToClient(socket, response);
        return;
    }

    QFile* file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly))
    {
        QJsonObject data;
        data["taskId"] = taskId;
        data["success"] = false;
        data["error"] = "无法打开文件";
        
        NetworkMessage response(NetworkMessageType::TransferReady, data);
        sendMessageToClient(socket, response);
        
        delete file;
        return;
    }

    m_activeFiles.insert(taskId, file);

    QJsonObject data;
    data["taskId"] = taskId;
    data["success"] = true;
    data["fileSize"] = fileInfo.size();
    
    NetworkMessage response(NetworkMessageType::TransferReady, data);
    sendMessageToClient(socket, response);
    
    Logger::instance().info("准备发送文件: " + filePath + " (大小: " + QString::number(fileInfo.size()) + ")");
}

void FileTransferService::processUploadRequest(QTcpSocket* socket, const NetworkMessage& message)
{
    QString taskId = message.data["taskId"].toString();
    QString filePath = message.data["filePath"].toString();
    qint64 fileSize = message.data["fileSize"].toVariant().toLongLong();
    QString fileName = message.data["fileName"].toString();
    
    QString fullPath = filePath;
    if (!filePath.endsWith("/") && !filePath.endsWith("\\"))
    {
        fullPath += "/";
    }
    fullPath += fileName;

    QFile* file = new QFile(fullPath);
    if (!file->open(QIODevice::WriteOnly))
    {
        QJsonObject data;
        data["taskId"] = taskId;
        data["success"] = false;
        data["error"] = "无法创建文件";
        
        NetworkMessage response(NetworkMessageType::TransferReady, data);
        sendMessageToClient(socket, response);
        
        delete file;
        return;
    }

    m_activeFiles.insert(taskId, file);

    TransferTaskInfo task;
    task.taskId = taskId;
    task.targetPath = fullPath;
    task.totalSize = fileSize;
    task.state = TransferState::Running;
    m_transferTasks.insert(taskId, task);

    QJsonObject data;
    data["taskId"] = taskId;
    data["success"] = true;
    
    NetworkMessage response(NetworkMessageType::TransferReady, data);
    sendMessageToClient(socket, response);
    
    Logger::instance().info("准备接收文件: " + fullPath);
}

void FileTransferService::processTransferData(QTcpSocket* socket, const NetworkMessage& message)
{
    QString taskId = message.data["taskId"].toString();
    qint64 dataSize = message.data["size"].toVariant().toLongLong();
    
    QByteArray chunkData = socket->read(dataSize);
    
    QFile* file = m_activeFiles.value(taskId);
    if (!file)
    {
        return;
    }

    file->write(chunkData);
    
    if (m_transferTasks.contains(taskId))
    {
        TransferTaskInfo& task = m_transferTasks[taskId];
        updateTransferProgress(taskId, task.transferredSize + dataSize);
    }
}

void FileTransferService::processTransferComplete(QTcpSocket* socket, const NetworkMessage& message)
{
    QString taskId = message.data["taskId"].toString();
    
    QFile* file = m_activeFiles.take(taskId);
    if (file)
    {
        file->close();
        delete file;
    }

    completeTransfer(taskId, true);
    
    QJsonObject data;
    data["taskId"] = taskId;
    data["success"] = true;
    NetworkMessage response(NetworkMessageType::TransferComplete, data);
    sendMessageToClient(socket, response);
}

void FileTransferService::processTransferCancel(QTcpSocket* socket, const NetworkMessage& message)
{
    QString taskId = message.data["taskId"].toString();
    
    QFile* file = m_activeFiles.take(taskId);
    if (file)
    {
        file->close();
        delete file;
    }

    if (m_transferTasks.contains(taskId))
    {
        completeTransfer(taskId, false, "对方取消传输");
    }
}

void FileTransferService::processClientResponse(QTcpSocket* socket, const NetworkMessage& message)
{
    switch (message.type)
    {
    case NetworkMessageType::FileListResponse:
    {
        QString requestId = message.data["requestId"].toString();
        bool success = message.data["success"].toBool();
        QString error = message.data["error"].toString();
        
        QList<FileInfoData> files;
        if (success)
        {
            QJsonArray fileArray = message.data["files"].toArray();
            for (const QJsonValue& value : fileArray)
            {
                files.append(FileInfoData::fromJson(value.toObject()));
            }
        }
        
        m_pendingRequests.remove(requestId);
        emit fileListResponse(requestId, files, success, error);
        break;
    }
    case NetworkMessageType::TransferReady:
    {
        QString taskId = message.data["taskId"].toString();
        bool success = message.data["success"].toBool();
        QString error = message.data["error"].toString();
        
        if (!success)
        {
            completeTransfer(taskId, false, error);
            return;
        }

        if (!m_transferTasks.contains(taskId))
        {
            return;
        }

        TransferTaskInfo& task = m_transferTasks[taskId];
        
        if (task.direction == TransferDirection::Download)
        {
            task.totalSize = message.data["fileSize"].toVariant().toLongLong();
            
            QFile* file = new QFile(task.targetPath);
            if (!file->open(QIODevice::WriteOnly))
            {
                completeTransfer(taskId, false, "无法创建本地文件");
                delete file;
                return;
            }
            
            m_activeFiles.insert(taskId, file);
            task.state = TransferState::Running;
            emit transferStateChanged(taskId, TransferState::Running);
            
            Logger::instance().info("开始下载文件: " + task.sourcePath);
        }
        else if (task.direction == TransferDirection::Upload)
        {
            QFile* file = new QFile(task.sourcePath);
            if (!file->open(QIODevice::ReadOnly))
            {
                completeTransfer(taskId, false, "无法打开本地文件");
                delete file;
                return;
            }
            
            m_activeFiles.insert(taskId, file);
            task.state = TransferState::Running;
            emit transferStateChanged(taskId, TransferState::Running);
            
            Logger::instance().info("开始上传文件: " + task.sourcePath);
            
            QTimer::singleShot(0, this, &FileTransferService::onSendDataChunk);
        }
        break;
    }
    case NetworkMessageType::TransferData:
    {
        QString taskId = message.data["taskId"].toString();
        qint64 dataSize = message.data["size"].toVariant().toLongLong();
        
        QByteArray chunkData = socket->read(dataSize);
        
        QFile* file = m_activeFiles.value(taskId);
        if (file)
        {
            file->write(chunkData);
            
            if (m_transferTasks.contains(taskId))
            {
                TransferTaskInfo& task = m_transferTasks[taskId];
                updateTransferProgress(taskId, task.transferredSize + dataSize);
            }
        }
        break;
    }
    case NetworkMessageType::TransferComplete:
    {
        QString taskId = message.data["taskId"].toString();
        
        QFile* file = m_activeFiles.take(taskId);
        if (file)
        {
            file->close();
            delete file;
        }
        
        completeTransfer(taskId, true);
        break;
    }
    default:
        break;
    }
}

QList<FileInfoData> FileTransferService::getLocalFileList(const QString& path)
{
    QList<FileInfoData> files;
    
    QDir dir(path);
    if (!dir.exists())
    {
        dir.setPath(QDir::rootPath());
    }

    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name | QDir::DirsFirst);
    
    for (const QFileInfo& entry : entries)
    {
        FileInfoData info;
        info.name = entry.fileName();
        info.path = entry.absoluteFilePath();
        info.isDirectory = entry.isDir();
        info.size = entry.size();
        info.lastModified = entry.lastModified().toString("yyyy-MM-dd hh:mm:ss");
        info.isReadable = entry.isReadable();
        info.isWritable = entry.isWritable();
        
        files.append(info);
    }

    FileInfoData parentInfo;
    parentInfo.name = "..";
    parentInfo.path = dir.absolutePath();
    parentInfo.isDirectory = true;
    files.prepend(parentInfo);

    return files;
}

QString FileTransferService::generateTaskId()
{
    return "task_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + "_" + 
           QString::number(QRandomGenerator::global()->bounded(10000));
}

void FileTransferService::updateTransferProgress(const QString& taskId, qint64 transferred)
{
    if (!m_transferTasks.contains(taskId))
    {
        return;
    }

    TransferTaskInfo& task = m_transferTasks[taskId];
    task.transferredSize = transferred;
    task.progress = static_cast<int>((transferred * 100) / task.totalSize);
    
    emit transferProgress(taskId, transferred, task.totalSize, task.progress);
}

void FileTransferService::completeTransfer(const QString& taskId, bool success, const QString& errorMessage)
{
    if (!m_transferTasks.contains(taskId))
    {
        return;
    }

    TransferTaskInfo& task = m_transferTasks[taskId];
    task.state = success ? TransferState::Completed : TransferState::Failed;
    task.endTime = QDateTime::currentMSecsSinceEpoch();
    task.errorMessage = errorMessage;
    
    emit transferStateChanged(taskId, task.state);
    emit transferCompleted(taskId, success, errorMessage);
    
    Logger::instance().info("传输任务完成: " + taskId + " (" + (success ? "成功" : "失败: " + errorMessage) + ")");
}