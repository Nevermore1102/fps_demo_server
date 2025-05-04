#include "LuaVM.h"
#include <iostream>

LuaVM::LuaVM() : L_(nullptr) {}

LuaVM::~LuaVM() {
    close();
}

bool LuaVM::init() {
    L_ = luaL_newstate();
    if (!L_) {
        std::cerr << "Failed to create Lua state" << std::endl;
        return false;
    }
    
    luaL_openlibs(L_);
    return true;
}

bool LuaVM::loadScript(const std::string& path) {
    if (!L_) {
        std::cerr << "Lua state not initialized" << std::endl;
        return false;
    }

    if (luaL_dofile(L_, path.c_str()) != 0) {
        std::cerr << "Failed to load Lua script: " << lua_tostring(L_, -1) << std::endl;
        return false;
    }

    return true;
}

void LuaVM::close() {
    if (L_) {
        lua_close(L_);
        L_ = nullptr;
    }
} 