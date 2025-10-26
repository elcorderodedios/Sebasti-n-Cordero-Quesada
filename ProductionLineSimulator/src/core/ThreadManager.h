#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QAtomicInt>
#include <QList>
#include <QDateTime>
#include <memory>

struct ThreadInfo {
    QString name;
    QThread::Priority priority;
    QDateTime startTime;
    QString status;
    qint64 threadId;
    bool isActive;
};

class ThreadManager : public QObject
{
    Q_OBJECT

public:
    explicit ThreadManager(QObject* parent = nullptr);
    virtual ~ThreadManager();
    
    // Thread registration
    void registerThread(QThread* thread, const QString& name, QThread::Priority priority = QThread::NormalPriority);
    void unregisterThread(QThread* thread);
    
    // Thread lifecycle management
    bool startThread(const QString& name);
    bool stopThread(const QString& name);
    bool pauseThread(const QString& name);
    bool resumeThread(const QString& name);
    
    // Status queries
    QList<ThreadInfo> getThreadInfo() const;
    ThreadInfo getThreadInfo(const QString& name) const;
    int getActiveThreadCount() const;
    bool isThreadActive(const QString& name) const;
    
    // Health monitoring
    void enableHealthMonitoring(bool enabled = true);
    void setHealthCheckInterval(int intervalMs);
    
    // Cleanup operations
    void performCleanup();
    void terminateUnresponsiveThreads();

signals:
    void threadRegistered(const QString& threadName);
    void threadUnregistered(const QString& threadName);
    void threadStarted(const QString& threadName);
    void threadStopped(const QString& threadName);
    void threadHealthAlert(const QString& threadName, const QString& issue);
    void cleanupPerformed(int threadsAffected);

private slots:
    void onHealthCheckTimer();
    void onThreadFinished();

private:
    struct ManagedThread {
        QThread* thread;
        QString name;
        QThread::Priority priority;
        QDateTime startTime;
        QDateTime lastHealthCheck;
        QAtomicInt isActive;
        bool isRegistered;
    };
    
    mutable QMutex m_threadsMutex;
    QList<std::shared_ptr<ManagedThread>> m_threads;
    
    // Health monitoring
    QTimer* m_healthTimer;
    bool m_healthMonitoringEnabled;
    int m_healthCheckInterval;
    
    // Helper methods
    std::shared_ptr<ManagedThread> findThread(const QString& name) const;
    std::shared_ptr<ManagedThread> findThread(QThread* thread) const;
    void updateThreadHealth(std::shared_ptr<ManagedThread> managedThread);
    QString getThreadStatus(std::shared_ptr<ManagedThread> managedThread) const;
    void logThreadEvent(const QString& event, const QString& threadName);
};

#endif // THREADMANAGER_H
