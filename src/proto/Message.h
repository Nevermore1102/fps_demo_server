#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <google/protobuf/message.h>
#include "NetworkMessage.pb.h"

// 基础消息类
class Message {
public:
    Message() = default;
    Message(MessageType type, const std::vector<uint8_t>& body = {})
        : msg_type_(type)
        , body_(body) {}

    // 序列化和反序列化
    bool serialize(std::vector<uint8_t>& out) const;
    bool deserialize(const std::vector<uint8_t>& in);

    // 获取消息信息
    MessageType getType() const { return msg_type_; }
    const std::vector<uint8_t>& getBody() const { return body_; }
    size_t getSize() const { return sizeof(uint32_t) + body_.size(); }

    // 打印消息详情
    void logMessage() const;

    // 用protobuf对象设置body
    template<typename ProtoMsg>
    void setBodyFromProto(const ProtoMsg& proto) {
        body_.resize(proto.ByteSizeLong());
        proto.SerializeToArray(body_.data(), static_cast<int>(body_.size()));
    }

    // 获取body为protobuf对象
    template<typename ProtoMsg>
    bool getBodyAsProto(ProtoMsg& proto) const {
        return proto.ParseFromArray(body_.data(), static_cast<int>(body_.size()));
    }

    // 打印原始数据
    void printRawData() const;

private:
    MessageType msg_type_;
    std::vector<uint8_t> body_;
}; 