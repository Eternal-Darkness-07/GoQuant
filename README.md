# Trade Simulator

A high-performance trade simulator leveraging real-time market data to estimate transaction costs and market impact for cryptocurrency exchanges.

## Features

- Real-time L2 orderbook data processing
- Transaction cost analysis
- Market impact modeling (Almgren-Chriss)
- Slippage estimation
- Fee calculation
- Interactive UI for parameter configuration and result visualization

## Dependencies

- C++17 compatible compiler (GCC 8+, Clang 7+, or MSVC 19.14+)
- CMake 3.10+
- Boost 1.70+ (for Asio and Beast WebSocket)
- OpenSSL
- Qt 5.12+ (for UI components)
- nlohmann/json (for JSON parsing)

## Building

### Installing Dependencies

#### Ubuntu/Debian

```bash
# Update package list
sudo apt update

# Install build tools and libraries
sudo apt install build-essential cmake git
sudo apt install libboost-all-dev libssl-dev
sudo apt install qt5-default qtcharts5-dev
sudo apt install nlohmann-json3-dev
```

#### macOS (using Homebrew)

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake boost openssl qt@5 nlohmann-json
brew link qt@5
```

#### Windows (using vcpkg)

```bash
# Clone vcpkg if not already installed
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat

# Install dependencies
vcpkg install boost:x64-windows openssl:x64-windows qt5:x64-windows nlohmann-json:x64-windows
```

### Building the Project

```bash
# Clone the repository
git clone https://github.com/Eternal-Darkness-07/trade-simulator.git
cd trade-simulator

# Create a build directory
mkdir -p build
cd build

# Configure and build
cmake ..
make -j$(nproc)  # On Windows with MSVC, use: cmake --build . --config Release

# Install (optional)
sudo make install  # On Windows: cmake --build . --target install
```

## Usage

```bash
# Run the trade simulator
./trade_simulator
```

### VPN Requirements

To access OKX market data, you may need to use a VPN depending on your location. The simulator connects to a WebSocket endpoint that streams OKX market data.

## Architecture

- `include/data/`: Data handling and WebSocket client header files
- `include/models/`: Market impact and regression models header files
- `include/ui/`: User interface components header files
- `include/utils/`: Utility functions header files
- `src/data/`: Implementation files for data handling
- `src/models/`: Implementation files for models
- `src/ui/`: Implementation files for UI components
- `src/utils/`: Implementation files for utility functions

## User Interface

The simulator interface is divided into two main panels:

1. **Left Panel**: Input parameters
   - Exchange selection
   - Symbol selection
   - Order type (market)
   - Quantity (in USD)
   - Volatility setting
   - Fee tier selection

2. **Right Panel**: Simulation results
   - Expected slippage
   - Expected fees
   - Expected market impact
   - Net cost (total)
   - Maker/Taker proportion
   - Internal processing latency

## Performance

The simulator is optimized for high performance with the following characteristics:

- Fast processing of L2 orderbook updates
- Efficient implementation of transaction cost models
- Multi-threaded architecture for responsive UI
- Memory-efficient data structures for real-time processing

For detailed performance metrics and optimization techniques, see the [Performance Documentation](docs/performance.md).

## Documentation

See the `docs/` directory for more detailed documentation:

- [Models and Algorithms](docs/models.md): Details on the mathematical models used
- [Performance Optimization](docs/performance.md): Information on performance techniques
- [API Endpoints](docs/api_endpoints.md): WebSocket API endpoint documentation

## License

This project is licensed under the MIT License - see the LICENSE file for details. 
