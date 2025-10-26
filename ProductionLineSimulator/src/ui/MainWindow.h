#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QStatusBar>
#include <QMenuBar>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QTableWidget>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QSplitter>
#include <QTimer>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <memory>

QT_CHARTS_USE_NAMESPACE

class ProductionController;
class Logger;
struct LogEntry;
struct ThreadInfo;

class StationWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Production control
    void onStartAllClicked();
    void onPauseAllClicked();
    void onStopAllClicked();
    void onResetClicked();
    
    // File operations
    void onSaveClicked();
    void onLoadClicked();
    void onSettingsClicked();
    
    // Station control
    void onStationStartClicked(const QString& stationName);
    void onStationPauseClicked(const QString& stationName);
    void onStationStopClicked(const QString& stationName);
    
    // Production events
    void onProductionStarted();
    void onProductionPaused();
    void onProductionStopped();
    void onProductionReset();
    void onProductFinished(const QString& productId);
    void onStatisticsUpdated();
    void onErrorOccurred(const QString& error);
    
    // Logging
    void onLogEntryAdded(const LogEntry& entry);
    
    // UI updates
    void onUpdateTimer();
    void onStationStateChanged(const QString& stationName, int newState);
    void onStationMetricsUpdated(const QString& stationName, int queueDepth, double throughput);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    // Core components
    std::unique_ptr<ProductionController> m_controller;
    std::unique_ptr<Logger> m_logger;
    
    // UI Components
    QWidget* m_centralWidget;
    QVBoxLayout* m_centralLayout;
    
    // Toolbar and actions
    QToolBar* m_toolbar;
    QAction* m_startAllAction;
    QAction* m_pauseAllAction;
    QAction* m_stopAllAction;
    QAction* m_resetAction;
    QAction* m_saveAction;
    QAction* m_loadAction;
    QAction* m_settingsAction;
    
    // Production line visualization
    QWidget* m_productionLineWidget;
    QHBoxLayout* m_productionLineLayout;
    QList<StationWidget*> m_stationWidgets;
    
    // Dock widgets
    QDockWidget* m_logsDock;
    QDockWidget* m_metricsDock;
    QDockWidget* m_threadsDock;
    QDockWidget* m_settingsDock;
    
    // Logs pane
    QTextEdit* m_logsTextEdit;
    
    // Metrics pane
    QWidget* m_metricsWidget;
    QVBoxLayout* m_metricsLayout;
    QChartView* m_throughputChartView;
    QChartView* m_bufferChartView;
    QChart* m_throughputChart;
    QChart* m_bufferChart;
    QList<QLineSeries*> m_throughputSeries;
    QList<QLineSeries*> m_bufferSeries;
    
    // Threads pane
    QTableWidget* m_threadsTable;
    
    // Settings pane
    QWidget* m_settingsWidget;
    QVBoxLayout* m_settingsLayout;
    QComboBox* m_modeComboBox;
    QSpinBox* m_bufferSizeSpinBox;
    QSlider* m_productionRateSlider;
    QDoubleSpinBox* m_failureRateSpinBox;
    QCheckBox* m_ipcEnabledCheckBox;
    QSpinBox* m_seedSpinBox;
    
    // Status bar
    QLabel* m_statusLabel;
    QLabel* m_finishedCountLabel;
    QLabel* m_elapsedTimeLabel;
    QProgressBar* m_productionProgress;
    
    // Timers
    QTimer* m_updateTimer;
    QDateTime m_productionStartTime;
    
    // Charts data
    static constexpr int MAX_CHART_POINTS = 60; // 1 minute of data at 1s intervals
    
    // Initialization methods
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupCentralWidget();
    void setupDockWidgets();
    void setupStatusBar();
    void setupCharts();
    void connectSignals();
    
    // UI helpers
    void createStationWidgets();
    void updateStationWidget(const QString& stationName);
    void updateThreadsTable();
    void updateMetrics();
    void updateElapsedTime();
    void addLogEntry(const QString& text, const QString& level = "INFO");
    void showError(const QString& message);
    void showInfo(const QString& message);
    
    // Settings
    void loadSettings();
    void saveSettings();
    void applySettings();
    
    // File operations
    bool saveState();
    bool loadState();
};

// Custom widget for station visualization
class StationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StationWidget(const QString& stationName, QWidget* parent = nullptr);
    
    void updateState(int state);
    void updateMetrics(int queueDepth, double throughput);
    void updateCurrentProduct(const QString& productId);

signals:
    void startRequested(const QString& stationName);
    void pauseRequested(const QString& stationName);
    void stopRequested(const QString& stationName);

private:
    QString m_stationName;
    QVBoxLayout* m_layout;
    
    // UI elements
    QLabel* m_nameLabel;
    QLabel* m_stateLabel;
    QLabel* m_productLabel;
    QLabel* m_queueLabel;
    QLabel* m_throughputLabel;
    QProgressBar* m_queueProgress;
    QPushButton* m_startButton;
    QPushButton* m_pauseButton;
    QPushButton* m_stopButton;
    
    // State
    int m_currentState;
    int m_queueDepth;
    double m_throughput;
    QString m_currentProductId;
    
    void setupUI();
    void updateButtonStates();
    QString stateToString(int state) const;
    QColor stateToColor(int state) const;

private slots:
    void onStartClicked();
    void onPauseClicked();
    void onStopClicked();
};

#endif // MAINWINDOW_H
