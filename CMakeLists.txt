cmake_minimum_required(VERSION 3.6)

project(nfd-parser-cpp)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add src directory to include path
include_directories(${CMAKE_SOURCE_DIR}/src)

# Build each cpp file as a separate executable
add_executable(ce src/ce.cpp)
add_executable(cow_tool src/cow_tool.cpp)
add_executable(http_client_curl src/http_client_curl.cpp)
add_executable(letool src/letool.cpp)
add_executable(lztool src/lztool.cpp)
add_executable(yetool src/yetool.cpp)
