#include "Logger.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include <QMutexLocker>

Logger::Logger(QObject* parent)
    : QObject(parent)
    , m_minLogLevel(LogLevel::Info)
    , m_logToFile(true)
    , m_logToConsole(true)
    , m_maxFileSize(10 * 1024 * 1024) // 10 MB
    , m_maxBackupFiles(5)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
    , m_stopping(0)
{
    // Set default log file path
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.prodline";
    QDir().mkpath(configDir + "/logs");
    m_logFilePath = configDir + "/logs/app.log";
    
    initializeWorker();
    
    info("Logger initialized", "System");
}

Logger::~Logger()
{
    info("Logger shutting down", "System");
    shutdownWorker();
}

void Logger::log(const QString& message, LogLevel level, const QString& category)
{
    if (level < m_minLogLevel) {
        return;
    }
    
    // Create log entry
    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.level = level;
    entry.category = category;
    entry.message = message;
    entry.threadName = QThread::currentThread()->objectName();
    if (entry.threadName.isEmpty()) {
        entry.threadName = QString("Thread-%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    }
    
    // Add to queue
    {
        QMutexLocker locker(&m_queueMutex);
        m_logQueue.enqueue(entry);
        m_queueCondition.wakeOne();
    }
    
    // Console output (synchronous)
    if (m_logToConsole) {
        QString formattedEntry = formatLogEntry(entry);
        
        if (level >= LogLevel::Error) {
            qCritical().noquote() << formattedEntry;
        } else if (level >= LogLevel::Warning) {
            qWarning().noquote() << formattedEntry;
        } else {
            qDebug().noquote() << formattedEntry;
        }
    }
    
    emit logEntryAdded(entry);
}

void Logger::debug(const QString& message, const QString& category)
{
    log(message, LogLevel::Debug, category);
}

void Logger::info(const QString& message, const QString& category)
{
    log(message, LogLevel::Info, category);
}

void Logger::warning(const QString& message, const QString& category)
{
    log(message, LogLevel::Warning, category);
}

void Logger::error(const QString& message, const QString& category)
{
    log(message, LogLevel::Error, category);
}

void Logger::critical(const QString& message, const QString& category)
{
    log(message, LogLevel::Critical, category);
}

void Logger::setLogFilePath(const QString& path)
{
    m_logFilePath = path;
    
    // Ensure directory exists
    QFileInfo fileInfo(path);
    QDir().mkpath(fileInfo.absolutePath());
}

void Logger::rotateLogFile()
{
    // This will be handled by the worker thread
    info("Log file rotation requested", "Logger");
}

void Logger::clearLogs()
{
    QMutexLocker locker(&m_queueMutex);
    m_logQueue.clear();
    
    // Clear log file
    QFile logFile(m_logFilePath);
    if (logFile.exists()) {
        logFile.remove();
    }
    
    info("Logs cleared", "Logger");
}

qint64 Logger::getCurrentLogSize() const
{
    QFileInfo fileInfo(m_logFilePath);
    return fileInfo.exists() ? fileInfo.size() : 0;
}

int Logger::getPendingLogCount() const
{
    QMutexLocker locker(&m_queueMutex);
    return m_logQueue.size();
}

void Logger::handleWorkerFinished()
{
    // Worker thread finished
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
        m_workerThread->deleteLater();
        m_workerThread = nullptr;
    }
}

QString Logger::formatLogEntry(const LogEntry& entry) const
{
    QString levelStr = levelToString(entry.level);
    return QString("[%1] [%2] [%3] [%4] %5")
           .arg(entry.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz"))
           .arg(levelStr)
           .arg(entry.category)
           .arg(entry.threadName)
           .arg(entry.message);
}

QString Logger::levelToString(LogLevel level) const
{
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Critical: return "CRIT ";
    }
    return "UNKNOWN";
}

void Logger::initializeWorker()
{
    m_workerThread = new QThread(this);
    m_worker = new LoggerWorker(this);
    m_worker->moveToThread(m_workerThread);
    
    connect(m_workerThread, &QThread::started, m_worker, &LoggerWorker::processLogs);
    connect(m_workerThread, &QThread::finished, this, &Logger::handleWorkerFinished);
    
    m_workerThread->start();
}

void Logger::shutdownWorker()
{
    m_stopping.storeRelease(1);
    
    // Wake up worker thread
    {
        QMutexLocker locker(&m_queueMutex);
        m_queueCondition.wakeAll();
    }
    
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        if (!m_workerThread->wait(3000)) {
            m_workerThread->terminate();
            m_workerThread->wait(1000);
        }
    }
}

