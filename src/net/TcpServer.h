/**
 * @file TcpServer.h
 * @brief TCP服务器的实现
 * 
 * 该模块负责：
 * - 监听和接受客户端连接
 * - 管理客户端连接的生命周期
 * - 处理数据的收发
 * - 提供回调机制处理连接事件
 * 
 * @author Nevermore1102
 * @date 2025-05-05
 */

#pragma once

#include <event2/listener.h>
#include <event2/event.h>
#include <string>
#include <memory>
#include <functional>
#include "Connection.h"

class TcpServer {
public:
    using MessageCallback = Connection::MessageCallback;
    using NewConnectionCallback = std::function<void(const std::shared_ptr<Connection>&)>;

    TcpServer(const std::string& host, uint16_t port);
    ~TcpServer();

    // 启动和停止服务器
    bool start();
    void stop();

    // 设置回调
    void setMessageCallback(MessageCallback cb) { message_cb_ = cb; }
    void setNewConnectionCallback(NewConnectionCallback cb) { new_conn_cb_ = cb; }

    // 广播消息给所有连接
    void broadcast(const Message& msg);

private:
    static void acceptCallback(struct evconnlistener* listener,
                             evutil_socket_t fd,
                             struct sockaddr* addr,
                             int socklen,
                             void* ctx);
    static void acceptErrorCallback(struct evconnlistener* listener, void* ctx);

    void onAccept(evutil_socket_t fd, struct sockaddr* addr);
    void onAcceptError();

    struct event_base* base_;
    struct evconnlistener* listener_;
    std::string host_;
    uint16_t port_;
    bool running_;

    MessageCallback message_cb_;
    NewConnectionCallback new_conn_cb_;
}; 