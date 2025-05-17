#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include "script/LuaVM.h"
#include "proto/Message.h"

/**
 * @brief Lua引擎类
 * 
 * 负责管理所有Lua相关的功能，包括：
 * - Lua脚本的加载和管理
 * - 消息处理器的注册和管理
 * - Lua与C++的交互
 */
class LuaEngine {
public:
    LuaEngine() : lua_vm_(std::make_shared<LuaVM>()) {}

    bool init() {
        if (!lua_vm_->init()) {
            return false;
        }
        return true;
    }

    /**
     * @brief 加载Lua脚本
     * @param script_path 脚本路径
     */
    bool loadScript(const std::string& script_path) {
        return lua_vm_->loadScript(script_path);
    }

    /**
     * @brief 注册消息处理器
     * @param msg_type 消息类型
     * @param handler_name Lua处理函数名
     */
    void registerMessageHandler(MessageType msg_type, const std::string& handler_name) {
        lua_vm_->registerMessageHandler(msg_type, handler_name);
    }

    /**
     * @brief 注册所有默认的消息处理器
     */
    void registerDefaultHandlers() {
        // 玩家相关处理器
        registerMessageHandler(MessageType::PLAYER_UPDATE, "handle_player_update");
        registerMessageHandler(MessageType::PLAYER_SHOOT, "handle_player_shoot");
        registerMessageHandler(MessageType::PLAYER_HIT, "handle_player_hit");
        

    }

    /**
     * @brief 获取LuaVM实例
     */
    std::shared_ptr<LuaVM> getLuaVM() const {
        return lua_vm_;
    }

private:
    std::shared_ptr<LuaVM> lua_vm_;
}; 