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

// #include <iostream>
#include <signal.h>
#include <event2/event.h>

// 

#include <spdlog/spdlog.h>
// #include "net/TcpServer.h"
// #include "core/EventLoop.h"
// #include "proto/Message.h"
// #include "script/LuaVM.h"
#include "game/GameServer.h"
// #include "test/TestStorage.h"
#include "log/log_macro.h"
// 定义是否运行测试的宏
// #define RUN_TESTS 1


void test_play() {
        Room room; 

    // 创建4个玩家并加入房间
    std::vector<std::shared_ptr<Player>> players;
    std::vector<std::string> playerIds = {"p0", "p1", "p2", "p3"};
    for (int i = 0; i < 4; ++i) {
        auto player = std::make_shared<Player>(playerIds[i], nullptr);
        room.addPlayer(player);
        players.push_back(player);
    }

    // 测试回合0~8的匹配序列
    std::cout << "=== Match Sequence for 4 Players ===\n";
    for(int round = 0; round <= 8; ++round){
        room.nextRound(); // 假定每调用一次就是下一回合（可根据实际定义调整）
        std::cout << "Round " << round << ":\n";
        for(const auto& player : players){
            int enemyIdx = room.getCurrentEnemyIdx(player->GetPlayerId());
            std::string enemyId = (enemyIdx >= 0 && enemyIdx < players.size()) ? players[enemyIdx]->GetPlayerId() : "None";
            std::cout << "  Player " << player->GetPlayerId() << " vs " << enemyId << '\n';
        }
    }
    // return 0;
}


int main() {
    // 忽略 SIGPIPE 信号
    signal(SIGPIPE, SIG_IGN);

    // 初始化日志

    // spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [tid:%t] %v");  
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [tid:%t] [%s:%#] %v");
    spdlog::set_level(spdlog::level::debug);
    // LOG_INFO("Server starting...");
    LOG_INFO("Server starting...");

    // test_play();

// #if RUN_TESTS
//     // 运行测试
//     if (!test::TestStorage::runAllTests()) {
//         LOG_ERROR("存储模块测试失败");
//         return -1;
//     }
//     LOG_INFO("所有测试通过");
// #endif

    // 创建并运行服务器
    GameServer server;
    if (!server.init()) {
        LOG_ERROR("Failed to init server");
        return 1;
    }

    server.run();
    return 0;
} 