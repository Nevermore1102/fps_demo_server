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

class LuaVM {
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
    
private:
    lua_State* L_;
    std::unordered_map<std::string, std::string> loadedScripts_;
    
    // 错误处理
    void handleError(const std::string& msg);
    
    // 注册基础函数
    void registerBaseFunctions();
}; 