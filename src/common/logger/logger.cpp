#include "common/logger/logger.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <chrono>
#include <ctime>
#include <iomanip>

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

Logger::Logger()
    : m_logFile(nullptr),
      m_logStream(nullptr),
      m_initialized(false),
      m_consoleEnabled(true),
      m_fileEnabled(true),
      m_minLevel(Debug),
      m_colorSupported(false)
{
    initConsoleColor();
}

Logger::~Logger()
{
    QMutexLocker locker(&m_mutex);
    if (m_logStream)
    {
        delete m_logStream;
        m_logStream = nullptr;
    }
    if (m_logFile)
    {
        m_logFile->close();
        delete m_logFile;
        m_logFile = nullptr;
    }
}

void Logger::initConsoleColor()
{
#ifdef Q_OS_WIN
    m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (m_hConsole == INVALID_HANDLE_VALUE)
    {
        m_colorSupported = false;
        return;
    }

    DWORD mode = 0;
    if (GetConsoleMode(m_hConsole, &mode))
    {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (SetConsoleMode(m_hConsole, mode))
        {
            m_colorSupported = true;
        }
        else
        {
            DWORD fallbackMode = mode & ~ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(m_hConsole, fallbackMode);
            m_colorSupported = false;
        }
    }

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#else
    const char* term = getenv("TERM");
    m_colorSupported = (term != nullptr && strlen(term) > 0) || isatty(fileno(stdout));
#endif
}

void Logger::init(const QString& logFilePath)
{
    QMutexLocker locker(&m_mutex);
    if (m_initialized)
    {
        return;
    }

    if (!logFilePath.isEmpty())
    {
        QFileInfo fileInfo(logFilePath);
        QDir dir = fileInfo.absoluteDir();
        if (!dir.exists())
        {
            dir.mkpath(".");
        }

        m_logFile = new QFile(logFilePath);
        if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        {
            std::cerr << "无法打开日志文件: " << logFilePath.toStdString() << std::endl;
            delete m_logFile;
            m_logFile = nullptr;
        }
        else
        {
            m_logStream = new QTextStream(m_logFile);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            m_logStream->setCodec("UTF-8");
#endif
        }
    }

    m_initialized = true;
    info("日志系统初始化成功");
}

void Logger::setConsoleEnabled(bool enabled)
{
    m_consoleEnabled = enabled;
}

void Logger::setFileEnabled(bool enabled)
{
    m_fileEnabled = enabled;
}

void Logger::setMinLevel(LogLevel level)
{
    m_minLevel = level;
}

QString Logger::getTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &now_time_t);
#else
    localtime_r(&now_time_t, &tm_buf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << now_ms.count();

    return QString::fromStdString(oss.str());
}

QString Logger::levelToString(LogLevel level)
{
    switch (level)
    {
        case Debug:
            return "DEBUG";
        case Info:
            return "INFO";
        case Warning:
            return "WARN";
        case Error:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

void Logger::setConsoleColor(LogLevel level)
{
    if (!m_colorSupported)
        return;

    const char* colorCode = "";
    switch (level)
    {
        case Debug:
            colorCode = "\033[36m";
            break;
        case Info:
            colorCode = "\033[32m";
            break;
        case Warning:
            colorCode = "\033[33m";
            break;
        case Error:
            colorCode = "\033[31m";
            break;
    }
    std::cout << colorCode;
}

void Logger::resetConsoleColor()
{
    if (!m_colorSupported)
        return;
    std::cout << "\033[0m";
}

void Logger::printColoredLog(LogLevel level, const QString& message)
{
    setConsoleColor(level);

    QString levelStr = levelToString(level);
    QString timestamp = getTimestamp();

    std::cout << "[" << timestamp.toStdString() << "] "
              << "[" << levelStr.toStdString() << "] " << message.toStdString();

    resetConsoleColor();
    std::cout << std::endl;

    std::cout.flush();
}

void Logger::log(LogLevel level, const QString& message)
{
    if (level < m_minLevel)
    {
        return;
    }

    QMutexLocker locker(&m_mutex);

    QString timestamp = getTimestamp();
    QString levelStr = levelToString(level);
    QString logMessage = QString("[%1] [%2] %3").arg(timestamp, levelStr, message);

    if (m_fileEnabled && m_initialized && m_logStream)
    {
        *m_logStream << logMessage << "\n";
        m_logStream->flush();
    }

    if (m_consoleEnabled)
    {
        printColoredLog(level, message);
    }
}

void Logger::debug(const QString& message)
{
    log(Debug, message);
}

void Logger::info(const QString& message)
{
    log(Info, message);
}

void Logger::warning(const QString& message)
{
    log(Warning, message);
}

void Logger::error(const QString& message)
{
    log(Error, message);
}
