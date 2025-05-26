# WebSocket API Endpoints

This document provides information about the WebSocket API endpoints used by the Trade Simulator.

## OKX API

The simulator connects to the OKX exchange's public WebSocket API for market data.

### Endpoint

The Trade Simulator uses the following specialized endpoint for L2 orderbook data:

```
wss://ws.gomarket-cpp.goquant.io/ws/l2-orderbook/okx/BTC-USDT-SWAP
```

This endpoint provides a simplified interface to access OKX orderbook data without requiring authentication.

### Message Format

The WebSocket server returns messages in the following JSON format:

```json
{
  "timestamp": "2025-05-04T10:39:13Z",
  "exchange": "OKX",
  "symbol": "BTC-USDT-SWAP",
  "asks": [
    ["95445.5", "9.06"],
    ["95448", "2.05"],
    ...
  ],
  "bids": [
    ["95445.4", "1104.23"],
    ["95445.3", "0.02"],
    ...
  ]
}
```

where:
- `timestamp`: ISO 8601 timestamp of the orderbook snapshot
- `exchange`: Name of the exchange (always "OKX")
- `symbol`: Trading pair
- `asks`: List of [price, size] pairs for sell orders, sorted by price ascending
- `bids`: List of [price, size] pairs for buy orders, sorted by price descending

### Connection Management

The simulator handles the WebSocket connection with the following features:

1. **Automatic Reconnection**: The client automatically attempts to reconnect if the connection is lost
2. **Exponential Backoff**: Reconnection attempts use exponential backoff to avoid overwhelming the server
3. **Health Monitoring**: The connection is monitored for health and reconnected if no messages are received within a timeout period

## Direct OKX API Information

If you need to connect directly to OKX's API, you can use the following information:

### Official WebSocket Endpoint

```
wss://ws.okx.com:8443/ws/v5/public
```

### Subscription Message

To subscribe to the orderbook data directly on OKX, send:

```json
{
  "op": "subscribe",
  "args": [
    {
      "channel": "books",
      "instId": "BTC-USDT-SWAP"
    }
  ]
}
```

### Authentication

For private API endpoints, OKX requires:
1. API Key
2. Passphrase
3. Timestamp
4. Signature

The signature is generated using:
```
base64(hmac-sha256(timestamp + method + requestPath + body, secretKey))
```

### Rate Limits

OKX imposes the following rate limits:
- Maximum 30 requests per second per IP
- Maximum 3 connections per IP

### Documentation

For detailed information about the OKX API, refer to their official documentation:
[OKX API Documentation](https://www.okx.com/docs-v5/en/)

## VPN Requirements

As mentioned in the project requirements, you need to use a VPN to access OKX services from certain regions. The simulator is designed to work with a VPN connection when necessary. 