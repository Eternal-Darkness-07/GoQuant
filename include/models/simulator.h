#pragma once

#include <memory>
#include <string>
#include <functional>
#include <mutex>
#include <atomic>
#include <QMetaType>

#include "data/websocket_client.h"
#include "data/orderbook_processor.h"
#include "models/market_impact.h"
#include "models/transaction_cost.h"

namespace trade_simulator {
namespace models {

/**
 * @brief Input parameters for the simulator
 */
struct SimulatorParams {
    std::string exchange = "OKX";
    std::string symbol = "BTC-USDT";
    std::string orderType = "market";
    double quantity = 100.0;  // In USD equivalent
    double volatility = 0.0;  // Market parameter (will be overridden by market data)
    int feeTier = 0;
    
    // Default constructor
    SimulatorParams() = default;
};

/**
 * @brief Output metrics from the simulator
 */
struct SimulatorOutput {
    // Cost metrics
    double expectedSlippage = 0.0;
    double expectedFees = 0.0;
    double expectedMarketImpact = 0.0;
    double netCost = 0.0;
    double makerProportion = 0.0;
    
    // Performance metrics
    double internalLatency = 0.0;  // In microseconds
    
    // Market metrics
    double midprice = 0.0;
    double spread = 0.0;
    double marketVolatility = 0.0;
    
    // Default constructor
    SimulatorOutput() = default;
};

/**
 * @brief Callback type for simulator output updates
 */
using SimulatorCallback = std::function<void(const SimulatorOutput&)>;

/**
 * @brief Main simulator class that integrates all components
 */
class Simulator {
public:
    /**
     * @brief Constructor
     * @param callback Function to call with updated simulation results
     */
    explicit Simulator(SimulatorCallback callback);
    
    /**
     * @brief Destructor
     */
    ~Simulator();
    
    /**
     * @brief Start the simulator
     */
    void start();
    
    /**
     * @brief Stop the simulator
     */
    void stop();
    
    /**
     * @brief Update the simulator parameters
     * @param params New parameters
     */
    void updateParams(const SimulatorParams& params);
    
    /**
     * @brief Get the current simulator parameters
     * @return Current parameters
     */
    SimulatorParams getParams() const;
    
    /**
     * @brief Get the latest simulator output
     * @return Latest output
     */
    SimulatorOutput getLatestOutput() const;
    
    /**
     * @brief Check if the simulator is running
     * @return True if running, false otherwise
     */
    bool isRunning() const;
    
private:
    // Callback for output updates
    SimulatorCallback callback_;
    
    // Parameters and output
    SimulatorParams params_;
    SimulatorOutput latestOutput_;
    mutable std::mutex paramsMutex_;
    mutable std::mutex outputMutex_;
    
    // Running state
    std::atomic<bool> isRunning_{false};
    
    // Components
    std::shared_ptr<data::WebSocketClient> webSocketClient_;
    std::shared_ptr<data::OrderbookProcessor> orderbookProcessor_;
    std::shared_ptr<MarketImpactModel> marketImpactModel_;
    std::shared_ptr<TransactionCostModel> transactionCostModel_;
    
    /**
     * @brief Initialize the simulator components
     */
    void initializeComponents();
    
    /**
     * @brief Handle orderbook statistics updates
     * @param stats Updated orderbook statistics
     */
    void onOrderbookStats(const data::OrderbookStats& stats);
    
    /**
     * @brief Update simulation results based on current market conditions
     * @param stats Current orderbook statistics
     */
    void updateSimulation(const data::OrderbookStats& stats);
};

} // namespace models
} // namespace trade_simulator

// Declare the metatype for SimulatorOutput globally after its definition
Q_DECLARE_METATYPE(trade_simulator::models::SimulatorOutput) 