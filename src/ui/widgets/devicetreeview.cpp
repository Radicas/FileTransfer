/**
 * @file devicetreeview.cpp
 * @brief 设备树视图控件实现
 */

#include "ui/widgets/devicetreeview.h"
#include "common/logger/logger.h"
#include <QApplication>
#include <QStyle>

DeviceTreeView::DeviceTreeView(QWidget* parent)
    : QWidget(parent)
    , m_treeView(nullptr)
    , m_model(nullptr)
    , m_titleLabel(nullptr)
    , m_refreshButton(nullptr)
    , m_mainLayout(nullptr)
{
    initUI();
    initModel();

    connect(&DeviceManager::instance(), &DeviceManager::deviceAdded,
            this, &DeviceTreeView::onDeviceAdded);
    connect(&DeviceManager::instance(), &DeviceManager::deviceRemoved,
            this, &DeviceTreeView::onDeviceRemoved);
    connect(&DeviceManager::instance(), &DeviceManager::deviceUpdated,
            this, &DeviceTreeView::onDeviceUpdated);
    connect(&DeviceManager::instance(), &DeviceManager::deviceStatusChanged,
            this, &DeviceTreeView::onDeviceStatusChanged);

    Logger::instance().info("设备树视图初始化完成");
}

DeviceTreeView::~DeviceTreeView()
{
}

void DeviceTreeView::initUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(5);

    m_titleLabel = new QLabel("设备列表", this);
    m_titleLabel->setStyleSheet("font-weight: bold; padding: 5px;");
    m_mainLayout->addWidget(m_titleLabel);

    m_treeView = new QTreeView(this);
    m_treeView->setHeaderHidden(true);
    m_treeView->setIndentation(10);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_mainLayout->addWidget(m_treeView);

    m_refreshButton = new QPushButton("刷新", this);
    m_refreshButton->setFixedHeight(30);
    m_mainLayout->addWidget(m_refreshButton);

    connect(m_refreshButton, &QPushButton::clicked, this, &DeviceTreeView::refreshDeviceList);
    connect(m_treeView, &QTreeView::doubleClicked, this, &DeviceTreeView::onDoubleClicked);
}

void DeviceTreeView::initModel()
{
    m_model = new QStandardItemModel(this);
    m_treeView->setModel(m_model);

    QStandardItem* rootItem = m_model->invisibleRootItem();

    QStandardItem* localItem = new QStandardItem("本机");
    localItem->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    localItem->setData("local", Qt::UserRole);
    m_model->appendRow(localItem);

    QStandardItem* remoteItem = new QStandardItem("远程设备");
    remoteItem->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirIcon));
    m_model->appendRow(remoteItem);
}

QString DeviceTreeView::getSelectedDeviceId() const
{
    QModelIndex index = m_treeView->currentIndex();
    if (!index.isValid())
    {
        return QString();
    }

    QStandardItem* item = m_model->itemFromIndex(index);
    if (!item)
    {
        return QString();
    }

    QString deviceId = item->data(Qt::UserRole).toString();
    if (deviceId == "local")
    {
        return QString();
    }

    return deviceId;
}

DeviceInfo DeviceTreeView::getSelectedDevice() const
{
    QString deviceId = getSelectedDeviceId();
    if (deviceId.isEmpty())
    {
        return DeviceManager::instance().getLocalDevice();
    }
    return DeviceManager::instance().getDevice(deviceId);
}

void DeviceTreeView::refreshDeviceList()
{
    QList<DeviceInfo> devices = DeviceManager::instance().getOnlineDevices();
    
    QStandardItem* remoteItem = m_model->item(1);
    if (!remoteItem)
    {
        return;
    }

    remoteItem->removeRows(0, remoteItem->rowCount());

    for (const DeviceInfo& device : devices)
    {
        QStandardItem* deviceItem = new QStandardItem(device.deviceName);
        deviceItem->setIcon(getStatusIcon(device.status));
        deviceItem->setData(device.deviceId, Qt::UserRole);
        deviceItem->setToolTip(device.ipAddress + "\n" + device.osType);
        remoteItem->appendRow(deviceItem);
        m_deviceItems.insert(device.deviceId, deviceItem);
    }

    Logger::instance().info("设备列表已刷新，共 " + QString::number(devices.size()) + " 个设备");
}

