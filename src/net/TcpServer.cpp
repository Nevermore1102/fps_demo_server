#include "TcpServer.h"
#include <iostream>
#include <cstring>

TcpServer::TcpServer() : listener_(nullptr) {}

TcpServer::~TcpServer() {
    if (listener_) {
        evconnlistener_free(listener_);
    }
}

bool TcpServer::init(struct event_base* base, int port) {
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(0);
    sin.sin_port = htons(port);

    listener_ = evconnlistener_new_bind(
        base, on_accept, this,
        LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
        -1, (struct sockaddr*)&sin, sizeof(sin));

    if (!listener_) {
        std::cerr << "Failed to create listener" << std::endl;
        return false;
    }

    return true;
}

void TcpServer::on_accept(struct evconnlistener* listener, evutil_socket_t fd,
                         struct sockaddr* address, int socklen, void* ctx) {
    TcpServer* server = static_cast<TcpServer*>(ctx);
    struct event_base* base = evconnlistener_get_base(listener);
    struct bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    if (server->connection_cb_) {
        server->connection_cb_(bev);
    }

    bufferevent_setcb(bev, on_read, NULL, on_error, server);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
}

void TcpServer::on_read(struct bufferevent* bev, void* ctx) {
    TcpServer* server = static_cast<TcpServer*>(ctx);
    char buf[1024];
    size_t n = bufferevent_read(bev, buf, sizeof(buf) - 1);
    
    if (n > 0 && server->read_cb_) {
        server->read_cb_(bev, buf, n);
    }
}

void TcpServer::on_error(struct bufferevent* bev, short events, void* ctx) {
    TcpServer* server = static_cast<TcpServer*>(ctx);
    
    if (server->error_cb_) {
        server->error_cb_(bev, events);
    }
    
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_free(bev);
    }
} 