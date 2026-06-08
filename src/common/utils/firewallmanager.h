/**
 * @file firewallmanager.h
 * @brief 防火墙管理器类定义
 * @date 2026-06-01
 * @version 1.0
 * 
 * @details 自动检测防火墙状态，提示用户配置防火墙规则。
 * 支持Windows和macOS系统。
 */

#ifndef FIREWALLMANAGER_H
#define FIREWALLMANAGER_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QMessageBox>
#include <QApplication>

/**
 * @class FirewallManager
 * @brief 防火墙管理器类
 * 
 * @details 该类负责：
 * - 检测防火墙端口是否开放
 * - 提示用户配置防火墙规则
 * - 提供一键配置功能
 */
class FirewallManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return FirewallManager单例引用
     */
    static FirewallManager& instance();

    /**
     * @brief 检查防火墙端口是否已开放
     * @return 是否已开放
     */
    bool checkPortsOpen();

    /**
     * @brief 检查是否有管理员权限
     * @return 是否有管理员权限
     */
    bool hasAdminPrivileges();

    /**
     * @brief 尝试添加防火墙规则
     * @return 是否成功
     */
    bool addFirewallRules();

    /**
     * @brief 显示防火墙配置提示对话框
     * @param parent 父窗口
     * @return 用户选择结果
     */
    int showConfigDialog(QWidget* parent = nullptr);

    /**
     * @brief 获取防火墙状态描述
     * @return 状态描述文本
     */
    QString getStatusDescription();

    /**
     * @brief 初始化防火墙检查
     * @param parent 父窗口
     * @param autoConfigure 是否自动尝试配置
     */
    void initialize(QWidget* parent = nullptr, bool autoConfigure = false);

signals:
    /**
     * @brief 防火墙状态变化信号
     * @param portsOpen 端口是否开放
     */
    void firewallStatusChanged(bool portsOpen);

    /**
     * @brief 防火墙配置完成信号
     * @param success 是否成功
     */
    void configurationCompleted(bool success);

private:
    FirewallManager();
    ~FirewallManager();
    FirewallManager(const FirewallManager&) = delete;
    FirewallManager& operator=(const FirewallManager&) = delete;

    /**
     * @brief 检查Windows防火墙规则
     * @return 是否已配置
     */
    bool checkWindowsFirewall();

    /**
     * @brief 检查macOS防火墙规则
     * @return 是否已配置
     */
    bool checkMacOSFirewall();

    /**
     * @brief 添加Windows防火墙规则
     * @return 是否成功
     */
    bool addWindowsRules();

    /**
     * @brief 添加macOS防火墙规则
     * @return 是否成功
     */
    bool addMacOSRules();

    /**
     * @brief 运行命令
     * @param command 命令字符串
     * @return 命令输出
     */
    QString runCommand(const QString& command);

    /**
     * @brief 检查Windows管理员权限
     * @return 是否有权限
     */
    bool checkWindowsAdmin();

    /**
     * @brief 检查macOS root权限
     * @return 是否有权限
     */
    bool checkMacOSRoot();

    bool m_portsOpen;           ///< 端口是否开放
    bool m_hasAdmin;            ///< 是否有管理员权限
    bool m_checked;             ///< 是否已检查
};

#endif  // FIREWALLMANAGER_H