#pragma once

#include <cstdint>
#include <string>
#include <vector>

// 消息类型定义
enum class MessageType : uint16_t {
    // 系统消息
    HEARTBEAT = 0x0001,
    LOGIN = 0x0002,
    LOGOUT = 0x0003,
    
    // 游戏消息
    PLAYER_UPDATE = 0x0101,
    PLAYER_SHOOT = 0x0102,
    PLAYER_HIT = 0x0103,
    GAME_STATE = 0x0104,
};

// 消息头
struct MessageHeader {
    MessageType type;
    uint32_t length;
    uint32_t sequence;
};

// 基础消息类
class Message {
public:
    Message(MessageType type);
    virtual ~Message() = default;
    
    // 序列化/反序列化
    virtual bool serialize(std::vector<uint8_t>& buffer) const = 0;
    virtual bool deserialize(const std::vector<uint8_t>& buffer) = 0;
    
    MessageType getType() const { return type_; }
    
protected:
    MessageType type_;
};

// 玩家状态更新消息
class PlayerUpdateMessage : public Message {
public:
    PlayerUpdateMessage();
    
    bool serialize(std::vector<uint8_t>& buffer) const override;
    bool deserialize(const std::vector<uint8_t>& buffer) override;
    
    // 玩家状态
    float health;
    int ammo;
    float x, y, z;
    float rotation;
}; 