/**
 * @file thememanager.cpp
 * @brief 主题管理器实现
 * @date 2026-03-02
 * @version 3.0.0
 * 
 * 更新说明：
 * - v3.0.0: 新增赛博朋克和极简现代两套主题
 * - v2.0.0: 优化为Fusion风格支持，QPalette优先于QSS应用
 */

#include "common/theme/thememanager.h"
#include <QDebug>
#include <QFile>
#include "common/logger/logger.h"

ThemeManager& ThemeManager::instance()
{
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager() : m_currentTheme(ThemeType::Industrial), m_initialized(false)
{
    initColorScheme();
}

ThemeManager::~ThemeManager() {}

ThemeManager::ThemeInfo ThemeManager::getThemeInfo(ThemeType theme) const
{
    switch (theme)
    {
        case ThemeType::Cyberpunk:
            return {"赛博朋克", ":/styles/theme-cyberpunk.qss"};
        case ThemeType::Modern:
            return {"极简现代", ":/styles/theme-modern.qss"};
        case ThemeType::Industrial:
        default:
            return {"工业风复古", ":/styles/theme-industrial.qss"};
    }
}

bool ThemeManager::init()
{
    if (m_initialized)
    {
        return true;
    }

    ThemeInfo themeInfo = getThemeInfo(m_currentTheme);
    m_styleSheet = loadStyleSheet(themeInfo.styleFile);
    if (m_styleSheet.isEmpty())
    {
        Logger::instance().warning(QString("主题管理器：无法加载%1样式表，使用默认样式").arg(themeInfo.name));
    }

    m_initialized = true;
    Logger::instance().info("主题管理器初始化完成");
    return true;
}

void ThemeManager::applyTheme(QApplication* app)
{
    if (!app)
    {
        Logger::instance().error("主题管理器：QApplication指针为空");
        return;
    }

    if (!m_initialized)
    {
        init();
    }

    applyColorScheme();

    app->setStyleSheet(m_styleSheet);

    ThemeInfo themeInfo = getThemeInfo(m_currentTheme);
    Logger::instance().info(QString("%1主题已应用到应用程序").arg(themeInfo.name));
}

void ThemeManager::setTheme(ThemeType theme)
{
    if (m_currentTheme == theme)
    {
        return;
    }

    m_currentTheme = theme;
    m_initialized = false;
    initColorScheme();
    init();
    emit themeChanged(theme);

    ThemeInfo themeInfo = getThemeInfo(theme);
    Logger::instance().info(QString("主题已切换为: %1").arg(themeInfo.name));
}

ThemeType ThemeManager::currentTheme() const
{
    return m_currentTheme;
}

QString ThemeManager::loadStyleSheet(const QString& resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        Logger::instance().error("主题管理器：无法打开样式文件: " + resourcePath);
        return QString();
    }

    QString styleSheet = QString::fromUtf8(file.readAll());
    file.close();

    return styleSheet;
}

QColor ThemeManager::themeColor(const QString& colorName) const
{
    return m_colorScheme.value(colorName, QColor());
}

QString ThemeManager::currentStyleSheet() const
{
    return m_styleSheet;
}

