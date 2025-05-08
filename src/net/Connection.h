#pragma once

#include <event2/bufferevent.h>
#include <string>
#include <memory>
#include <functional>
#include "proto/Message.h"

class Connection : public std::enable_shared_from_this<Connection> {
public:
    using MessageCallback = std::function<void(const std::shared_ptr<Connection>&, const Message&)>;
    using CloseCallback = std::function<void(const std::shared_ptr<Connection>&)>;

    Connection(struct bufferevent* bev);
    ~Connection();

    // 连接管理
    void setMessageCallback(MessageCallback cb) { message_cb_ = cb; }
    void setCloseCallback(CloseCallback cb) { close_cb_ = cb; }
    void close();
    
    // 消息发送
    bool sendMessage(const Message& msg);
    
    // 连接信息
    const std::string& getId() const { return id_; }
    bool isConnected() const { return connected_; }
    struct bufferevent* getBev() const { return bev_; }

private:
    void onRead();
    void onError(short events);
    void handleMessage(const std::vector<uint8_t>& data);
    
    static void readCallback(struct bufferevent* bev, void* ctx);
    static void errorCallback(struct bufferevent* bev, short events, void* ctx);

    struct bufferevent* bev_;
    std::string id_;
    bool connected_;
    MessageCallback message_cb_;
    CloseCallback close_cb_;
    
    // 消息缓冲区
    std::vector<uint8_t> read_buffer_;
    static constexpr size_t MAX_MESSAGE_SIZE = 1024 * 1024;  // 1MB
}; 