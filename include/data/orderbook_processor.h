#pragma once

#include <deque>
#include <mutex>
#include <vector>
#include <functional>
#include <chrono>
#include <atomic>

#include "data/orderbook_types.h"

namespace trade_simulator {
namespace data {

/**
 * @brief Class for processing orderbook data and calculating statistics
 */
class OrderbookProcessor {
public:
    /**
     * @brief Callback type for orderbook statistics updates
     */
    using StatsCallback = std::function<void(const OrderbookStats&)>;

    /**
     * @brief Constructor
     * @param statsCallback Function to call with updated orderbook statistics
     * @param historyWindowSize Number of orderbook updates to keep for historical calculations
     */
    OrderbookProcessor(StatsCallback statsCallback, size_t historyWindowSize = 100);

    /**
     * @brief Process a new orderbook update
     * @param data The new orderbook data
     */
    void processOrderbook(const OrderbookData& data);

    /**
     * @brief Get the latest orderbook statistics
     * @return The latest orderbook statistics
     */
    OrderbookStats getLatestStats() const;

    /**
     * @brief Get the average processing latency in microseconds
     * @return Average processing latency
     */
    double getAverageLatency() const;

private:
    // Callback for statistics updates
    StatsCallback statsCallback_;
    
    // History of orderbook data and statistics
    size_t historyWindowSize_;
    std::deque<OrderbookData> orderbookHistory_;
    mutable std::mutex historyMutex_;
    
    // Latest statistics
    OrderbookStats latestStats_;
    mutable std::mutex statsMutex_;
    
    // Performance metrics
    std::atomic<uint64_t> totalProcessingTime_{0};
    std::atomic<uint64_t> processedUpdates_{0};

    /**
     * @brief Calculate orderbook statistics from the given data
     * @param data The orderbook data
     * @return Calculated statistics
     */
    OrderbookStats calculateStats(const OrderbookData& data);

    /**
     * @brief Calculate price volatility from historical data
     * @return Price volatility
     */
    double calculateVolatility() const;

    /**
     * @brief Calculate volume-weighted average price
     * @param priceLevels Price levels to use
     * @param maxLevels Maximum number of levels to include (0 = all)
     * @return Volume-weighted average price
     */
    double calculateVWAP(const PriceLevels& priceLevels, size_t maxLevels = 0) const;

    /**
     * @brief Update the history with new orderbook data
     * @param data New orderbook data to add to history
     */
    void updateHistory(const OrderbookData& data);
};

} // namespace data
} // namespace trade_simulator 