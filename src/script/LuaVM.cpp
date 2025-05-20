#include "script/LuaVM.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

// 添加send_response函数的C++实现
static int lua_send_response(lua_State* L) {
    // 获取参数
    int msgType = luaL_checkinteger(L, 1);
    const char* body = luaL_checkstring(L, 2);
    
    // 获取LuaVM实例
    lua_getglobal(L, "LUA_VM");
    LuaVM* vm = static_cast<LuaVM*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    
    if (!vm || !vm->current_connection_) {
        spdlog::error("Failed to send response: no valid connection");
        lua_pushboolean(L, false);
        return 1;
    }
    
    // 创建响应消息
    std::vector<uint8_t> bodyData(body, body + strlen(body));
    Message response(static_cast<MessageType>(msgType), bodyData);
    
    // 发送响应
    bool success = vm->current_connection_->sendMessage(response);
    if (!success) {
        spdlog::error("Failed to send response message");
    }
    
    lua_pushboolean(L, success);
    return 1;
}

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
    
    // 注册LUA_VM全局变量
    lua_pushlightuserdata(L_, this);
    lua_setglobal(L_, "LUA_VM");
    
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
    
    // 获取函数
    lua_getglobal(L_, funcName.c_str());
    if (!lua_isfunction(L_, -1)) {
        spdlog::error("Function {} is not found or not a function", funcName);
        lua_pop(L_, 1);
        return false;
    }
    
    // 移动函数到栈底
    lua_insert(L_, 1);
    
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
            case 't':  // table已经在栈中，不需要额外操作
                break;
            default:
                va_end(args);
                return false;
        }
        ++nargs;
    }
    
    va_end(args);
    
    // 调用函数
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
    // 注册send_response函数
    registerFunction("send_response", lua_send_response);
    
    // 注册MessageType枚举
    lua_newtable(L_);
    
    // 注册消息类型枚举
    lua_pushinteger(L_, static_cast<int>(MessageType::HEARTBEAT));
    lua_setfield(L_, -2, "HEARTBEAT");
    
    lua_pushinteger(L_, static_cast<int>(MessageType::PLAYER_UPDATE));
    lua_setfield(L_, -2, "PLAYER_UPDATE");
    
    lua_pushinteger(L_, static_cast<int>(MessageType::PLAYER_ATTRIBUTE));
    lua_setfield(L_, -2, "PLAYER_ATTRIBUTE");
    
    lua_pushinteger(L_, static_cast<int>(MessageType::PLAYER_STATE));
    lua_setfield(L_, -2, "PLAYER_STATE");
    
    lua_pushinteger(L_, static_cast<int>(MessageType::PLAYER_JOIN));
    lua_setfield(L_, -2, "PLAYER_JOIN");
    
    lua_pushinteger(L_, static_cast<int>(MessageType::PLAYER_LEAVE));
    lua_setfield(L_, -2, "PLAYER_LEAVE");
    
    // 将MessageType表设置为全局变量
    lua_setglobal(L_, "MessageType");
}

bool LuaVM::handleMessage(const Message& msg) {
    auto it = message_handlers_.find(msg.getType());
    if (it == message_handlers_.end()) {
        return false;
    }

    const std::string& handlerName = it->second;
    
    // 将消息压入Lua栈
    if (!pushMessageToLua(msg)) {
        return false;
    }
    
    // 调用Lua处理函数
    bool result = callFunction(handlerName.c_str(), "t");
    
    return result;
}

bool LuaVM::registerMessageHandler(MessageType type, const std::string& luaFuncName) {
    message_handlers_[type] = luaFuncName;
    spdlog::info("Registered Lua message handler for type {}: {}", 
                 static_cast<int>(type), luaFuncName);
    return true;
}

bool LuaVM::unregisterMessageHandler(MessageType type) {
    auto it = message_handlers_.find(type);
    if (it != message_handlers_.end()) {
        message_handlers_.erase(it);
        spdlog::info("Unregistered Lua message handler for type {}", 
                     static_cast<int>(type));
        return true;
    }
    return false;
}

bool LuaVM::pushMessageToLua(const Message& msg) {
    lua_newtable(L_);
    
    // 压入消息类型
    lua_pushstring(L_, "type");
    lua_pushinteger(L_, static_cast<int>(msg.getType()));
    lua_settable(L_, -3);
    
    // 压入消息体
    lua_pushstring(L_, "body");
    lua_pushlstring(L_, reinterpret_cast<const char*>(msg.getBody().data()), 
                    msg.getBody().size());
    lua_settable(L_, -3);
    
    return true;
}

bool LuaVM::getMessageFromLua(Message& msg) {
    if (!lua_istable(L_, -1)) {
        return false;
    }
    
    // 获取消息类型
    lua_pushstring(L_, "type");
    lua_gettable(L_, -2);
    if (!lua_isinteger(L_, -1)) {
        lua_pop(L_, 1);
        return false;
    }
    MessageType type = static_cast<MessageType>(lua_tointeger(L_, -1));
    lua_pop(L_, 1);
    
    // 获取消息体
    lua_pushstring(L_, "body");
    lua_gettable(L_, -2);
    if (!lua_isstring(L_, -1)) {
        lua_pop(L_, 1);
        return false;
    }
    size_t len;
    const char* body = lua_tolstring(L_, -1, &len);
    std::vector<uint8_t> bodyData(body, body + len);
    lua_pop(L_, 1);
    
    msg = Message(type, bodyData);
    return true;
} 