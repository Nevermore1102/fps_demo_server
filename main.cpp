#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <lua.hpp>
#include <iostream>
#include <string>
#include <cstring>

// 全局Lua状态
lua_State* L = nullptr;

// 错误回调函数
void on_error(struct bufferevent* bev, short events, void* ctx) {
    if (events & BEV_EVENT_ERROR) {
        std::cerr << "Error from bufferevent" << std::endl;
    }
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_free(bev);
    }
}

// 读取回调函数
void on_read(struct bufferevent* bev, void* ctx) {
    char buf[1024];
    int n = bufferevent_read(bev, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        std::cout << "Received: " << buf << std::endl;
        // TODO: 处理接收到的数据
    }
}

// 回调函数：当有新的连接时
void on_accept(struct evconnlistener* listener, evutil_socket_t fd,
    struct sockaddr* address, int socklen, void* ctx) {
    std::cout << "New connection accepted" << std::endl;
    
    struct event_base* base = evconnlistener_get_base(listener);
    struct bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    
    // 设置回调函数
    bufferevent_setcb(bev, on_read, NULL, on_error, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
}

// 初始化Lua环境
bool init_lua() {
    L = luaL_newstate();
    if (!L) {
        std::cerr << "Failed to create Lua state" << std::endl;
        return false;
    }
    
    luaL_openlibs(L);
    
    // 加载并执行Lua脚本
    if (luaL_dofile(L, "lua/logic.lua") != 0) {
        std::cerr << "Failed to load Lua script: " << lua_tostring(L, -1) << std::endl;
        lua_close(L);
        return false;
    }
    
    return true;
}

int main() {
    // 初始化Lua
    if (!init_lua()) {
        return 1;
    }
    
    // 创建event base
    struct event_base* base = event_base_new();
    if (!base) {
        std::cerr << "Failed to create event base" << std::endl;
        return 1;
    }
    
    // 创建监听器
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(0);
    sin.sin_port = htons(8888);
    
    struct evconnlistener* listener = evconnlistener_new_bind(
        base, on_accept, NULL,
        LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
        -1, (struct sockaddr*)&sin, sizeof(sin));
    
    if (!listener) {
        std::cerr << "Failed to create listener" << std::endl;
        return 1;
    }
    
    std::cout << "Server started on port 8888" << std::endl;
    
    // 进入事件循环
    event_base_dispatch(base);
    
    // 清理
    evconnlistener_free(listener);
    event_base_free(base);
    lua_close(L);
    
    return 0;
} 