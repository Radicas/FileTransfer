/**
 * @file devicediscoverer.cpp
 * @brief 设备发现服务类实现
 */

#include "core/network/devicediscoverer.h"
#include "common/logger/logger.h"
#include <QNetworkInterface>
#include <QJsonDocument>
#include <QUuid>

DeviceDiscoverer& DeviceDiscoverer::instance()
{
    static DeviceDiscoverer instance;
    return instance;
}

DeviceDiscoverer::DeviceDiscoverer()
    : m_udpSocket(nullptr)
    , m_heartbeatTimer(nullptr)
    , m_timeoutCheckTimer(nullptr)
    , m_running(false)
{
    m_udpSocket = new QUdpSocket(this);
    m_heartbeatTimer = new QTimer(this);
    m_timeoutCheckTimer = new QTimer(this);

    connect(m_udpSocket, &QUdpSocket::readyRead, this, &DeviceDiscoverer::onReadyRead);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &DeviceDiscoverer::onSendHeartbeat);
    connect(m_timeoutCheckTimer, &QTimer::timeout, this, &DeviceDiscoverer::onCheckTimeout);
}

DeviceDiscoverer::~DeviceDiscoverer()
{
    stop();
}

bool DeviceDiscoverer::initialize(const DeviceInfoData& localDeviceInfo)
{
    m_localDeviceInfo = localDeviceInfo;
    
    if (m_localDeviceInfo.deviceId.isEmpty())
    {
        m_localDeviceInfo.deviceId = generateDeviceId();
    }

    if (!m_udpSocket->bind(QHostAddress::Any, NetworkConstants::DISCOVER_PORT, QUdpSocket::ShareAddress))
    {
        Logger::instance().error("设备发现服务绑定端口失败: " + m_udpSocket->errorString());
        return false;
    }

    Logger::instance().info("设备发现服务初始化成功，本地设备ID: " + m_localDeviceInfo.deviceId);
    return true;
}

void DeviceDiscoverer::start()
{
    if (m_running)
    {
        return;
    }

    m_running = true;
    m_heartbeatTimer->start(NetworkConstants::HEARTBEAT_INTERVAL);
    m_timeoutCheckTimer->start(NetworkConstants::HEARTBEAT_TIMEOUT / 2);

    NetworkMessage msg(NetworkMessageType::DiscoverBroadcast, m_localDeviceInfo.toJson(), m_localDeviceInfo.deviceId);
    sendBroadcastMessage(msg);

    Logger::instance().info("设备发现服务已启动");
    emit started();
}

void DeviceDiscoverer::stop()
{
    if (!m_running)
    {
        return;
    }

    sendLeaveNotification();

    m_running = false;
    m_heartbeatTimer->stop();
    m_timeoutCheckTimer->stop();
    m_udpSocket->close();
    m_devices.clear();
    m_lastActivity.clear();

    Logger::instance().info("设备发现服务已停止");
    emit stopped();
}

QList<DeviceInfoData> DeviceDiscoverer::getOnlineDevices() const
{
    return m_devices.values();
}

DeviceInfoData DeviceDiscoverer::getDeviceInfo(const QString& deviceId) const
{
    return m_devices.value(deviceId);
}

bool DeviceDiscoverer::isDeviceOnline(const QString& deviceId) const
{
    return m_devices.contains(deviceId);
}

DeviceInfoData DeviceDiscoverer::getLocalDeviceInfo() const
{
    return m_localDeviceInfo;
}

void DeviceDiscoverer::sendLeaveNotification()
{
    NetworkMessage msg(NetworkMessageType::DiscoverLeave, QJsonObject(), m_localDeviceInfo.deviceId);
    sendBroadcastMessage(msg);
    Logger::instance().info("已发送设备离线通知");
}

void DeviceDiscoverer::onReadyRead()
{
    while (m_udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress senderAddress;
        quint16 senderPort;

        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &senderAddress, &senderPort);

        NetworkMessage message = NetworkMessage::deserialize(datagram);
        
        if (message.senderId == m_localDeviceInfo.deviceId)
        {
            continue;
        }

        processMessage(message, senderAddress);
    }
}

void DeviceDiscoverer::onSendHeartbeat()
{
    NetworkMessage msg(NetworkMessageType::DiscoverHeartbeat, m_localDeviceInfo.toJson(), m_localDeviceInfo.deviceId);
    sendBroadcastMessage(msg);
}

void DeviceDiscoverer::onCheckTimeout()
{
    QDateTime now = QDateTime::currentDateTime();
    QList<QString> offlineDevices;

    for (auto it = m_lastActivity.begin(); it != m_lastActivity.end(); ++it)
    {
        int elapsed = it.value().msecsTo(now);
        if (elapsed > NetworkConstants::HEARTBEAT_TIMEOUT)
        {
            offlineDevices.append(it.key());
        }
    }

    for (const QString& deviceId : offlineDevices)
    {
        DeviceInfoData deviceInfo = m_devices.value(deviceId);
        Logger::instance().info("设备离线: " + deviceInfo.deviceName + " (" + deviceInfo.ipAddress + ")");
        
        m_devices.remove(deviceId);
        m_lastActivity.remove(deviceId);
        emit deviceOffline(deviceId);
    }
}

