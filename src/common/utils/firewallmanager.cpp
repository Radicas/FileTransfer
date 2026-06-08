/**
 * @file firewallmanager.cpp
 * @brief 防火墙管理器类实现
 */

#include "common/utils/firewallmanager.h"
#include "common/logger/logger.h"
#include <QProcess>
#include <QMessageBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QSysInfo>
#include <QFile>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#elif defined(Q_OS_MAC)
#include <unistd.h>
#endif

FirewallManager& FirewallManager::instance()
{
    static FirewallManager instance;
    return instance;
}

FirewallManager::FirewallManager()
    : m_portsOpen(false)
    , m_hasAdmin(false)
    , m_checked(false)
{
}

FirewallManager::~FirewallManager()
{
}

bool FirewallManager::checkPortsOpen()
{
    if (m_checked)
    {
        return m_portsOpen;
    }

#ifdef Q_OS_WIN
    m_portsOpen = checkWindowsFirewall();
#elif defined(Q_OS_MAC)
    m_portsOpen = checkMacOSFirewall();
#else
    m_portsOpen = true;
#endif

    m_hasAdmin = hasAdminPrivileges();
    m_checked = true;

    return m_portsOpen;
}

bool FirewallManager::hasAdminPrivileges()
{
#ifdef Q_OS_WIN
    return checkWindowsAdmin();
#elif defined(Q_OS_MAC)
    return checkMacOSRoot();
#else
    return true;
#endif
}

bool FirewallManager::checkWindowsAdmin()
{
#ifdef Q_OS_WIN
    BOOL isAdmin = FALSE;
    HANDLE hToken = nullptr;
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        TOKEN_ELEVATION elevation;
        DWORD size;
        
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size))
        {
            isAdmin = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    
    return isAdmin != FALSE;
#else
    return false;
#endif
}

bool FirewallManager::checkMacOSRoot()
{
#ifdef Q_OS_MAC
    return geteuid() == 0;
#else
    return false;
#endif
}

bool FirewallManager::checkWindowsFirewall()
{
#ifdef Q_OS_WIN
    QProcess process;
    process.start("netsh", QStringList() << "advfirewall" << "firewall" 
                  << "show" << "rule" << "name=FileTransfer TCP In");
    process.waitForFinished(5000);
    
    QString output = process.readAllStandardOutput();
    bool tcpOpen = output.contains("38889");
    
    process.start("netsh", QStringList() << "advfirewall" << "firewall" 
                  << "show" << "rule" << "name=FileTransfer UDP In");
    process.waitForFinished(5000);
    
    output = process.readAllStandardOutput();
    bool udpOpen = output.contains("38888");
    
    return tcpOpen && udpOpen;
#else
    return true;
#endif
}

bool FirewallManager::checkMacOSFirewall()
{
#ifdef Q_OS_MAC
    QString output = runCommand("pfctl -s rules 2>/dev/null | grep -E 'port 38888|port 38889'");
    bool hasUdpRule = output.contains("port 38888");
    bool hasTcpRule = output.contains("port 38889");
    
    if (hasUdpRule && hasTcpRule)
    {
        Logger::instance().info("macOS防火墙规则已配置");
        return true;
    }
    
    QFile pfConf("/etc/pf.conf.filetransfer");
    if (pfConf.exists())
    {
        Logger::instance().info("发现pf配置文件，尝试加载规则");
        QProcess process;
        process.start("pfctl", QStringList() << "-ef" << "/etc/pf.conf.filetransfer");
        process.waitForFinished(5000);
        if (process.exitCode() == 0)
        {
            Logger::instance().info("pf规则加载成功");
            return true;
        }
    }
    
    Logger::instance().warning("macOS防火墙规则未配置");
    return false;
#else
    return true;
#endif
}

QString FirewallManager::runCommand(const QString& command)
{
    QProcess process;
    process.start(command);
    process.waitForFinished(5000);
    return process.readAllStandardOutput();
}

bool FirewallManager::addFirewallRules()
{
#ifdef Q_OS_WIN
    return addWindowsRules();
#elif defined(Q_OS_MAC)
    return addMacOSRules();
#else
    return true;
#endif
}

bool FirewallManager::addWindowsRules()
{
#ifdef Q_OS_WIN
    if (!m_hasAdmin)
    {
        Logger::instance().warning("没有管理员权限，无法配置防火墙");
        return false;
    }

    Logger::instance().info("正在添加Windows防火墙规则...");

    QStringList rules;
    rules << "advfirewall firewall add rule name=\"FileTransfer UDP In (设备发现)\" dir=in action=allow protocol=UDP localport=38888";
    rules << "advfirewall firewall add rule name=\"FileTransfer UDP Out (设备发现)\" dir=out action=allow protocol=UDP localport=38888";
    rules << "advfirewall firewall add rule name=\"FileTransfer TCP In (文件传输)\" dir=in action=allow protocol=TCP localport=38889";
    rules << "advfirewall firewall add rule name=\"FileTransfer TCP Out (文件传输)\" dir=out action=allow protocol=TCP localport=38889";

    bool allSuccess = true;
    for (const QString& rule : rules)
    {
        QProcess process;
        process.start("netsh", QStringList() << rule.split(" "));
        process.waitForFinished(10000);
        
        if (process.exitCode() != 0)
        {
            Logger::instance().error("添加防火墙规则失败: " + rule);
            allSuccess = false;
        }
        else
        {
            Logger::instance().info("添加防火墙规则成功: " + rule);
        }
    }

    if (allSuccess)
    {
        m_portsOpen = true;
        m_checked = true;
        emit firewallStatusChanged(true);
    }

    return allSuccess;
#else
    return false;
#endif
}

