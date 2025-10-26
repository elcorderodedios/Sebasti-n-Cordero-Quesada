#include "ProductionController.h"
#include "stations/Intake.h"
#include "stations/Assembler.h"
#include "stations/QualityInspection.h"
#include "stations/Packaging.h"
#include "stations/Shipping.h"
#include "ThreadManager.h"
#include "../stats/StatsAggregator.h"
#include "../logging/Logger.h"
#include <QDebug>

ProductionController::ProductionController(QObject* parent)
    : QObject(parent)
    , m_mode(ProductionMode::ThreadsOnly)
    , m_isRunning(0)
    , m_isPaused(0)
    , m_finishedCount(0)
    , m_bufferCapacity(20)
    , m_metricsTimer(new QTimer(this))
{
    // Initialize components
    initializeStations();
    initializeBuffers();
    connectStations();
    connectSignals();
    
    // Setup metrics timer
    m_metricsTimer->setInterval(1000); // Update every second
    connect(m_metricsTimer, &QTimer::timeout, this, &ProductionController::onMetricsTimer);
    
    // Initialize support systems
    m_threadManager = std::make_unique<ThreadManager>(this);
    m_statsAggregator = std::make_unique<StatsAggregator>(this);
    m_logger = std::make_unique<Logger>(this);
    
    logEvent("Production controller initialized");
}

ProductionController::~ProductionController()
{
    stopProduction();
}

void ProductionController::startProduction()
{
    if (m_isRunning.loadAcquire()) {
        return;
    }
    
    logEvent("Starting production line...");
    
    m_isRunning.storeRelease(1);
    m_isPaused.storeRelease(0);
    
    // Start all stations
    for (WorkStation* station : getAllStations()) {
        station->startStation();
    }
    
    // Start metrics timer
    m_metricsTimer->start();
    
    emit productionStarted();
    logEvent("Production line started successfully");
}

void ProductionController::pauseProduction()
{
    if (!m_isRunning.loadAcquire() || m_isPaused.loadAcquire()) {
        return;
    }
    
    logEvent("Pausing production line...");
    
    m_isPaused.storeRelease(1);
    
    // Pause all stations
    for (WorkStation* station : getAllStations()) {
        station->pauseStation();
    }
    
    emit productionPaused();
    logEvent("Production line paused");
}

void ProductionController::resumeProduction()
{
    if (!m_isRunning.loadAcquire() || !m_isPaused.loadAcquire()) {
        return;
    }
    
    logEvent("Resuming production line...");
    
    m_isPaused.storeRelease(0);
    
    // Resume all stations
    for (WorkStation* station : getAllStations()) {
        station->resumeStation();
    }
    
    emit productionResumed();
    logEvent("Production line resumed");
}

void ProductionController::stopProduction()
{
    if (!m_isRunning.loadAcquire()) {
        return;
    }
    
    logEvent("Stopping production line...");
    
    m_isRunning.storeRelease(0);
    m_isPaused.storeRelease(0);
    
    // Stop metrics timer
    m_metricsTimer->stop();
    
    // Stop all stations
    for (WorkStation* station : getAllStations()) {
        station->stopStation();
    }
    
    emit productionStopped();
    logEvent("Production line stopped");
}

void ProductionController::resetProduction()
{
    logEvent("Resetting production line...");
    
    // Stop if running
    if (m_isRunning.loadAcquire()) {
        stopProduction();
    }
    
    // Clear all buffers
    m_intakeToAssemblerBuffer->clear();
    m_assemblerToQualityBuffer->clear();
    m_qualityToPackagingBuffer->clear();
    m_packagingToShippingBuffer->clear();
    
    // Reset statistics
    m_finishedCount.storeRelease(0);
    for (WorkStation* station : getAllStations()) {
        station->resetStatistics();
    }
    
    if (m_statsAggregator) {
        m_statsAggregator->reset();
    }
    
    emit productionReset();
    logEvent("Production line reset complete");
}

void ProductionController::setBufferCapacity(int capacity)
{
    m_bufferCapacity = capacity;
    // Note: Changing capacity of existing buffers would require recreation
    logEvent(QString("Buffer capacity set to %1").arg(capacity));
}

void ProductionController::configureStation(const QString& stationName, int minTime, int maxTime, double failRate)
{
    WorkStation* station = getStation(stationName);
    if (station) {
        station->setProcessingTime(minTime, maxTime);
        station->setFailureRate(failRate);
        logEvent(QString("Configured station %1: %2-%3ms, %4% failure rate")
                 .arg(stationName).arg(minTime).arg(maxTime).arg(failRate * 100));
    }
}

QList<WorkStation*> ProductionController::getStations() const
{
    return getAllStations();
}

WorkStation* ProductionController::getStation(const QString& name) const
{
    for (WorkStation* station : getAllStations()) {
        if (station->getName() == name) {
            return station;
        }
    }
    return nullptr;
}

