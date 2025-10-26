#ifndef PRODUCTIONCONTROLLER_H
#define PRODUCTIONCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QAtomicInt>
#include <memory>
#include <vector>
#include "WorkStation.h"
#include "Buffer.h"
#include "model/Product.h"

// Forward declarations for stations
class Intake;
class Assembler;
class QualityInspection;
class Packaging;
class Shipping;

class ThreadManager;
class StatsAggregator;
class Logger;

enum class ProductionMode {
    ThreadsOnly,
    ProcessesWithIPC
};

class ProductionController : public QObject
{
    Q_OBJECT

public:
    explicit ProductionController(QObject* parent = nullptr);
    virtual ~ProductionController();
    
    // Production control
    void startProduction();
    void pauseProduction();
    void resumeProduction();
    void stopProduction();
    void resetProduction();
    
    // Configuration
    void setProductionMode(ProductionMode mode) { m_mode = mode; }
    ProductionMode getProductionMode() const { return m_mode; }
    void setBufferCapacity(int capacity);
    void configureStation(const QString& stationName, int minTime, int maxTime, double failRate);
    
    // Status
    bool isRunning() const { return m_isRunning.loadAcquire(); }
    bool isPaused() const { return m_isPaused.loadAcquire(); }
    int getFinishedProductCount() const { return m_finishedCount.loadAcquire(); }
    
    // Station access
    QList<WorkStation*> getStations() const;
    WorkStation* getStation(const QString& name) const;

signals:
    void productionStarted();
    void productionPaused();
    void productionResumed();
    void productionStopped();
    void productionReset();
    void productFinished(const QString& productId);
    void statisticsUpdated();
    void errorOccurred(const QString& error);

private slots:
    void onProductProcessed(const QString& stationName, const QString& productId);
    void onProductRejected(const QString& stationName, const QString& productId);
    void onStationError(const QString& stationName, const QString& error);
    void onMetricsTimer();

private:
    // Core components
    ProductionMode m_mode;
    QAtomicInt m_isRunning;
    QAtomicInt m_isPaused;
    QAtomicInt m_finishedCount;
    
    // Stations
    std::unique_ptr<Intake> m_intakeStation;
    std::unique_ptr<Assembler> m_assemblerStation;
    std::unique_ptr<QualityInspection> m_qualityStation;
    std::unique_ptr<Packaging> m_packagingStation;
    std::unique_ptr<Shipping> m_shippingStation;
    
    // Buffers between stations
    std::shared_ptr<Buffer<std::shared_ptr<Product>>> m_intakeToAssemblerBuffer;
    std::shared_ptr<Buffer<std::shared_ptr<Product>>> m_assemblerToQualityBuffer;
    std::shared_ptr<Buffer<std::shared_ptr<Product>>> m_qualityToPackagingBuffer;
    std::shared_ptr<Buffer<std::shared_ptr<Product>>> m_packagingToShippingBuffer;
    
    // Support systems
    std::unique_ptr<ThreadManager> m_threadManager;
    std::unique_ptr<StatsAggregator> m_statsAggregator;
    std::unique_ptr<Logger> m_logger;
    
    // Timers
    QTimer* m_metricsTimer;
    
    // Configuration
    int m_bufferCapacity;
    
    // Initialization
    void initializeStations();
    void initializeBuffers();
    void connectStations();
    void connectSignals();
    
    // Helper methods
    void updateStatistics();
    void logEvent(const QString& message);
    QList<WorkStation*> getAllStations() const;
};

#endif // PRODUCTIONCONTROLLER_H
