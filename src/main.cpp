/**
 * @file main.cpp
 * @brief 游戏服务器主程序入口
 * 
 * 该文件包含：
 * - GameServer类的实现，用于整合各个模块
 * - 服务器的初始化和启动逻辑
 * - 基本的错误处理和资源管理
 * 
 * @author Nevermore1102
 * @date 2025-05-05
 */

#include "core/EventLoop.h"
#include "net/TcpServer.h"
#include "script/LuaVM.h"
#include <iostream>
#include <string>

class GameServer {
public:
    GameServer() : port_(8888) {}
    
    bool init() {
        if (!event_loop_.init()) {
            return false;
        }
        
        if (!lua_vm_.init()) {
            return false;
        }
        
        if (!loadLuaScripts()) {
            return false;
        }
        
        if (!initNetwork()) {
            return false;
        }
        
        return true;
    }
    
    void run() {
        event_loop_.run();
    }
    
private:
    bool loadLuaScripts() {
        return lua_vm_.loadScript("scripts/core/init.lua");
    }
    
    bool initNetwork() {
        tcp_server_.setConnectionCallback([](struct bufferevent* bev) {
            std::cout << "New connection accepted" << std::endl;
        });
        
        tcp_server_.setReadCallback([](struct bufferevent* bev, const char* data, size_t len) {
            std::cout << "Received: " << std::string(data, len) << std::endl;
        });
        
        tcp_server_.setErrorCallback([](struct bufferevent* bev, short events) {
            if (events & BEV_EVENT_ERROR) {
                std::cerr << "Error from bufferevent" << std::endl;
            }
        });
        
        return tcp_server_.init(event_loop_.getBase(), port_);
    }
    
    EventLoop event_loop_;
    TcpServer tcp_server_;
    LuaVM lua_vm_;
    int port_;
};

int main() {
    GameServer server;
    
    if (!server.init()) {
        std::cerr << "Failed to initialize server" << std::endl;
        return 1;
    }
    
    std::cout << "Server started on port 8888" << std::endl;
    server.run();
    
    return 0;
} 