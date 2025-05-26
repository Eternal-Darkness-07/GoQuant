#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <utility>

namespace trade_simulator {
namespace data {

/**
 * @brief Price and size pair used in orderbook levels
 */
using PriceLevel = std::pair<double, double>;

/**
 * @brief Collection of price levels (price, size) for one side of the orderbook
 */
using PriceLevels = std::vector<PriceLevel>;

/**
 * @brief Structure representing the full order book data
 */
struct OrderbookData {
    std::string timestamp;
    std::string exchange;
    std::string symbol;
    PriceLevels asks;  // Sorted ascending by price
    PriceLevels bids;  // Sorted descending by price
    std::chrono::steady_clock::time_point received_time;

    // Constructor
    OrderbookData()
        : timestamp{}, exchange{}, symbol{}, asks{}, bids{},
          received_time{std::chrono::steady_clock::now()} {}
};

/**
 * @brief Structure to store orderbook statistics used for modeling
 */
struct OrderbookStats {
    double midprice = 0.0;
    double spread = 0.0;
    double best_ask = 0.0;
    double best_bid = 0.0;
    double weighted_ask_price = 0.0;  // Volume-weighted
    double weighted_bid_price = 0.0;  // Volume-weighted
    double total_ask_size = 0.0;
    double total_bid_size = 0.0;
    double order_imbalance = 0.0;     // Ratio of bid vs ask volume
    double price_volatility = 0.0;    // Recent price changes
    
    // Performance metrics
    std::chrono::microseconds processing_latency{0};
};

} // namespace data
} // namespace trade_simulator 