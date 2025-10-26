#include "WorkStation.h"
#include <QRandomGenerator>
#include <QDebug>
#include <QCoreApplication>

WorkStation::WorkStation(const QString& name, QObject* parent)
    : QThread(parent)
    , m_name(name)
    , m_state(static_cast<int>(StationState::Idle))
    , m_stopping(0)
    , m_minProcessingTime(100)
    , m_maxProcessingTime(500)
    , m_failureRate(0.0)
    , m_processedCount(0)
    , m_rejectedCount(0)
    , m_metricsTimer(nullptr)
{
    // Create metrics timer in main thread
    m_metricsTimer = new QTimer(this);
    m_metricsTimer->setInterval(1000); // Update every second
    connect(m_metricsTimer, &QTimer::timeout, this, &WorkStation::onMetricsTimer);
}

WorkStation::~WorkStation()
{
    stopStation();
    wait(); // Wait for thread to finish
}

void WorkStation::startStation()
{
    QMutexLocker locker(&m_controlMutex);
    
    if (getState() == StationState::Idle || getState() == StationState::Stopped) {
        setState(StationState::Running);
        m_stopping.storeRelease(0);
        m_startTime = QDateTime::currentDateTime();
        
        if (m_metricsTimer) {
            m_metricsTimer->start();
        }
        
        start(); // Start thread
        onStationStarted();
        logActivity("Station started");
    }
}

void WorkStation::pauseStation()
{
    QMutexLocker locker(&m_controlMutex);
    
    if (getState() == StationState::Running) {
        setState(StationState::Paused);
        logActivity("Station paused");
    }
}

void WorkStation::resumeStation()
{
    QMutexLocker locker(&m_controlMutex);
    
    if (getState() == StationState::Paused) {
        setState(StationState::Running);
        m_pauseCondition.wakeAll();
        logActivity("Station resumed");
    }
}

void WorkStation::stopStation()
{
    {
        QMutexLocker locker(&m_controlMutex);
        setState(StationState::Stopping);
        m_stopping.storeRelease(1);
        m_pauseCondition.wakeAll();
    }
    
    if (m_metricsTimer) {
        m_metricsTimer->stop();
    }
    
    // Wait for thread to finish
    if (isRunning()) {
        wait(5000); // 5 second timeout
        if (isRunning()) {
            terminate(); // Force termination if needed
            wait(1000);
        }
    }
    
    setState(StationState::Stopped);
    onStationStopped();
    logActivity("Station stopped");
}

void WorkStation::setProcessingTime(int minMs, int maxMs)
{
    m_minProcessingTime = minMs;
    m_maxProcessingTime = maxMs;
}

void WorkStation::setInputBuffer(std::shared_ptr<Buffer<std::shared_ptr<Product>>> buffer)
{
    m_inputBuffer = buffer;
}

void WorkStation::setOutputBuffer(std::shared_ptr<Buffer<std::shared_ptr<Product>>> buffer)
{
    m_outputBuffer = buffer;
}

QString WorkStation::getCurrentProduct() const
{
    QMutexLocker locker(&m_controlMutex);
    return m_currentProductId;
}

double WorkStation::getThroughput() const
{
    if (!m_startTime.isValid()) {
        return 0.0;
    }
    
    qint64 elapsedMs = m_startTime.msecsTo(QDateTime::currentDateTime());
    if (elapsedMs <= 0) {
        return 0.0;
    }
    
    return (m_processedCount.loadAcquire() * 60000.0) / elapsedMs; // items per minute
}

void WorkStation::resetStatistics()
{
    m_processedCount.storeRelease(0);
    m_rejectedCount.storeRelease(0);
    m_startTime = QDateTime::currentDateTime();
}

void WorkStation::run()
{
    setState(StationState::Running);
    
    while (!checkStopping()) {
        waitIfPaused();
        
        if (checkStopping()) {
            break;
        }
        
        // Try to get product from input buffer
        std::shared_ptr<Product> product;
        if (m_inputBuffer && m_inputBuffer->pop(product)) {
            {
                QMutexLocker locker(&m_controlMutex);
                m_currentProductId = product->getId();
            }
            
            // Process the product
            bool processed = false;
            try {
                processed = processProduct(product);
            } catch (const std::exception& e) {
                setState(StationState::Error);
                emit errorOccurred(m_name, QString("Processing error: %1").arg(e.what()));
                processed = false;
            }
            
            if (processed) {
                // Try to pass to output buffer
                if (m_outputBuffer) {
                    if (m_outputBuffer->push(product)) {
                        m_processedCount.fetchAndAddRelaxed(1);
                        emit productProcessed(m_name, product->getId());
                    } else {
                        setState(StationState::Blocked);
                        logActivity("Output buffer blocked");
                    }
                } else {
                    // No output buffer (final station)
                    m_processedCount.fetchAndAddRelaxed(1);
                    emit productProcessed(m_name, product->getId());
                }
            } else {
                m_rejectedCount.fetchAndAddRelaxed(1);
                emit productRejected(m_name, product->getId());
            }
            
            {
                QMutexLocker locker(&m_controlMutex);
                m_currentProductId.clear();
            }
            
            if (getState() == StationState::Blocked) {
                setState(StationState::Running);
            }
        } else {
            // No input available, short wait
            msleep(10);
        }
    }
    
    setState(StationState::Stopped);
}

bool WorkStation::shouldRejectProduct() const
{
    if (m_failureRate <= 0.0) {
        return false;
    }
    
    double random = QRandomGenerator::global()->generateDouble();
    return random < m_failureRate;
}

void WorkStation::setState(StationState newState)
{
    StationState oldState = static_cast<StationState>(m_state.fetchAndStoreRelaxed(static_cast<int>(newState)));
    if (oldState != newState) {
        emit stateChanged(m_name, newState);
    }
}

void WorkStation::logActivity(const QString& message)
{
    qDebug() << QString("[%1] %2").arg(m_name, message);
}

int WorkStation::getRandomProcessingTime() const
{
    if (m_maxProcessingTime <= m_minProcessingTime) {
        return m_minProcessingTime;
    }
    
    return QRandomGenerator::global()->bounded(m_minProcessingTime, m_maxProcessingTime + 1);
}

void WorkStation::waitIfPaused()
{
    QMutexLocker locker(&m_controlMutex);
    while (getState() == StationState::Paused && !checkStopping()) {
        m_pauseCondition.wait(&m_controlMutex, 100);
    }
}

bool WorkStation::checkStopping() const
{
    return m_stopping.loadAcquire() != 0;
}

void WorkStation::onMetricsTimer()
{
    if (m_inputBuffer) {
        emit metricsUpdated(m_name, m_inputBuffer->size(), getThroughput());
    }
}