bool FirewallManager::addMacOSRules()
{
#ifdef Q_OS_MAC
    if (!m_hasAdmin)
    {
        Logger::instance().warning("没有root权限，无法配置防火墙");
        return false;
    }

    Logger::instance().info("正在添加macOS防火墙规则...");

    QString pfConf = "/etc/pf.conf.filetransfer";
    QString confContent = "# FileTransfer 端口规则\n"
                          "pass in proto udp from any to any port 38888\n"
                          "pass out proto udp from any to any port 38888\n"
                          "pass in proto tcp from any to any port 38889\n"
                          "pass out proto tcp from any to any port 38889\n";

    QProcess process;
    process.start("sh", QStringList() << "-c" << QString("echo '%1' > %2").arg(confContent, pfConf));
    process.waitForFinished(5000);

    if (process.exitCode() != 0)
    {
        Logger::instance().error("创建pf配置文件失败");
        return false;
    }

    process.start("pfctl", QStringList() << "-ef" << pfConf);
    process.waitForFinished(5000);

    if (process.exitCode() != 0)
    {
        Logger::instance().error("加载pf规则失败");
        return false;
    }

    Logger::instance().info("macOS防火墙规则添加成功");
    m_portsOpen = true;
    m_checked = true;
    emit firewallStatusChanged(true);

    return true;
#else
    return false;
#endif
}

QString FirewallManager::getStatusDescription()
{
    if (!m_checked)
    {
        checkPortsOpen();
    }

    QString desc;
    desc = "防火墙状态:\n";
    desc += QString("  UDP 38888 (设备发现): %1\n").arg(m_portsOpen ? "已开放" : "未开放");
    desc += QString("  TCP 38889 (文件传输): %1\n").arg(m_portsOpen ? "已开放" : "未开放");
    desc += QString("  管理员权限: %1\n").arg(m_hasAdmin ? "有" : "无");

    return desc;
}

int FirewallManager::showConfigDialog(QWidget* parent)
{
    if (!m_checked)
    {
        checkPortsOpen();
    }

    if (m_portsOpen)
    {
        Logger::instance().info("防火墙端口已开放，无需配置");
        return QMessageBox::Accepted;
    }

    QString message;
    message = "FileTransfer 需要开放以下端口才能正常工作:\n\n";
    message += "  • UDP 38888 - 设备发现广播\n";
    message += "  • TCP 38889 - 文件传输服务\n\n";
    
    if (m_hasAdmin)
    {
        message += "检测到您有管理员权限，可以自动配置防火墙。\n";
        message += "是否立即配置?";
        
        QMessageBox box(parent);
        box.setWindowTitle("防火墙配置");
        box.setText(message);
        box.setIcon(QMessageBox::Question);
        
        QPushButton* autoBtn = box.addButton("自动配置", QMessageBox::AcceptRole);
        QPushButton* manualBtn = box.addButton("稍后手动配置", QMessageBox::RejectRole);
        box.setDefaultButton(autoBtn);
        
        box.exec();
        
        if (box.clickedButton() == autoBtn)
        {
            bool success = addFirewallRules();
            if (success)
            {
                QMessageBox::information(parent, "配置成功", 
                    "防火墙规则已成功配置！\n现在可以正常使用文件传输功能。");
                emit configurationCompleted(true);
            }
            else
            {
                QMessageBox::warning(parent, "配置失败", 
                    "防火墙规则配置失败。\n请尝试手动运行脚本:\n"
                    "scripts\\add_firewall_rules.bat (Windows)\n"
                    "sudo bash scripts/add_firewall_rules_macos.sh (macOS)");
                emit configurationCompleted(false);
            }
            
            return QMessageBox::Accepted;
        }
        
        return QMessageBox::Rejected;
    }
    else
    {
        message += "需要管理员权限才能配置防火墙。\n\n";
        message += "请手动运行以下脚本:\n";
        
#ifdef Q_OS_WIN
        message += "  以管理员身份运行: scripts\\add_firewall_rules.bat\n";
#else
        message += "  sudo bash scripts/add_firewall_rules_macos.sh\n";
#endif
        
        QMessageBox box(parent);
        box.setWindowTitle("防火墙配置");
        box.setText(message);
        box.setIcon(QMessageBox::Warning);
        
        QPushButton* okBtn = box.addButton("我知道了", QMessageBox::AcceptRole);
        QPushButton* helpBtn = box.addButton("查看帮助", QMessageBox::HelpRole);
        box.setDefaultButton(okBtn);
        
        box.exec();
        
        if (box.clickedButton() == helpBtn)
        {
            QMessageBox::information(parent, "防火墙配置帮助",
                "防火墙配置步骤:\n\n"
                "Windows:\n"
                "1. 右键点击 scripts\\add_firewall_rules.bat\n"
                "2. 选择'以管理员身份运行'\n"
                "3. 等待配置完成\n\n"
                "macOS:\n"
                "1. 打开终端\n"
                "2. 运行: sudo bash scripts/add_firewall_rules_macos.sh\n"
                "3. 输入管理员密码\n"
                "4. 等待配置完成");
            return QMessageBox::Help;
        }
        
        return QMessageBox::Ok;
    }
}

void FirewallManager::initialize(QWidget* parent, bool autoConfigure)
{
    checkPortsOpen();
    
    Logger::instance().info(getStatusDescription());
    
    if (!m_portsOpen)
    {
        Logger::instance().warning("防火墙端口未开放，文件传输功能可能受限");
        
        if (autoConfigure && m_hasAdmin)
        {
            addFirewallRules();
        }
        else
        {
            showConfigDialog(parent);
        }
    }
}