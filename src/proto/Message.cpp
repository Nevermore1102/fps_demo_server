#include "Message.h"
#include <cstring>
#include <spdlog/spdlog.h>

bool Message::serialize(std::vector<uint8_t>& out) const {
    // 计算总大小
    size_t total_size = sizeof(MessageHeader) + body_.size();
    out.resize(total_size);

    // 复制消息头
    std::memcpy(out.data(), &header_, sizeof(MessageHeader));

    // 复制消息体
    if (!body_.empty()) {
        std::memcpy(out.data() + sizeof(MessageHeader), body_.data(), body_.size());
    }

    spdlog::debug("Serialized message: type={}, body_size={}, total_size={}", 
                  header_.msg_id, header_.body_size, total_size);
    return true;
}

bool Message::deserialize(const std::vector<uint8_t>& in) {
    // 检查数据大小
    if (in.size() < sizeof(MessageHeader)) {
        spdlog::error("Message too small: {} bytes", in.size());
        return false;
    }

    // 复制消息头
    std::memcpy(&header_, in.data(), sizeof(MessageHeader));

    // 检查消息体大小
    if (in.size() != sizeof(MessageHeader) + header_.body_size) {
        spdlog::error("Invalid message size: got {} bytes, expected {} bytes", 
                     in.size(), sizeof(MessageHeader) + header_.body_size);
        return false;
    }

    // 复制消息体
    if (header_.body_size > 0) {
        body_.resize(header_.body_size);
        std::memcpy(body_.data(), in.data() + sizeof(MessageHeader), header_.body_size);
    } else {
        body_.clear();
    }

    spdlog::debug("Deserialized message: type={}, body_size={}, total_size={}", 
                  header_.msg_id, header_.body_size, in.size());
    return true;
}