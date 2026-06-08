/**
 * @file messagebus.cpp
 * @brief 消息总线核心类实现
 * @date 2026-03-08
 * @version 1.0
 */

#include "messagebus.h"
#include <QCoreApplication>
#include <QMutexLocker>
#include <QRandomGenerator>
#include <QThread>
#include <QWaitCondition>
#include <memory>
#include "common/logger/logger.h"

MessageBus& MessageBus::instance()
{
    static MessageBus instance;
    return instance;
}

MessageBus::MessageBus() : QObject(nullptr), m_nextSubscriptionId(1), m_enabled(true)
{
    qRegisterMetaType<Message>("Message");
    qRegisterMetaType<MessageType>("MessageType");
    qRegisterMetaType<MessagePriority>("MessagePriority");

    connect(
        this, &MessageBus::messagePublished, this, [this](const Message& message) { dispatchMessage(message); },
        Qt::QueuedConnection);

    Logger::instance().debug("消息总线初始化完成");
}

MessageBus::~MessageBus()
{
    QMutexLocker locker(&m_mutex);
    m_subscribers.clear();
    m_subscriptionTypes.clear();
    m_receiverSubscriptions.clear();
    Logger::instance().debug("消息总线销毁");
}

int MessageBus::subscribe(MessageType type, QObject* receiver, MessageHandler handler, int priority)
{
    if (!receiver || !handler)
    {
        Logger::instance().warning("订阅失败：接收者或处理函数无效");
        return -1;
    }

    QMutexLocker locker(&m_mutex);

    int subscriptionId = generateSubscriptionId();

    SubscriberInfo info(receiver, handler, priority);

    QList<SubscriberInfo>& subscribers = m_subscribers[type];
    subscribers.append(info);
    std::sort(subscribers.begin(), subscribers.end(),
              [](const SubscriberInfo& a, const SubscriberInfo& b) { return a.priority > b.priority; });

    m_subscriptionTypes[subscriptionId] = type;
    m_receiverSubscriptions[receiver].insert(subscriptionId);

    if (receiver != this)
    {
        connect(
            receiver, &QObject::destroyed, this, [this, receiver]() { unsubscribeAll(receiver); },
            Qt::DirectConnection);
    }

    Logger::instance().debug(QString("订阅消息成功 - 类型: %1, 订阅ID: %2, 接收者: %3")
                                 .arg(static_cast<int>(type))
                                 .arg(subscriptionId)
                                 .arg(receiver->metaObject() ? receiver->metaObject()->className() : "Unknown"));

    emit subscriberCountChanged(type, subscribers.size());

    return subscriptionId;
}

QList<int> MessageBus::subscribeMultiple(const QList<MessageType>& types, QObject* receiver, MessageHandler handler,
                                         int priority)
{
    QList<int> subscriptionIds;
    for (MessageType type : types)
    {
        subscriptionIds.append(subscribe(type, receiver, handler, priority));
    }
    return subscriptionIds;
}

bool MessageBus::unsubscribe(int subscriptionId)
{
    QMutexLocker locker(&m_mutex);

    if (!m_subscriptionTypes.contains(subscriptionId))
    {
        Logger::instance().warning(QString("取消订阅失败：未找到订阅ID %1").arg(subscriptionId));
        return false;
    }

    MessageType type = m_subscriptionTypes[subscriptionId];
    m_subscriptionTypes.remove(subscriptionId);

    for (auto it = m_receiverSubscriptions.begin(); it != m_receiverSubscriptions.end(); ++it)
    {
        it.value().remove(subscriptionId);
        if (it.value().isEmpty())
        {
            m_receiverSubscriptions.erase(it);
            break;
        }
    }

    QList<SubscriberInfo>& subscribers = m_subscribers[type];
    for (int i = subscribers.size() - 1; i >= 0; --i)
    {
        if (subscribers[i].receiver == nullptr)
        {
            subscribers.removeAt(i);
        }
    }

    Logger::instance().debug(QString("取消订阅成功 - 订阅ID: %1").arg(subscriptionId));
    emit subscriberCountChanged(type, subscribers.size());

    return true;
}

void MessageBus::unsubscribeAll(QObject* receiver)
{
    if (!receiver)
        return;

    QMutexLocker locker(&m_mutex);

    if (!m_receiverSubscriptions.contains(receiver))
    {
        return;
    }

    QSet<int> subscriptionIds = m_receiverSubscriptions[receiver];
    m_receiverSubscriptions.remove(receiver);

    for (int subscriptionId : subscriptionIds)
    {
        if (m_subscriptionTypes.contains(subscriptionId))
        {
            MessageType type = m_subscriptionTypes[subscriptionId];
            m_subscriptionTypes.remove(subscriptionId);

            QList<SubscriberInfo>& subscribers = m_subscribers[type];
            for (int i = subscribers.size() - 1; i >= 0; --i)
            {
                if (subscribers[i].receiver == receiver)
                {
                    subscribers.removeAt(i);
                }
            }

            emit subscriberCountChanged(type, subscribers.size());
        }
    }

    Logger::instance().debug(QString("取消所有订阅 - 接收者: %1, 数量: %2")
                                 .arg(receiver->metaObject() ? receiver->metaObject()->className() : "Unknown")
                                 .arg(subscriptionIds.size()));
}

