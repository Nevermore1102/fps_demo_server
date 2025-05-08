cmake_minimum_required(VERSION 3.10)
project(GameServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 查找必要的包
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVENT REQUIRED libevent)
pkg_check_modules(LUA REQUIRED lua5.3)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${LIBEVENT_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIRS}
    /usr/include
    /usr/local/include
)

# 源文件
set(SOURCES
    src/main.cpp
    src/core/EventLoop.cpp
    src/net/TcpServer.cpp
    src/script/LuaVM.cpp
)

# 可执行文件
add_executable(game_server ${SOURCES})

# 链接库
target_link_libraries(game_server
    ${LIBEVENT_LIBRARIES}
    ${LUA_LIBRARIES}
)

# 安装规则
install(TARGETS game_server DESTINATION bin)
install(DIRECTORY lua/ DESTINATION lua)
install(DIRECTORY save/ DESTINATION save) 