#include "script/LuaVM.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

// 添加send_response函数的C++实现
static int lua_send_response(lua_State* L) {
    // 获取参数
    int msgType = luaL_checkinteger(L, 1);
    size_t len = 0;
    const char* body = lua_tolstring(L, 2, &len);
    
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
    std::vector<uint8_t> bodyData(body, body + len);
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
    
    // 注册PlayerData类
    registerPlayerDataClass();
    
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

// PlayerData Lua绑定函数实现
int LuaVM::lua_playerdata_new(lua_State* L) {
    const char* player_id = luaL_checkstring(L, 1);
    PlayerData* data = new PlayerData(player_id);
    
    // 创建用户数据
    PlayerData** ud = static_cast<PlayerData**>(lua_newuserdata(L, sizeof(PlayerData*)));
    *ud = data;
    
    // 设置元表
    luaL_getmetatable(L, "PlayerData");
    lua_setmetatable(L, -2);
    
    return 1;
}

int LuaVM::lua_playerdata_update_position(lua_State* L) {
    //注意此处为PlayerData* data = *static_cast<PlayerData**>
    //保障lua和c++侧的PlayerData* 指向同一个对象
    PlayerData* data = *static_cast<PlayerData**>(luaL_checkudata(L, 1, "PlayerData"));
    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);
    float z = luaL_checknumber(L, 4);
    data->updatePosition(x, y, z);
    return 0;
}

int LuaVM::lua_playerdata_update_rotation(lua_State* L) {
    PlayerData* data = *static_cast<PlayerData**>(luaL_checkudata(L, 1, "PlayerData"));
    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);
    float z = luaL_checknumber(L, 4);
    data->updateRotation(x, y, z);
    return 0;
}

int LuaVM::lua_playerdata_update_velocity(lua_State* L) {
    PlayerData* data = *static_cast<PlayerData**>(luaL_checkudata(L, 1, "PlayerData"));
    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);
    float z = luaL_checknumber(L, 4);
    data->updateVelocity(x, y, z);
    return 0;
}

int LuaVM::lua_playerdata_update_is_grounded(lua_State* L) {
    PlayerData* data = *static_cast<PlayerData**>(luaL_checkudata(L, 1, "PlayerData"));
    bool is_grounded = lua_toboolean(L, 2);
    data->updateIsGrounded(is_grounded);
    return 0;
}

int LuaVM::lua_playerdata_update_health(lua_State* L) {
    PlayerData* data = *static_cast<PlayerData**>(luaL_checkudata(L, 1, "PlayerData"));
    float health = luaL_checknumber(L, 2);
    data->updateHealth(health);
    return 0;
}

// 添加load方法的绑定
int LuaVM::lua_playerdata_load(lua_State* L) {
    PlayerData* data = *static_cast<PlayerData**>(luaL_checkudata(L, 1, "PlayerData"));
    bool success = data->load();
    lua_pushboolean(L, success);
    return 1;
}

// 添加save方法的绑定
int LuaVM::lua_playerdata_save(lua_State* L) {
    PlayerData* data = *static_cast<PlayerData**>(luaL_checkudata(L, 1, "PlayerData"));
    bool success = data->save();
    lua_pushboolean(L, success);
    return 1;
}

// 添加getState方法的绑定
int LuaVM::lua_playerdata_get_state(lua_State* L) {
    PlayerData* data = *static_cast<PlayerData**>(luaL_checkudata(L, 1, "PlayerData"));
    const PlayerState& state = data->getState();
    
    // 创建一个新的表来存储状态数据
    lua_newtable(L);
    
    // 添加所有状态字段
    lua_pushnumber(L, state.health);
    lua_setfield(L, -2, "health");
    
    lua_pushnumber(L, state.ammo);
    lua_setfield(L, -2, "ammo");
    
    lua_pushnumber(L, state.x);
    lua_setfield(L, -2, "x");
    
    lua_pushnumber(L, state.y);
    lua_setfield(L, -2, "y");
    
    lua_pushnumber(L, state.z);
    lua_setfield(L, -2, "z");
    
    lua_pushnumber(L, state.rotation_x);
    lua_setfield(L, -2, "rotation_x");
    
    lua_pushnumber(L, state.rotation_y);
    lua_setfield(L, -2, "rotation_y");
    
    lua_pushnumber(L, state.rotation_z);
    lua_setfield(L, -2, "rotation_z");
    
    lua_pushnumber(L, state.velocity_x);
    lua_setfield(L, -2, "velocity_x");
    
    lua_pushnumber(L, state.velocity_y);
    lua_setfield(L, -2, "velocity_y");
    
    lua_pushnumber(L, state.velocity_z);
    lua_setfield(L, -2, "velocity_z");
    
    lua_pushboolean(L, state.is_grounded);
    lua_setfield(L, -2, "is_grounded");
    
    return 1;
}

void LuaVM::registerPlayerDataClass() {
    // 创建PlayerData元表
    luaL_newmetatable(L_, "PlayerData");
    
    // 设置元表的__index为自身
    lua_pushvalue(L_, -1);
    lua_setfield(L_, -2, "__index");
    
    // 注册方法
    lua_pushcfunction(L_, lua_playerdata_update_position);
    lua_setfield(L_, -2, "update_position");
    
    lua_pushcfunction(L_, lua_playerdata_update_rotation);
    lua_setfield(L_, -2, "update_rotation");
    
    lua_pushcfunction(L_, lua_playerdata_update_velocity);
    lua_setfield(L_, -2, "update_velocity");
    
    lua_pushcfunction(L_, lua_playerdata_update_is_grounded);
    lua_setfield(L_, -2, "update_is_grounded");
    
    lua_pushcfunction(L_, lua_playerdata_update_health);
    lua_setfield(L_, -2, "update_health");
    
    // 添加load和save方法
    lua_pushcfunction(L_, lua_playerdata_load);
    lua_setfield(L_, -2, "load");
    
    lua_pushcfunction(L_, lua_playerdata_save);
    lua_setfield(L_, -2, "save");
    
    // 添加getState方法
    lua_pushcfunction(L_, lua_playerdata_get_state);
    lua_setfield(L_, -2, "getState");
    
    // 创建PlayerData表
    lua_newtable(L_);
    
    // 注册构造函数
    lua_pushcfunction(L_, lua_playerdata_new);
    lua_setfield(L_, -2, "new");
    
    // 设置元表
    lua_pushvalue(L_, -2);
    lua_setmetatable(L_, -2);
    
    // 将PlayerData表设置为全局变量
    lua_setglobal(L_, "PlayerData");
    
    // 弹出元表
    lua_pop(L_, 1);
}

bool LuaVM::registerPlayerData(const std::string& player_id) {
    // 确保PlayerData类已注册
    registerPlayerDataClass();
    
    // 创建新的PlayerData实例
    lua_getglobal(L_, "PlayerData");
    if (!lua_istable(L_, -1)) {
        spdlog::error("Failed to get PlayerData table");
        return false;
    }
    
    // 调用构造函数
    lua_pushstring(L_, player_id.c_str());
    if (lua_pcall(L_, 1, 1, 0) != 0) {
        spdlog::error("Failed to create PlayerData instance: {}", lua_tostring(L_, -1));
        return false;
    }
    
    return true;
}