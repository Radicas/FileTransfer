/**
 * @file logger.h
 * @brief 日志系统模块，提供跨平台彩色日志输出功能
 * @date 2026-03-03
 * @version 2.0
 * 
 * @details 该日志系统支持：
 * - 跨平台彩色输出（Windows/Linux/macOS）
 * - 日志分级（Debug/Info/Warning/Error）
 * - 文件日志记录
 * - 线程安全
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <QDateTime>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QTextStream>
#include <iostream>
#include <sstream>
#include <string>

/**
 * @brief 日志级别枚举
 */
enum LogLevel
{
    Debug,    ///< 调试信息 - 青色
    Info,     ///< 一般信息 - 绿色
    Warning,  ///< 警告信息 - 黄色
    Error     ///< 错误信息 - 红色
};

/**
 * @brief 日志系统类，提供跨平台彩色日志记录功能
 * 
 * @details 该类采用单例模式，支持：
 * - 控制台彩色输出（使用std::cout/std::cerr）
 * - 文件日志记录
 * - 多线程安全
 * - 跨平台兼容（Windows/Linux/macOS）
 */
class Logger
{
   public:
    /**
     * @brief 获取Logger单例实例
     * @return Logger单例引用
     */
    static Logger& instance();

    /**
     * @brief 初始化日志系统
     * @param logFilePath 日志文件路径，如果为空则不记录到文件
     */
    void init(const QString& logFilePath);

    /**
     * @brief 设置控制台日志输出开关
     * @param enabled 是否启用控制台输出，默认为true
     */
    void setConsoleEnabled(bool enabled);

    /**
     * @brief 设置文件日志输出开关
     * @param enabled 是否启用文件输出，默认为true
     */
    void setFileEnabled(bool enabled);

    /**
     * @brief 设置最低日志级别
     * @param level 低于此级别的日志将不被输出
     */
    void setMinLevel(LogLevel level);

    /**
     * @brief 记录日志
     * @param level 日志级别
     * @param message 日志消息
     */
    void log(LogLevel level, const QString& message);

    /**
     * @brief 记录调试信息
     * @param message 日志消息
     */
    void debug(const QString& message);

    /**
     * @brief 记录一般信息
     * @param message 日志消息
     */
    void info(const QString& message);

    /**
     * @brief 记录警告信息
     * @param message 日志消息
     */
    void warning(const QString& message);

    /**
     * @brief 记录错误信息
     * @param message 日志消息
     */
    void error(const QString& message);

   private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief 将日志级别转换为字符串
     * @param level 日志级别
     * @return 日志级别字符串
     */
    QString levelToString(LogLevel level);

    /**
     * @brief 获取当前时间戳字符串
     * @return 格式化的时间戳字符串
     */
    QString getTimestamp();

    /**
     * @brief 初始化控制台颜色支持
     */
    void initConsoleColor();

    /**
     * @brief 设置控制台输出颜色
     * @param level 日志级别，根据级别设置不同颜色
     */
    void setConsoleColor(LogLevel level);

    /**
     * @brief 重置控制台颜色为默认
     */
    void resetConsoleColor();

    /**
     * @brief 输出彩色日志到控制台
     * @param level 日志级别
     * @param message 格式化的日志消息
     */
    void printColoredLog(LogLevel level, const QString& message);

    QFile* m_logFile;          ///< 日志文件指针
    QTextStream* m_logStream;  ///< 日志文件流
    QMutex m_mutex;            ///< 互斥锁，保证线程安全
    bool m_initialized;        ///< 初始化标志
    bool m_consoleEnabled;     ///< 控制台输出开关
    bool m_fileEnabled;        ///< 文件输出开关
    LogLevel m_minLevel;       ///< 最低日志级别
    bool m_colorSupported;     ///< 是否支持彩色输出

#ifdef Q_OS_WIN
    void* m_hConsole;  ///< Windows控制台句柄
#endif
};

#endif  // LOGGER_H
