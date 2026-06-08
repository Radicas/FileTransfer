/**
 * @file devicetreeview.h
 * @brief 设备树视图控件定义
 * @date 2026-06-01
 * @version 1.0
 * 
 * @details 显示所有发现的设备，支持设备选择和状态显示。
 */

#ifndef DEVICETREEVIEW_H
#define DEVICETREEVIEW_H

#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include "core/device/devicemanager.h"

/**
 * @class DeviceTreeView
 * @brief 设备树视图控件
 * 
 * @details 该控件负责：
 * - 显示所有发现的设备列表
 * - 显示设备状态（在线/离线/繁忙）
 * - 支持设备选择
 * - 提供刷新设备列表功能
 */
class DeviceTreeView : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    explicit DeviceTreeView(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~DeviceTreeView();

    /**
     * @brief 获取当前选中的设备ID
     * @return 设备ID，如果没有选中返回空字符串
     */
    QString getSelectedDeviceId() const;

    /**
     * @brief 获取当前选中的设备信息
     * @return 设备信息
     */
    DeviceInfo getSelectedDevice() const;

    /**
     * @brief 刷新设备列表
     */
    void refreshDeviceList();

signals:
    /**
     * @brief 设备选择变化信号
     * @param deviceId 选中的设备ID
     */
    void deviceSelected(const QString& deviceId);

    /**
     * @brief 设备双击信号
     * @param deviceId 双击的设备ID
     */
    void deviceDoubleClicked(const QString& deviceId);

private slots:
    /**
     * @brief 处理设备添加
     * @param device 新添加的设备
     */
    void onDeviceAdded(const DeviceInfo& device);

    /**
     * @brief 处理设备移除
     * @param deviceId 移除的设备ID
     */
    void onDeviceRemoved(const QString& deviceId);

    /**
     * @brief 处理设备更新
     * @param device 更新的设备
     */
    void onDeviceUpdated(const DeviceInfo& device);

    /**
     * @brief 处理设备状态变化
     * @param deviceId 设备ID
     * @param status 新状态
     */
    void onDeviceStatusChanged(const QString& deviceId, DeviceStatus status);

    /**
     * @brief 处理树视图选择变化
     */
    void onSelectionChanged();

    /**
     * @brief 处理树视图双击
     * @param index 点击的索引
     */
    void onDoubleClicked(const QModelIndex& index);

private:
    /**
     * @brief 初始化UI
     */
    void initUI();

    /**
     * @brief 初始化模型
     */
    void initModel();

    /**
     * @brief 更新设备项
     * @param device 设备信息
     */
    void updateDeviceItem(const DeviceInfo& device);

    /**
     * @brief 获取设备状态图标
     * @param status 设备状态
     * @return 状态图标
     */
    QIcon getStatusIcon(DeviceStatus status) const;

    /**
     * @brief 获取设备状态文本
     * @param status 设备状态
     * @return 状态文本
     */
    QString getStatusText(DeviceStatus status) const;

    /**
     * @brief 查找设备项
     * @param deviceId 设备ID
     * @return 设备项，如果不存在返回nullptr
     */
    QStandardItem* findDeviceItem(const QString& deviceId) const;

    QTreeView* m_treeView;                 ///< 树视图控件
    QStandardItemModel* m_model;           ///< 标准项模型
    QLabel* m_titleLabel;                  ///< 标题标签
    QPushButton* m_refreshButton;          ///< 刷新按钮
    QVBoxLayout* m_mainLayout;             ///< 主布局
    QMap<QString, QStandardItem*> m_deviceItems;  ///< 设备项映射
};

#endif  // DEVICETREEVIEW_H