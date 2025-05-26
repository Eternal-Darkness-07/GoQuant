#include "models/simulator.h"
#include <iostream>
#include <chrono>
#include <functional>

namespace trade_simulator {
namespace models {

Simulator::Simulator(SimulatorCallback callback)
    : callback_(std::move(callback)) {
    
    initializeComponents();
}

Simulator::~Simulator() {
    stop();
}

void Simulator::start() {
    if (isRunning_) {
        return;
    }
    
    isRunning_ = true;
    
   
    if (webSocketClient_) {
        webSocketClient_->start();
    }
}

void Simulator::stop() {
    if (!isRunning_) {
        return;
    }
    
    isRunning_ = false;
    
   
    if (webSocketClient_) {
        webSocketClient_->stop();
    }
}

void Simulator::updateParams(const SimulatorParams& params) {
    std::lock_guard<std::mutex> lock(paramsMutex_);
    params_ = params;
    
    // Update market impact model with new volatility
    if (marketImpactModel_) {
        auto currentParams = marketImpactModel_->getParameters();
        currentParams.volatility = params_.volatility;
        marketImpactModel_->setParameters(currentParams);
    }
    
    // Update fee model with new fee tier
    if (transactionCostModel_) {
        auto currentFeeModel = transactionCostModel_->getFeeModel();
        currentFeeModel.feeTier = params_.feeTier;
        
        // Adjust fee rates based on tier
        // Higher tier means lower fees
        if (params_.feeTier == 0) {
            currentFeeModel.makerFeeRate = 0.0002;  // 0.02%
            currentFeeModel.takerFeeRate = 0.0005;  // 0.05%
        } else if (params_.feeTier == 1) {
            currentFeeModel.makerFeeRate = 0.00015;  // 0.015%
            currentFeeModel.takerFeeRate = 0.0004;   // 0.04%
        } else if (params_.feeTier == 2) {
            currentFeeModel.makerFeeRate = 0.0001;   // 0.01%
            currentFeeModel.takerFeeRate = 0.0003;   // 0.03%
        } else if (params_.feeTier >= 3) {
            currentFeeModel.makerFeeRate = 0.00005;  // 0.005%
            currentFeeModel.takerFeeRate = 0.0002;   // 0.02%
        }
        
        transactionCostModel_->setFeeModel(currentFeeModel);
    }
}

SimulatorParams Simulator::getParams() const {
    std::lock_guard<std::mutex> lock(paramsMutex_);
    return params_;
}

SimulatorOutput Simulator::getLatestOutput() const {
    std::lock_guard<std::mutex> lock(outputMutex_);
    return latestOutput_;
}

bool Simulator::isRunning() const {
    return isRunning_;
}

void Simulator::initializeComponents() {
    // Create market impact model
    marketImpactModel_ = std::make_shared<MarketImpactModel>();
    
    // Create transaction cost model
    transactionCostModel_ = std::make_shared<TransactionCostModel>(marketImpactModel_);
    
    // Create orderbook processor
    orderbookProcessor_ = std::make_shared<data::OrderbookProcessor>(
        [this](const data::OrderbookStats& stats) {
            onOrderbookStats(stats);
        }
    );
    
    // Create websocket client
    webSocketClient_ = std::make_shared<data::WebSocketClient>(
        [this](const data::OrderbookData& data) {
            if (orderbookProcessor_) {
                orderbookProcessor_->processOrderbook(data);
            }
        }
    );
}

void Simulator::onOrderbookStats(const data::OrderbookStats& stats) {
    if (!isRunning_) {
        return;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Update the simulation with the new stats
    updateSimulation(stats);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    
    // Update internal latency in the output
    {
        std::lock_guard<std::mutex> lock(outputMutex_);
        latestOutput_.internalLatency = static_cast<double>(latency);
    }
}

void Simulator::updateSimulation(const data::OrderbookStats& stats) {
    // Get a copy of the current parameters
    SimulatorParams params;
    {
        std::lock_guard<std::mutex> lock(paramsMutex_);
        params = params_;
    }
    
    // Create a new output structure
    SimulatorOutput output;
     
    // Set market metrics 
    output.midprice = stats.midprice;
    output.spread = stats.spread;
    output.marketVolatility = stats.price_volatility;
    
    // Calculate quantity in base units (BTC) from USD value
    double baseQuantity = 0.0;
    if (stats.midprice > 0.0) {
        baseQuantity = params.quantity / stats.midprice;
    }
    
    // Default to buy side
    bool orderSide = true;
    
    // Calculate transaction costs
    if (transactionCostModel_) {
        auto [slippage, marketImpact, fees, totalCost] = 
            transactionCostModel_->calculateTotalCost(baseQuantity, orderSide, stats);
        
        output.expectedSlippage = slippage;
        output.expectedMarketImpact = marketImpact;
        output.expectedFees = fees;
        output.netCost = totalCost;
        output.makerProportion = transactionCostModel_->predictMakerProportion(
            baseQuantity, orderSide, stats);
    }
    
    // Update latest output
    {
        std::lock_guard<std::mutex> lock(outputMutex_);
        latestOutput_ = output;
    } 
    
    // Notify callback
    callback_(output); 
}

} // namespace models
} // namespace trade_simulator 