void ProductionController::initializeStations()
{
    m_intakeStation = std::make_unique<Intake>(this);
    m_assemblerStation = std::make_unique<Assembler>(this);
    m_qualityStation = std::make_unique<QualityInspection>(this);
    m_packagingStation = std::make_unique<Packaging>(this);
    m_shippingStation = std::make_unique<Shipping>(this);
}

void ProductionController::initializeBuffers()
{
    m_intakeToAssemblerBuffer = std::make_shared<Buffer<std::shared_ptr<Product>>>(m_bufferCapacity);
    m_assemblerToQualityBuffer = std::make_shared<Buffer<std::shared_ptr<Product>>>(m_bufferCapacity);
    m_qualityToPackagingBuffer = std::make_shared<Buffer<std::shared_ptr<Product>>>(m_bufferCapacity);
    m_packagingToShippingBuffer = std::make_shared<Buffer<std::shared_ptr<Product>>>(m_bufferCapacity);
}

void ProductionController::connectStations()
{
    // Connect intake to assembler
    m_intakeStation->setOutputBuffer(m_intakeToAssemblerBuffer);
    m_assemblerStation->setInputBuffer(m_intakeToAssemblerBuffer);
    
    // Connect assembler to quality inspection
    m_assemblerStation->setOutputBuffer(m_assemblerToQualityBuffer);
    m_qualityStation->setInputBuffer(m_assemblerToQualityBuffer);
    
    // Connect quality inspection to packaging
    m_qualityStation->setOutputBuffer(m_qualityToPackagingBuffer);
    m_packagingStation->setInputBuffer(m_qualityToPackagingBuffer);
    
    // Connect packaging to shipping
    m_packagingStation->setOutputBuffer(m_packagingToShippingBuffer);
    m_shippingStation->setInputBuffer(m_packagingToShippingBuffer);
    
    // Shipping has no output buffer (final stage)
}

void ProductionController::connectSignals()
{
    // Connect all station signals
    for (WorkStation* station : getAllStations()) {
        connect(station, &WorkStation::productProcessed,
                this, &ProductionController::onProductProcessed);
        connect(station, &WorkStation::productRejected,
                this, &ProductionController::onProductRejected);
        connect(station, &WorkStation::errorOccurred,
                this, &ProductionController::onStationError);
    }
}

void ProductionController::onProductProcessed(const QString& stationName, const QString& productId)
{
    logEvent(QString("Product %1 processed by %2").arg(productId, stationName));
    
    // If this was the shipping station, increment finished count
    if (stationName == "Shipping") {
        m_finishedCount.fetchAndAddRelaxed(1);
        emit productFinished(productId);
        logEvent(QString("Product %1 finished! Total completed: %2")
                 .arg(productId).arg(m_finishedCount.loadAcquire()));
    }
}

void ProductionController::onProductRejected(const QString& stationName, const QString& productId)
{
    logEvent(QString("Product %1 rejected by %2").arg(productId, stationName));
}

void ProductionController::onStationError(const QString& stationName, const QString& error)
{
    QString message = QString("Station %1 error: %2").arg(stationName, error);
    logEvent(message);
    emit errorOccurred(message);
}

void ProductionController::onMetricsTimer()
{
    updateStatistics();
    emit statisticsUpdated();
}

void ProductionController::updateStatistics()
{
    if (m_statsAggregator) {
        // Update statistics with current production data
        QVariantMap stats;
        stats["finished_count"] = m_finishedCount.loadAcquire();
        stats["intake_buffer_size"] = m_intakeToAssemblerBuffer->size();
        stats["assembler_buffer_size"] = m_assemblerToQualityBuffer->size();
        stats["quality_buffer_size"] = m_qualityToPackagingBuffer->size();
        stats["packaging_buffer_size"] = m_packagingToShippingBuffer->size();
        
        for (WorkStation* station : getAllStations()) {
            QString prefix = station->getName().toLower().replace(" ", "_");
            stats[prefix + "_throughput"] = station->getThroughput();
            stats[prefix + "_processed"] = station->getProcessedCount();
        }
        
        m_statsAggregator->updateStats(stats);
    }
}

void ProductionController::logEvent(const QString& message)
{
    qDebug() << "[ProductionController]" << message;
    if (m_logger) {
        m_logger->log(QString("[Controller] %1").arg(message));
    }
}

QList<WorkStation*> ProductionController::getAllStations() const
{
    QList<WorkStation*> stations;
    if (m_intakeStation) stations.append(m_intakeStation.get());
    if (m_assemblerStation) stations.append(m_assemblerStation.get());
    if (m_qualityStation) stations.append(m_qualityStation.get());
    if (m_packagingStation) stations.append(m_packagingStation.get());
    if (m_shippingStation) stations.append(m_shippingStation.get());
    return stations;
}
