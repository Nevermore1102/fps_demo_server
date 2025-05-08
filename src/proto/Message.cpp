#include "proto/Message.h"
#include <cstring>

Message::Message(MessageType type) : type_(type) {}

PlayerUpdateMessage::PlayerUpdateMessage()
    : Message(MessageType::PLAYER_UPDATE) {
    health = 0.0f;
    ammo = 0;
    x = y = z = 0.0f;
    rotation = 0.0f;
}

bool PlayerUpdateMessage::serialize(std::vector<uint8_t>& buffer) const {
    // 计算消息大小
    size_t size = sizeof(MessageHeader) + 
                  sizeof(float) * 5 +  // health, x, y, z, rotation
                  sizeof(int);         // ammo
    
    buffer.resize(size);
    uint8_t* ptr = buffer.data();
    
    // 写入消息头
    MessageHeader header;
    header.type = type_;
    header.length = size - sizeof(MessageHeader);
    header.sequence = 0;  // TODO: 实现序列号生成
    
    memcpy(ptr, &header, sizeof(MessageHeader));
    ptr += sizeof(MessageHeader);
    
    // 写入消息体
    memcpy(ptr, &health, sizeof(float));
    ptr += sizeof(float);
    
    memcpy(ptr, &ammo, sizeof(int));
    ptr += sizeof(int);
    
    memcpy(ptr, &x, sizeof(float));
    ptr += sizeof(float);
    
    memcpy(ptr, &y, sizeof(float));
    ptr += sizeof(float);
    
    memcpy(ptr, &z, sizeof(float));
    ptr += sizeof(float);
    
    memcpy(ptr, &rotation, sizeof(float));
    
    return true;
}

bool PlayerUpdateMessage::deserialize(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(MessageHeader)) {
        return false;
    }
    
    const uint8_t* ptr = buffer.data();
    
    // 读取消息头
    MessageHeader header;
    memcpy(&header, ptr, sizeof(MessageHeader));
    ptr += sizeof(MessageHeader);
    
    if (header.type != type_ || 
        buffer.size() != sizeof(MessageHeader) + header.length) {
        return false;
    }
    
    // 读取消息体
    memcpy(&health, ptr, sizeof(float));
    ptr += sizeof(float);
    
    memcpy(&ammo, ptr, sizeof(int));
    ptr += sizeof(int);
    
    memcpy(&x, ptr, sizeof(float));
    ptr += sizeof(float);
    
    memcpy(&y, ptr, sizeof(float));
    ptr += sizeof(float);
    
    memcpy(&z, ptr, sizeof(float));
    ptr += sizeof(float);
    
    memcpy(&rotation, ptr, sizeof(float));
    
    return true;
} 