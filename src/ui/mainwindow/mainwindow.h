/**
 * @file mainwindow.h
 * @brief 主窗口类定义
 * @date 2026-06-01
 * @version 2.0
 * 
 * @details 应用程序主窗口，包含设备树、文件浏览器面板和传输进度面板。
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QSplitter>
#include <QLabel>
#include "ui/widgets/devicetreeview.h"
#include "ui/widgets/filebrowserpanel.h"
#include "ui/widgets/transferprogresspanel.h"
#include "core/device/devicemanager.h"
#include "core/network/devicediscoverer.h"
#include "core/network/filetransferservice.h"

/**
 * @class MainWindow
 * @brief 应用程序主窗口类
 * 
 * @details 提供应用程序的主界面框架，包括：
 * - 左侧：设备树视图
 * - 中间：双文件浏览器面板（本地和远程）
 * - 底部：传输进度面板
 * - 菜单栏、工具栏和状态栏
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    explicit MainWindow(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~MainWindow();

private slots:
    /**
     * @brief 处理设备选择变化
     * @param deviceId 选中的设备ID
     */
    void onDeviceSelected(const QString& deviceId);

    /**
     * @brief 处理设备双击
     * @param deviceId 双击的设备ID
     */
    void onDeviceDoubleClicked(const QString& deviceId);

    /**
     * @brief 处理本地文件选择变化
     * @param files 选中的文件列表
     */
    void onLocalFileSelected(const QList<FileItem>& files);

    /**
     * @brief 处理远程文件选择变化
     * @param files 选中的文件列表
     */
    void onRemoteFileSelected(const QList<FileItem>& files);

    /**
     * @brief 处理文件拖拽到远程面板
     * @param files 拖拽的文件列表
     * @param targetPath 目标路径
     */
    void onFilesDroppedToRemote(const QList<FileItem>& files, const QString& targetPath);

    /**
     * @brief 处理文件拖拽到本地面板
     * @param files 拖拽的文件列表
     * @param targetPath 目标路径
     */
    void onFilesDroppedToLocal(const QList<FileItem>& files, const QString& targetPath);

    /**
     * @brief 处理传输数量变化
     * @param count 传输数量
     */
    void onTransferCountChanged(int count);

    /**
     * @brief 处理设备发现
     * @param deviceInfo 发现的设备信息
     */
    void onDeviceDiscovered(const DeviceInfoData& deviceInfo);

    /**
     * @brief 处理设备离线
     * @param deviceId 离线的设备ID
     */
    void onDeviceOffline(const QString& deviceId);

    /**
     * @brief 新建文件菜单项
     */
    void onNewFile();

    /**
     * @brief 打开文件菜单项
     */
    void onOpenFile();

    /**
     * @brief 保存文件菜单项
     */
    void onSaveFile();

    /**
     * @brief 关于对话框
     */
    void onAbout();

    /**
     * @brief 设置菜单项
     */
    void onSettings();

    /**
     * @brief 刷新设备列表
     */
    void onRefreshDevices();

private:
    /**
     * @brief 初始化菜单栏
     */
    void initMenuBar();

    /**
     * @brief 初始化工具栏
     */
    void initToolBar();

    /**
     * @brief 初始化状态栏
     */
    void initStatusBar();

    /**
     * @brief 初始化中心区域
     */
    void initCentralWidget();

    /**
     * @brief 初始化核心服务
     */
    void initCoreServices();

    /**
     * @brief 连接信号槽
     */
    void connectSignals();

    /**
     * @brief 开始文件传输
     * @param files 要传输的文件列表
     * @param sourcePanel 源面板
     * @param targetPanel 目标面板
     */
    void startFileTransfer(const QList<FileItem>& files, FileBrowserPanel* sourcePanel, FileBrowserPanel* targetPanel);

    /**
     * @brief 更新状态栏
     */
    void updateStatusBar();

    /**
     * @brief 获取本地设备信息
     * @return 本地设备信息数据
     */
    DeviceInfoData getLocalDeviceInfoData() const;

    QToolBar* m_mainToolBar;               ///< 主工具栏
    QSplitter* m_mainSplitter;             ///< 主分割器
    QSplitter* m_fileSplitter;             ///< 文件面板分割器
    DeviceTreeView* m_deviceTreeView;      ///< 设备树视图
    FileBrowserPanel* m_localFilePanel;    ///< 本地文件面板
    FileBrowserPanel* m_remoteFilePanel;   ///< 远程文件面板
    TransferProgressPanel* m_transferPanel;///< 传输进度面板
    QLabel* m_statusLabel;                 ///< 状态标签
    QLabel* m_deviceCountLabel;            ///< 设备数量标签
    QLabel* m_transferCountLabel;          ///< 传输数量标签

    QString m_currentRemoteDeviceId;       ///< 当前选中的远程设备ID
};

#endif  // MAINWINDOW_H