void DeviceTreeView::onDeviceAdded(const DeviceInfo& device)
{
    QStandardItem* remoteItem = m_model->item(1);
    if (!remoteItem)
    {
        return;
    }

    QStandardItem* deviceItem = new QStandardItem(device.deviceName);
    deviceItem->setIcon(getStatusIcon(device.status));
    deviceItem->setData(device.deviceId, Qt::UserRole);
    deviceItem->setToolTip(device.ipAddress + "\n" + device.osType);
    remoteItem->appendRow(deviceItem);
    m_deviceItems.insert(device.deviceId, deviceItem);

    Logger::instance().info("设备树添加设备: " + device.deviceName);
}

void DeviceTreeView::onDeviceRemoved(const QString& deviceId)
{
    QStandardItem* item = findDeviceItem(deviceId);
    if (item)
    {
        QStandardItem* parentItem = item->parent();
        if (parentItem)
        {
            parentItem->removeRow(item->row());
        }
        m_deviceItems.remove(deviceId);
    }

    Logger::instance().info("设备树移除设备: " + deviceId);
}

void DeviceTreeView::onDeviceUpdated(const DeviceInfo& device)
{
    updateDeviceItem(device);
}

void DeviceTreeView::onDeviceStatusChanged(const QString& deviceId, DeviceStatus status)
{
    DeviceInfo device = DeviceManager::instance().getDevice(deviceId);
    if (!device.deviceId.isEmpty())
    {
        device.status = status;
        updateDeviceItem(device);
    }
}

void DeviceTreeView::onSelectionChanged()
{
    QString deviceId = getSelectedDeviceId();
    emit deviceSelected(deviceId);
}

void DeviceTreeView::onDoubleClicked(const QModelIndex& index)
{
    QStandardItem* item = m_model->itemFromIndex(index);
    if (!item)
    {
        return;
    }

    QString deviceId = item->data(Qt::UserRole).toString();
    if (!deviceId.isEmpty() && deviceId != "local")
    {
        emit deviceDoubleClicked(deviceId);
    }
}

void DeviceTreeView::updateDeviceItem(const DeviceInfo& device)
{
    QStandardItem* item = findDeviceItem(device.deviceId);
    if (item)
    {
        item->setText(device.deviceName);
        item->setIcon(getStatusIcon(device.status));
        item->setToolTip(device.ipAddress + "\n" + device.osType + "\n" + getStatusText(device.status));
    }
}

QIcon DeviceTreeView::getStatusIcon(DeviceStatus status) const
{
    switch (status)
    {
    case DeviceStatus::Online:
        return QApplication::style()->standardIcon(QStyle::SP_DialogYesButton);
    case DeviceStatus::Busy:
        return QApplication::style()->standardIcon(QStyle::SP_BrowserReload);
    case DeviceStatus::Offline:
        return QApplication::style()->standardIcon(QStyle::SP_DialogNoButton);
    case DeviceStatus::Error:
        return QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical);
    default:
        return QApplication::style()->standardIcon(QStyle::SP_FileIcon);
    }
}

QString DeviceTreeView::getStatusText(DeviceStatus status) const
{
    switch (status)
    {
    case DeviceStatus::Online:
        return "在线";
    case DeviceStatus::Busy:
        return "繁忙";
    case DeviceStatus::Offline:
        return "离线";
    case DeviceStatus::Error:
        return "错误";
    default:
        return "未知";
    }
}

QStandardItem* DeviceTreeView::findDeviceItem(const QString& deviceId) const
{
    return m_deviceItems.value(deviceId, nullptr);
}