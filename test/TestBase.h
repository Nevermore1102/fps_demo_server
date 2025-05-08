#pragma once
#include <string>
#include <memory>
#include "proto/Message.h"

// 测试基类
class TestBase {
public:
    TestBase(const std::string& name) : name_(name) {}
    virtual ~TestBase() = default;

    // 获取测试名称
    const std::string& getName() const { return name_; }

    // 运行测试
    virtual bool run() = 0;

    // 发送消息的接口
    virtual void sendMessage(const Message& msg) = 0;

protected:
    std::string name_;
};

// 测试客户端基类
class TestClientBase {
public:
    TestClientBase(const std::string& host, uint16_t port);
    virtual ~TestClientBase();

    // 连接服务器
    bool connect();
    
    // 运行事件循环
    void run();
    
    // 运行一次事件循环
    void runOnce();
    
    // 关闭连接
    void close();
    
    // 清理资源
    void cleanup();
    
    // 发送消息
    void sendMessage(const Message& msg);

protected:
    // 消息处理回调
    virtual void onMessageReceived(const Message& msg) = 0;
    
    // 连接状态回调
    virtual void onConnected() = 0;
    virtual void onDisconnected() = 0;
    virtual void onError(const std::string& error) = 0;

private:
    static void readCallback(struct bufferevent* bev, void* ctx);
    static void errorCallback(struct bufferevent* bev, short events, void* ctx);

    std::string host_;
    uint16_t port_;
    struct event_base* base_;
    struct bufferevent* bev_;
    bool is_destroying_;
}; 