#include <event2/event.h>
#include <spdlog/spdlog.h>
#include "net/TcpServer.h"
#include "core/EventLoop.h"
#include "proto/Message.h"
#include "script/LuaVM.h"
#include "MessageProcessor.h"
#include "CppEngine.h"
#include <memory>

class GameServer {
public:
    GameServer() 
        : port_(8888)
        , tcp_server_("0.0.0.0", port_)
        , lua_vm_(std::make_shared<LuaVM>())
        , cpp_engine_(std::make_shared<CppEngine>()) {
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
        if (!lua_vm_->init()) {
            return false;
        }

        // 加载消息处理脚本
        if (!lua_vm_->loadScript("scripts/message_handlers.lua")) {
            return false;
        }

        // 注册Lua消息处理器
        lua_vm_->registerMessageHandler(MessageType::PLAYER_UPDATE, "handle_player_update");
        lua_vm_->registerMessageHandler(MessageType::PLAYER_SHOOT, "handle_player_shoot");
        lua_vm_->registerMessageHandler(MessageType::PLAYER_HIT, "handle_player_hit");

        return true;
    }

    bool initNetwork() {
        // 初始化消息处理器成员变量，确保共用同一个 LuaVM 和 CppEngine 实例
        message_processor_ = std::make_shared<MessageProcessor>(lua_vm_, cpp_engine_);

        // 设置消息回调，捕获 this
        tcp_server_.setMessageCallback(
            [this](const std::shared_ptr<Connection>& conn, const Message& msg) {
                spdlog::info("Received message from {}, type: {}", 
                            conn->getId(), static_cast<int>(msg.getType()));
                // 使用成员变量处理消息
                if (!message_processor_->processMessage(conn, msg)) {
                    spdlog::warn("Message not handled: {}", static_cast<int>(msg.getType()));
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
    std::shared_ptr<LuaVM> lua_vm_;
    std::shared_ptr<CppEngine> cpp_engine_;
    std::shared_ptr<MessageProcessor> message_processor_;
};
