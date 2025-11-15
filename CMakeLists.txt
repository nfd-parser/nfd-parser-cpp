cmake_minimum_required(VERSION 3.6)

project(nfd-parser-cpp)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加源码目录的包含路径
include_directories(${CMAKE_SOURCE_DIR}/src)

# 指定输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 链接所需的库
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

# target 链接的全部库 (等同于上面的参数)
set(LIBRARIES
    m
    dl
    log
    z
    curl
    jsoncpp
    ssl
    crypto
)

# 通用编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfatal-errors")

# 为可执行文件构建目标
add_executable(ce src/ce.cpp)
add_executable(cow_tool src/cow_tool.cpp)
add_executable(http_client_curl src/http_client_curl.cpp)
add_executable(letool src/letool.cpp)
add_executable(lztool src/lztool.cpp)
add_executable(yetool src/yetool.cpp)

# 链接依赖库
target_link_libraries(ce ${LIBRARIES})
target_link_libraries(cow_tool ${LIBRARIES})
target_link_libraries(http_client_curl ${LIBRARIES})
target_link_libraries(letool ${LIBRARIES})
target_link_libraries(lztool ${LIBRARIES})
target_link_libraries(yetool ${LIBRARIES})