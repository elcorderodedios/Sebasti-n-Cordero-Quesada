#include "ThreadManager.h"
#include <QDebug>
#include <QMutexLocker>

ThreadManager::ThreadManager(QObject* parent)
    : QObject(parent)
    , m_healthTimer(new QTimer(this))
    , m_healthMonitoringEnabled(true)
    , m_healthCheckInterval(5000) // 5 seconds default
{
    // Setup health monitoring timer
    m_healthTimer->setInterval(m_healthCheckInterval);
    connect(m_healthTimer, &QTimer::timeout, this, &ThreadManager::onHealthCheckTimer);
    
    if (m_healthMonitoringEnabled) {
        m_healthTimer->start();
    }
    
    logThreadEvent("ThreadManager initialized", "System");
}

ThreadManager::~ThreadManager()
{
    // Clean shutdown of all threads
    QMutexLocker locker(&m_threadsMutex);
    
    for (auto& managedThread : m_threads) {
        if (managedThread->thread && managedThread->thread->isRunning()) {
            managedThread->thread->quit();
            if (!managedThread->thread->wait(3000)) {
                managedThread->thread->terminate();
                managedThread->thread->wait(1000);
            }
        }
    }
    
    m_threads.clear();
    logThreadEvent("ThreadManager destroyed", "System");
}

void ThreadManager::registerThread(QThread* thread, const QString& name, QThread::Priority priority)
{
    if (!thread) {
        return;
    }
    
    QMutexLocker locker(&m_threadsMutex);
    
    // Check if thread is already registered
    if (findThread(thread)) {
        logThreadEvent("Thread already registered", name);
        return;
    }
    
    // Create managed thread info
    auto managedThread = std::make_shared<ManagedThread>();
    managedThread->thread = thread;
    managedThread->name = name;
    managedThread->priority = priority;
    managedThread->startTime = QDateTime::currentDateTime();
    managedThread->lastHealthCheck = QDateTime::currentDateTime();
    managedThread->isActive.storeRelease(0);
    managedThread->isRegistered = true;
    
    // Set thread priority
    thread->setPriority(priority);
    
    // Connect thread finished signal
    connect(thread, &QThread::finished, this, &ThreadManager::onThreadFinished);
    
    m_threads.append(managedThread);
    
    emit threadRegistered(name);
    logThreadEvent("Thread registered", name);
}

void ThreadManager::unregisterThread(QThread* thread)
{
    if (!thread) {
        return;
    }
    
    QMutexLocker locker(&m_threadsMutex);
    
    auto managedThread = findThread(thread);
    if (!managedThread) {
        return;
    }
    
    QString name = managedThread->name;
    managedThread->isRegistered = false;
    
    // Remove from list
    m_threads.removeAll(managedThread);
    
    emit threadUnregistered(name);
    logThreadEvent("Thread unregistered", name);
}

bool ThreadManager::startThread(const QString& name)
{
    QMutexLocker locker(&m_threadsMutex);
    
    auto managedThread = findThread(name);
    if (!managedThread || !managedThread->thread) {
        return false;
    }
    
    if (managedThread->thread->isRunning()) {
        logThreadEvent("Thread already running", name);
        return true;
    }
    
    managedThread->startTime = QDateTime::currentDateTime();
    managedThread->isActive.storeRelease(1);
    managedThread->thread->start();
    
    emit threadStarted(name);
    logThreadEvent("Thread started", name);
    return true;
}

bool ThreadManager::stopThread(const QString& name)
{
    QMutexLocker locker(&m_threadsMutex);
    
    auto managedThread = findThread(name);
    if (!managedThread || !managedThread->thread) {
        return false;
    }
    
    if (!managedThread->thread->isRunning()) {
        logThreadEvent("Thread already stopped", name);
        return true;
    }
    
    managedThread->isActive.storeRelease(0);
    managedThread->thread->quit();
    
    // Wait for graceful shutdown
    if (!managedThread->thread->wait(3000)) {
        logThreadEvent("Thread forced termination", name);
        managedThread->thread->terminate();
        managedThread->thread->wait(1000);
    }
    
    emit threadStopped(name);
    logThreadEvent("Thread stopped", name);
    return true;
}

bool ThreadManager::pauseThread(const QString& name)
{
    // Note: Qt doesn't have built-in thread pausing
    // This would need to be implemented at the WorkStation level
    logThreadEvent("Thread pause requested (implementation specific)", name);
    return true;
}

bool ThreadManager::resumeThread(const QString& name)
{
    // Note: Qt doesn't have built-in thread resuming
    // This would need to be implemented at the WorkStation level
    logThreadEvent("Thread resume requested (implementation specific)", name);
    return true;
}

QList<ThreadInfo> ThreadManager::getThreadInfo() const
{
    QMutexLocker locker(&m_threadsMutex);
    QList<ThreadInfo> infoList;
    
    for (const auto& managedThread : m_threads) {
        ThreadInfo info;
        info.name = managedThread->name;
        info.priority = managedThread->priority;
        info.startTime = managedThread->startTime;
        info.status = getThreadStatus(managedThread);
        info.threadId = managedThread->thread ? 
                       reinterpret_cast<qint64>(managedThread->thread->currentThreadId()) : 0;
        info.isActive = managedThread->isActive.loadAcquire() != 0;
        
        infoList.append(info);
    }
    
    return infoList;
}

