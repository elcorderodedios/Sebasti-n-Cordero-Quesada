#include "StatsAggregator.h"
#include <QDebug>
#include <QMutexLocker>
#include <algorithm>

StatsAggregator::StatsAggregator(QObject* parent)
    : QObject(parent)
    , m_updateTimer(new QTimer(this))
    , m_maxHistorySize(300) // 5 minutes at 1s intervals
    , m_startTime(QDateTime::currentDateTime())
{
    // Setup update timer
    m_updateTimer->setInterval(1000); // Update every second
    connect(m_updateTimer, &QTimer::timeout, this, &StatsAggregator::onUpdateTimer);
    
    // Initialize current stats
    reset();
    
    logStats("StatsAggregator initialized");
}

StatsAggregator::~StatsAggregator()
{
    logStats("StatsAggregator destroyed");
}

void StatsAggregator::updateStats(const QVariantMap& stats)
{
    QMutexLocker locker(&m_statsMutex);
    
    // Merge new stats with current stats
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        m_currentStats[it.key()] = it.value();
    }
    
    // Add timestamp
    m_currentStats["timestamp"] = QDateTime::currentDateTime();
    
    // Add to history
    m_history.append(qMakePair(QDateTime::currentDateTime(), m_currentStats));
    
    // Trim history to max size
    while (m_history.size() > m_maxHistorySize) {
        m_history.removeFirst();
    }
    
    // Calculate aggregated stats
    calculateAggregatedStats();
    
    // Check for alerts
    checkAlerts(stats);
    
    emit statsUpdated(m_currentStats);
}

QVariantMap StatsAggregator::getCurrentStats() const
{
    QMutexLocker locker(&m_statsMutex);
    return m_currentStats;
}

QList<QVariantMap> StatsAggregator::getHistory(int maxEntries) const
{
    QMutexLocker locker(&m_statsMutex);
    
    QList<QVariantMap> result;
    int startIndex = qMax(0, m_history.size() - maxEntries);
    
    for (int i = startIndex; i < m_history.size(); ++i) {
        result.append(m_history[i].second);
    }
    
    return result;
}

QVariantMap StatsAggregator::getAggregatedStats() const
{
    QMutexLocker locker(&m_statsMutex);
    return m_aggregatedStats;
}

double StatsAggregator::getThroughput(const QString& metric) const
{
    QMutexLocker locker(&m_statsMutex);
    
    QString key = (metric == "overall") ? "overall_throughput" : metric + "_throughput";
    return getStatValue(key).toDouble();
}

double StatsAggregator::getAverageProcessingTime(const QString& station) const
{
    QMutexLocker locker(&m_statsMutex);
    
    if (station.isEmpty()) {
        return getStatValue("average_processing_time").toDouble();
    } else {
        QString key = station.toLower().replace(" ", "_") + "_avg_time";
        return getStatValue(key).toDouble();
    }
}

int StatsAggregator::getWipCount() const
{
    QMutexLocker locker(&m_statsMutex);
    
    // Sum all buffer sizes to get work in progress
    int wip = 0;
    wip += getStatValue("intake_buffer_size").toInt();
    wip += getStatValue("assembler_buffer_size").toInt();
    wip += getStatValue("quality_buffer_size").toInt();
    wip += getStatValue("packaging_buffer_size").toInt();
    
    return wip;
}

double StatsAggregator::getUtilization(const QString& station) const
{
    QMutexLocker locker(&m_statsMutex);
    
    if (station.isEmpty()) {
        return getStatValue("overall_utilization").toDouble();
    } else {
        QString key = station.toLower().replace(" ", "_") + "_utilization";
        return getStatValue(key).toDouble();
    }
}

QVariantMap StatsAggregator::getBufferMetrics() const
{
    QMutexLocker locker(&m_statsMutex);
    
    QVariantMap bufferMetrics;
    bufferMetrics["intake_buffer"] = getStatValue("intake_buffer_size").toInt();
    bufferMetrics["assembler_buffer"] = getStatValue("assembler_buffer_size").toInt();
    bufferMetrics["quality_buffer"] = getStatValue("quality_buffer_size").toInt();
    bufferMetrics["packaging_buffer"] = getStatValue("packaging_buffer_size").toInt();
    bufferMetrics["total_wip"] = getWipCount();
    
    return bufferMetrics;
}

