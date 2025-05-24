/**
 * @file LuaVM.h
 * @brief Lua虚拟机管理器
 * 
 * 该模块负责：
 * - 初始化和管理Lua环境
 * - 加载和执行Lua脚本
 * - 提供C++和Lua的交互接口
 * - 管理Lua状态和资源
 * 
 * @author Nevermore1102
 * @date 2025-05-05
 */

#pragma once

#include <lua.hpp>
#include <string>
#include <memory>
#include <unordered_map>
#include "proto/Message.h"
#include "net/Connection.h"
#include "data/PlayerData.h"

// 前向声明
static int lua_send_response(lua_State* L);

class LuaVM {
    friend int lua_send_response(lua_State* L);
    
public:
    LuaVM();
    ~LuaVM();
    
    bool init();
    bool loadScript(const std::string& filename);
    bool reloadScript(const std::string& filename);
    
    // 注册 C++ 函数到 Lua
    void registerFunction(const std::string& name, lua_CFunction func);
    
    // 调用 Lua 函数
    bool callFunction(const std::string& funcName, const char* format, ...);
    
    // 获取 Lua 状态
    lua_State* getState() { return L_; }

    // 消息处理相关方法
    bool handleMessage(const Message& msg);
    bool registerMessageHandler(MessageType type, const std::string& luaFuncName);
    bool unregisterMessageHandler(MessageType type);


    // 设置当前连接
    void setCurrentConnection(const std::shared_ptr<Connection>& conn) { current_connection_ = conn; }
    
private:
    lua_State* L_;
    std::unordered_map<std::string, std::string> loadedScripts_;
    std::unordered_map<MessageType, std::string> message_handlers_;
    std::shared_ptr<Connection> current_connection_;
    
    // 错误处理
    void handleError(const std::string& msg);
    
    // 注册基础函数
    void registerBaseFunctions();

    // 消息处理辅助方法
    bool pushMessageToLua(const Message& msg);
    bool getMessageFromLua(Message& msg);

    // PlayerData Lua绑定函数
    static int lua_playerdata_new(lua_State* L);
    static int lua_playerdata_update_position(lua_State* L);
    static int lua_playerdata_update_rotation(lua_State* L);
    static int lua_playerdata_update_velocity(lua_State* L);
    static int lua_playerdata_update_is_grounded(lua_State* L);
    static int lua_playerdata_update_health(lua_State* L);
    static int lua_playerdata_load(lua_State* L);
    static int lua_playerdata_save(lua_State* L);
    
    // 注册PlayerData类到Lua
    void registerPlayerDataClass();
    bool registerPlayerData(const std::string& player_id);
}; 