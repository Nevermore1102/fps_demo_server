cmake_minimum_required(VERSION 3.10)
project(GameServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 查找protobuf包
find_package(Protobuf REQUIRED)

# 定义是否运行测试的选项
option(RUN_TESTS "Run tests during startup" OFF)

# 查找必要的包
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVENT REQUIRED libevent)
pkg_check_modules(LUA REQUIRED lua5.3)

# 查找依赖包
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(spdlog REQUIRED)
find_package(SQLite3 REQUIRED)

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

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${LIBEVENT_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIRS}
    /usr/include
    /usr/local/include
    ${OPENSSL_INCLUDE_DIR}
    ${SPDLOG_INCLUDE_DIRS}
    ${UUID_INCLUDE_DIR}
    ${SQLite3_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
)

# 自动查找所有源文件
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.h"
)

# 可执行文件
add_executable(game_server ${SOURCES})

# 添加测试定义
if(RUN_TESTS)
    target_compile_definitions(game_server PRIVATE RUN_TESTS=1)
else()
    target_compile_definitions(game_server PRIVATE RUN_TESTS=0)
endif()

# 链接库
target_link_libraries(game_server
    ${LIBEVENT_LIBRARIES}
    ${LUA_LIBRARIES}
    Threads::Threads
    spdlog::spdlog
    ${UUID_LIBRARY}
    ${SQLite3_LIBRARIES}
    ${Protobuf_LIBRARIES}
)

# 生成protobuf文件
set(PROTO_GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/proto)
set(PROTO_FILES src/proto/NetworkMessage.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
target_sources(game_server PRIVATE ${PROTO_SRCS} ${PROTO_HDRS})

# 设置protobuf生成文件的输出目录
set_target_properties(game_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

# 设置protobuf生成文件的输出目录
set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS}
    PROPERTIES
    GENERATED 1
    HEADER_FILE_ONLY 0
    GENERATED_LOCATION ${PROTO_GEN_DIR}
)

# 安装规则
install(TARGETS game_server DESTINATION bin)
install(DIRECTORY lua/ DESTINATION lua)
install(DIRECTORY save/ DESTINATION save)

# 添加测试目录
# add_subdirectory(test)

# 设置输出目录
set_target_properties(game_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
) 