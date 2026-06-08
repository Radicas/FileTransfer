/**
 * @file messagetypes.h
 * @brief 消息类型定义，定义系统中所有模块间通信的消息类型
 * @date 2026-03-08
 * @version 1.0
 * 
 * @details 该文件定义了消息总线系统中使用的所有消息类型，
 * 采用枚举方式定义，便于扩展和维护。
 */

#ifndef MESSAGETYPES_H
#define MESSAGETYPES_H

#include <QDateTime>
#include <QString>
#include <QVariant>

/**
 * @brief 消息类型枚举
 * 
 * @details 定义系统中所有模块间通信的消息类型
 * 按模块功能分组，便于管理和扩展
 */
enum class MessageType
{
    None = 0,

    DataMessagesStart = 1000,
    DataAdded,
    DataUpdated,
    DataDeleted,
    DataChanged,
    DataMessagesEnd,

    FileMessagesStart = 2000,
    FileOpened,
    FileSaved,
    FileClosed,
    FileMessagesEnd,

    UIMessagesStart = 4000,
    UIThemeChanged,
    UILanguageChanged,
    UIStatusMessage,
    UIRefreshRequested,
    UIMessagesEnd,

    SystemMessagesStart = 9000,
    SystemShutdownRequested,
    SystemErrorOccurred,
    SystemMessagesEnd
};

/**
 * @brief 消息优先级枚举
 */
enum class MessagePriority
{
    Low,
    Normal,
    High,
    Urgent
};

/**
 * @brief 消息数据结构
 * 
 * @details 封装消息的所有信息，包括类型、数据、时间戳等
 */
struct Message
{
    MessageType type;          ///< 消息类型
    QVariant data;             ///< 消息数据
    QString sender;            ///< 发送者标识
    MessagePriority priority;  ///< 消息优先级
    QDateTime timestamp;       ///< 时间戳
    QString correlationId;     ///< 关联ID，用于请求-响应模式

    Message() : type(MessageType::None), priority(MessagePriority::Normal), timestamp(QDateTime::currentDateTime()) {}

    Message(MessageType msgType, const QVariant& msgData, const QString& msgSender = QString())
        : type(msgType),
          data(msgData),
          sender(msgSender),
          priority(MessagePriority::Normal),
          timestamp(QDateTime::currentDateTime())
    {
    }
};

#endif  // MESSAGETYPES_H
