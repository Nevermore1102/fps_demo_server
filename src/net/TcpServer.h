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
#include <event2/bufferevent.h>
#include <memory>
#include <string>
#include <functional>

class TcpServer {
public:
    using ConnectionCallback = std::function<void(struct bufferevent*)>;
    using ReadCallback = std::function<void(struct bufferevent*, const char*, size_t)>;
    using ErrorCallback = std::function<void(struct bufferevent*, short)>;

    TcpServer();
    ~TcpServer();

    bool init(struct event_base* base, int port);
    void setConnectionCallback(ConnectionCallback cb) { connection_cb_ = cb; }
    void setReadCallback(ReadCallback cb) { read_cb_ = cb; }
    void setErrorCallback(ErrorCallback cb) { error_cb_ = cb; }

private:
    static void on_accept(struct evconnlistener* listener, evutil_socket_t fd,
                         struct sockaddr* address, int socklen, void* ctx);
    static void on_read(struct bufferevent* bev, void* ctx);
    static void on_error(struct bufferevent* bev, short events, void* ctx);

    struct evconnlistener* listener_;
    ConnectionCallback connection_cb_;
    ReadCallback read_cb_;
    ErrorCallback error_cb_;
}; 