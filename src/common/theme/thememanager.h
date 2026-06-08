/**
 * @file thememanager.h
 * @brief 主题管理器类，负责应用程序的样式管理和主题切换
 * @date 2026-03-02
 * @version 3.0.0
 * 
 * 更新说明：
 * - v3.0.0: 新增赛博朋克和极简现代两套主题
 * - v2.0.0: 优化为Fusion风格支持，QPalette优先于QSS应用
 */

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QObject>
#include <QPalette>
#include <QString>

/**
 * @enum ThemeType
 * @brief 主题类型枚举
 */
enum class ThemeType
{
    Industrial,  ///< 工业风复古主题
    Cyberpunk,   ///< 赛博朋克主题
    Modern       ///< 极简现代主题
};

/**
 * @class ThemeManager
 * @brief 主题管理器单例类，负责加载和应用应用程序样式
 * 
 * 该类提供统一的样式管理功能，支持：
 * - 从QSS文件加载样式
 * - 主题切换（浅色/深色）
 * - 统一的颜色方案管理
 * - 全局样式应用
 */
class ThemeManager : public QObject
{
    Q_OBJECT

   public:
    /**
     * @brief 获取单例实例
     * @return ThemeManager单例引用
     */
    static ThemeManager& instance();

    /**
     * @brief 初始化主题管理器
     * @return 初始化成功返回true，否则返回false
     */
    bool init();

    /**
     * @brief 应用当前主题到应用程序
     * @param app QApplication指针
     */
    void applyTheme(QApplication* app);

    /**
     * @brief 设置主题类型
     * @param theme 主题类型
     */
    void setTheme(ThemeType theme);

    /**
     * @brief 获取当前主题类型
     * @return 当前主题类型
     */
    ThemeType currentTheme() const;

    /**
     * @brief 从资源文件加载样式表
     * @param resourcePath 资源文件路径
     * @return 样式表内容
     */
    QString loadStyleSheet(const QString& resourcePath);

    /**
     * @brief 获取主题颜色
     * @param colorName 颜色名称（如 "primary", "secondary", "accent"等）
     * @return 颜色值
     */
    QColor themeColor(const QString& colorName) const;

    /**
     * @brief 获取当前样式表
     * @return 当前应用的样式表内容
     */
    QString currentStyleSheet() const;

   signals:
    /**
     * @brief 主题变更信号
     * @param theme 新主题类型
     */
    void themeChanged(ThemeType theme);

   private:
    struct ThemeInfo
    {
        QString name;
        QString styleFile;
    };

    ThemeManager();
    ~ThemeManager();
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    ThemeInfo getThemeInfo(ThemeType theme) const;
    void initColorScheme();
    void applyColorScheme();

    ThemeType m_currentTheme;             ///< 当前主题类型
    QString m_styleSheet;                 ///< 当前样式表
    QMap<QString, QColor> m_colorScheme;  ///< 颜色方案映射
    bool m_initialized;                   ///< 初始化标志
};

#endif  // THEMEMANAGER_H
