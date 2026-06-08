/**
 * @file devicemanager.h
 * @brief 设备管理器类定义
 * @date 2026-06-01
 * @version 1.0
 * 
 * @details 管理所有发现的设备，提供设备状态跟踪和设备信息查询功能。
 */

#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QAbstractListModel>
#include "core/network/networkprotocol.h"

/**
 * @brief 设备状态枚举
 */
enum class DeviceStatus
{
    Offline,        ///< 离线
    Online,         ///< 在线
    Busy,           ///< 繁忙（正在传输）
    Error           ///< 错误状态
};

/**
 * @class DeviceInfo
 * @brief 设备信息类
 * 
 * @details 存储设备的完整信息，包括网络信息、状态和传输统计。
 */
class DeviceInfo
{
public:
    QString deviceId;           ///< 设备唯一标识
    QString deviceName;         ///< 设备名称
    QString ipAddress;          ///< IP地址
    int transferPort;           ///< 文件传输端口
    QString osType;             ///< 操作系统类型
    QString userName;           ///< 用户名
    DeviceStatus status;        ///< 设备状态
    int activeTransfers;        ///< 正在进行的传输数量
    qint64 totalTransferred;    ///< 总传输数据量
    QDateTime lastSeen;         ///< 最后见到时间

    DeviceInfo() : transferPort(0), status(DeviceStatus::Offline), activeTransfers(0), totalTransferred(0) {}

    DeviceInfo(const DeviceInfoData& data)
        : deviceId(data.deviceId)
        , deviceName(data.deviceName)
        , ipAddress(data.ipAddress)
        , transferPort(data.transferPort)
        , osType(data.osType)
        , userName(data.userName)
        , status(DeviceStatus::Online)
        , activeTransfers(0)
        , totalTransferred(0)
        , lastSeen(QDateTime::currentDateTime())
    {}

    DeviceInfoData toDeviceInfoData() const
    {
        DeviceInfoData data;
        data.deviceId = deviceId;
        data.deviceName = deviceName;
        data.ipAddress = ipAddress;
        data.transferPort = transferPort;
        data.osType = osType;
        data.userName = userName;
        return data;
    }
};

/**
 * @class DeviceListModel
 * @brief 设备列表模型类
 * 
 * @details 为Qt视图组件提供设备列表数据的模型。
 */
class DeviceListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum DeviceRoles
    {
        DeviceIdRole = Qt::UserRole + 1,
        DeviceNameRole,
        IpAddressRole,
        OsTypeRole,
        StatusRole,
        ActiveTransfersRole,
        LastSeenRole
    };

    explicit DeviceListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addDevice(const DeviceInfo& device);
    void updateDevice(const DeviceInfo& device);
    void removeDevice(const QString& deviceId);
    void clearDevices();
    DeviceInfo getDevice(int index) const;
    DeviceInfo getDevice(const QString& deviceId) const;
    QList<DeviceInfo> getAllDevices() const;
    int findDeviceIndex(const QString& deviceId) const;

private:
    QList<DeviceInfo> m_devices;
    QMap<QString, int> m_deviceIndexMap;
};

/**
 * @class DeviceManager
 * @brief 设备管理器类
 * 
 * @details 管理所有发现的设备，提供：
 * - 设备列表管理
 * - 设备状态跟踪
 * - 设备信息查询
 * - 与设备发现服务的集成
 */
class DeviceManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return DeviceManager单例引用
     */
    static DeviceManager& instance();

    /**
     * @brief 初始化设备管理器
     */
    void initialize();

    /**
     * @brief 获取设备列表模型
     * @return 设备列表模型指针
     */
    DeviceListModel* getDeviceListModel();

    /**
     * @brief 获取所有在线设备
     * @return 设备信息列表
     */
    QList<DeviceInfo> getOnlineDevices() const;

    /**
     * @brief 获取指定设备信息
     * @param deviceId 设备ID
     * @return 设备信息
     */
    DeviceInfo getDevice(const QString& deviceId) const;

    /**
     * @brief 检查设备是否在线
     * @param deviceId 设备ID
     * @return 是否在线
     */
    bool isDeviceOnline(const QString& deviceId) const;

    /**
     * @brief 获取本地设备信息
     * @return 本地设备信息
     */
    DeviceInfo getLocalDevice() const;

    /**
     * @brief 更新设备传输状态
     * @param deviceId 设备ID
     * @param activeTransfers 正在进行的传输数量
     */
    void updateDeviceTransferStatus(const QString& deviceId, int activeTransfers);

    /**
     * @brief 增加设备传输数据量
     * @param deviceId 设备ID
     * @param bytes 传输的字节数
     */
    void addDeviceTransferBytes(const QString& deviceId, qint64 bytes);

signals:
    /**
     * @brief 设备添加信号
     * @param device 新添加的设备信息
     */
    void deviceAdded(const DeviceInfo& device);

    /**
     * @brief 设备移除信号
     * @param deviceId 移除的设备ID
     */
    void deviceRemoved(const QString& deviceId);

    /**
     * @brief 设备更新信号
     * @param device 更新的设备信息
     */
    void deviceUpdated(const DeviceInfo& device);

    /**
     * @brief 设备状态变化信号
     * @param deviceId 设备ID
     * @param status 新状态
     */
    void deviceStatusChanged(const QString& deviceId, DeviceStatus status);

private slots:
    void onDeviceDiscovered(const DeviceInfoData& deviceInfo);
    void onDeviceOffline(const QString& deviceId);
    void onDeviceUpdated(const DeviceInfoData& deviceInfo);

private:
    DeviceManager();
    ~DeviceManager();
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;

    DeviceListModel* m_deviceListModel;     ///< 设备列表模型
    DeviceInfo m_localDevice;               ///< 本地设备信息
};

#endif  // DEVICEMANAGER_H