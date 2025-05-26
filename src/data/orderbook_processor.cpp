#include "data/orderbook_processor.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>

namespace trade_simulator {
namespace data {

OrderbookProcessor::OrderbookProcessor(StatsCallback statsCallback, size_t historyWindowSize)
    : statsCallback_(std::move(statsCallback)),
      historyWindowSize_(historyWindowSize) {
    // Initialize the latestStats with default values
}

void OrderbookProcessor::processOrderbook(const OrderbookData& data) {
    auto startTime = std::chrono::steady_clock::now();
    
    // Update the order book history
    updateHistory(data);
    
    // Calculate statistics
    OrderbookStats stats = calculateStats(data);
    
    // Measure processing time
    auto endTime = std::chrono::steady_clock::now();
    auto processingTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    
    // Update performance metrics 
    totalProcessingTime_ += processingTime;
    processedUpdates_++;
    
    // Store the processing latency in the stats
    stats.processing_latency = std::chrono::microseconds(processingTime);
    
    // Update latest stats
    {
        std::lock_guard<std::mutex> lock(statsMutex_);
        latestStats_ = stats; 
    }
    
    // Notify the callback
    statsCallback_(stats); 
}

OrderbookStats OrderbookProcessor::getLatestStats() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return latestStats_;
}

double OrderbookProcessor::getAverageLatency() const {
    uint64_t updates = processedUpdates_.load();
    if (updates == 0) {
        return 0.0;
    }
    return static_cast<double>(totalProcessingTime_.load()) / updates;
}

OrderbookStats OrderbookProcessor::calculateStats(const OrderbookData& data) {
    OrderbookStats stats;
    
    // Make sure we have bid and ask data
    if (data.asks.empty() || data.bids.empty()) {
        return stats;
    }
    
    // Calculate best bid and ask
    stats.best_ask = data.asks[0].first;
    stats.best_bid = data.bids[0].first;
    
    // Calculate mid price and spread
    stats.midprice = (stats.best_ask + stats.best_bid) / 2.0;
    stats.spread = stats.best_ask - stats.best_bid;
    
    // Calculate weighted prices
    stats.weighted_ask_price = calculateVWAP(data.asks, 10);
    stats.weighted_bid_price = calculateVWAP(data.bids, 10);
    
    // Calculate total sizes
    stats.total_ask_size = std::accumulate(data.asks.begin(), data.asks.end(), 0.0,
        [](double sum, const PriceLevel& level) { return sum + level.second; });
    
    stats.total_bid_size = std::accumulate(data.bids.begin(), data.bids.end(), 0.0,
        [](double sum, const PriceLevel& level) { return sum + level.second; });
    
    // Calculate order imbalance
    if (stats.total_ask_size > 0) {
        stats.order_imbalance = stats.total_bid_size / stats.total_ask_size;
    }
    
    // Calculate volatility from history
    stats.price_volatility = calculateVolatility();
    
    return stats;
}

double OrderbookProcessor::calculateVolatility() const {
    std::lock_guard<std::mutex> lock(historyMutex_);
    
    if (orderbookHistory_.size() < 2) {
        return 0.0;
    }
    
    // Extract midprices from history
    std::vector<double> midprices;
    midprices.reserve(orderbookHistory_.size());
    
    for (const auto& orderbook : orderbookHistory_) {
        if (!orderbook.asks.empty() && !orderbook.bids.empty()) {
            double bestAsk = orderbook.asks[0].first;
            double bestBid = orderbook.bids[0].first;
            double midprice = (bestAsk + bestBid) / 2.0;
            midprices.push_back(midprice);
        }
    }
    
    if (midprices.size() < 2) {
        return 0.0;
    }
    
    // Calculate returns
    std::vector<double> returns;
    returns.reserve(midprices.size() - 1);
    
    for (size_t i = 1; i < midprices.size(); i++) {
        double ret = (midprices[i] - midprices[i-1]) / midprices[i-1];
        returns.push_back(ret);
    }
    
    // Calculate standard deviation of returns
    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    
    double sqSum = std::accumulate(returns.begin(), returns.end(), 0.0,
        [mean](double sum, double x) {
            double diff = x - mean;
            return sum + diff * diff;
        });
    
    double variance = sqSum / returns.size();
    return std::sqrt(variance);
}

double OrderbookProcessor::calculateVWAP(const PriceLevels& priceLevels, size_t maxLevels) const {
    if (priceLevels.empty()) {
        return 0.0;
    }
    
    size_t levelsToUse = maxLevels > 0 ? std::min(maxLevels, priceLevels.size()) : priceLevels.size();
    
    double priceSum = 0.0;
    double volumeSum = 0.0;
    
    for (size_t i = 0; i < levelsToUse; i++) {
        double price = priceLevels[i].first;
        double volume = priceLevels[i].second;
        
        priceSum += price * volume;
        volumeSum += volume;
    }
    
    if (volumeSum > 0.0) {
        return priceSum / volumeSum;
    }
    
    return 0.0;
}

void OrderbookProcessor::updateHistory(const OrderbookData& data) {
    std::lock_guard<std::mutex> lock(historyMutex_);
    
    // Add the new data to history
    orderbookHistory_.push_back(data);
    
    // Remove oldest entries if we exceed the window size
    while (orderbookHistory_.size() > historyWindowSize_) {
        orderbookHistory_.pop_front();
    }
}

} // namespace data
} // namespace trade_simulator 