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
    size_t len = evbuffer_get_length(input);
    
    if (len == 0) return;

    // 读取数据
    std::vector<uint8_t> data(len);
    evbuffer_remove(input, data.data(), len);

    // 处理所有完整的消息
    size_t offset = 0;
    while (offset + sizeof(MessageHeader) <= data.size()) {
        const MessageHeader* header = reinterpret_cast<const MessageHeader*>(data.data() + offset);
        size_t total_size = sizeof(MessageHeader) + header->body_size;

        if (offset + total_size > data.size()) {
            // 消息不完整，等待更多数据
            break;
        }

        // 处理单条消息
        std::vector<uint8_t> message_data(data.begin() + offset, data.begin() + offset + total_size);
        Message msg;
        if (msg.deserialize(message_data)) {
            client->onMessageReceived(msg);
        } else {
            client->onError("Failed to deserialize message at offset " + std::to_string(offset));
        }

        offset += total_size;
    }

    // 如果有未处理完的数据，放回缓冲区
    if (offset < data.size()) {
        std::vector<uint8_t> remaining(data.begin() + offset, data.end());
        evbuffer_prepend(input, remaining.data(), remaining.size());
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