#include "Connection.h"
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/event.h>
#include <spdlog/spdlog.h>
#include <cstring>
#include <uuid/uuid.h>
#include <arpa/inet.h>  // for htonl/ntohl

// 最大消息大小限制（10MB）
constexpr size_t MAX_MESSAGE_SIZE = 10 * 1024 * 1024;

Connection::Connection(struct bufferevent* bev) 
    : bev_(bev), connected_(true) {
    // 生成唯一ID
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse_lower(uuid, uuid_str);
    id_ = uuid_str;

    // 设置回调
    bufferevent_setcb(bev_, readCallback, nullptr, errorCallback, this);
    bufferevent_enable(bev_, BEV_EVENT_READING | BEV_EVENT_WRITING);
}

Connection::~Connection() {
    close();
}

void Connection::close() {
    if (connected_) {
        connected_ = false;
        if (close_cb_) {
            // 在调用回调之前先保存一个智能指针，防止提前释放
            auto self = shared_from_this();
            close_cb_(self);
        }
        if (bev_) {
            bufferevent_free(bev_);
            bev_ = nullptr;
        }
    }
}

bool Connection::sendMessage(const Message& msg) {
    if (!connected_ || !bev_) {
        return false;
    }

    // 序列化消息
    std::vector<uint8_t> data;
    if (!msg.serialize(data)) {
        spdlog::error("Failed to serialize message");
        return false;
    }

    // 发送数据
    if (bufferevent_write(bev_, data.data(), data.size()) < 0) {
        spdlog::error("Failed to write to buffer");
        return false;
    }

    return true;
}

void Connection::onRead() {
    struct evbuffer* input = bufferevent_get_input(bev_);
    size_t len = evbuffer_get_length(input);
    
    if (len == 0) {
        return;
    }

    // 读取数据到缓冲区
    size_t old_size = read_buffer_.size();
    read_buffer_.resize(old_size + len);
    evbuffer_remove(input, read_buffer_.data() + old_size, len);

    // 处理消息
    while (read_buffer_.size() >= sizeof(uint32_t)) {  // 至少要有4字节长度字段
        // 读取4字节长度（网络字节序）
        uint32_t net_len = 0;
        std::memcpy(&net_len, read_buffer_.data(), sizeof(uint32_t));
        uint32_t body_len = ntohl(net_len);
        size_t total_size = sizeof(uint32_t) + body_len;

        if (body_len > MAX_MESSAGE_SIZE) {
            spdlog::error("Message body too large: {} bytes (max: {})", body_len, MAX_MESSAGE_SIZE);
            close();
            return;
        }

        if (read_buffer_.size() < total_size) {
            break;  // 等待更多数据
        }

        // 处理完整消息
        std::vector<uint8_t> message_data(read_buffer_.begin(), read_buffer_.begin() + total_size);
        handleMessage(message_data);

        // 移除已处理的数据
        read_buffer_.erase(read_buffer_.begin(), read_buffer_.begin() + total_size);
    }
}

void Connection::onError(short events) {
    if (events & BEV_EVENT_EOF) {
        spdlog::info("Connection closed by peer: {}", id_);
    } else if (events & BEV_EVENT_ERROR) {
        spdlog::error("Connection error: {}", id_);
    }
    close();
}

void Connection::handleMessage(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(uint32_t)) {
        spdlog::error("Message too small: {} bytes", data.size());
        return;
    }

    // 读取4字节长度（网络字节序）
    uint32_t net_len = 0;
    std::memcpy(&net_len, data.data(), sizeof(uint32_t));
    uint32_t body_len = ntohl(net_len);

    if (data.size() != sizeof(uint32_t) + body_len) {
        spdlog::error("Invalid message size: got {} bytes, expected {} bytes", 
                     data.size(), sizeof(uint32_t) + body_len);
        return;
    }

    // 创建消息对象并反序列化
    Message msg;
    if (!msg.deserialize(data)) {
        spdlog::error("Failed to deserialize message");
        return;
    }

    // 获取消息类型（确保使用正确的字节序）
    MessageType type = msg.getType();
    spdlog::debug("Received message type: {}", static_cast<uint32_t>(type));

    // 调用消息回调
    if (message_cb_) {
        auto self = shared_from_this();
        message_cb_(self, msg);
    }
}

void Connection::readCallback(struct bufferevent* bev, void* ctx) {
    auto conn = static_cast<Connection*>(ctx);
    conn->onRead();
}

void Connection::errorCallback(struct bufferevent* bev, short events, void* ctx) {
    auto conn = static_cast<Connection*>(ctx);
    conn->onError(events);
} 