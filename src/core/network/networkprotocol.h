/**
 * @file networkprotocol.h
 * @brief 网络通信协议定义
 * @date 2026-06-01
 * @version 1.0
 * 
 * @details 定义设备发现和文件传输的通信协议，
 * 包括消息类型、数据结构和常量定义。
 */

#ifndef NETWORKPROTOCOL_H
#define NETWORKPROTOCOL_H

#include <QByteArray>
#include <QHostAddress>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QRandomGenerator>

/**
 * @brief 网络消息类型枚举
 */
enum class NetworkMessageType
{
    None = 0,
    
    DiscoverStart = 100,
    DiscoverBroadcast,      ///< 设备发现广播消息
    DiscoverResponse,       ///< 设备发现响应消息
    DiscoverHeartbeat,      ///< 心跳消息
    DiscoverLeave,          ///< 设备离线消息
    
    FileRequestStart = 200,
    FileListRequest,        ///< 文件列表请求
    FileListResponse,       ///< 文件列表响应
    FileDownloadRequest,    ///< 文件下载请求
    FileDownloadResponse,   ///< 文件下载响应
    FileUploadRequest,      ///< 文件上传请求
    FileUploadResponse,     ///< 文件上传响应
    FileDeleteRequest,      ///< 文件删除请求
    FileDeleteResponse,     ///< 文件删除响应
    FileCreateDirRequest,   ///< 创建目录请求
    FileCreateDirResponse,  ///< 创建目录响应
    
    TransferStart = 300,
    TransferData,           ///< 文件数据块
    TransferComplete,       ///< 传输完成
    TransferError,          ///< 传输错误
    TransferCancel,         ///< 取消传输
    
    TransferRequestStart = 400,
    TransferPullRequest,    ///< 拉取文件请求（从对方下载）
    TransferPushRequest,    ///< 推送文件请求（向对方上传）
    TransferReady,          ///< 准备接收/发送
};

/**
 * @brief 网络常量定义
 */
namespace NetworkConstants
{
    constexpr int DISCOVER_PORT = 38888;        ///< 设备发现UDP端口
    constexpr int TRANSFER_PORT = 38889;        ///< 文件传输TCP端口
    constexpr int HEARTBEAT_INTERVAL = 5000;    ///< 心跳间隔（毫秒）
    constexpr int HEARTBEAT_TIMEOUT = 15000;    ///< 心跳超时（毫秒）
    constexpr int TRANSFER_CHUNK_SIZE = 65536;  ///< 文件传输块大小（64KB）
    constexpr int DISCOVER_INTERVAL = 3000;     ///< 发现广播间隔（毫秒）
    constexpr int MAX_RETRY_COUNT = 3;          ///< 最大重试次数
}

/**
 * @brief 设备信息数据结构（网络传输用）
 */
struct DeviceInfoData
{
    QString deviceId;       ///< 设备唯一标识
    QString deviceName;     ///< 设备名称
    QString ipAddress;      ///< IP地址
    int transferPort;       ///< 文件传输端口
    QString osType;         ///< 操作系统类型
    QString userName;       ///< 用户名
    
    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["deviceId"] = deviceId;
        obj["deviceName"] = deviceName;
        obj["ipAddress"] = ipAddress;
        obj["transferPort"] = transferPort;
        obj["osType"] = osType;
        obj["userName"] = userName;
        return obj;
    }
    
    static DeviceInfoData fromJson(const QJsonObject& obj)
    {
        DeviceInfoData data;
        data.deviceId = obj["deviceId"].toString();
        data.deviceName = obj["deviceName"].toString();
        data.ipAddress = obj["ipAddress"].toString();
        data.transferPort = obj["transferPort"].toInt(NetworkConstants::TRANSFER_PORT);
        data.osType = obj["osType"].toString();
        data.userName = obj["userName"].toString();
        return data;
    }
};

/**
 * @brief 文件信息数据结构
 */
struct FileInfoData
{
    QString name;           ///< 文件/目录名称
    QString path;           ///< 完整路径
    bool isDirectory;       ///< 是否为目录
    qint64 size;            ///< 文件大小（字节）
    QString lastModified;   ///< 最后修改时间
    bool isReadable;        ///< 是否可读
    bool isWritable;        ///< 是否可写
    
    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["name"] = name;
        obj["path"] = path;
        obj["isDirectory"] = isDirectory;
        obj["size"] = size;
        obj["lastModified"] = lastModified;
        obj["isReadable"] = isReadable;
        obj["isWritable"] = isWritable;
        return obj;
    }
    
    static FileInfoData fromJson(const QJsonObject& obj)
    {
        FileInfoData data;
        data.name = obj["name"].toString();
        data.path = obj["path"].toString();
        data.isDirectory = obj["isDirectory"].toBool();
        data.size = obj["size"].toVariant().toLongLong();
        data.lastModified = obj["lastModified"].toString();
        data.isReadable = obj["isReadable"].toBool(true);
        data.isWritable = obj["isWritable"].toBool(true);
        return data;
    }
};

/**
 * @brief 网络消息封装类
 */
class NetworkMessage
{
public:
    NetworkMessageType type;    ///< 消息类型
    QString messageId;          ///< 消息唯一标识
    QJsonObject data;           ///< 消息数据
    QString senderId;           ///< 发送者设备ID
    
    NetworkMessage() : type(NetworkMessageType::None) {}
    
    NetworkMessage(NetworkMessageType msgType, const QJsonObject& msgData, const QString& sender = QString())
        : type(msgType), data(msgData), senderId(sender)
    {
        messageId = generateMessageId();
    }
    
    QByteArray serialize() const
    {
        QJsonObject obj;
        obj["type"] = static_cast<int>(type);
        obj["messageId"] = messageId;
        obj["senderId"] = senderId;
        obj["data"] = data;
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
    
    static NetworkMessage deserialize(const QByteArray& bytes)
    {
        NetworkMessage msg;
        QJsonDocument doc = QJsonDocument::fromJson(bytes);
        if (!doc.isNull() && doc.isObject())
        {
            QJsonObject obj = doc.object();
            msg.type = static_cast<NetworkMessageType>(obj["type"].toInt());
            msg.messageId = obj["messageId"].toString();
            msg.senderId = obj["senderId"].toString();
            msg.data = obj["data"].toObject();
        }
        return msg;
    }
    
    static QString generateMessageId()
    {
        return QString::number(QDateTime::currentMSecsSinceEpoch()) + "_" + 
               QString::number(QRandomGenerator::global()->bounded(10000));
    }
};

#endif  // NETWORKPROTOCOL_H