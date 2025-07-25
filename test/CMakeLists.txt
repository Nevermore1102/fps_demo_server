cmake_minimum_required(VERSION 3.10)
project(TestClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 查找protobuf包
find_package(Protobuf REQUIRED)

# 查找必要的包
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVENT REQUIRED libevent)
pkg_check_modules(LUA REQUIRED lua5.3)
find_package(fmt REQUIRED)

# 查找依赖包
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(spdlog REQUIRED)
find_package(SQLite3 REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/test
    ${LIBEVENT_INCLUDE_DIRS}
)

# 添加源文件
set(SOURCES
    client.cpp
    TestBase.cpp
    ${CMAKE_SOURCE_DIR}/src/proto/Message.cpp
    ${CMAKE_SOURCE_DIR}/src/proto/NetworkMessage.pb.cc
)
# # 添加源文件
# set(SOURCES
#     client.cpp
#     TestBase.cpp
#     ${CMAKE_SOURCE_DIR}/../src/proto/Message.cpp
#     ${CMAKE_SOURCE_DIR}/../src/proto/Message.h
#     ${CMAKE_SOURCE_DIR}/../src/proto/Message.pb.cc
#     ${CMAKE_SOURCE_DIR}/../src/proto/Message.pb.h
# )
# 自动查找所有源文件
# file(GLOB_RECURSE SOURCES 
#     "src/*.cpp"
#     "src/*.h"
# )

# 添加可执行文件
add_executable(test_client ${SOURCES})

# 链接库
target_link_libraries(test_client
    event
    event_core
    event_extra
    pthread
    fmt::fmt
    protobuf
) 