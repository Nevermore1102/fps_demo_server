#include "script/LuaVM.h"
#include <stdexcept>

LuaVM::LuaVM() : L_(nullptr) {}

LuaVM::~LuaVM() {
    if (L_) {
        lua_close(L_);
        L_ = nullptr;
    }
}

bool LuaVM::init() {
    L_ = luaL_newstate();
    if (!L_) {
        return false;
    }
    
    luaL_openlibs(L_);
    registerBaseFunctions();
    return true;
}

bool LuaVM::loadScript(const std::string& filename) {
    if (!L_) {
        return false;
    }
    
    if (luaL_loadfile(L_, filename.c_str()) || lua_pcall(L_, 0, 0, 0)) {
        handleError("Failed to load script: " + filename);
        return false;
    }
    
    loadedScripts_[filename] = filename;
    return true;
}

bool LuaVM::reloadScript(const std::string& filename) {
    if (loadedScripts_.find(filename) == loadedScripts_.end()) {
        return false;
    }
    
    return loadScript(filename);
}

void LuaVM::registerFunction(const std::string& name, lua_CFunction func) {
    if (!L_) {
        return;
    }
    
    lua_pushcfunction(L_, func);
    lua_setglobal(L_, name.c_str());
}

bool LuaVM::callFunction(const std::string& funcName, const char* format, ...) {
    if (!L_) {
        return false;
    }
    
    lua_getglobal(L_, funcName.c_str());
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        return false;
    }
    
    va_list args;
    va_start(args, format);
    
    int nargs = 0;
    for (const char* ptr = format; *ptr; ++ptr) {
        switch (*ptr) {
            case 'i':
                lua_pushinteger(L_, va_arg(args, int));
                break;
            case 'n':
                lua_pushnumber(L_, va_arg(args, double));
                break;
            case 's':
                lua_pushstring(L_, va_arg(args, const char*));
                break;
            case 'b':
                lua_pushboolean(L_, va_arg(args, int));
                break;
            default:
                va_end(args);
                return false;
        }
        ++nargs;
    }
    
    va_end(args);
    
    if (lua_pcall(L_, nargs, 0, 0) != 0) {
        handleError("Failed to call function: " + funcName);
        return false;
    }
    
    return true;
}

void LuaVM::handleError(const std::string& msg) {
    if (L_) {
        const char* error = lua_tostring(L_, -1);
        if (error) {
            throw std::runtime_error(msg + "\n" + error);
        }
        lua_pop(L_, 1);
    }
}

void LuaVM::registerBaseFunctions() {
    // TODO: 注册基础函数
} 