QVariantMap StatsAggregator::getErrorRates() const
{
    QMutexLocker locker(&m_statsMutex);
    
    QVariantMap errorRates;
    errorRates["intake_error_rate"] = getStatValue("intake_error_rate", QVariantMap()).toDouble();
    errorRates["assembler_error_rate"] = getStatValue("assembler_error_rate", QVariantMap()).toDouble();
    errorRates["quality_error_rate"] = getStatValue("quality_error_rate", QVariantMap()).toDouble();
    errorRates["packaging_error_rate"] = getStatValue("packaging_error_rate", QVariantMap()).toDouble();
    errorRates["shipping_error_rate"] = getStatValue("shipping_error_rate", QVariantMap()).toDouble();
    errorRates["overall_error_rate"] = getStatValue("overall_error_rate", QVariantMap()).toDouble();
    
    return errorRates;
}

void StatsAggregator::setUpdateInterval(int intervalMs)
{
    m_updateTimer->setInterval(intervalMs);
    logStats(QString("Update interval set to %1ms").arg(intervalMs));
}

void StatsAggregator::setMaxHistorySize(int maxSize)
{
    QMutexLocker locker(&m_statsMutex);
    m_maxHistorySize = maxSize;
    
    // Trim current history if needed
    while (m_history.size() > m_maxHistorySize) {
        m_history.removeFirst();
    }
    
    logStats(QString("Max history size set to %1").arg(maxSize));
}

void StatsAggregator::reset()
{
    QMutexLocker locker(&m_statsMutex);
    
    m_currentStats.clear();
    m_history.clear();
    m_aggregatedStats.clear();
    m_startTime = QDateTime::currentDateTime();
    
    // Initialize with default values
    m_currentStats["finished_count"] = 0;
    m_currentStats["intake_buffer_size"] = 0;
    m_currentStats["assembler_buffer_size"] = 0;
    m_currentStats["quality_buffer_size"] = 0;
    m_currentStats["packaging_buffer_size"] = 0;
    m_currentStats["overall_throughput"] = 0.0;
    m_currentStats["overall_utilization"] = 0.0;
    
    emit statsUpdated(m_currentStats);
    emit aggregatedStatsChanged(m_aggregatedStats);
    
    logStats("Statistics reset");
}

void StatsAggregator::onUpdateTimer()
{
    // Timer-based updates for calculated metrics
    QMutexLocker locker(&m_statsMutex);
    
    // Update runtime metrics
    qint64 elapsedSeconds = m_startTime.secsTo(QDateTime::currentDateTime());
    m_currentStats["runtime_seconds"] = elapsedSeconds;
    
    // Calculate overall throughput
    double finishedCount = getStatValue("finished_count").toDouble();
    if (elapsedSeconds > 0) {
        m_currentStats["overall_throughput"] = (finishedCount * 60.0) / elapsedSeconds; // per minute
    }
    
    // Update aggregated stats
    calculateAggregatedStats();
    
    emit statsUpdated(m_currentStats);
}

