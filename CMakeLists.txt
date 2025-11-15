cmake_minimum_required(VERSION 3.10)

project(nfd-parser-cpp)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify build directory (modern CMake supports this)
# The build directory is typically specified when running cmake, e.g.: cmake -B build
# But we can set a default output directory for binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find required packages
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# Include directories
include_directories(
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${JSONCPP_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/ce.cpp
    src/cow_tool.cpp
    src/http_client_curl.cpp
    src/letool.cpp
    src/lztool.cpp
    src/yetool.cpp
)

# Create executables for each source file (each has its own main)
add_executable(ce src/ce.cpp)
add_executable(cow_tool src/cow_tool.cpp)
add_executable(http_client_curl src/http_client_curl.cpp)
add_executable(letool src/letool.cpp)
add_executable(lztool src/lztool.cpp)
add_executable(yetool src/yetool.cpp)

# Link libraries for each executable
# Using the required flags: -lcurl -ljsoncpp -lssl -lcrypto
foreach(target ce cow_tool http_client_curl letool lztool yetool)
    target_link_libraries(${target}
        ${CURL_LIBRARIES}
        ${JSONCPP_LIBRARIES}
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endforeach()
