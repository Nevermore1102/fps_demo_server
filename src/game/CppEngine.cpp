#include "CppEngine.h"
#include <spdlog/spdlog.h>
#include "proto/Message.h"
#include "proto/NetworkMessage.pb.h"

CppEngine::CppEngine() {
    // 可在此初始化需要的成员
}

bool CppEngine::handleMessage(const std::shared_ptr<Connection>& conn, const Message& msg) {
    switch (msg.getType()) {
        case MessageType::HEARTBEAT:
            onHeartbeat(conn, msg);
            break;
        // case MessageType::PLAYER_UPDATE:
        //     onPlayerUpdate(conn, msg);
        //     break;
        // case MessageType::PLAYER_ATTRIBUTE:
        //     onPlayerAttribute(conn, msg);
        //     break;
        // case MessageType::PLAYER_STATE:
        //     onPlayerState(conn, msg);
        //     break;
        // case MessageType::PLAYER_JOIN:
        //     onPlayerJoin(conn, msg);
        //     break;
        // case MessageType::PLAYER_LEAVE:
        //     onPlayerLeave(conn, msg);
        //     break;
        default:
            spdlog::warn("Unknown message type: {}", static_cast<uint32_t>(msg.getType()));
            return false;
    }
    return true;
}

void CppEngine::onHeartbeat(const std::shared_ptr<Connection>& conn, const Message& msg) {
    spdlog::debug("Received heartbeat from {}", conn->getId());
    
    msg.logMessage();

    NetworkMessage row_msg;
    // 尝试从消息体中解析protobuf对象
    if (!msg.getBodyAsProto(row_msg)) {
        spdlog::error("Failed to parse heartbeat message body");
        return;
    }
    // 打印解析后的NetworkMessage内容
    spdlog::info("P arsed NetworkMessage:");
    spdlog::info("  Message Type: {}", MessageType_Name(row_msg.msg_id()));
    spdlog::info("  Player ID: {}", row_msg.player_id());
    spdlog::info("  Timestamp: {}", row_msg.timestamp());
    spdlog::info("  data: {}", row_msg.heartbeat().data());


    // 发送心跳响应
    NetworkMessage pb_msg;
    pb_msg.set_msg_id(MessageType::HEARTBEAT);
    pb_msg.set_timestamp(static_cast<uint32_t>(time(nullptr)));
    
    // 创建心跳消息体
    HeartbeatMessage* heartbeat = pb_msg.mutable_heartbeat();
    heartbeat->set_data("服务端收到心跳包");


    Message response;
    response.setBodyFromProto(pb_msg);
    
    if (!conn->sendMessage(response)) {
        spdlog::error("Failed to send heartbeat response");
    }
     spdlog::info("发送心跳结束", conn->getId());
}

// void CppEngine::onPlayerUpdate(const std::shared_ptr<Connection>& conn, const Message& msg) {
//     NetworkMessage pb_msg;
//     if (!msg.getBodyAsProto(pb_msg)) {
//         spdlog::error("Failed to parse player update message");
//         return;
//     }
    
//     if (pb_msg.has_player_update()) {
//         const auto& update = pb_msg.player_update();
//         spdlog::debug("Player update: pos=({}, {}, {}), rot=({}, {}, {})", 
//                      update.position_x(), update.position_y(), update.position_z(),
//                      update.rotation_x(), update.rotation_y(), update.rotation_z());
//     }
// }

// void CppEngine::onPlayerAttribute(const std::shared_ptr<Connection>& conn, const Message& msg) {
//     NetworkMessage pb_msg;
//     if (!msg.getBodyAsProto(pb_msg)) {
//         spdlog::error("Failed to parse player attribute message");
//         return;
//     }
    
//     if (pb_msg.has_player_attribute()) {
//         const auto& attr = pb_msg.player_attribute();
//         spdlog::debug("Player attribute: health={}, armor={}", attr.health(), attr.armor());
//     }
// }

// void CppEngine::onPlayerState(const std::shared_ptr<Connection>& conn, const Message& msg) {
//     NetworkMessage pb_msg;
//     if (!msg.getBodyAsProto(pb_msg)) {
//         spdlog::error("Failed to parse player state message");
//         return;
//     }
    
//     if (pb_msg.has_player_state()) {
//         const auto& state = pb_msg.player_state();
//         spdlog::debug("Player state: alive={}, team={}", state.is_alive(), state.team_id());
//     }
// }

void CppEngine::onPlayerJoin(const std::shared_ptr<Connection>& conn, const Message& msg) {
    spdlog::info("Player joined: {}", conn->getId());
}

void CppEngine::onPlayerLeave(const std::shared_ptr<Connection>& conn, const Message& msg) {
    spdlog::info("Player left: {}", conn->getId());
} 