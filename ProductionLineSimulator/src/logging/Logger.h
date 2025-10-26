#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QMutex>
#include <QTextStream>
#include <QFile>
#include <QDateTime>
#include <QQueue>
#include <QThread>
#include <QWaitCondition>
#include <QAtomicInt>

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4
};

struct LogEntry {
    QDateTime timestamp;
    LogLevel level;
    QString category;
    QString message;
    QString threadName;
};

class LoggerWorker;

class Logger : public QObject
{
    Q_OBJECT

public:
    explicit Logger(QObject* parent = nullptr);
    virtual ~Logger();
    
    // Logging methods
    void log(const QString& message, LogLevel level = LogLevel::Info, const QString& category = "General");
    void debug(const QString& message, const QString& category = "General");
    void info(const QString& message, const QString& category = "General");
    void warning(const QString& message, const QString& category = "General");
    void error(const QString& message, const QString& category = "General");
    void critical(const QString& message, const QString& category = "General");
    
    // Configuration
    void setLogLevel(LogLevel minLevel) { m_minLogLevel = minLevel; }
    void setLogToFile(bool enabled) { m_logToFile = enabled; }
    void setLogToConsole(bool enabled) { m_logToConsole = enabled; }
    void setMaxFileSize(qint64 maxSize) { m_maxFileSize = maxSize; }
    void setMaxBackupFiles(int maxBackups) { m_maxBackupFiles = maxBackups; }
    
    // File management
    void setLogFilePath(const QString& path);
    void rotateLogFile();
    void clearLogs();
    
    // Status
    qint64 getCurrentLogSize() const;
    int getPendingLogCount() const;

signals:
    void logEntryAdded(const LogEntry& entry);
    void logFileRotated(const QString& newFilePath);
    void logError(const QString& error);

private slots:
    void handleWorkerFinished();

private:
    // Core properties
    LogLevel m_minLogLevel;
    bool m_logToFile;
    bool m_logToConsole;
    QString m_logFilePath;
    qint64 m_maxFileSize;
    int m_maxBackupFiles;
    
    // Worker thread for async logging
    QThread* m_workerThread;
    LoggerWorker* m_worker;
    
    // Thread-safe queue
    mutable QMutex m_queueMutex;
    QQueue<LogEntry> m_logQueue;
    QWaitCondition m_queueCondition;
    QAtomicInt m_stopping;
    
    // Helper methods
    QString formatLogEntry(const LogEntry& entry) const;
    QString levelToString(LogLevel level) const;
    void initializeWorker();
    void shutdownWorker();
};

// Worker class for background logging
class LoggerWorker : public QObject
{
    Q_OBJECT

public:
    explicit LoggerWorker(Logger* logger, QObject* parent = nullptr);
    
public slots:
    void processLogs();

private:
    Logger* m_logger;
    QFile m_logFile;
    QTextStream m_stream;
    
    void writeLogEntry(const LogEntry& entry);
    void openLogFile();
    void closeLogFile();
    void checkFileRotation();
    QString getBackupFileName(int index) const;
};

#endif // LOGGER_H
