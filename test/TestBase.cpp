#include "TestBase.h"
#include <iostream>
#include <cstring>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

TestClientBase::TestClientBase(const std::string& host, uint16_t port)
    : host_(host), port_(port), base_(nullptr), bev_(nullptr), is_destroying_(false) {}

TestClientBase::~TestClientBase() {
    is_destroying_ = true;
    cleanup();
}

bool TestClientBase::connect() {
    // 创建事件基础
    base_ = event_base_new();
    if (!base_) {
        onError("Failed to create event base");
        return false;
    }

    // 创建 socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        onError("Failed to create socket");
        return false;
    }

    // 设置服务器地址
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port_);
    inet_pton(AF_INET, host_.c_str(), &sin.sin_addr);

    // 连接服务器
    if (::connect(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        onError("Failed to connect to server");
        return false;
    }

    // 创建 bufferevent
    bev_ = bufferevent_socket_new(base_, sock, BEV_OPT_CLOSE_ON_FREE);
    if (!bev_) {
        onError("Failed to create bufferevent");
        return false;
    }

    // 设置回调
    bufferevent_setcb(bev_, readCallback, nullptr, errorCallback, this);
    bufferevent_enable(bev_, BEV_EVENT_READING | BEV_EVENT_WRITING);

    onConnected();
    return true;
}

void TestClientBase::run() {
    if (base_) {
        event_base_dispatch(base_);
    }
}

void TestClientBase::runOnce() {
    if (base_) {
        event_base_loop(base_, EVLOOP_NONBLOCK);
    }
}

void TestClientBase::close() {
    if (bev_) {
        bufferevent_setcb(bev_, nullptr, nullptr, nullptr, nullptr);
        bufferevent_free(bev_);
        bev_ = nullptr;
    }
    if (!is_destroying_) {
        onDisconnected();
    }
}

void TestClientBase::cleanup() {
    close();
    if (base_) {
        event_base_free(base_);
        base_ = nullptr;
    }
}

void TestClientBase::sendMessage(const Message& msg) {
    if (!bev_) {
        onError("Cannot send message: bufferevent is null");
        return;
    }

    std::vector<uint8_t> data;
    if (!msg.serialize(data)) {
        onError("Failed to serialize message");
        return;
    }

    if (bufferevent_write(bev_, data.data(), data.size()) < 0) {
        onError("Failed to write to buffer");
    }
}

void TestClientBase::readCallback(struct bufferevent* bev, void* ctx) {
    auto client = static_cast<TestClientBase*>(ctx);
    if (client->is_destroying_) return;

    struct evbuffer* input = bufferevent_get_input(bev);
    size_t available = evbuffer_get_length(input);
    
    std::cout << "接收到数据，可用字节数: " << available << std::endl;
    
    if (available == 0) return;

    // 处理所有完整的消息
    while (available >= sizeof(uint32_t)) {
        // 先peek长度字段，不从缓冲区移除
        uint32_t net_len;
        if (evbuffer_copyout(input, &net_len, sizeof(net_len)) != sizeof(net_len)) {
            break;
        }
        
        // 转换为主机字节序
        uint32_t body_len = ntohl(net_len);
        
        std::cout << "消息体长度: " << body_len << std::endl;
        
        // 检查消息大小限制
        if (body_len > 10 * 1024 * 1024) {  // 10MB限制
            client->onError("Message body too large: " + std::to_string(body_len));
            return;
        }
        
        size_t total_msg_size = sizeof(uint32_t) + body_len;
        
        // 检查是否有完整的消息
        if (available < total_msg_size) {
            // 消息不完整，等待更多数据
            std::cout << "消息不完整，等待更多数据。需要: " << total_msg_size << ", 可用: " << available << std::endl;
            break;
        }

        // 读取完整的消息数据
        std::vector<uint8_t> message_data(total_msg_size);
        size_t actual_read = evbuffer_remove(input, message_data.data(), total_msg_size);
        
        if (actual_read != total_msg_size) {
            client->onError("Failed to read complete message");
            return;
        }

        std::cout << "成功读取完整消息，大小: " << total_msg_size << std::endl;

        // 反序列化消息
        Message msg;
        if (msg.deserialize(message_data)) {
            std::cout << "消息反序列化成功，类型: " << static_cast<int>(msg.getType()) << std::endl;
            client->onMessageReceived(msg);
        } else {
            client->onError("Failed to deserialize message");
            return;
        }

        // 更新可用数据长度
        available = evbuffer_get_length(input);
    }
}

void TestClientBase::errorCallback(struct bufferevent* bev, short events, void* ctx) {
    auto client = static_cast<TestClientBase*>(ctx);
    if (client->is_destroying_) return;

    if (events & BEV_EVENT_EOF) {
        client->onDisconnected();
    } else if (events & BEV_EVENT_ERROR) {
        client->onError("Connection error");
    }
    client->close();
} 