ThreadInfo ThreadManager::getThreadInfo(const QString& name) const
{
    QMutexLocker locker(&m_threadsMutex);
    
    auto managedThread = findThread(name);
    if (!managedThread) {
        return ThreadInfo(); // Return empty info
    }
    
    ThreadInfo info;
    info.name = managedThread->name;
    info.priority = managedThread->priority;
    info.startTime = managedThread->startTime;
    info.status = getThreadStatus(managedThread);
    info.threadId = managedThread->thread ? 
                   reinterpret_cast<qint64>(managedThread->thread->currentThreadId()) : 0;
    info.isActive = managedThread->isActive.loadAcquire() != 0;
    
    return info;
}

int ThreadManager::getActiveThreadCount() const
{
    QMutexLocker locker(&m_threadsMutex);
    
    int count = 0;
    for (const auto& managedThread : m_threads) {
        if (managedThread->isActive.loadAcquire() != 0) {
            count++;
        }
    }
    
    return count;
}

bool ThreadManager::isThreadActive(const QString& name) const
{
    QMutexLocker locker(&m_threadsMutex);
    
    auto managedThread = findThread(name);
    return managedThread && (managedThread->isActive.loadAcquire() != 0);
}

void ThreadManager::enableHealthMonitoring(bool enabled)
{
    m_healthMonitoringEnabled = enabled;
    
    if (enabled) {
        m_healthTimer->start();
    } else {
        m_healthTimer->stop();
    }
    
    logThreadEvent(enabled ? "Health monitoring enabled" : "Health monitoring disabled", "System");
}

void ThreadManager::setHealthCheckInterval(int intervalMs)
{
    m_healthCheckInterval = intervalMs;
    m_healthTimer->setInterval(intervalMs);
    
    logThreadEvent(QString("Health check interval set to %1ms").arg(intervalMs), "System");
}

void ThreadManager::performCleanup()
{
    QMutexLocker locker(&m_threadsMutex);
    
    int threadsAffected = 0;
    QDateTime now = QDateTime::currentDateTime();
    
    // Clean up finished threads
    for (auto it = m_threads.begin(); it != m_threads.end();) {
        auto& managedThread = *it;
        
        if (!managedThread->thread) {
            it = m_threads.erase(it);
            threadsAffected++;
            continue;
        }
        
        if (managedThread->thread->isFinished()) {
            managedThread->isActive.storeRelease(0);
            threadsAffected++;
            logThreadEvent("Cleaned up finished thread", managedThread->name);
        }
        
        ++it;
    }
    
    emit cleanupPerformed(threadsAffected);
    logThreadEvent(QString("Cleanup performed, %1 threads affected").arg(threadsAffected), "System");
}

void ThreadManager::terminateUnresponsiveThreads()
{
    QMutexLocker locker(&m_threadsMutex);
    QDateTime threshold = QDateTime::currentDateTime().addSecs(-30); // 30 second timeout
    
    for (auto& managedThread : m_threads) {
        if (managedThread->thread && managedThread->thread->isRunning()) {
            if (managedThread->lastHealthCheck < threshold) {
                logThreadEvent("Terminating unresponsive thread", managedThread->name);
                managedThread->thread->terminate();
                managedThread->isActive.storeRelease(0);
                emit threadHealthAlert(managedThread->name, "Thread terminated due to unresponsiveness");
            }
        }
    }
}

void ThreadManager::onHealthCheckTimer()
{
    QMutexLocker locker(&m_threadsMutex);
    
    for (auto& managedThread : m_threads) {
        updateThreadHealth(managedThread);
    }
}

void ThreadManager::onThreadFinished()
{
    QThread* thread = qobject_cast<QThread*>(sender());
    if (!thread) {
        return;
    }
    
    QMutexLocker locker(&m_threadsMutex);
    auto managedThread = findThread(thread);
    
    if (managedThread) {
        managedThread->isActive.storeRelease(0);
        emit threadStopped(managedThread->name);
        logThreadEvent("Thread finished", managedThread->name);
    }
}

std::shared_ptr<ThreadManager::ManagedThread> ThreadManager::findThread(const QString& name) const
{
    for (const auto& managedThread : m_threads) {
        if (managedThread->name == name && managedThread->isRegistered) {
            return managedThread;
        }
    }
    return nullptr;
}

std::shared_ptr<ThreadManager::ManagedThread> ThreadManager::findThread(QThread* thread) const
{
    for (const auto& managedThread : m_threads) {
        if (managedThread->thread == thread && managedThread->isRegistered) {
            return managedThread;
        }
    }
    return nullptr;
}

void ThreadManager::updateThreadHealth(std::shared_ptr<ManagedThread> managedThread)
{
    if (!managedThread || !managedThread->thread) {
        return;
    }
    
    managedThread->lastHealthCheck = QDateTime::currentDateTime();
    
    // Check if thread is still responsive
    if (managedThread->thread->isRunning() && managedThread->isActive.loadAcquire() != 0) {
        // Thread appears healthy
        return;
    }
    
    // Check for potential issues
    if (managedThread->isActive.loadAcquire() != 0 && !managedThread->thread->isRunning()) {
        emit threadHealthAlert(managedThread->name, "Thread marked active but not running");
        managedThread->isActive.storeRelease(0);
    }
}

QString ThreadManager::getThreadStatus(std::shared_ptr<ManagedThread> managedThread) const
{
    if (!managedThread || !managedThread->thread) {
        return "Invalid";
    }
    
    if (!managedThread->isRegistered) {
        return "Unregistered";
    }
    
    if (managedThread->thread->isRunning()) {
        if (managedThread->isActive.loadAcquire() != 0) {
            return "Running";
        } else {
            return "Running (Inactive)";
        }
    } else if (managedThread->thread->isFinished()) {
        return "Finished";
    } else {
        return "Stopped";
    }
}

void ThreadManager::logThreadEvent(const QString& event, const QString& threadName)
{
    qDebug() << QString("[ThreadManager] %1: %2").arg(threadName, event);
}
