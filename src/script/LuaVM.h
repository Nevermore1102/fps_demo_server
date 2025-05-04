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

class LuaVM {
public:
    LuaVM();
    ~LuaVM();

    bool init();
    bool loadScript(const std::string& path);
    void close();

    lua_State* getState() const { return L_; }

private:
    lua_State* L_;
}; 