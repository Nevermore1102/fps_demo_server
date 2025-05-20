#include "Message.h"
#include <cstring>
#include <spdlog/spdlog.h>
#include <arpa/inet.h> // for htonl/ntohl

// 最大消息大小限制（10MB）
constexpr size_t MAX_MESSAGE_SIZE = 10 * 1024 * 1024;

bool Message::serialize(std::vector<uint8_t>& out) const {
    // 检查消息体大小
    if (body_.size() > MAX_MESSAGE_SIZE) {
        spdlog::error("Message body too large: {} bytes (max: {})", body_.size(), MAX_MESSAGE_SIZE);
        return false;
    }

    // 先写4字节body长度（网络字节序）
    uint32_t net_len = htonl(static_cast<uint32_t>(body_.size()));
    out.resize(sizeof(net_len) + body_.size());
    std::memcpy(out.data(), &net_len, sizeof(net_len));
    
    if (!body_.empty()) {
        std::memcpy(out.data() + sizeof(net_len), body_.data(), body_.size());
    }
    
    // 解析protobuf消息以获取实际的消息类型
    NetworkMessage pb_msg;
    if (pb_msg.ParseFromArray(body_.data(), static_cast<int>(body_.size()))) {
        spdlog::debug("Serialized message: type={}, body_size={}, total_size={}", 
                     static_cast<int>(pb_msg.msg_id()), body_.size(), out.size());
    } else {
        spdlog::debug("Serialized message: body_size={}, total_size={}", 
                     body_.size(), out.size());
    }
    return true;
}

bool Message::deserialize(const std::vector<uint8_t>& in) {
    // 检查最小长度
    if (in.size() < sizeof(uint32_t)) {
        spdlog::error("Message too small for length header: {} bytes", in.size());
        return false;
    }

    // 读取并转换长度
    uint32_t net_len = 0;
    std::memcpy(&net_len, in.data(), sizeof(net_len));
    uint32_t body_len = ntohl(net_len);

    // 检查消息体大小是否合理
    if (body_len > MAX_MESSAGE_SIZE) {
        spdlog::error("Message body too large: {} bytes (max: {})", body_len, MAX_MESSAGE_SIZE);
        return false;
    }

    // 检查总长度是否匹配
    if (in.size() != sizeof(uint32_t) + body_len) {
        spdlog::error("Invalid message size: got {} bytes, expected {} bytes", 
                     in.size(), sizeof(uint32_t) + body_len);
        return false;
    }

    // 复制消息体
    body_.resize(body_len);
    if (body_len > 0) {
        std::memcpy(body_.data(), in.data() + sizeof(uint32_t), body_len);
    }

    // 尝试从protobuf消息中获取类型
    NetworkMessage pb_msg;
    if (pb_msg.ParseFromArray(body_.data(), static_cast<int>(body_len))) {
        msg_type_ = static_cast<MessageType>(pb_msg.msg_id());
        spdlog::debug("Deserialized message: type={}, body_size={}, total_size={}", 
                     static_cast<int>(msg_type_), body_len, in.size());
        return true;
    }

    spdlog::error("Failed to parse protobuf message");
    return false;
}

void Message::logMessage() const {
    spdlog::debug("消息详情:");
    spdlog::debug("  - 类型: {}", static_cast<int>(msg_type_));
    spdlog::debug("  - 大小: {}", body_.size());
    
    std::string hex_data;
    for (uint8_t b : body_) {
        hex_data += fmt::format("{:02x} ", b);
    }
    spdlog::debug("  - 原始数据: {}", hex_data);
}

//打印原始数据
void Message::printRawData() const {
    std::string hex_data;
    for (uint8_t b : body_) {
        hex_data += fmt::format("{:02x} ", b);
    }
    spdlog::info("原始数据: {}", hex_data);
}