void DeviceDiscoverer::sendBroadcastMessage(const NetworkMessage& message)
{
    QByteArray data = message.serialize();
    
    QList<QHostAddress> broadcastAddresses;
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    
    for (const QNetworkInterface& interface : interfaces)
    {
        if (interface.flags() & QNetworkInterface::IsUp &&
            interface.flags() & QNetworkInterface::IsRunning &&
            !(interface.flags() & QNetworkInterface::IsLoopBack))
        {
            QList<QNetworkAddressEntry> entries = interface.addressEntries();
            for (const QNetworkAddressEntry& entry : entries)
            {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                {
                    QHostAddress broadcast = entry.broadcast();
                    if (!broadcast.isNull())
                    {
                        broadcastAddresses.append(broadcast);
                    }
                }
            }
        }
    }

    if (broadcastAddresses.isEmpty())
    {
        broadcastAddresses.append(QHostAddress::Broadcast);
    }

    for (const QHostAddress& address : broadcastAddresses)
    {
        m_udpSocket->writeDatagram(data, address, NetworkConstants::DISCOVER_PORT);
    }
}

void DeviceDiscoverer::sendResponseMessage(const NetworkMessage& message, const QHostAddress& address, quint16 port)
{
    QByteArray data = message.serialize();
    m_udpSocket->writeDatagram(data, address, port);
}

void DeviceDiscoverer::processMessage(const NetworkMessage& message, const QHostAddress& senderAddress)
{
    switch (message.type)
    {
    case NetworkMessageType::DiscoverBroadcast:
        processDiscoverBroadcast(message, senderAddress);
        break;
    case NetworkMessageType::DiscoverResponse:
        processDiscoverResponse(message);
        break;
    case NetworkMessageType::DiscoverHeartbeat:
        processHeartbeat(message);
        break;
    case NetworkMessageType::DiscoverLeave:
        processLeaveNotification(message);
        break;
    default:
        break;
    }
}

void DeviceDiscoverer::processDiscoverBroadcast(const NetworkMessage& message, const QHostAddress& senderAddress)
{
    DeviceInfoData deviceInfo = DeviceInfoData::fromJson(message.data);
    
    if (!m_devices.contains(deviceInfo.deviceId))
    {
        Logger::instance().info("发现新设备: " + deviceInfo.deviceName + " (" + deviceInfo.ipAddress + ")");
        m_devices.insert(deviceInfo.deviceId, deviceInfo);
        m_lastActivity.insert(deviceInfo.deviceId, QDateTime::currentDateTime());
        emit deviceDiscovered(deviceInfo);
    }

    NetworkMessage response(NetworkMessageType::DiscoverResponse, m_localDeviceInfo.toJson(), m_localDeviceInfo.deviceId);
    sendResponseMessage(response, senderAddress, NetworkConstants::DISCOVER_PORT);
}

void DeviceDiscoverer::processDiscoverResponse(const NetworkMessage& message)
{
    DeviceInfoData deviceInfo = DeviceInfoData::fromJson(message.data);
    
    if (!m_devices.contains(deviceInfo.deviceId))
    {
        Logger::instance().info("收到设备响应: " + deviceInfo.deviceName + " (" + deviceInfo.ipAddress + ")");
        m_devices.insert(deviceInfo.deviceId, deviceInfo);
        m_lastActivity.insert(deviceInfo.deviceId, QDateTime::currentDateTime());
        emit deviceDiscovered(deviceInfo);
    }
    else
    {
        m_devices[deviceInfo.deviceId] = deviceInfo;
        m_lastActivity[deviceInfo.deviceId] = QDateTime::currentDateTime();
        emit deviceUpdated(deviceInfo);
    }
}

void DeviceDiscoverer::processHeartbeat(const NetworkMessage& message)
{
    DeviceInfoData deviceInfo = DeviceInfoData::fromJson(message.data);
    
    if (m_devices.contains(deviceInfo.deviceId))
    {
        m_lastActivity[deviceInfo.deviceId] = QDateTime::currentDateTime();
        
        if (m_devices[deviceInfo.deviceId].deviceName != deviceInfo.deviceName ||
            m_devices[deviceInfo.deviceId].ipAddress != deviceInfo.ipAddress)
        {
            m_devices[deviceInfo.deviceId] = deviceInfo;
            emit deviceUpdated(deviceInfo);
        }
    }
    else
    {
        Logger::instance().info("从心跳中发现设备: " + deviceInfo.deviceName);
        m_devices.insert(deviceInfo.deviceId, deviceInfo);
        m_lastActivity.insert(deviceInfo.deviceId, QDateTime::currentDateTime());
        emit deviceDiscovered(deviceInfo);
    }
}

void DeviceDiscoverer::processLeaveNotification(const NetworkMessage& message)
{
    QString deviceId = message.senderId;
    
    if (m_devices.contains(deviceId))
    {
        DeviceInfoData deviceInfo = m_devices.value(deviceId);
        Logger::instance().info("设备主动离线: " + deviceInfo.deviceName);
        
        m_devices.remove(deviceId);
        m_lastActivity.remove(deviceId);
        emit deviceOffline(deviceId);
    }
}

void DeviceDiscoverer::updateDeviceActivity(const QString& deviceId)
{
    m_lastActivity[deviceId] = QDateTime::currentDateTime();
}

QString DeviceDiscoverer::generateDeviceId()
{
    QString machineId;
    
#ifdef Q_OS_WIN
    machineId = QSysInfo::machineUniqueId();
#else
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& interface : interfaces)
    {
        if (!(interface.flags() & QNetworkInterface::IsLoopBack))
        {
            machineId = interface.hardwareAddress();
            break;
        }
    }
#endif

    if (machineId.isEmpty())
    {
        machineId = QUuid::createUuid().toString();
    }

    return "FT_" + machineId.replace("-", "").replace("{", "").replace("}", "").left(16);
}