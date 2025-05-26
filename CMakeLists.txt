cmake_minimum_required(VERSION 3.10)
project(TradeSimulator VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Set compiler flags
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Boost is needed for asio and beast (WebSocket)
find_package(Boost REQUIRED COMPONENTS system)

# Qt for UI
find_package(Qt5 COMPONENTS Widgets Charts REQUIRED)

# Add include directories
include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${Boost_INCLUDE_DIRS}
)

# Qt settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

# Define executable
add_executable(trade_simulator 
  ${SOURCES}
  ${HEADERS}
)

# Link libraries
target_link_libraries(trade_simulator
  PRIVATE
  ${Boost_LIBRARIES}
  OpenSSL::SSL
  OpenSSL::Crypto
  Threads::Threads
  Qt5::Widgets
  Qt5::Charts
)

# Installation
install(TARGETS trade_simulator
  RUNTIME DESTINATION bin
)

# Packaging
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "trade-simulator")
set(CPACK_PACKAGE_VENDOR "GoQuant")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Trade Simulator - High performance market data simulator")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
include(CPack) 