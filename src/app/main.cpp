/**
 * @file main.cpp
 * @brief 应用程序入口文件
 * @details 负责应用程序初始化、主窗口创建和事件循环
 */

#include <QApplication>
#include <QStyleFactory>
#include "common/logger/logger.h"
#include "common/theme/thememanager.h"
#include "common/utils/firewallmanager.h"
#include "ui/mainwindow/mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    Q_INIT_RESOURCE(theme);

    app.setApplicationName("FileTransfer");
    app.setOrganizationName("FileTransfer");

    app.setStyle(QStyleFactory::create("Fusion"));

    ThemeManager::instance().init();
    ThemeManager::instance().applyTheme(&app);

    Logger::instance().info("应用程序启动");
    Logger::instance().info("系统: " + QSysInfo::prettyProductName());
    Logger::instance().info("主机名: " + QSysInfo::machineHostName());

    FirewallManager::instance().initialize(nullptr, false);

    MainWindow window;
    window.show();

    int result = app.exec();

    Logger::instance().info("应用程序退出，返回码: " + QString::number(result));
    return result;
}