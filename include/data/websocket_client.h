#pragma once

#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/strand.hpp>

#include "data/orderbook_types.h"

namespace trade_simulator {
namespace data {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

/**
 * @brief WebSocket client for connecting to L2 orderbook data stream
 */
class WebSocketClient {
public:
    /**
     * @brief Callback type for orderbook data updates
     */
    using OrderbookCallback = std::function<void(const OrderbookData&)>;

    /**
     * @brief Constructor
     * @param callback Function to call with each new orderbook update
     */
    WebSocketClient(OrderbookCallback callback);

    /**
     * @brief Destructor
     */
    ~WebSocketClient();

    /**
     * @brief Start the WebSocket client
     */
    void start();

    /**
     * @brief Stop the WebSocket client
     */
    void stop();

    /**
     * @brief Check if the client is connected
     * @return True if connected, false otherwise
     */
    bool isConnected() const;

    /**
     * @brief Check if the connection is healthy
     * @param maxIdleSeconds Maximum seconds without a message before connection is unhealthy
     * @return True if healthy, false otherwise
     */
    bool isHealthy(int maxIdleSeconds = 10) const;

private:
    // WebSocket endpoint
    const std::string kHost = "ws.gomarket-cpp.goquant.io";
    const std::string kPort = "443";
    const std::string kTarget = "/ws/l2-orderbook/okx/BTC-USDT-SWAP";
    
    // Connection parameters
    const int kReconnectInitialDelayMs = 1000;
    const int kReconnectMaxDelayMs = 60000;
    
    // Callback for orderbook updates
    OrderbookCallback callback_;
    
    // Asio
    net::io_context ioc_;
    std::unique_ptr<std::thread> ioThread_;
    
    // Connection state
    std::atomic<bool> isConnected_{false};
    std::atomic<bool> shouldRun_{false};
    std::chrono::steady_clock::time_point lastMessageTime_;
    mutable std::mutex stateMutex_;
    
    // Reconnect logic
    int reconnectDelayMs_{kReconnectInitialDelayMs};
    
    /**
     * @brief Run the IO service on a separate thread
     */
    void runIoService();
    
    /**
     * @brief Connect to the WebSocket
     */
    void connect();
    
    /**
     * @brief Process a received message
     * @param message The message received from the WebSocket
     */
    void processMessage(const std::string& message);
    
    /**
     * @brief Parse orderbook data from JSON message
     * @param jsonMessage The JSON message from the WebSocket
     * @return OrderbookData structure with parsed data
     */
    OrderbookData parseOrderbookData(const std::string& jsonMessage);
    
    /**
     * @brief Reconnect to the WebSocket with exponential backoff
     */
    void reconnect();
};

} // namespace data
} // namespace trade_simulator 