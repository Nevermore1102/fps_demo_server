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

class GameServer {
public:
    GameServer() 
        : port_(8888)
        , tcp_server_("0.0.0.0", port_) {
    }

    bool init() {
        // 初始化事件循环
        if (!event_loop_.init()) {
            spdlog::error("Failed to init event loop");
            return false;
        }

        // 初始化Lua环境
        if (!initLua()) {
            spdlog::error("Failed to init Lua environment");
            return false;
        }

        // 初始化网络
        if (!initNetwork()) {
            spdlog::error("Failed to init network");
            return false;
        }

        return true;
    }

    void run() {
        event_loop_.run();
    }

private:
    bool initLua() {
        if (!lua_vm_.init()) {
            return false;
        }

        // 加载消息处理脚本
        if (!lua_vm_.loadScript("scripts/message_handlers.lua")) {
            return false;
        }

        // 注册Lua消息处理器
        lua_vm_.registerMessageHandler(MessageType::PLAYER_UPDATE, "handle_player_update");
        lua_vm_.registerMessageHandler(MessageType::PLAYER_SHOOT, "handle_player_shoot");
        lua_vm_.registerMessageHandler(MessageType::PLAYER_HIT, "handle_player_hit");

        return true;
    }

    bool initNetwork() {
        // 设置消息回调
        tcp_server_.setMessageCallback(
            [this](const std::shared_ptr<Connection>& conn, const Message& msg) {
                spdlog::info("Received message from {}, type: {}", 
                            conn->getId(), static_cast<int>(msg.getType()));
                
                // 设置当前连接
                lua_vm_.setCurrentConnection(conn);
                
                // 尝试由Lua处理消息
                if (lua_vm_.handleMessage(msg)) {
                    spdlog::info("Message handled by Lua");
                    return;
                }
                
                // 如果Lua没有处理，则由C++处理
                switch (msg.getType()) {
                    case MessageType::HEARTBEAT:
                        // 回复心跳
                        spdlog::info("Sending heartbeat response to {}", conn->getId());
                        if (!conn->sendMessage(Message(MessageType::HEARTBEAT))) {
                            spdlog::error("Failed to send heartbeat response");
                        }
                        break;
                        
                    case MessageType::LOGIN:
                        // 回复登录成功
                        spdlog::info("Sending login response to {}", conn->getId());
                        if (!conn->sendMessage(Message(MessageType::LOGIN))) {
                            spdlog::error("Failed to send login response");
                        }
                        break;
                        
                    default:
                        spdlog::warn("Unknown message type: {}", static_cast<int>(msg.getType()));
                        break;
                }
            });

        // 设置新连接回调
        tcp_server_.setNewConnectionCallback(
            [](const std::shared_ptr<Connection>& conn) {
                spdlog::info("New connection: {}", conn->getId());
            });

        // 启动服务器
        return tcp_server_.start();
    }

    uint16_t port_;
    EventLoop event_loop_;
    TcpServer tcp_server_;
    LuaVM lua_vm_;
};

int main() {
    // 忽略 SIGPIPE 信号
    signal(SIGPIPE, SIG_IGN);

    // 初始化日志
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Server starting...");

    // 创建并运行服务器
    GameServer server;
    if (!server.init()) {
        spdlog::error("Failed to init server");
        return 1;
    }

    server.run();
    return 0;
} 