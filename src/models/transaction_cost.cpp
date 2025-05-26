#include "models/transaction_cost.h"
#include <cmath>
#include <algorithm>

namespace trade_simulator {
namespace models {

TransactionCostModel::TransactionCostModel(
    std::shared_ptr<MarketImpactModel> marketImpactModel,
    const FeeModel& feeModel)
    : marketImpactModel_(marketImpactModel),
      feeModel_(feeModel) {
}

void TransactionCostModel::setFeeModel(const FeeModel& feeModel) {
    feeModel_ = feeModel;
}

FeeModel TransactionCostModel::getFeeModel() const {
    return feeModel_;
}

double TransactionCostModel::calculateSlippage(double orderSize, bool orderSide, 
                                            const data::OrderbookStats& stats) const {
    // For this implementation, we'll use a linear regression model
    // to predict slippage based on order size, volatility, and order imbalance
    
    double relativeSizeToDepth = 0.0;
    
    if (orderSide) {  // Buy order  
        if (stats.total_ask_size > 0.0) {
            relativeSizeToDepth = orderSize / stats.total_ask_size;
        }
    } else {  // Sell order
        if (stats.total_bid_size > 0.0) {
            relativeSizeToDepth = orderSize / stats.total_bid_size;
        }
    }
    
    // Apply regression model
    double slippageEstimate = slippageIntercept_ +   
                             (slippageVolumeFactor_ * relativeSizeToDepth) +
                             (slippageVolatilityFactor_ * stats.price_volatility) +
                             (slippageImbalanceFactor_ * (stats.order_imbalance - 1.0));
    
    // Convert to price impact
    // For buys: positive slippage means paying more
    // For sells: positive slippage means receiving less
    double priceSlippage = slippageEstimate * stats.midprice;
    
    // Apply a minimum slippage equal to half the spread
    double minSlippage = stats.spread / 2.0;   
    
    return std::max(priceSlippage, minSlippage);
}

double TransactionCostModel::calculateFees(double orderSize, double orderPrice, 
                                        double makerProportion) const {
    // Ensure maker proportion is between 0 and 1
    makerProportion = std::clamp(makerProportion, 0.0, 1.0);
    
    // Calculate taker proportion
    double takerProportion = 1.0 - makerProportion;
    
    // Calculate notional value
    double notionalValue = orderSize * orderPrice;
    
    // Calculate fee components
    double makerFee = notionalValue * makerProportion * feeModel_.makerFeeRate;
    double takerFee = notionalValue * takerProportion * feeModel_.takerFeeRate;
    
    // Total fee
    return makerFee + takerFee;
}

double TransactionCostModel::predictMakerProportion(double orderSize, bool orderSide,
                                                  const data::OrderbookStats& stats) const {
    // Simplified logistic regression model for maker/taker proportion
    // For market orders, this will typically be very low (close to 0)
    // But some venues might allow market orders to rest as limit orders
    
    // Relative order size compared to market depth
    double relativeSizeToDepth = 0.0;
    
    if (orderSide) {  // Buy order 
        if (stats.total_ask_size > 0.0) {
            relativeSizeToDepth = orderSize / stats.total_ask_size;
        }
    } else {  // Sell order
        if (stats.total_bid_size > 0.0) {
            relativeSizeToDepth = orderSize / stats.total_bid_size;
        }
    }
    
    // For market orders, maker proportion is typically very low
    // The larger the order relative to available liquidity, the lower maker proportion
    double makerProportion = std::exp(-5.0 * relativeSizeToDepth);
    
    // Account for market volatility - higher volatility means less maker proportion
    makerProportion *= std::exp(-2.0 * stats.price_volatility);
    
    // Limit to [0, 0.1] range for market orders (rarely more than 10% maker)
    return std::clamp(makerProportion, 0.0, 0.1);
}

std::tuple<double, double, double, double> TransactionCostModel::calculateTotalCost(
    double orderSize, bool orderSide, const data::OrderbookStats& stats) const {
    
    // Calculate expected slippage
    double slippage = calculateSlippage(orderSize, orderSide, stats);
    
    // Calculate market impact using the Almgren-Chriss model
    double marketImpact = 0.0;
    if (marketImpactModel_) { 
        marketImpact = marketImpactModel_->calculateMarketImpact(orderSize, orderSide, stats);
    }
     
    // Predict maker/taker proportion
    double makerProportion = predictMakerProportion(orderSize, orderSide, stats);
    
    // Calculate expected fees
    // Use midprice as the reference price for fee calculation
    double fees = calculateFees(orderSize, stats.midprice, makerProportion); 
    
    // Calculate total cost
    double totalCost = slippage + marketImpact + fees;
    
    return std::make_tuple(slippage, marketImpact, fees, totalCost);
}

} // namespace models
} // namespace trade_simulator 