// LoggerWorker implementation
LoggerWorker::LoggerWorker(Logger* logger, QObject* parent)
    : QObject(parent)
    , m_logger(logger)
{
}

void LoggerWorker::processLogs()
{
    openLogFile();
    
    while (m_logger->m_stopping.loadAcquire() == 0) {
        LogEntry entry;
        bool hasEntry = false;
        
        // Get next log entry
        {
            QMutexLocker locker(&m_logger->m_queueMutex);
            
            // Wait for entries or stop signal
            while (m_logger->m_logQueue.isEmpty() && m_logger->m_stopping.loadAcquire() == 0) {
                m_logger->m_queueCondition.wait(&m_logger->m_queueMutex, 100);
            }
            
            if (!m_logger->m_logQueue.isEmpty()) {
                entry = m_logger->m_logQueue.dequeue();
                hasEntry = true;
            }
        }
        
        if (hasEntry) {
            writeLogEntry(entry);
            checkFileRotation();
        }
    }
    
    // Process remaining entries
    while (true) {
        LogEntry entry;
        bool hasEntry = false;
        
        {
            QMutexLocker locker(&m_logger->m_queueMutex);
            if (!m_logger->m_logQueue.isEmpty()) {
                entry = m_logger->m_logQueue.dequeue();
                hasEntry = true;
            }
        }
        
        if (!hasEntry) {
            break;
        }
        
        writeLogEntry(entry);
    }
    
    closeLogFile();
}

void LoggerWorker::writeLogEntry(const LogEntry& entry)
{
    if (!m_logger->m_logToFile || !m_logFile.isOpen()) {
        return;
    }
    
    QString formatted = m_logger->formatLogEntry(entry);
    m_stream << formatted << Qt::endl;
    m_stream.flush();
}

void LoggerWorker::openLogFile()
{
    if (m_logger->m_logFilePath.isEmpty()) {
        return;
    }
    
    m_logFile.setFileName(m_logger->m_logFilePath);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_stream.setDevice(&m_logFile);
    } else {
        emit m_logger->logError(QString("Failed to open log file: %1").arg(m_logger->m_logFilePath));
    }
}

void LoggerWorker::closeLogFile()
{
    if (m_logFile.isOpen()) {
        m_stream.flush();
        m_logFile.close();
    }
}

void LoggerWorker::checkFileRotation()
{
    if (!m_logFile.isOpen() || m_logger->m_maxFileSize <= 0) {
        return;
    }
    
    if (m_logFile.size() >= m_logger->m_maxFileSize) {
        // Perform rotation
        closeLogFile();
        
        // Move existing backups
        for (int i = m_logger->m_maxBackupFiles - 1; i > 0; --i) {
            QString oldName = getBackupFileName(i - 1);
            QString newName = getBackupFileName(i);
            
            if (QFile::exists(newName)) {
                QFile::remove(newName);
            }
            
            if (QFile::exists(oldName)) {
                QFile::rename(oldName, newName);
            }
        }
        
        // Move current log to backup
        QString backupName = getBackupFileName(0);
        if (QFile::exists(backupName)) {
            QFile::remove(backupName);
        }
        QFile::rename(m_logger->m_logFilePath, backupName);
        
        // Reopen log file
        openLogFile();
        
        emit m_logger->logFileRotated(m_logger->m_logFilePath);
    }
}

QString LoggerWorker::getBackupFileName(int index) const
{
    QFileInfo fileInfo(m_logger->m_logFilePath);
    return QString("%1/%2.%3.%4")
           .arg(fileInfo.absolutePath())
           .arg(fileInfo.baseName())
           .arg(index)
           .arg(fileInfo.completeSuffix());
}

#include "Logger.moc"
