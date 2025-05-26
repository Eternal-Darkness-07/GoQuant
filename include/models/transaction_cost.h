#pragma once

#include <string>
#include <memory>
#include "data/orderbook_types.h"
#include "models/market_impact.h"

namespace trade_simulator {
namespace models {

/**
 * @brief Fee model structure for different fee tiers
 */
struct FeeModel {
    double makerFeeRate = 0.0002;  // Default maker fee rate (0.02%)
    double takerFeeRate = 0.0005;  // Default taker fee rate (0.05%)
    int feeTier = 0;               // Fee tier (0 = base, higher = better rates)
    
    // Constructor with default values
    FeeModel() = default;
    
    // Constructor with custom values
    FeeModel(double maker, double taker, int tier)
        : makerFeeRate(maker), takerFeeRate(taker), feeTier(tier) {}
};

/**
 * @brief Transaction cost model for estimating execution costs
 */
class TransactionCostModel {
public:
    /**
     * @brief Constructor
     * @param marketImpactModel Market impact model to use
     * @param feeModel Fee model to use
     */
    TransactionCostModel(std::shared_ptr<MarketImpactModel> marketImpactModel,
                        const FeeModel& feeModel = FeeModel());
    
    /**
     * @brief Set the fee model
     * @param feeModel New fee model
     */
    void setFeeModel(const FeeModel& feeModel);
    
    /**
     * @brief Get the current fee model
     * @return Current fee model
     */
    FeeModel getFeeModel() const;
    
    /**
     * @brief Calculate expected slippage for a market order
     * @param orderSize Size of the order in base units
     * @param orderSide True for buy, false for sell
     * @param stats Current orderbook statistics
     * @return Expected slippage in price units
     */
    double calculateSlippage(double orderSize, bool orderSide, 
                            const data::OrderbookStats& stats) const;
    
    /**
     * @brief Calculate expected fees for a market order
     * @param orderSize Size of the order in base units
     * @param orderPrice Price of the order
     * @param makerProportion Expected proportion of the order filled as maker (0.0-1.0)
     * @return Expected fees in price units
     */
    double calculateFees(double orderSize, double orderPrice, double makerProportion) const;
    
    /**
     * @brief Predict maker/taker proportion based on order and market conditions
     * @param orderSize Size of the order in base units
     * @param orderSide True for buy, false for sell
     * @param stats Current orderbook statistics
     * @return Expected maker proportion (0.0-1.0)
     */
    double predictMakerProportion(double orderSize, bool orderSide,
                                 const data::OrderbookStats& stats) const;
    
    /**
     * @brief Calculate all transaction costs for a market order
     * @param orderSize Size of the order in base units
     * @param orderSide True for buy, false for sell
     * @param stats Current orderbook statistics
     * @return Tuple of (slippage, marketImpact, fees, totalCost) in price units
     */
    std::tuple<double, double, double, double> calculateTotalCost(
        double orderSize, bool orderSide, const data::OrderbookStats& stats) const;
    
private:
    std::shared_ptr<MarketImpactModel> marketImpactModel_;
    FeeModel feeModel_;
    
    // Regression coefficients for slippage model
    double slippageIntercept_ = 0.0;
    double slippageVolumeFactor_ = 0.1;
    double slippageVolatilityFactor_ = 0.2;
    double slippageImbalanceFactor_ = -0.05;
};

} // namespace models
} // namespace trade_simulator 