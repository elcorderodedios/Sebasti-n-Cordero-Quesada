#ifndef STATSAGGREGATOR_H
#define STATSAGGREGATOR_H

#include <QObject>
#include <QTimer>
#include <QVariantMap>
#include <QDateTime>
#include <QMutex>
#include <QList>
#include <QPair>

class StatsAggregator : public QObject
{
    Q_OBJECT

public:
    explicit StatsAggregator(QObject* parent = nullptr);
    virtual ~StatsAggregator();
    
    // Statistics update
    void updateStats(const QVariantMap& stats);
    
    // Data retrieval
    QVariantMap getCurrentStats() const;
    QList<QVariantMap> getHistory(int maxEntries = 60) const;
    QVariantMap getAggregatedStats() const;
    
    // Specific metrics
    double getThroughput(const QString& metric = "overall") const;
    double getAverageProcessingTime(const QString& station = "") const;
    int getWipCount() const; // Work In Progress
    double getUtilization(const QString& station = "") const;
    QVariantMap getBufferMetrics() const;
    QVariantMap getErrorRates() const;
    
    // Configuration
    void setUpdateInterval(int intervalMs);
    void setMaxHistorySize(int maxSize);
    void reset();

signals:
    void statsUpdated(const QVariantMap& currentStats);
    void aggregatedStatsChanged(const QVariantMap& aggregatedStats);
    void alertTriggered(const QString& alertType, const QString& message);

private slots:
    void onUpdateTimer();

private:
    // Core data
    mutable QMutex m_statsMutex;
    QVariantMap m_currentStats;
    QList<QPair<QDateTime, QVariantMap>> m_history;
    QVariantMap m_aggregatedStats;
    
    // Configuration
    QTimer* m_updateTimer;
    int m_maxHistorySize;
    QDateTime m_startTime;
    
    // Metrics calculation
    void calculateAggregatedStats();
    void checkAlerts(const QVariantMap& stats);
    double calculateMovingAverage(const QString& key, int windowSize = 10) const;
    double calculateTrend(const QString& key, int windowSize = 5) const;
    void updateThroughputMetrics(const QVariantMap& stats);
    void updateUtilizationMetrics(const QVariantMap& stats);
    
    // Alert thresholds
    static constexpr double HIGH_QUEUE_UTILIZATION = 0.8;
    static constexpr double LOW_THROUGHPUT_THRESHOLD = 0.5;
    static constexpr double HIGH_ERROR_RATE = 0.1;
    
    // Helper methods
    QVariant getStatValue(const QString& key, const QVariantMap& stats = QVariantMap()) const;
    void logStats(const QString& message);
};

#endif // STATSAGGREGATOR_H
