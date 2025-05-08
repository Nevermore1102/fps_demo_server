#pragma once

#include <cstdint>
#include <string>
#include <vector>

// 消息头结构
struct MessageHeader {
    uint32_t msg_id;      // 消息ID
    uint32_t body_size;   // 消息体大小
};

// 消息类型枚举
enum class MessageType : uint32_t {
    HEARTBEAT = 1,
    LOGIN = 2,
    LOGOUT = 3,
    PLAYER_UPDATE = 4,
    PLAYER_SHOOT = 5,
    PLAYER_HIT = 6,
    GAME_STATE = 7
};

// 基础消息类
class Message {
public:
    Message() = default;
    Message(MessageType type, const std::vector<uint8_t>& body = {})
        : header_{static_cast<uint32_t>(type), static_cast<uint32_t>(body.size())}
        , body_(body) {}

    // 序列化和反序列化
    bool serialize(std::vector<uint8_t>& out) const;
    bool deserialize(const std::vector<uint8_t>& in);

    // 获取消息信息
    MessageType getType() const { return static_cast<MessageType>(header_.msg_id); }
    const std::vector<uint8_t>& getBody() const { return body_; }
    size_t getSize() const { return sizeof(MessageHeader) + body_.size(); }

private:
    MessageHeader header_;
    std::vector<uint8_t> body_;
}; 