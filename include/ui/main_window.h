#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTimer>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <memory>

#include "models/simulator.h"

namespace trade_simulator {
namespace ui {

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

using namespace QtCharts;

/**
 * @brief Main application window for the trade simulator
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~MainWindow();

private slots:
    /**
     * @brief Start or stop the simulator
     */
    void onStartStopButtonClicked();

    /**
     * @brief Update parameters from UI inputs
     */
    void onParametersChanged();

    /**
     * @brief Update UI with simulator output
     * @param output Latest simulator output
     */
    void updateOutput(const models::SimulatorOutput& output);

    /**
     * @brief Update the chart with new data
     */
    void updateChart();

private:
    // UI components
    Ui::MainWindow *ui;

    // Left panel - Input parameters
    QWidget *inputPanel;
    QComboBox *exchangeComboBox;
    QComboBox *symbolComboBox;
    QComboBox *orderTypeComboBox;
    QDoubleSpinBox *quantitySpinBox;
    QDoubleSpinBox *volatilitySpinBox;
    QSpinBox *feeTierSpinBox;
    QPushButton *startStopButton;

    // Right panel - Output values
    QWidget *outputPanel;
    QLabel *slippageLabel;
    QLabel *feesLabel;
    QLabel *marketImpactLabel;
    QLabel *netCostLabel;
    QLabel *makerTakerLabel;
    QLabel *latencyLabel;

    // Chart components
    QChart *chart;
    QChartView *chartView;
    QLineSeries *slippageSeries;
    QLineSeries *impactSeries;
    QLineSeries *feesSeries;
    QLineSeries *totalCostSeries;
    QValueAxis *axisX;
    QValueAxis *axisY;
    int dataPointCounter;
    static constexpr int kMaxDataPoints = 100;

    // Simulator
    std::shared_ptr<models::Simulator> simulator;
    QTimer updateTimer;

    /**
     * @brief Set up the UI components
     */
    void setupUi();

    /**
     * @brief Set up the input panel
     */
    void setupInputPanel();

    /**
     * @brief Set up the output panel
     */
    void setupOutputPanel();

    /**
     * @brief Set up the chart
     */
    void setupChart();

    /**
     * @brief Connect signals and slots
     */
    void connectSignals();

    /**
     * @brief Format a number as USD currency
     * @param value Value to format
     * @return Formatted string
     */
    QString formatCurrency(double value) const;

    /**
     * @brief Format a number as percentage
     * @param value Value to format (0.1 = 10%)
     * @return Formatted string
     */
    QString formatPercentage(double value) const;
};

} // namespace ui
} // namespace trade_simulator 