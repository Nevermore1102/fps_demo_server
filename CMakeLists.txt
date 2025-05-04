cmake_minimum_required(VERSION 3.10)
project(game_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置Lua版本
set(LUA_VERSION 5.3)

# 查找依赖包
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVENT REQUIRED libevent)
pkg_check_modules(LUA REQUIRED lua${LUA_VERSION})

# 添加源文件
add_executable(game_server main.cpp)

# 包含目录
target_include_directories(game_server PRIVATE
    ${LIBEVENT_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(game_server PRIVATE
    ${LIBEVENT_LIBRARIES}
    ${LUA_LIBRARIES}
)

# 安装规则
install(TARGETS game_server DESTINATION bin)
install(DIRECTORY lua/ DESTINATION lua)
install(DIRECTORY save/ DESTINATION save) 