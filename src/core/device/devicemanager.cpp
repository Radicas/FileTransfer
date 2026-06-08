/**
 * @file devicemanager.cpp
 * @brief 设备管理器类实现
 */

#include "core/device/devicemanager.h"
#include "core/network/devicediscoverer.h"
#include "common/logger/logger.h"
#include <QSysInfo>
#include <QStandardPaths>
#include <QNetworkInterface>

DeviceListModel::DeviceListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int DeviceListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return m_devices.size();
}

QVariant DeviceListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_devices.size())
    {
        return QVariant();
    }

    const DeviceInfo& device = m_devices.at(index.row());

    switch (role)
    {
    case DeviceIdRole:
        return device.deviceId;
    case DeviceNameRole:
        return device.deviceName;
    case IpAddressRole:
        return device.ipAddress;
    case OsTypeRole:
        return device.osType;
    case StatusRole:
        return static_cast<int>(device.status);
    case ActiveTransfersRole:
        return device.activeTransfers;
    case LastSeenRole:
        return device.lastSeen;
    case Qt::DisplayRole:
        return device.deviceName;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> DeviceListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[DeviceIdRole] = "deviceId";
    roles[DeviceNameRole] = "deviceName";
    roles[IpAddressRole] = "ipAddress";
    roles[OsTypeRole] = "osType";
    roles[StatusRole] = "status";
    roles[ActiveTransfersRole] = "activeTransfers";
    roles[LastSeenRole] = "lastSeen";
    return roles;
}

void DeviceListModel::addDevice(const DeviceInfo& device)
{
    if (m_deviceIndexMap.contains(device.deviceId))
    {
        return;
    }

    int index = m_devices.size();
    beginInsertRows(QModelIndex(), index, index);
    m_devices.append(device);
    m_deviceIndexMap.insert(device.deviceId, index);
    endInsertRows();
}

void DeviceListModel::updateDevice(const DeviceInfo& device)
{
    if (!m_deviceIndexMap.contains(device.deviceId))
    {
        addDevice(device);
        return;
    }

    int index = m_deviceIndexMap.value(device.deviceId);
    m_devices[index] = device;
    emit dataChanged(createIndex(index, 0), createIndex(index, 0));
}

void DeviceListModel::removeDevice(const QString& deviceId)
{
    if (!m_deviceIndexMap.contains(deviceId))
    {
        return;
    }

    int index = m_deviceIndexMap.value(deviceId);
    beginRemoveRows(QModelIndex(), index, index);
    m_devices.removeAt(index);
    m_deviceIndexMap.remove(deviceId);
    
    for (auto it = m_deviceIndexMap.begin(); it != m_deviceIndexMap.end(); ++it)
    {
        if (it.value() > index)
        {
            it.value()--;
        }
    }
    endRemoveRows();
}

void DeviceListModel::clearDevices()
{
    beginResetModel();
    m_devices.clear();
    m_deviceIndexMap.clear();
    endResetModel();
}

DeviceInfo DeviceListModel::getDevice(int index) const
{
    if (index >= 0 && index < m_devices.size())
    {
        return m_devices.at(index);
    }
    return DeviceInfo();
}

DeviceInfo DeviceListModel::getDevice(const QString& deviceId) const
{
    if (m_deviceIndexMap.contains(deviceId))
    {
        return m_devices.at(m_deviceIndexMap.value(deviceId));
    }
    return DeviceInfo();
}

QList<DeviceInfo> DeviceListModel::getAllDevices() const
{
    return m_devices;
}

int DeviceListModel::findDeviceIndex(const QString& deviceId) const
{
    return m_deviceIndexMap.value(deviceId, -1);
}

DeviceManager& DeviceManager::instance()
{
    static DeviceManager instance;
    return instance;
}

DeviceManager::DeviceManager()
    : m_deviceListModel(nullptr)
{
    m_deviceListModel = new DeviceListModel(this);
}

DeviceManager::~DeviceManager()
{
}

