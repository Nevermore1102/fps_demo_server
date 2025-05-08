cmake_minimum_required(VERSION 3.10)
project(GameServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 查找必要的包
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVENT REQUIRED libevent)
pkg_check_modules(LUA REQUIRED lua5.3)

# 查找依赖包
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(spdlog REQUIRED)

# 查找 uuid
find_path(UUID_INCLUDE_DIR uuid/uuid.h
    /usr/include
    /usr/local/include
)

find_library(UUID_LIBRARY
    NAMES uuid
    PATHS
    /usr/lib
    /usr/local/lib
    /usr/lib/x86_64-linux-gnu
)

if(NOT UUID_INCLUDE_DIR OR NOT UUID_LIBRARY)
    message(FATAL_ERROR "uuid not found")
endif()

# 查找 MySQL
find_path(MYSQL_INCLUDE_DIR mysql/mysql.h
    /usr/include/mysql
    /usr/local/include/mysql
    /usr/include
    /usr/local/include
)

find_library(MYSQL_LIBRARIES
    NAMES mysqlclient
    PATHS
    /usr/lib
    /usr/local/lib
    /usr/lib/x86_64-linux-gnu
)

if(NOT MYSQL_INCLUDE_DIR OR NOT MYSQL_LIBRARIES)
    message(FATAL_ERROR "MySQL not found")
endif()

# 添加 libevent
find_library(EVENT_CORE_LIB event_core)
find_library(EVENT_EXTRA_LIB event_extra)
find_library(EVENT_PTHREADS_LIB event_pthreads)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${LIBEVENT_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIRS}
    /usr/include
    /usr/local/include
    ${OPENSSL_INCLUDE_DIR}
    ${MYSQL_INCLUDE_DIR}
    ${EVENT_INCLUDE_DIR}
    ${SPDLOG_INCLUDE_DIRS}
    ${UUID_INCLUDE_DIR}
)

# 自动查找所有源文件
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.h"
)

# 可执行文件
add_executable(game_server ${SOURCES})

# 链接库
target_link_libraries(game_server
    ${LIBEVENT_LIBRARIES}
    ${LUA_LIBRARIES}
    ${EVENT_CORE_LIB}
    ${EVENT_EXTRA_LIB}
    ${EVENT_PTHREADS_LIB}
    Threads::Threads
    spdlog::spdlog
    ${UUID_LIBRARY}
    ${MYSQL_LIBRARIES}
)

# 安装规则
install(TARGETS game_server DESTINATION bin)
install(DIRECTORY lua/ DESTINATION lua)
install(DIRECTORY save/ DESTINATION save)

# 添加测试目录
add_subdirectory(test) 