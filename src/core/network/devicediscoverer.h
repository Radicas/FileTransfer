/**
 * @file devicediscoverer.h
 * @brief 设备发现服务类定义
 * @date 2026-06-01
 * @version 1.0
 * 
 * @details 使用UDP广播机制在局域网中发现其他运行此软件的设备。
 * 支持设备发现、心跳检测和设备离线检测。
 */

#ifndef DEVICEDISCOVERER_H
#define DEVICEDISCOVERER_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QMap>
#include <QDateTime>
#include <QHostAddress>
#include "core/network/networkprotocol.h"

/**
 * @class DeviceDiscoverer
 * @brief 设备发现服务类
 * 
 * @details 该类负责：
 * - 通过UDP广播发现局域网中的其他设备
 * - 发送和接收心跳消息，维护设备在线状态
 * - 检测设备离线并通知
 * - 提供当前在线设备列表
 */
class DeviceDiscoverer : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return DeviceDiscoverer单例引用
     */
    static DeviceDiscoverer& instance();

    /**
     * @brief 初始化设备发现服务
     * @param localDeviceInfo 本地设备信息
     * @return 是否初始化成功
     */
    bool initialize(const DeviceInfoData& localDeviceInfo);

    /**
     * @brief 启动设备发现
     */
    void start();

    /**
     * @brief 停止设备发现
     */
    void stop();

    /**
     * @brief 获取当前在线设备列表
     * @return 设备信息列表
     */
    QList<DeviceInfoData> getOnlineDevices() const;

    /**
     * @brief 获取指定设备信息
     * @param deviceId 设备ID
     * @return 设备信息，如果不存在返回空
     */
    DeviceInfoData getDeviceInfo(const QString& deviceId) const;

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
    DeviceInfoData getLocalDeviceInfo() const;

    /**
     * @brief 发送设备离线通知
     */
    void sendLeaveNotification();

signals:
    /**
     * @brief 设备发现信号
     * @param deviceInfo 新发现的设备信息
     */
    void deviceDiscovered(const DeviceInfoData& deviceInfo);

    /**
     * @brief 设备离线信号
     * @param deviceId 离线的设备ID
     */
    void deviceOffline(const QString& deviceId);

    /**
     * @brief 设备信息更新信号
     * @param deviceInfo 更新的设备信息
     */
    void deviceUpdated(const DeviceInfoData& deviceInfo);

    /**
     * @brief 发现服务启动信号
     */
    void started();

    /**
     * @brief 发现服务停止信号
     */
    void stopped();

    /**
     * @brief 错误发生信号
     * @param errorMessage 错误消息
     */
    void errorOccurred(const QString& errorMessage);

private slots:
    /**
     * @brief 处理UDP数据接收
     */
    void onReadyRead();

    /**
     * @brief 发送心跳广播
     */
    void onSendHeartbeat();

    /**
     * @brief 检查设备超时
     */
    void onCheckTimeout();

private:
    DeviceDiscoverer();
    ~DeviceDiscoverer();
    DeviceDiscoverer(const DeviceDiscoverer&) = delete;
    DeviceDiscoverer& operator=(const DeviceDiscoverer&) = delete;

    /**
     * @brief 发送广播消息
     * @param message 网络消息
     */
    void sendBroadcastMessage(const NetworkMessage& message);

    /**
     * @brief 发送响应消息到指定地址
     * @param message 网络消息
     * @param address 目标地址
     * @param port 目标端口
     */
    void sendResponseMessage(const NetworkMessage& message, const QHostAddress& address, quint16 port);

    /**
     * @brief 处理接收到的消息
     * @param message 网络消息
     * @param senderAddress 发送者地址
     */
    void processMessage(const NetworkMessage& message, const QHostAddress& senderAddress);

    /**
     * @brief 处理发现广播消息
     * @param message 网络消息
     * @param senderAddress 发送者地址
     */
    void processDiscoverBroadcast(const NetworkMessage& message, const QHostAddress& senderAddress);

    /**
     * @brief 处理发现响应消息
     * @param message 网络消息
     */
    void processDiscoverResponse(const NetworkMessage& message);

    /**
     * @brief 处理心跳消息
     * @param message 网络消息
     */
    void processHeartbeat(const NetworkMessage& message);

    /**
     * @brief 处理离线消息
     * @param message 网络消息
     */
    void processLeaveNotification(const NetworkMessage& message);

    /**
     * @brief 更新设备最后活动时间
     * @param deviceId 设备ID
     */
    void updateDeviceActivity(const QString& deviceId);

    /**
     * @brief 生成设备唯一标识
     * @return 设备ID
     */
    QString generateDeviceId();

    QUdpSocket* m_udpSocket;                    ///< UDP套接字
    QTimer* m_heartbeatTimer;                   ///< 心跳定时器
    QTimer* m_timeoutCheckTimer;                ///< 超时检查定时器
    QMap<QString, DeviceInfoData> m_devices;    ///< 在线设备列表
    QMap<QString, QDateTime> m_lastActivity;    ///< 设备最后活动时间
    DeviceInfoData m_localDeviceInfo;           ///< 本地设备信息
    bool m_running;                             ///< 运行状态标志
};

#endif  // DEVICEDISCOVERER_H