void MessageBus::publish(MessageType type, const QVariant& data, const QString& sender, MessagePriority priority)
{
    if (!m_enabled)
    {
        return;
    }

    Message message(type, data, sender);
    message.priority = priority;

    Logger::instance().debug(QString("发布消息(同步) - 类型: %1, 发送者: %2").arg(static_cast<int>(type)).arg(sender));

    dispatchMessage(message);
}

void MessageBus::publishAsync(MessageType type, const QVariant& data, const QString& sender, MessagePriority priority)
{
    if (!m_enabled)
    {
        return;
    }

    Message message(type, data, sender);
    message.priority = priority;

    Logger::instance().debug(QString("发布消息(异步) - 类型: %1, 发送者: %2").arg(static_cast<int>(type)).arg(sender));

    emit messagePublished(message);
}

void MessageBus::publishMessage(const Message& message)
{
    if (!m_enabled)
    {
        return;
    }

    dispatchMessage(message);
}

Message MessageBus::sendRequest(MessageType type, const QVariant& data, const QString& sender, int timeout)
{
    Message request(type, data, sender);
    request.timestamp = QDateTime::currentDateTime();

    auto waitMutex = std::make_shared<QMutex>();
    auto waitCondition = std::make_shared<QWaitCondition>();
    auto response = std::make_shared<Message>();
    auto responseReceived = std::make_shared<bool>(false);

    QString correlationId =
        QString("req_%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(QRandomGenerator::global()->bounded(1000000));
    request.correlationId = correlationId;

    int subscriptionId = subscribe(
        type, this,
        [=](const Message& msg)
        {
            if (msg.correlationId == correlationId)
            {
                QMutexLocker locker(waitMutex.get());
                *response = msg;
                *responseReceived = true;
                waitCondition->wakeAll();
            }
        },
        1000);

    dispatchMessage(request);

    {
        QMutexLocker locker(waitMutex.get());
        if (!*responseReceived)
        {
            if (timeout < 0)
            {
                waitCondition->wait(waitMutex.get());
            }
            else
            {
                waitCondition->wait(waitMutex.get(), timeout);
            }
        }
    }

    unsubscribe(subscriptionId);

    if (!*responseReceived)
    {
        Logger::instance().warning(
            QString("请求超时 - 类型: %1, 关联ID: %2").arg(static_cast<int>(type)).arg(correlationId));
    }

    return *response;
}

int MessageBus::registerResponseHandler(MessageType type, QObject* receiver, MessageHandler handler)
{
    return subscribe(type, receiver, handler, 1000);
}

int MessageBus::subscriberCount(MessageType type) const
{
    QMutexLocker locker(&m_mutex);
    return m_subscribers.value(type).size();
}

void MessageBus::cleanup()
{
    cleanupInvalidSubscribers();
}

void MessageBus::setEnabled(bool enabled)
{
    m_enabled = enabled;
    Logger::instance().info(QString("消息总线%1").arg(enabled ? "已启用" : "已禁用"));
}

bool MessageBus::isEnabled() const
{
    return m_enabled;
}

void MessageBus::dispatchMessage(const Message& message)
{
    QMutexLocker locker(&m_mutex);

    if (!m_subscribers.contains(message.type))
    {
        Logger::instance().debug(QString("消息无订阅者 - 类型: %1").arg(static_cast<int>(message.type)));
        return;
    }

    QList<SubscriberInfo> subscribers = m_subscribers[message.type];

    QList<SubscriberInfo> validSubscribers;
    for (const SubscriberInfo& info : subscribers)
    {
        if (info.receiver != nullptr)
        {
            validSubscribers.append(info);
        }
    }

    if (validSubscribers.size() != subscribers.size())
    {
        m_subscribers[message.type] = validSubscribers;
    }

    locker.unlock();

    for (const SubscriberInfo& info : validSubscribers)
    {
        try
        {
            info.handler(message);
        }
        catch (const std::exception& e)
        {
            Logger::instance().error(
                QString("消息处理异常 - 类型: %1, 错误: %2").arg(static_cast<int>(message.type)).arg(e.what()));
        }
        catch (...)
        {
            Logger::instance().error(QString("消息处理未知异常 - 类型: %1").arg(static_cast<int>(message.type)));
        }
    }
}

void MessageBus::cleanupInvalidSubscribers()
{
    QMutexLocker locker(&m_mutex);

    for (auto it = m_subscribers.begin(); it != m_subscribers.end(); ++it)
    {
        QList<SubscriberInfo>& subscribers = it.value();
        for (int i = subscribers.size() - 1; i >= 0; --i)
        {
            if (subscribers[i].receiver == nullptr)
            {
                subscribers.removeAt(i);
            }
        }
    }

    Logger::instance().debug("清理无效订阅者完成");
}

int MessageBus::generateSubscriptionId()
{
    return m_nextSubscriptionId++;
}
