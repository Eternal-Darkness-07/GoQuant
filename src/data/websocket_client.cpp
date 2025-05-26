#include "data/websocket_client.h"

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

using json = nlohmann::json;

namespace trade_simulator {
namespace data {

WebSocketClient::WebSocketClient(OrderbookCallback callback)
    : callback_(std::move(callback)),
      lastMessageTime_(std::chrono::steady_clock::now()) {
    // Nothing else to initialize
}

WebSocketClient::~WebSocketClient() {
    stop();
}

void WebSocketClient::start() {
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    if (shouldRun_) {
        return; // Already running
    }
    
    shouldRun_ = true; 
    reconnectDelayMs_ = kReconnectInitialDelayMs;
    
    // Start IO service in a separate thread
    ioThread_ = std::make_unique<std::thread>([this]() {
        runIoService();
    });
}

void WebSocketClient::stop() {
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        if (!shouldRun_) {
            return; // Already stopped
        }
        shouldRun_ = false;
    }
    
    // Wait for IO thread to finish
    if (ioThread_ && ioThread_->joinable()) {
        ioThread_->join();
        ioThread_.reset();
    }
    
    isConnected_ = false;
}

bool WebSocketClient::isConnected() const {
    return isConnected_;
}

bool WebSocketClient::isHealthy(int maxIdleSeconds) const {
    if (!isConnected_) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(stateMutex_);
    auto now = std::chrono::steady_clock::now();
    auto idleTime = std::chrono::duration_cast<std::chrono::seconds>(now - lastMessageTime_).count();
    
    return idleTime < maxIdleSeconds;
}

void WebSocketClient::runIoService() {
    while (shouldRun_) {
        try {
            // Reset the io_context
            ioc_.restart();
            
            // Connect to the WebSocket
            connect();
            
            // Run the IO context until it's stopped
            ioc_.run();
        }
        catch (const std::exception& e) {
            std::cerr << "WebSocket error: " << e.what() << std::endl;
        }
        
        // If we should still be running, reconnect after delay
        if (shouldRun_) {
            reconnect();
        }
    }
}

void WebSocketClient::connect() {
    try {
        // The SSL context is required for secure WebSockets
        ssl::context ctx{ssl::context::tlsv12_client};
        
        // Verify the certificate
        ctx.set_verify_mode(ssl::verify_peer);
        ctx.set_default_verify_paths();
        
        // These objects perform our I/O
        tcp::resolver resolver{ioc_};
        websocket::stream<ssl::stream<tcp::socket>> ws{ioc_, ctx};
        
        // Look up the domain name
        auto const results = resolver.resolve(kHost, kPort);
        
        // Make the connection on the IP address we get from a lookup
        auto ep = net::connect(ws.next_layer().next_layer(), results);
        
        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), kHost.c_str())) {
            throw beast::system_error(
                beast::error_code(
                    static_cast<int>(::ERR_get_error()),
                    net::error::get_ssl_category()),
                "Failed to set SNI Hostname");
        }
        
        // SSL handshake
        ws.next_layer().handshake(ssl::stream_base::client);
        
        // WebSocket handshake
        ws.handshake(kHost, kTarget);
        
        // Set connection state
        isConnected_ = true;
        
        std::cout << "Connected to WebSocket server: " << kHost << kTarget << std::endl;
        
        // Read loop
        while (shouldRun_ && isConnected_) {
            beast::flat_buffer buffer;
            
            // Read a message
            ws.read(buffer);
            
            // Update last message time
            { 
                std::lock_guard<std::mutex> lock(stateMutex_);
                lastMessageTime_ = std::chrono::steady_clock::now(); 
            }
            
            // Extract the message
            std::string message = beast::buffers_to_string(buffer.data());
            
            // Process the message
            processMessage(message); 
        }
        
        // Close the WebSocket connection
        ws.close(websocket::close_code::normal);
    }
    catch (const beast::system_error& se) {
        // Special handling for system errors
        if (se.code() != websocket::error::closed) {
            std::cerr << "WebSocket system error: " << se.code().message() << std::endl;
        }
        isConnected_ = false;
    }
    catch (const std::exception& e) {
        std::cerr << "WebSocket exception: " << e.what() << std::endl;
        isConnected_ = false;
    }
}

void WebSocketClient::processMessage(const std::string& message) {
    try {
        // Parse orderbook data from the message
        OrderbookData orderbookData = parseOrderbookData(message);
        
        // Call the callback with the processed data
        callback_(orderbookData);
    } 
    catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;
    }
}

OrderbookData WebSocketClient::parseOrderbookData(const std::string& jsonMessage) {
    auto startTime = std::chrono::steady_clock::now();
    
    // Parse JSON
    json data = json::parse(jsonMessage);
    
    // Create orderbook data
    OrderbookData orderbookData;
    orderbookData.received_time = startTime;
    
    // Basic validation of the data
    if (!data.contains("timestamp") || !data.contains("exchange") || 
        !data.contains("symbol") || !data.contains("asks") || !data.contains("bids")) {
        throw std::runtime_error("Missing required fields in data");
    }
    
    // Extract the fields
    orderbookData.timestamp = data["timestamp"].get<std::string>();
    orderbookData.exchange = data["exchange"].get<std::string>();
    orderbookData.symbol = data["symbol"].get<std::string>();
    
    // Process asks
    for (const auto& ask : data["asks"]) {
        if (ask.size() >= 2) {
            double price = std::stod(ask[0].get<std::string>());
            double size = std::stod(ask[1].get<std::string>());
            orderbookData.asks.emplace_back(price, size);
        }
    }
    
    // Process bids
    for (const auto& bid : data["bids"]) {
        if (bid.size() >= 2) {
            double price = std::stod(bid[0].get<std::string>());
            double size = std::stod(bid[1].get<std::string>());
            orderbookData.bids.emplace_back(price, size);
        }
    }
    
    return orderbookData;
}

void WebSocketClient::reconnect() {
    isConnected_ = false;
    
    std::cout << "Reconnecting in " << reconnectDelayMs_ / 1000.0 << " seconds..." << std::endl;
    
    // Wait for the reconnect delay
    std::this_thread::sleep_for(std::chrono::milliseconds(reconnectDelayMs_));
    
    // Increase reconnect delay with exponential backoff
    reconnectDelayMs_ = std::min(reconnectDelayMs_ * 2, kReconnectMaxDelayMs);
}

} // namespace data
} // namespace trade_simulator 