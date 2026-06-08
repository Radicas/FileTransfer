/**
 * @file messagebus.h
 * @brief 消息总线核心类，实现模块间的解耦通信
 * @date 2026-03-08
 * @version 1.0
 * 
 * @details 消息总线采用发布-订阅模式，提供：
 * - 类型安全的消息传递
 * - 支持同步和异步消息处理
 * - 支持消息过滤和优先级
 * - 线程安全
 * - 支持请求-响应模式
 */

#ifndef MESSAGEBUS_H
#define MESSAGEBUS_H

#include <QList>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QSet>
#include <functional>
#include "messagetypes.h"

class MessageBus;

/**
 * @brief 消息订阅者信息结构
 */
struct SubscriberInfo
{
    QObject* receiver;                            ///< 接收者对象
    std::function<void(const Message&)> handler;  ///< 消息处理函数
    int priority;                                 ///< 订阅优先级

    SubscriberInfo() : receiver(nullptr), priority(0) {}

    SubscriberInfo(QObject* obj, std::function<void(const Message&)> func, int prio = 0)
        : receiver(obj), handler(func), priority(prio)
    {
    }
};

/**
 * @brief 消息总线类，实现模块间的解耦通信
 * 
 * @details 该类采用单例模式，提供线程安全的消息传递机制。
 * 模块通过订阅特定类型的消息来接收通知，通过发布消息来发送通知。
 * 
 * @example
 * @code
 * // 订阅消息
 * MessageBus::instance().subscribe(MessageType::DataAdded, this, 
 *     [](const Message& msg) {
 *         // 处理数据添加消息
 *     });
 * 
 * // 发布消息
 * MessageBus::instance().publish(MessageType::DataAdded, 
 *     QVariant::fromValue(data), "DataManager");
 * @endcode
 */
class MessageBus : public QObject
{
    Q_OBJECT

   public:
    using MessageHandler = std::function<void(const Message&)>;

    /**
     * @brief 获取MessageBus单例实例
     * @return MessageBus单例引用
     */
    static MessageBus& instance();

    /**
     * @brief 订阅特定类型的消息
     * @param type 消息类型
     * @param receiver 接收者对象（用于生命周期管理）
     * @param handler 消息处理函数
     * @param priority 订阅优先级（数值越大优先级越高）
     * @return 订阅ID，用于取消订阅
     */
    int subscribe(MessageType type, QObject* receiver, MessageHandler handler, int priority = 0);

    /**
     * @brief 订阅多种类型的消息
     * @param types 消息类型列表
     * @param receiver 接收者对象
     * @param handler 消息处理函数
     * @param priority 订阅优先级
     * @return 订阅ID列表
     */
    QList<int> subscribeMultiple(const QList<MessageType>& types, QObject* receiver, MessageHandler handler,
                                 int priority = 0);

    /**
     * @brief 取消订阅
     * @param subscriptionId 订阅ID
     * @return 是否成功取消
     */
    bool unsubscribe(int subscriptionId);

    /**
     * @brief 取消某个接收者的所有订阅
     * @param receiver 接收者对象
     */
    void unsubscribeAll(QObject* receiver);

    /**
     * @brief 发布消息（同步方式）
     * @param type 消息类型
     * @param data 消息数据
     * @param sender 发送者标识
     * @param priority 消息优先级
     */
    void publish(MessageType type, const QVariant& data = QVariant(), const QString& sender = QString(),
                 MessagePriority priority = MessagePriority::Normal);

    /**
     * @brief 发布消息（异步方式，通过信号槽）
     * @param type 消息类型
     * @param data 消息数据
     * @param sender 发送者标识
     * @param priority 消息优先级
     */
    void publishAsync(MessageType type, const QVariant& data = QVariant(), const QString& sender = QString(),
                      MessagePriority priority = MessagePriority::Normal);

    /**
     * @brief 发布消息对象
     * @param message 消息对象
     */
    void publishMessage(const Message& message);

    /**
     * @brief 发送请求并等待响应（同步）
     * @param type 请求消息类型
     * @param data 请求数据
     * @param sender 发送者标识
     * @param timeout 超时时间（毫秒），-1表示无限等待
     * @return 响应消息
     */
    Message sendRequest(MessageType type, const QVariant& data = QVariant(), const QString& sender = QString(),
                        int timeout = 5000);

    /**
     * @brief 注册响应处理器
     * @param type 请求消息类型
     * @param receiver 接收者对象
     * @param handler 响应处理函数
     * @return 订阅ID
     */
    int registerResponseHandler(MessageType type, QObject* receiver, MessageHandler handler);

    /**
     * @brief 获取订阅者数量
     * @param type 消息类型
     * @return 订阅者数量
     */
    int subscriberCount(MessageType type) const;

    /**
     * @brief 清理已销毁对象的订阅
     */
    void cleanup();

    /**
     * @brief 启用/禁用消息总线
     * @param enabled 是否启用
     */
    void setEnabled(bool enabled);

    /**
     * @brief 检查消息总线是否启用
     * @return 是否启用
     */
    bool isEnabled() const;

   signals:
    /**
     * @brief 消息发布信号（用于异步发布）
     * @param message 消息对象
     */
    void messagePublished(const Message& message);

    /**
     * @brief 订阅者数量变化信号
     * @param type 消息类型
     * @param count 新的订阅者数量
     */
    void subscriberCountChanged(MessageType type, int count);

   private:
    MessageBus();
    ~MessageBus();
    MessageBus(const MessageBus&) = delete;
    MessageBus& operator=(const MessageBus&) = delete;

    /**
     * @brief 处理消息分发
     * @param message 消息对象
     */
    void dispatchMessage(const Message& message);

    /**
     * @brief 清理无效的订阅者
     */
    void cleanupInvalidSubscribers();

    /**
     * @brief 生成唯一的订阅ID
     * @return 订阅ID
     */
    int generateSubscriptionId();

    QMap<MessageType, QList<SubscriberInfo>> m_subscribers;  ///< 按消息类型分组的订阅者
    QMap<int, MessageType> m_subscriptionTypes;              ///< 订阅ID到消息类型的映射
    QMap<QObject*, QSet<int>> m_receiverSubscriptions;       ///< 接收者到订阅ID的映射
    mutable QMutex m_mutex;                                  ///< 互斥锁
    int m_nextSubscriptionId;                                ///< 下一个订阅ID
    bool m_enabled;                                          ///< 是否启用
};

#endif  // MESSAGEBUS_H
