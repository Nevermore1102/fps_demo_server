/**
 * @file main.cpp
 * @brief 游戏服务器主程序入口
 * 
 * 该文件包含：
 * - GameServer类的实现，用于整合各个模块
 * - 服务器的初始化和启动逻辑
 * - 基本的错误处理和资源管理
 * 
 * @author Nevermore1102
 * @date 2025-05-05
 */

#include <iostream>
#include <signal.h>
#include <event2/event.h>
#include <spdlog/spdlog.h>
#include "net/TcpServer.h"
#include "core/EventLoop.h"
#include "proto/Message.h"
#include "script/LuaVM.h"
#include "game/GameServer.h"
#include "test/TestStorage.h"

// 定义是否运行测试的宏
#define RUN_TESTS 1

int main() {
    // 忽略 SIGPIPE 信号
    signal(SIGPIPE, SIG_IGN);

    // 初始化日志
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Server starting...");

#if RUN_TESTS
    // 运行测试
    if (!test::TestStorage::runAllTests()) {
        spdlog::error("存储模块测试失败");
        return -1;
    }
    spdlog::info("所有测试通过");
#endif

    // 创建并运行服务器
    GameServer server;
    if (!server.init()) {
        spdlog::error("Failed to init server");
        return 1;
    }

    server.run();
    return 0;
} 