void StatsAggregator::calculateAggregatedStats()
{
    // Calculate moving averages and trends
    if (m_history.isEmpty()) {
        return;
    }
    
    m_aggregatedStats.clear();
    
    // Calculate moving averages
    m_aggregatedStats["throughput_avg_1min"] = calculateMovingAverage("overall_throughput", 60);
    m_aggregatedStats["throughput_avg_5min"] = calculateMovingAverage("overall_throughput", 300);
    m_aggregatedStats["wip_avg"] = calculateMovingAverage("wip_count", 60);
    
    // Calculate trends
    m_aggregatedStats["throughput_trend"] = calculateTrend("overall_throughput", 10);
    m_aggregatedStats["wip_trend"] = calculateTrend("wip_count", 10);
    
    // Calculate peak values
    double maxThroughput = 0.0;
    double maxWip = 0.0;
    
    for (const auto& entry : m_history) {
        const QVariantMap& stats = entry.second;
        maxThroughput = qMax(maxThroughput, stats.value("overall_throughput", 0.0).toDouble());
        maxWip = qMax(maxWip, static_cast<double>(getWipCount()));
    }
    
    m_aggregatedStats["peak_throughput"] = maxThroughput;
    m_aggregatedStats["peak_wip"] = maxWip;
    
    // Calculate efficiency metrics
    int totalProcessed = 0;
    int totalRejected = 0;
    
    QStringList stations = {"intake", "assembler", "quality_inspection", "packaging", "shipping"};
    for (const QString& station : stations) {
        totalProcessed += getStatValue(station + "_processed").toInt();
        // Note: rejection tracking would need to be implemented in stations
    }
    
    if (totalProcessed > 0) {
        m_aggregatedStats["overall_efficiency"] = 1.0 - (static_cast<double>(totalRejected) / totalProcessed);
    }
    
    emit aggregatedStatsChanged(m_aggregatedStats);
}

void StatsAggregator::checkAlerts(const QVariantMap& stats)
{
    // Check for high queue utilization
    QStringList bufferKeys = {"intake_buffer_size", "assembler_buffer_size", 
                             "quality_buffer_size", "packaging_buffer_size"};
    
    for (const QString& key : bufferKeys) {
        double utilization = stats.value(key, 0).toDouble() / 20.0; // Assuming capacity of 20
        if (utilization > HIGH_QUEUE_UTILIZATION) {
            emit alertTriggered("HIGH_QUEUE_UTIL", 
                               QString("Buffer %1 utilization: %2%").arg(key).arg(utilization * 100, 0, 'f', 1));
        }
    }
    
    // Check for low throughput
    double currentThroughput = stats.value("overall_throughput", 0.0).toDouble();
    double expectedThroughput = 10.0; // Expected items per minute
    
    if (currentThroughput < expectedThroughput * LOW_THROUGHPUT_THRESHOLD) {
        emit alertTriggered("LOW_THROUGHPUT", 
                           QString("Throughput below threshold: %1 items/min").arg(currentThroughput, 0, 'f', 1));
    }
    
    // Check for high error rates
    QVariantMap errorRates = getErrorRates();
    for (auto it = errorRates.begin(); it != errorRates.end(); ++it) {
        double rate = it.value().toDouble();
        if (rate > HIGH_ERROR_RATE) {
            emit alertTriggered("HIGH_ERROR_RATE", 
                               QString("High error rate in %1: %2%").arg(it.key()).arg(rate * 100, 0, 'f', 1));
        }
    }
}

double StatsAggregator::calculateMovingAverage(const QString& key, int windowSize) const
{
    if (m_history.isEmpty()) {
        return 0.0;
    }
    
    int startIndex = qMax(0, m_history.size() - windowSize);
    double sum = 0.0;
    int count = 0;
    
    for (int i = startIndex; i < m_history.size(); ++i) {
        const QVariantMap& stats = m_history[i].second;
        if (stats.contains(key)) {
            sum += stats[key].toDouble();
            count++;
        }
    }
    
    return (count > 0) ? sum / count : 0.0;
}

double StatsAggregator::calculateTrend(const QString& key, int windowSize) const
{
    if (m_history.size() < windowSize) {
        return 0.0;
    }
    
    // Simple linear trend calculation (slope)
    int startIndex = m_history.size() - windowSize;
    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumXX = 0.0;
    
    for (int i = 0; i < windowSize; ++i) {
        double x = i;
        double y = m_history[startIndex + i].second.value(key, 0.0).toDouble();
        
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumXX += x * x;
    }
    
    double n = windowSize;
    double slope = (n * sumXY - sumX * sumY) / (n * sumXX - sumX * sumX);
    
    return slope;
}

QVariant StatsAggregator::getStatValue(const QString& key, const QVariantMap& stats) const
{
    const QVariantMap& sourceStats = stats.isEmpty() ? m_currentStats : stats;
    return sourceStats.value(key, QVariant());
}

void StatsAggregator::logStats(const QString& message)
{
    qDebug() << QString("[StatsAggregator] %1").arg(message);
}
