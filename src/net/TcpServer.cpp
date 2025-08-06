#include "TcpServer.h"
#include "ConnectionPool.h"
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <spdlog/spdlog.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

TcpServer::TcpServer(const std::string& host, uint16_t port)
    : base_(nullptr)
    , listener_(nullptr)
    , host_(host)
    , port_(port)
    , running_(false) {
}

TcpServer::~TcpServer() {
    stop();
}

bool TcpServer::start() {
    if (running_) {
        return true;
    }

    // 创建事件基础
    base_ = event_base_new();
    if (!base_) {
        LOG_ERROR("Failed to create event base");
        return false;
    }

    // 创建监听器
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port_);
    if (host_.empty() || host_ == "0.0.0.0") {
        sin.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, host_.c_str(), &sin.sin_addr);
    }

    listener_ = evconnlistener_new_bind(base_,
                                      acceptCallback,
                                      this,
                                      LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                                      -1,
                                      (struct sockaddr*)&sin,
                                      sizeof(sin));

    if (!listener_) {
        LOG_ERROR("Failed to create listener");
        event_base_free(base_);
        base_ = nullptr;
        return false;
    }

    evconnlistener_set_error_cb(listener_, acceptErrorCallback);

    // 启动事件循环
    running_ = true;
    LOG_INFO("Server started on {}:{}", host_, port_);
    event_base_dispatch(base_);

    return true;
}

void TcpServer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    if (listener_) {
        evconnlistener_free(listener_);
        listener_ = nullptr;
    }

    if (base_) {
        event_base_loopbreak(base_);
        event_base_free(base_);
        base_ = nullptr;
    }

    LOG_INFO("Server stopped");
}

void TcpServer::broadcast(const Message& msg) {
    auto connections = ConnectionPool::getInstance().getAllConnections();
    for (const auto& conn : connections) {
        conn->sendMessage(msg);
    }
}

// 设置保活机制的函数
int TcpServer::enable_keepalive(int socket_fd) {
    int keepalive = 1;
    int keepidle = 300;     // 5分钟后开始探测
    int keepintvl = 60;     // 探测间隔60秒
    int keepcnt = 3;        // 探测3次

    // 启用保活
    if (setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE, 
                   &keepalive, sizeof(keepalive)) < 0) {
        perror("setsockopt SO_KEEPALIVE");
        return -1;
    }

    // 设置保活参数
    if (setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPIDLE, 
                   &keepidle, sizeof(keepidle)) < 0) {
        perror("setsockopt TCP_KEEPIDLE");
        return -1;
    }

    if (setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, 
                   &keepintvl, sizeof(keepintvl)) < 0) {
        perror("setsockopt TCP_KEEPINTVL");
        return -1;
    }

    if (setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPCNT, 
                   &keepcnt, sizeof(keepcnt)) < 0) {
        perror("setsockopt TCP_KEEPCNT");
        return -1;
    }

    return 0;
}

void TcpServer::acceptCallback(struct evconnlistener* listener,
                             evutil_socket_t fd,
                             struct sockaddr* addr,
                             int socklen,
                             void* ctx) {
    auto server = static_cast<TcpServer*>(ctx);
    server->onAccept(fd, addr);
}

void TcpServer::acceptErrorCallback(struct evconnlistener* listener, void* ctx) {
    auto server = static_cast<TcpServer*>(ctx);
    server->onAcceptError();
}

void TcpServer::onAccept(evutil_socket_t fd, struct sockaddr* addr) {
    // 为新连接启用保活机制
    if (enable_keepalive(fd) < 0) {
        fprintf(stderr, "Failed to enable keepalive for fd %d\n", fd);
        evutil_closesocket(fd);
        return;
    }
    // 创建新的 bufferevent
    struct bufferevent* bev = bufferevent_socket_new(base_, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        LOG_ERROR("Failed to create bufferevent");
        return;
    }

    // 创建新的连接
    auto conn = std::make_shared<Connection>(bev);
    
    // 设置消息回调
    if (message_cb_) {
        conn->setMessageCallback(message_cb_);
    }

    // 设置关闭回调
    if (close_conn_cb_) {
        conn->setCloseCallback(close_conn_cb_);
    }
    else {
        conn->setCloseCallback([this](const std::shared_ptr<Connection>& conn) {
            LOG_INFO("Connection closed: {}", conn->getId());
            ConnectionPool::getInstance().removeConnection(conn->getId());
        });
    }

    // 添加到连接池
    ConnectionPool::getInstance().addConnection(conn);

    // 通知新连接
    if (new_conn_cb_) {
        new_conn_cb_(conn);
    }

    // 获取客户端地址
    char client_addr[INET_ADDRSTRLEN];
    struct sockaddr_in* client_sin = (struct sockaddr_in*)addr;
    inet_ntop(AF_INET, &client_sin->sin_addr, client_addr, sizeof(client_addr));
    LOG_INFO("New connection from {}:{}", client_addr, ntohs(client_sin->sin_port));
}

void TcpServer::onAcceptError() {
    int err = EVUTIL_SOCKET_ERROR();
    LOG_ERROR("Accept error: {}", evutil_socket_error_to_string(err));
} 