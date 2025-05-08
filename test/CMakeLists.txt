cmake_minimum_required(VERSION 3.10)
project(TestClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找必要的包
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVENT REQUIRED libevent)

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
)

# 添加可执行文件
add_executable(test_client ${SOURCES})

# 链接库
target_link_libraries(test_client
    event
    event_core
    event_extra
    pthread
) 