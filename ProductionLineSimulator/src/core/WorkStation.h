#ifndef WORKSTATION_H
#define WORKSTATION_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>
#include <QAtomicInt>
#include <QString>
#include <memory>
#include "Buffer.h"
#include "model/Product.h"

enum class StationState {
    Idle,
    Running,
    Paused,
    Blocked,
    Stopping,
    Stopped,
    Error
};

class WorkStation : public QThread
{
    Q_OBJECT

public:
    explicit WorkStation(const QString& name, QObject* parent = nullptr);
    virtual ~WorkStation();
    
    // Station control
    void startStation();
    void pauseStation();
    void resumeStation();
    void stopStation();
    
    // Configuration
    void setProcessingTime(int minMs, int maxMs);
    void setFailureRate(double rate) { m_failureRate = rate; }
    
    // Buffer management
    void setInputBuffer(std::shared_ptr<Buffer<std::shared_ptr<Product>>> buffer);
    void setOutputBuffer(std::shared_ptr<Buffer<std::shared_ptr<Product>>> buffer);
    std::shared_ptr<Buffer<std::shared_ptr<Product>>> getInputBuffer() const { return m_inputBuffer; }
    std::shared_ptr<Buffer<std::shared_ptr<Product>>> getOutputBuffer() const { return m_outputBuffer; }
    
    // Status
    StationState getState() const { return static_cast<StationState>(m_state.loadAcquire()); }
    QString getName() const { return m_name; }
    QString getCurrentProduct() const;
    int getProcessedCount() const { return m_processedCount.loadAcquire(); }
    double getThroughput() const; // items per minute
    
    // Statistics
    void resetStatistics();

signals:
    void stateChanged(const QString& stationName, StationState newState);
    void productProcessed(const QString& stationName, const QString& productId);
    void productRejected(const QString& stationName, const QString& productId);
    void errorOccurred(const QString& stationName, const QString& error);
    void metricsUpdated(const QString& stationName, int queueDepth, double throughput);

protected:
    // Main thread execution
    void run() override;
    
    // Virtual methods for station-specific processing
    virtual bool processProduct(std::shared_ptr<Product> product) = 0;
    virtual void onStationStarted() {}
    virtual void onStationStopped() {}
    
    // Utility methods
    bool shouldRejectProduct() const;
    void updateMetrics();
    void setState(StationState newState);
    void logActivity(const QString& message);

private slots:
    void onMetricsTimer();

private:
    // Core properties
    QString m_name;
    QAtomicInt m_state;
    
    // Buffers
    std::shared_ptr<Buffer<std::shared_ptr<Product>>> m_inputBuffer;
    std::shared_ptr<Buffer<std::shared_ptr<Product>>> m_outputBuffer;
    
    // Synchronization
    mutable QMutex m_controlMutex;
    QWaitCondition m_pauseCondition;
    QAtomicInt m_stopping;
    
    // Processing configuration
    int m_minProcessingTime;
    int m_maxProcessingTime;
    double m_failureRate;
    
    // Statistics
    QAtomicInt m_processedCount;
    QAtomicInt m_rejectedCount;
    QDateTime m_startTime;
    QString m_currentProductId;
    
    // Metrics timer
    QTimer* m_metricsTimer;
    
    // Helper methods
    int getRandomProcessingTime() const;
    void waitIfPaused();
    bool checkStopping() const;
};

#endif // WORKSTATION_H
