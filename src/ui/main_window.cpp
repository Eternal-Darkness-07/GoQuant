#include "ui/main_window.h"
#include <QApplication>
#include <QScreen>
#include <QStyle>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QTableWidget>
#include <QSplitter>
#include <QDebug>

namespace trade_simulator {
namespace ui {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), 
      ui(nullptr),
      dataPointCounter(0) {
    
    setupUi();
    setupInputPanel();
    setupOutputPanel();
    setupChart();
    connectSignals();
    
    // Create simulator
    simulator = std::make_shared<models::Simulator>(
        [this](const models::SimulatorOutput& output) {
            // This will be called from a different thread, so use Qt's signal/slot mechanism
            QMetaObject::invokeMethod(this, "updateOutput", Qt::QueuedConnection,
                                    Q_ARG(trade_simulator::models::SimulatorOutput, output));
        } 
    );
    
    // Start update timer (for chart updates)
    updateTimer.setInterval(1000);  // Update every second
    updateTimer.start();
    
    // Set window title and geometry
    setWindowTitle("Trade Simulator");
    
    // Center window on screen with reasonable size
    const QRect availableGeometry = QApplication::primaryScreen()->availableGeometry();
    resize(availableGeometry.width() * 0.8, availableGeometry.height() * 0.8);
    move((availableGeometry.width() - width()) / 2,
         (availableGeometry.height() - height()) / 2);    
}

MainWindow::~MainWindow() {
    if (simulator) {
        simulator->stop();
    }
    
    delete ui;
}

void MainWindow::setupUi() {
    // Create central widget
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Create main layout
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    
    // Create left and right panels
    inputPanel = new QWidget(centralWidget);
    outputPanel = new QWidget(centralWidget);
    
    // Create splitter for resizable panels
    QSplitter *splitter = new QSplitter(Qt::Horizontal, centralWidget);
    splitter->addWidget(inputPanel);
    splitter->addWidget(outputPanel);
    
    // Set size policy for panels
    splitter->setStretchFactor(0, 1);  // Input panel
    splitter->setStretchFactor(1, 2);  // Output panel
    
    mainLayout->addWidget(splitter);
}

void MainWindow::setupInputPanel() {
    // Create layout for input panel
    QVBoxLayout *layout = new QVBoxLayout(inputPanel);
    
    // Create group box for parameters
    QGroupBox *parametersGroup = new QGroupBox("Input Parameters", inputPanel);
    QFormLayout *formLayout = new QFormLayout(parametersGroup);
    
    // Exchange
    exchangeComboBox = new QComboBox(parametersGroup);
    exchangeComboBox->addItem("OKX");
    formLayout->addRow("Exchange:", exchangeComboBox);
    
    // Symbol
    symbolComboBox = new QComboBox(parametersGroup);
    symbolComboBox->addItem("BTC-USDT");
    symbolComboBox->addItem("ETH-USDT");
    symbolComboBox->addItem("SOL-USDT");
    formLayout->addRow("Symbol:", symbolComboBox);
    
    // Order Type
    orderTypeComboBox = new QComboBox(parametersGroup);
    orderTypeComboBox->addItem("Market");
    formLayout->addRow("Order Type:", orderTypeComboBox);
    
    // Quantity
    quantitySpinBox = new QDoubleSpinBox(parametersGroup);
    quantitySpinBox->setRange(1.0, 10000.0);
    quantitySpinBox->setValue(100.0);
    quantitySpinBox->setSingleStep(10.0);
    quantitySpinBox->setPrefix("$ ");
    formLayout->addRow("Quantity (USD):", quantitySpinBox);
    
    // Volatility
    volatilitySpinBox = new QDoubleSpinBox(parametersGroup);
    volatilitySpinBox->setRange(0.0, 1.0);
    volatilitySpinBox->setValue(0.1);
    volatilitySpinBox->setSingleStep(0.01);
    volatilitySpinBox->setDecimals(3);
    formLayout->addRow("Volatility:", volatilitySpinBox);
    
    // Fee Tier
    feeTierSpinBox = new QSpinBox(parametersGroup);
    feeTierSpinBox->setRange(0, 3);
    feeTierSpinBox->setValue(0);
    formLayout->addRow("Fee Tier:", feeTierSpinBox);
    
    // Start/Stop button
    startStopButton = new QPushButton("Start Simulator", parametersGroup);
    
    // Add group box and button to layout
    layout->addWidget(parametersGroup);
    layout->addWidget(startStopButton);
    layout->addStretch();
}

void MainWindow::setupOutputPanel() {
    // Create layout for output panel
    QVBoxLayout *layout = new QVBoxLayout(outputPanel);
    
    // Create group box for output values
    QGroupBox *outputGroup = new QGroupBox("Simulation Results", outputPanel);
    QFormLayout *formLayout = new QFormLayout(outputGroup);
    
    // Create labels for output values
    slippageLabel = new QLabel("$ 0.00", outputGroup);
    feesLabel = new QLabel("$ 0.00", outputGroup);
    marketImpactLabel = new QLabel("$ 0.00", outputGroup);
    netCostLabel = new QLabel("$ 0.00", outputGroup);
    makerTakerLabel = new QLabel("0.0% / 100.0%", outputGroup);
    latencyLabel = new QLabel("0 μs", outputGroup);
    
    // Add labels to form layout
    formLayout->addRow("Expected Slippage:", slippageLabel);
    formLayout->addRow("Expected Fees:", feesLabel);
    formLayout->addRow("Expected Market Impact:", marketImpactLabel);
    formLayout->addRow("Net Cost:", netCostLabel);
    formLayout->addRow("Maker/Taker:", makerTakerLabel);
    formLayout->addRow("Internal Latency:", latencyLabel);
    
    // Create chart view
    chart = new QChart();
    chartView = new QChartView(chart, outputPanel);
    chartView->setRenderHint(QPainter::Antialiasing);
    
    // Add group box and chart view to layout
    layout->addWidget(outputGroup);
    layout->addWidget(chartView);
    
    // Set size ratio of results to chart
    layout->setStretchFactor(outputGroup, 1);
    layout->setStretchFactor(chartView, 3);
}

void MainWindow::setupChart() {
    // Create series for chart
    slippageSeries = new QLineSeries();
    impactSeries = new QLineSeries();
    feesSeries = new QLineSeries();
    totalCostSeries = new QLineSeries();
    
    // Set series names
    slippageSeries->setName("Slippage");
    impactSeries->setName("Market Impact");
    feesSeries->setName("Fees");
    totalCostSeries->setName("Total Cost");
    
    // Add series to chart
    chart->addSeries(slippageSeries);
    chart->addSeries(impactSeries);
    chart->addSeries(feesSeries);
    chart->addSeries(totalCostSeries);
    
    // Create axes
    axisX = new QValueAxis();
    axisY = new QValueAxis();
    
    // Set axes properties
    axisX->setTitleText("Time (s)");
    axisY->setTitleText("Cost (USD)");
    axisX->setRange(0, kMaxDataPoints);
    axisY->setRange(0, 1.0);  // Will auto-adjust as needed
    
    // Add axes to chart
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    
    // Attach axes to series
    slippageSeries->attachAxis(axisX);
    slippageSeries->attachAxis(axisY);
    impactSeries->attachAxis(axisX);
    impactSeries->attachAxis(axisY);
    feesSeries->attachAxis(axisX);
    feesSeries->attachAxis(axisY);
    totalCostSeries->attachAxis(axisX);
    totalCostSeries->attachAxis(axisY);
    
    // Set chart title
    chart->setTitle("Transaction Costs Over Time");
    
    // Enable chart legend
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
}

void MainWindow::connectSignals() {
    // Connect start/stop button
    connect(startStopButton, &QPushButton::clicked, this, &MainWindow::onStartStopButtonClicked);
    
    // Connect parameter change signals
    connect(exchangeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::onParametersChanged);
    connect(symbolComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::onParametersChanged);
    connect(orderTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::onParametersChanged);
    connect(quantitySpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &MainWindow::onParametersChanged);
    connect(volatilitySpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &MainWindow::onParametersChanged);
    connect(feeTierSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &MainWindow::onParametersChanged);
    
    // Connect update timer for chart
    connect(&updateTimer, &QTimer::timeout, this, &MainWindow::updateChart);
}

void MainWindow::onStartStopButtonClicked() {
    if (simulator) {
        if (simulator->isRunning()) {
            simulator->stop();
            startStopButton->setText("Start Simulator");
        } else {
            simulator->start();
            startStopButton->setText("Stop Simulator");
        }
    }
}

void MainWindow::onParametersChanged() {
    if (simulator) {
        models::SimulatorParams params;
        
        params.exchange = exchangeComboBox->currentText().toStdString();
        params.symbol = symbolComboBox->currentText().toStdString();
        params.orderType = orderTypeComboBox->currentText().toLower().toStdString();
        params.quantity = quantitySpinBox->value();
        params.volatility = volatilitySpinBox->value();
        params.feeTier = feeTierSpinBox->value();
        
        simulator->updateParams(params);
    }
}

void MainWindow::updateOutput(const models::SimulatorOutput& output) {
    // Update result labels
    slippageLabel->setText(formatCurrency(output.expectedSlippage));
    feesLabel->setText(formatCurrency(output.expectedFees));
    marketImpactLabel->setText(formatCurrency(output.expectedMarketImpact));
    netCostLabel->setText(formatCurrency(output.netCost));
    
    // Update maker/taker proportion
    double makerPercentage = output.makerProportion * 100.0;
    double takerPercentage = (1.0 - output.makerProportion) * 100.0;
    makerTakerLabel->setText(
        QString("%1% / %2%")
            .arg(makerPercentage, 0, 'f', 1)
            .arg(takerPercentage, 0, 'f', 1)
    );
    
    // Update latency
    if (output.internalLatency > 1000.0) {
        latencyLabel->setText(
            QString("%1 ms").arg(output.internalLatency / 1000.0, 0, 'f', 2)
        );
    } else {
        latencyLabel->setText(
            QString("%1 μs").arg(output.internalLatency, 0, 'f', 0)
        );
    }
}

void MainWindow::updateChart() {
    if (!simulator || !simulator->isRunning()) {
        return;
    }
    
    // Get latest output
    models::SimulatorOutput output = simulator->getLatestOutput();
    
    // Add data points to series
    slippageSeries->append(dataPointCounter, output.expectedSlippage);
    impactSeries->append(dataPointCounter, output.expectedMarketImpact);
    feesSeries->append(dataPointCounter, output.expectedFees);
    totalCostSeries->append(dataPointCounter, output.netCost);
    
    // Increment counter
    dataPointCounter++;
    
    // Remove old points if we exceed the maximum
    if (slippageSeries->count() > kMaxDataPoints) {
        slippageSeries->remove(0);
        impactSeries->remove(0);
        feesSeries->remove(0);
        totalCostSeries->remove(0);
    }
    
    // Update X axis range to show the most recent points
    if (dataPointCounter > kMaxDataPoints) {
        axisX->setRange(dataPointCounter - kMaxDataPoints, dataPointCounter);
    } else {
        axisX->setRange(0, std::max(kMaxDataPoints, dataPointCounter));
    }
    
    // Find the maximum Y value to auto-scale the Y axis
    double maxY = 0.0;
    for (int i = 0; i < totalCostSeries->count(); i++) {
        maxY = std::max(maxY, totalCostSeries->at(i).y());
    }
    
    // Add some margin to the Y axis
    if (maxY > 0.0) {
        axisY->setRange(0, maxY * 1.1);
    }
}

QString MainWindow::formatCurrency(double value) const {
    return QString("$ %1").arg(value, 0, 'f', 4);
}

QString MainWindow::formatPercentage(double value) const {
    return QString("%1%").arg(value * 100.0, 0, 'f', 2);
}

} // namespace ui
} // namespace trade_simulator 