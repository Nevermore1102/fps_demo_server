#pragma once
#include "script/LuaVM.h"
#include "CppEngine.h"
#include <memory>
#include <spdlog/spdlog.h>

class MessageProcessor {
public:
    MessageProcessor(std::shared_ptr<LuaVM> lua_vm_, std::shared_ptr<CppEngine> cpp_engine_)
        : lua_vm(lua_vm_), cpp_engine(cpp_engine_) {}

    bool processMessage(const std::shared_ptr<Connection>& conn, const Message& msg) {
        if (!conn) {
            spdlog::error("Connection is null");
            return false;
        }

        // 设置当前连接
        lua_vm->setCurrentConnection(conn);

        // 尝试由Lua处理消息
        if (lua_vm->handleMessage(msg)) {
            spdlog::info("Message handled by Lua");
            return true;
        }

        // 如果Lua没有处理，则由C++处理
        if (cpp_engine) {
            cpp_engine->handleMessage(conn, msg);
            return true;
        }

        spdlog::warn("Unknown message type: {}", static_cast<int>(msg.getType()));
        return false;
    }

private:
    std::shared_ptr<LuaVM> lua_vm;
    std::shared_ptr<CppEngine> cpp_engine;
}; 