void ThemeManager::initColorScheme()
{
    m_colorScheme.clear();

    switch (m_currentTheme)
    {
        case ThemeType::Cyberpunk:
            m_colorScheme["primary"] = QColor("#00F5FF");
            m_colorScheme["primaryHover"] = QColor("#00D4FF");
            m_colorScheme["primaryPressed"] = QColor("#00A8B8");

            m_colorScheme["secondary"] = QColor("#9D00FF");
            m_colorScheme["secondaryHover"] = QColor("#B400FF");

            m_colorScheme["accent"] = QColor("#FF00FF");

            m_colorScheme["background"] = QColor("#12121A");
            m_colorScheme["backgroundSecondary"] = QColor("#1A1A2E");
            m_colorScheme["backgroundTertiary"] = QColor("#252540");

            m_colorScheme["surface"] = QColor("#0A0A0F");
            m_colorScheme["surfaceHover"] = QColor("#1A1A2E");

            m_colorScheme["textPrimary"] = QColor("#FFFFFF");
            m_colorScheme["textSecondary"] = QColor("#B8B8D0");
            m_colorScheme["textMuted"] = QColor("#6B6B8D");
            m_colorScheme["textOnPrimary"] = QColor("#0A0A0F");

            m_colorScheme["border"] = QColor("#252540");
            m_colorScheme["borderFocus"] = QColor("#00F5FF");

            m_colorScheme["success"] = QColor("#05FFA1");
            m_colorScheme["warning"] = QColor("#FFD600");
            m_colorScheme["error"] = QColor("#FF2A6D");
            m_colorScheme["info"] = QColor("#00D4FF");

            m_colorScheme["shadow"] = QColor(0, 0, 0, 80);
            break;

        case ThemeType::Modern:
            m_colorScheme["primary"] = QColor("#3B82F6");
            m_colorScheme["primaryHover"] = QColor("#2563EB");
            m_colorScheme["primaryPressed"] = QColor("#1D4ED8");

            m_colorScheme["secondary"] = QColor("#71717A");
            m_colorScheme["secondaryHover"] = QColor("#52525B");

            m_colorScheme["accent"] = QColor("#8B5CF6");

            m_colorScheme["background"] = QColor("#FAFAFA");
            m_colorScheme["backgroundSecondary"] = QColor("#FFFFFF");
            m_colorScheme["backgroundTertiary"] = QColor("#F5F5F5");

            m_colorScheme["surface"] = QColor("#FFFFFF");
            m_colorScheme["surfaceHover"] = QColor("#F5F5F5");

            m_colorScheme["textPrimary"] = QColor("#18181B");
            m_colorScheme["textSecondary"] = QColor("#71717A");
            m_colorScheme["textMuted"] = QColor("#A1A1AA");
            m_colorScheme["textOnPrimary"] = QColor("#FFFFFF");

            m_colorScheme["border"] = QColor("#E5E5E5");
            m_colorScheme["borderFocus"] = QColor("#3B82F6");

            m_colorScheme["success"] = QColor("#22C55E");
            m_colorScheme["warning"] = QColor("#F59E0B");
            m_colorScheme["error"] = QColor("#EF4444");
            m_colorScheme["info"] = QColor("#3B82F6");

            m_colorScheme["shadow"] = QColor(0, 0, 0, 20);
            break;

        case ThemeType::Industrial:
        default:
            m_colorScheme["primary"] = QColor("#B87333");
            m_colorScheme["primaryHover"] = QColor("#C17F59");
            m_colorScheme["primaryPressed"] = QColor("#9A612B");

            m_colorScheme["secondary"] = QColor("#636E72");
            m_colorScheme["secondaryHover"] = QColor("#B2BEC3");

            m_colorScheme["accent"] = QColor("#B5A642");

            m_colorScheme["background"] = QColor("#2C2F31");
            m_colorScheme["backgroundSecondary"] = QColor("#232628");
            m_colorScheme["backgroundTertiary"] = QColor("#1A1D1E");

            m_colorScheme["surface"] = QColor("#232628");
            m_colorScheme["surfaceHover"] = QColor("#2C2F31");

            m_colorScheme["textPrimary"] = QColor("#E8E8E8");
            m_colorScheme["textSecondary"] = QColor("#9CA3AF");
            m_colorScheme["textMuted"] = QColor("#6B7280");
            m_colorScheme["textOnPrimary"] = QColor("#1A1D1E");

            m_colorScheme["border"] = QColor("#3D4042");
            m_colorScheme["borderFocus"] = QColor("#B87333");

            m_colorScheme["success"] = QColor("#4A7C59");
            m_colorScheme["warning"] = QColor("#C9A227");
            m_colorScheme["error"] = QColor("#8B3A3A");
            m_colorScheme["info"] = QColor("#4A6B8A");

            m_colorScheme["shadow"] = QColor(0, 0, 0, 50);
            break;
    }
}

void ThemeManager::applyColorScheme()
{
    QPalette palette;

    palette.setColor(QPalette::Window, m_colorScheme["background"]);
    palette.setColor(QPalette::WindowText, m_colorScheme["textPrimary"]);
    palette.setColor(QPalette::Base, m_colorScheme["surface"]);
    palette.setColor(QPalette::AlternateBase, m_colorScheme["backgroundSecondary"]);
    palette.setColor(QPalette::ToolTipBase, m_colorScheme["surface"]);
    palette.setColor(QPalette::ToolTipText, m_colorScheme["textPrimary"]);
    palette.setColor(QPalette::Text, m_colorScheme["textPrimary"]);
    palette.setColor(QPalette::Button, m_colorScheme["surface"]);
    palette.setColor(QPalette::ButtonText, m_colorScheme["textPrimary"]);
    palette.setColor(QPalette::BrightText, m_colorScheme["error"]);
    palette.setColor(QPalette::Link, m_colorScheme["primary"]);
    palette.setColor(QPalette::Highlight, m_colorScheme["primary"]);
    palette.setColor(QPalette::HighlightedText, m_colorScheme["textOnPrimary"]);

    qApp->setPalette(palette);
}