void DeviceManager::initialize()
{
    DeviceInfoData localData;
    localData.deviceId = DeviceDiscoverer::instance().getLocalDeviceInfo().deviceId;
    localData.deviceName = QSysInfo::machineHostName();
    localData.osType = QSysInfo::prettyProductName();
    localData.userName = QStandardPaths::writableLocation(QStandardPaths::HomeLocation).split("/").last();
    
#ifdef Q_OS_WIN
    localData.userName = QStandardPaths::writableLocation(QStandardPaths::HomeLocation).split("\\").last();
#endif

    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : addresses)
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && 
            !address.isLoopback())
        {
            localData.ipAddress = address.toString();
            break;
        }
    }

    localData.transferPort = NetworkConstants::TRANSFER_PORT;
    
    m_localDevice = DeviceInfo(localData);
    m_localDevice.status = DeviceStatus::Online;

    connect(&DeviceDiscoverer::instance(), &DeviceDiscoverer::deviceDiscovered,
            this, &DeviceManager::onDeviceDiscovered);
    connect(&DeviceDiscoverer::instance(), &DeviceDiscoverer::deviceOffline,
            this, &DeviceManager::onDeviceOffline);
    connect(&DeviceDiscoverer::instance(), &DeviceDiscoverer::deviceUpdated,
            this, &DeviceManager::onDeviceUpdated);

    Logger::instance().info("设备管理器初始化完成，本地设备: " + m_localDevice.deviceName);
}

DeviceListModel* DeviceManager::getDeviceListModel()
{
    return m_deviceListModel;
}

QList<DeviceInfo> DeviceManager::getOnlineDevices() const
{
    QList<DeviceInfo> devices;
    QList<DeviceInfo> allDevices = m_deviceListModel->getAllDevices();
    for (const DeviceInfo& device : allDevices)
    {
        if (device.status == DeviceStatus::Online || device.status == DeviceStatus::Busy)
        {
            devices.append(device);
        }
    }
    return devices;
}

DeviceInfo DeviceManager::getDevice(const QString& deviceId) const
{
    return m_deviceListModel->getDevice(deviceId);
}

bool DeviceManager::isDeviceOnline(const QString& deviceId) const
{
    DeviceInfo device = getDevice(deviceId);
    return device.status == DeviceStatus::Online || device.status == DeviceStatus::Busy;
}

DeviceInfo DeviceManager::getLocalDevice() const
{
    return m_localDevice;
}

void DeviceManager::updateDeviceTransferStatus(const QString& deviceId, int activeTransfers)
{
    DeviceInfo device = getDevice(deviceId);
    if (device.deviceId.isEmpty())
    {
        return;
    }

    device.activeTransfers = activeTransfers;
    device.status = activeTransfers > 0 ? DeviceStatus::Busy : DeviceStatus::Online;
    device.lastSeen = QDateTime::currentDateTime();
    
    m_deviceListModel->updateDevice(device);
    emit deviceStatusChanged(deviceId, device.status);
}

void DeviceManager::addDeviceTransferBytes(const QString& deviceId, qint64 bytes)
{
    DeviceInfo device = getDevice(deviceId);
    if (device.deviceId.isEmpty())
    {
        return;
    }

    device.totalTransferred += bytes;
    device.lastSeen = QDateTime::currentDateTime();
    
    m_deviceListModel->updateDevice(device);
}

void DeviceManager::onDeviceDiscovered(const DeviceInfoData& deviceInfo)
{
    DeviceInfo device(deviceInfo);
    device.status = DeviceStatus::Online;
    device.lastSeen = QDateTime::currentDateTime();
    
    m_deviceListModel->addDevice(device);
    emit deviceAdded(device);
    
    Logger::instance().info("设备管理器添加设备: " + device.deviceName);
}

void DeviceManager::onDeviceOffline(const QString& deviceId)
{
    DeviceInfo device = getDevice(deviceId);
    if (!device.deviceId.isEmpty())
    {
        device.status = DeviceStatus::Offline;
        m_deviceListModel->updateDevice(device);
        emit deviceStatusChanged(deviceId, DeviceStatus::Offline);
    }
    
    m_deviceListModel->removeDevice(deviceId);
    emit deviceRemoved(deviceId);
    
    Logger::instance().info("设备管理器移除设备: " + deviceId);
}

void DeviceManager::onDeviceUpdated(const DeviceInfoData& deviceInfo)
{
    DeviceInfo existingDevice = getDevice(deviceInfo.deviceId);
    
    DeviceInfo device(deviceInfo);
    device.status = existingDevice.status;
    device.activeTransfers = existingDevice.activeTransfers;
    device.totalTransferred = existingDevice.totalTransferred;
    device.lastSeen = QDateTime::currentDateTime();
    
    m_deviceListModel->updateDevice(device);
    emit deviceUpdated(device);
}