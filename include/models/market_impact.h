#pragma once

#include <cmath>
#include <vector>
#include <string>
#include "data/orderbook_types.h"

namespace trade_simulator {
namespace models {

/**
 * @brief Parameters for the Almgren-Chriss market impact model
 */
struct AlmgrenChrissParams {
    double permanentImpactFactor = 0.1;  // Permanent impact factor
    double temporaryImpactFactor = 0.1;  // Temporary impact factor
    double volatility = 0.0;             // Market volatility
    double timeHorizon = 1.0;            // Time horizon for execution (in seconds)
    double riskAversion = 1.0;           // Risk aversion parameter
    
    // Default constructor
    AlmgrenChrissParams() = default;
    
    // Constructor with parameters
    AlmgrenChrissParams(double permanent, double temporary, double vol, 
                         double time, double risk)
        : permanentImpactFactor(permanent), temporaryImpactFactor(temporary),
          volatility(vol), timeHorizon(time), riskAversion(risk) {}
};

/**
 * @brief Market impact model based on Almgren-Chriss
 * 
 * The Almgren-Chriss model for optimal execution divides price impact into:
 * 1. Permanent impact - affects all future trades
 * 2. Temporary impact - only affects the current trade
 * 
 * This class implements the model and provides methods to estimate the market impact
 * of a trade based on the current market conditions.
 */
class MarketImpactModel {
public:
    /**
     * @brief Constructor
     * @param params Initial parameters for the Almgren-Chriss model
     */
    explicit MarketImpactModel(const AlmgrenChrissParams& params = AlmgrenChrissParams());
    
    /**
     * @brief Set the model parameters
     * @param params New parameters for the model
     */
    void setParameters(const AlmgrenChrissParams& params);
    
    /**
     * @brief Get the current model parameters
     * @return Current parameters
     */
    AlmgrenChrissParams getParameters() const;
    
    /**
     * @brief Calculate the expected market impact for a market order
     * @param orderSize Size of the order in base units
     * @param orderSide True for buy, false for sell
     * @param stats Current orderbook statistics
     * @return Estimated market impact in price units
     */
    double calculateMarketImpact(double orderSize, bool orderSide, 
                                 const data::OrderbookStats& stats) const;
    
    /**
     * @brief Calculate the optimal execution schedule according to Almgren-Chriss
     * @param orderSize Total size to execute
     * @param orderSide True for buy, false for sell
     * @param stats Current orderbook statistics
     * @param numSteps Number of execution steps
     * @return Vector of trade sizes for each step
     */
    std::vector<double> calculateOptimalExecution(double orderSize, bool orderSide,
                                                  const data::OrderbookStats& stats, 
                                                  int numSteps = 10) const;
    
private:
    AlmgrenChrissParams params_;
    
    /**
     * @brief Calculate the permanent impact component
     * @param orderSize Size of the order
     * @param stats Orderbook statistics
     * @return Permanent impact
     */
    double calculatePermanentImpact(double orderSize, const data::OrderbookStats& stats) const;
    
    /**
     * @brief Calculate the temporary impact component
     * @param orderSize Size of the order
     * @param stats Orderbook statistics
     * @return Temporary impact
     */
    double calculateTemporaryImpact(double orderSize, const data::OrderbookStats& stats) const;
};

} // namespace models
} // namespace trade_simulator 