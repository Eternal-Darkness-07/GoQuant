#include "models/market_impact.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace trade_simulator {
namespace models {

MarketImpactModel::MarketImpactModel(const AlmgrenChrissParams& params)
    : params_(params) {
}

void MarketImpactModel::setParameters(const AlmgrenChrissParams& params) {
    params_ = params;
}

AlmgrenChrissParams MarketImpactModel::getParameters() const {
    return params_;
}

double MarketImpactModel::calculateMarketImpact(double orderSize, bool orderSide, 
                                              const data::OrderbookStats& stats) const {
    // Set sign based on order side (positive for buy, negative for sell)
    double sign = orderSide ? 1.0 : -1.0;
    
    // Calculate permanent and temporary impact components
    double permanentImpact = calculatePermanentImpact(orderSize, stats);
    double temporaryImpact = calculateTemporaryImpact(orderSize, stats);
    
    // Total impact is the sum of permanent and temporary impact
    double totalImpact = (permanentImpact + temporaryImpact) * sign;
    
    return totalImpact;
}

std::vector<double> MarketImpactModel::calculateOptimalExecution(
    double orderSize, bool orderSide, const data::OrderbookStats& stats, int numSteps) const {
    
    // Ensure numSteps is at least 1
    numSteps = std::max(1, numSteps);
    
    // Initialize result vector
    std::vector<double> executionSchedule(numSteps, 0.0);
    
    // If order size is zero or market has no liquidity, return empty schedule
    if (orderSize <= 0.0 || stats.total_ask_size <= 0.0 || stats.total_bid_size <= 0.0) {
        return executionSchedule;
    }
    
    // Use Almgren-Chriss model to determine optimal trading schedule
    // The model suggests that the optimal execution schedule is a linear trajectory
    // when the temporary impact has a square-root form
    
    // Simplified implementation: divide the order equally across time steps
    double baseSize = orderSize / numSteps;
    
    // Calculate risk adjustment factor
    double riskFactor = params_.riskAversion * std::pow(params_.volatility, 2.0) * 
                       params_.timeHorizon / (numSteps - 1);
    
    double effectiveSize = orderSize;
    double remainingSize = orderSize;
    
    for (int i = 0; i < numSteps; ++i) {
        // Calculate time step in [0,1]
        double timeStep = static_cast<double>(i) / (numSteps - 1);
        
        // Hyperbolic trading strategy gives more weight to early trades
        double weight = std::exp(-riskFactor * timeStep);
        
        // Scale the weight by total
        double totalWeight = 0.0;
        for (int j = 0; j < numSteps; ++j) {
            totalWeight += std::exp(-riskFactor * static_cast<double>(j) / (numSteps - 1));
        }
        
        // Calculate trade size for this step
        double tradeSize = (weight / totalWeight) * effectiveSize;
        
        // Ensure we don't exceed the remaining size due to rounding errors
        tradeSize = std::min(tradeSize, remainingSize);
        remainingSize -= tradeSize;
        
        executionSchedule[i] = tradeSize;
    }
    
    // Allocate any remaining size (due to rounding) to the last step
    if (remainingSize > 0.0) {
        executionSchedule.back() += remainingSize;
    }
    
    return executionSchedule;
}

double MarketImpactModel::calculatePermanentImpact(double orderSize, 
                                                const data::OrderbookStats& stats) const {
    // Almgren-Chriss model typically uses a linear permanent impact model:
    // permanent_impact_price_change = gamma * sigma * (orderSize / DailyVolume)
    // Or simply permanent_impact_cost_per_share = gamma_coeff * sigma * (orderSize / DailyVolume)
    // The current model seems to estimate a price change.
    
    // Scale the impact factor by market liquidity (less liquid = higher impact)
    double marketDepth = stats.total_ask_size + stats.total_bid_size;
    double scaledFactor = params_.permanentImpactFactor; // This is our base gamma coefficient
    
    if (marketDepth > 0.0) {
        // Adjust impact based on order size relative to market depth
        // This makes the effective gamma dependent on fraction of market consumed
        scaledFactor *= (1.0 + std::min(1.0, orderSize / marketDepth)); 
    }
    
    // Consider REAL-TIME volatility in the impact calculation
    // Higher volatility typically leads to higher impact
    // Using stats.price_volatility directly as sigma
    double volatilityFactor = stats.price_volatility; 
    
    // Price impact = base_gamma_scaled_by_depth * orderSize * sigma
    // This implies permanentImpactFactor is like gamma/DailyVolume or similar
    // Let's assume the existing structure implies: Impact = Factor * Size * VolatilityScaling
    // The original volatilityFactor was (1.0 + params_.volatility)
    // If params_.volatility is an annual vol (e.g. 0.2 for 20%) and stats.price_volatility is similar scale, direct use is better.
    // If stats.price_volatility is small (e.g. from short-term calculations), (1.0 + stats.price_volatility) might be more stable.
    // For now, let's assume stats.price_volatility is appropriately scaled for direct multiplication.
    return scaledFactor * orderSize * volatilityFactor;
}

double MarketImpactModel::calculateTemporaryImpact(double orderSize, 
                                                const data::OrderbookStats& stats) const {
    // Almgren-Chriss temporary impact cost per share h(v) = eta * sigma * (v/V_day)^delta
    // Total temporary impact cost = orderSize * h(v)
    // Here, v = orderSize / T_execution. If T_execution is short, v is high.
    // The model seems to calculate temporary impact price change, not cost directly.
    
    // Use square root model with scaling: temporary_impact_price_change = Factor * sigma * orderSize^alpha * OtherFactors
    const double alpha = 0.5;  // Square root power law
    double impactBase = std::pow(orderSize, alpha); // orderSize^0.5
    
    // Scale by market liquidity (spread & depth)
    double liquidityFactor = 1.0;
    if (stats.midprice > 0.0 && stats.spread > 0.0) { // Avoid division by zero
        // Wider spread means less liquidity and higher impact
        // Normalize spread to be a factor
        liquidityFactor *= (1.0 + stats.spread / stats.midprice);
    }
    
    // Consider order imbalance in the impact
    double imbalanceFactor = 1.0;
    if (stats.total_ask_size > 0 && stats.total_bid_size > 0) { // Avoid issues with zero depth
        // Using log to capture large imbalances, ensuring it's >= 1
        imbalanceFactor = std::max(1.0, std::abs(std::log(stats.order_imbalance)));
    }
    
    // params_.temporaryImpactFactor is our base eta coefficient
    // Multiply by REAL-TIME volatility (stats.price_volatility as sigma)
    // Temporary Impact Price Change = eta_base * sigma * orderSize^alpha * liquidityFactor * imbalanceFactor
    return params_.temporaryImpactFactor * stats.price_volatility * impactBase * liquidityFactor * imbalanceFactor;
}

} // namespace models
} // namespace trade_simulator 