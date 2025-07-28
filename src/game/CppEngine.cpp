#include "CppEngine.h"
#include <google/protobuf/message.h>
#include <spdlog/spdlog.h>
#include "proto/Message.h"
#include "proto/NetworkMessage.pb.h"
#include "Room/RoomManager.h"

CppEngine::CppEngine() {
    // 可在此初始化需要的成员
}

bool CppEngine::handleMessage(const std::shared_ptr<Connection>& conn, const Message& msg) {
    switch (msg.getType()) {
        case MessageType::HEARTBEAT:
            onHeartbeat(conn, msg);
            break;
        case MessageType::CONNECT:
            onConnect(conn, msg);
            break;
        case MessageType::START_MATCH:
            onStartMatch(conn, msg);
            break;
        case MessageType::BATTLE_PREP_SNAPSHOT:
            onPrepSnapshot(conn, msg);
            break;
        case MessageType::BATTLE_RESULT:
            onBattleResult(conn, msg);
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
    // spdlog::info("  Timestamp: {}", row_msg.timestamp());
    // spdlog::info("  data: {}", row_msg.heartbeat().data());


    // 发送心跳响应
    NetworkMessage pb_msg;
    pb_msg.set_msg_id(MessageType::HEARTBEAT);
    // pb_msg.set_timestamp(static_cast<uint32_t>(time(nullptr)));
    
    // 创建心跳消息体
    HeartbeatMessage* heartbeat = pb_msg.mutable_heartbeat();
    // heartbeat->set_data("服务端收到心跳包");


    Message response;
    response.setBodyFromProto(pb_msg);
    
    if (!conn->sendMessage(response)) {
        spdlog::error("Failed to send heartbeat response");
    }
     spdlog::info("发送心跳结束", conn->getId());
}

void CppEngine::onConnect(const std::shared_ptr<Connection>& conn, const Message& msg) {
    spdlog::info("Player connected: {}", conn->getId());
}

void CppEngine::onStartMatch(const std::shared_ptr<Connection>& conn, const Message& msg) {
    spdlog::info("Player Start Match: {}", conn->getId());
    
    // 解析消息获取玩家ID
    NetworkMessage pb_msg;
    if (!msg.getBodyAsProto(pb_msg)) {
        spdlog::error("Failed to parse start match message body");
        return;
    }
    
    std::string playerId = pb_msg.player_id();
    if (playerId.empty()) {
        // 如果消息中没有玩家ID，使用连接ID作为玩家ID
        // playerId = conn->getId();
        spdlog::error("Player ID is empty");
        return ;
    }
    
    spdlog::info("Player {} requesting to start match", playerId);
    
    // 创建新的Player对象
    auto player = std::make_shared<Player>(playerId, conn);
    player->setState(PlayerState::CONNECTED);
    
    // 获取RoomManager实例并加入匹配队列
    auto& roomManager = RoomManager::getInstance();
    roomManager.joinWaitRoom(player);
    
    spdlog::info("Player {} added to match queue", playerId);
    
    // 检查是否有房间可以开始游戏
    auto playerRoom = roomManager.getPlayerRoom(playerId);
    if (playerRoom && playerRoom->isFull()) {
        spdlog::info("Room {} is full, attempting to start game", playerRoom->getId());
        
        // 尝试开始游戏
        if (roomManager.startGameInRoom(std::to_string(playerRoom->getId()))) {
            spdlog::info("Game successfully started in room {}", playerRoom->getId());
        } else {
            spdlog::warn("Failed to start game in room {}", playerRoom->getId());
        }
    } else {
        if (playerRoom) {
            spdlog::info("Player {} assigned to room {}, waiting for more players ({}/{})", 
                        playerId, playerRoom->getId(), 
                        playerRoom->getPlayerCount(), MAX_PLAYERS);
        } else {
            spdlog::warn("Player {} not assigned to any room", playerId);
        }
    }
}

void CppEngine::onPrepSnapshot(const std::shared_ptr<Connection>& conn, const Message& msg) {
    spdlog::info("Received BATTLE_PREP_SNAPSHOT from connection: {}", conn->getId());
    
    // 解析消息
    NetworkMessage pb_msg;
    if (!msg.getBodyAsProto(pb_msg)) {
        spdlog::error("Failed to parse battle prep snapshot message body");
        return;
    }
    
    // 检查是否有快照数据
    if (!pb_msg.has_battle_prep_snapshot()) {
        spdlog::error("Message does not contain battle prep snapshot data");
        return;
    }
    
    const auto& snapshot = pb_msg.battle_prep_snapshot();
    
    // 提取消息数据
    std::string playerId = pb_msg.player_id();
    std::string matchId = snapshot.match_id();
    int32_t round = snapshot.round();
    std::string formationData = snapshot.formation_data();
    int32_t honorValue = snapshot.honor_value();
    
    spdlog::info("Processing snapshot - Player: {}, Match: {}, Round: {}, Honor: {}", 
                playerId, matchId, round, honorValue);
    
    // 验证数据完整性
    if (playerId.empty() || matchId.empty()) {
        spdlog::error("Invalid snapshot data: player_id or match_id is empty");
        return;
    }
    
    // 获取房间管理器并找到对应房间
    auto& roomManager = RoomManager::getInstance();
    auto room = roomManager.getRoom(matchId);
    
    if (!room) {
        spdlog::error("Room {} not found for snapshot", matchId);
        return;
    }
    
    // 验证玩家是否在房间中
    auto player = room->getPlayer(playerId);
    if (!player) {
        spdlog::error("Player {} not found in room {}", playerId, matchId);
        return;
    }
    
    // 记录玩家快照
    if (!room->recordPlayerSnapshot(playerId, formationData, honorValue, round)) {
        spdlog::error("Failed to record snapshot for player {} in room {}", playerId, matchId);
        return;
    }
    
    spdlog::info("Successfully recorded snapshot for player {} in room {}", playerId, matchId);
    
    // 检查是否所有玩家都已提交快照
    if (room->allSnapshotsReceived()) {
        spdlog::info("All snapshots received for room {}, broadcasting ALL_SNAPSHOTS", matchId);
        room->broadcastAllSnapshots();
        
        // 清理快照为下一轮准备
        room->clearSnapshots();
    } else {
        size_t receivedCount = 0;
        for (const auto& roomPlayer : room->getPlayers()) {
            // 这里需要一个方法来检查特定玩家是否已提交快照
            // 暂时用玩家数量和已收到的快照数量来判断
        }
        spdlog::info("Waiting for more snapshots in room {} (received: {}/{})", 
                    matchId, receivedCount, room->getPlayerCount());
    }
}

void CppEngine::onBattleResult(const std::shared_ptr<Connection>& conn, const Message& msg) {
    spdlog::info("Player Battle Result: {}", conn->getId());
    // // 解析消息获取玩家ID
    // NetworkMessage pb_msg;
    // if (!msg.getBodyAsProto(pb_msg)) {
    //     spdlog::error("Failed to parse battle result message body");
    //     return;
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

// void CppEngine::onPlayerJoin(const std::shared_ptr<Connection>& conn, const Message& msg) {
//     spdlog::info("Player joined: {}", conn->getId());
// }

// void CppEngine::onPlayerLeave(const std::shared_ptr<Connection>& conn, const Message& msg) {
//     spdlog::info("Player left: {}", conn->getId());
// } 