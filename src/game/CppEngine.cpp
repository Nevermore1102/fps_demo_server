#include "CppEngine.h"
#include <cstdint>
#include <google/protobuf/message.h>
#include <spdlog/spdlog.h>
#include <string>
#include "game/Player/Player.h"
#include "game/Room/Room.h"
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
        // 开始匹配
        case MessageType::START_MATCH:
            onStartMatch(conn, msg);
            break;
        // 数据加载完成
        case MessageType::DATA_LOADED:
            onDataLoaded(conn, msg);
            break;
        // 备战快照
        case MessageType::BATTLE_PREP_SNAPSHOT:
            onPrepSnapshot(conn, msg);
            break;
        // 战斗结果
        case MessageType::BATTLE_RESULT:
            onBattleResult(conn, msg);
            break;
        // 主动退出
        case MessageType::EXIT:
            onExit(conn,msg);
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
            LOG_WARN("Unknown message type: {}", static_cast<uint32_t>(msg.getType()));
            return false;
    }
    return true;
}

void CppEngine::onHeartbeat(const std::shared_ptr<Connection>& conn, const Message& msg) {
    LOG_DEBUG("Received heartbeat from {}", conn->getId());
    
    // msg.logMessage();

    NetworkMessage row_msg;
    // 尝试从消息体中解析protobuf对象
    if (!msg.getBodyAsProto(row_msg)) {
        LOG_ERROR("Failed to parse heartbeat message body");
        return;
    }
    // 打印解析后的NetworkMessage内容
    LOG_INFO("P arsed NetworkMessage:");
    LOG_INFO("  Message Type: {}", MessageType_Name(row_msg.msg_id()));
    LOG_INFO("  Player ID: {}", row_msg.player_id());
    // LOG_INFO("  Timestamp: {}", row_msg.timestamp());
    // LOG_INFO("  data: {}", row_msg.heartbeat().data());


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
        LOG_ERROR("Failed to send heartbeat response");
    }
     LOG_INFO("发送心跳结束", conn->getId());
}

void CppEngine::onConnect(const std::shared_ptr<Connection>& conn, const Message& msg) {
    LOG_INFO("Player connected: {}", conn->getId());
}

void CppEngine::onStartMatch(const std::shared_ptr<Connection>& conn, const Message& msg) {
    LOG_INFO("Player Start Match: {}", conn->getId());
    
    // 解析消息获取玩家ID
    NetworkMessage pb_msg;
    if (!msg.getBodyAsProto(pb_msg)) {
        LOG_ERROR("1 Failed to parse start match message body");
        return;
    }

    if (pb_msg.msg_id() != MessageType::START_MATCH || !pb_msg.has_start_match()) {
        LOG_ERROR("2 Failed to parse start match message body");
        return;
    }
    
    std::string playerId = pb_msg.start_match().player_info().player_id();
    std::string player_name = pb_msg.start_match().player_info().player_name();
    int32_t icon_id = pb_msg.start_match().player_info().icon_id();
    int32_t room_capacity = pb_msg.start_match().max_players_in_room();

    if (playerId.empty()) {
        // 如果消息中没有玩家ID，使用连接ID作为玩家ID
        // playerId = conn->getId();
        LOG_ERROR("Player ID is empty");
        return ;
    }
    
    LOG_INFO("Player {} requesting to start match", playerId);
    
    // 创建新的Player对象
    auto player = std::make_shared<Player>(playerId, conn);
    player->setState(PlayerState::CONNECTED);
    player->SetPlayerName(player_name);
    player->SetIconId(icon_id);
    
    // 获取RoomManager实例并加入匹配队列
    auto& roomManager = RoomManager::getInstance();
    roomManager.joinWaitRoom(player);
    
    LOG_INFO("Player {} added to match queue", playerId);
    
    // 检查是否有房间可以开始游戏
    // auto playerRoom = roomManager.getPlayerRoom(playerId);
    auto playerRoom = roomManager.getPlayerRoom(player);
    if (playerRoom && playerRoom->isFull()) {
        LOG_INFO("Room {} is full, attempting to start game", playerRoom->getId());
        
        // 尝试开始游戏
        if (roomManager.startGameInRoom(std::to_string(playerRoom->getId()))) {
            LOG_INFO("Game successfully started in room {}", playerRoom->getId());
        } else {
            LOG_WARN("Failed to start game in room {}", playerRoom->getId());
        }
    } else {
        if (playerRoom) {
            LOG_INFO("Player {} assigned to room {}, waiting for more players ({}/{})", 
                        playerId, playerRoom->getId(), 
                        playerRoom->getAllPlayerCount(), room_capacity);
        } else {
            LOG_WARN("Player {} not assigned to any room", playerId);
        }
    }
}

// 处理数据加载完成消息
void CppEngine::onDataLoaded(const std::shared_ptr<Connection>& conn, const Message& msg) {
    LOG_INFO("Player Data Loaded: {}", conn->getId());

    // 解析消息获取玩家ID
    NetworkMessage pb_msg;
    if (!msg.getBodyAsProto(pb_msg)) {
        LOG_ERROR("Failed to parse data loaded message body");
        return;
    }
    std::string playerId = pb_msg.player_id();
    if (playerId.empty()) {
        LOG_ERROR("Player ID is empty in data loaded message");
        return;
    }
    LOG_INFO("Player {} data loaded", playerId);
    // 获取房间管理器并找到对应房间
    auto& roomManager = RoomManager::getInstance();
    // auto room = roomManager.getPlayerRoom(playerId);

    auto player = roomManager.getPlayerByConnection(conn);
    auto room = roomManager.getPlayerRoom(player);

    if (!room) {
        LOG_ERROR("Room not found for player {}", playerId);
        return;
    }
    // 设置玩家数据加载状态
    room->setDataLoadStatus(playerId, true);
    LOG_INFO("Player {} data load status set to true in room {}", playerId, room->getId());

    // 检查是否所有玩家都已加载数据
    if (room->isAllPlayerDataLoaded()) {
        LOG_INFO("All players in room {} have loaded data, starting game", room->getId());
        room->startGame();
    }
}

// 备战结束，收到玩家快照，集齐快照后广播
void CppEngine::onPrepSnapshot(const std::shared_ptr<Connection>& conn, const Message& msg) {
    LOG_INFO("Received BATTLE_PREP_SNAPSHOT from connection: {}", conn->getId());
    
    // 解析消息
    NetworkMessage pb_msg;
    if (!msg.getBodyAsProto(pb_msg)) {
        LOG_ERROR("Failed to parse battle prep snapshot message body");
        return;
    }
    
    // 检查是否有快照数据
    if (!pb_msg.has_battle_prep_snapshot()) {
        LOG_ERROR("Message does not contain battle prep snapshot data");
        return;
    }
    
    const auto& snapshot = pb_msg.battle_prep_snapshot();
    
    // 提取消息数据
    std::string playerId = pb_msg.player_id();
    int32_t round = snapshot.round();
    std::string formationData = snapshot.formation_data();
    int32_t honorValue = snapshot.honor_value();

    // 获取房间管理器并找到对应房间
    auto& roomManager = RoomManager::getInstance();
    // auto room = roomManager.getPlayerRoom(playerId);
    auto player = roomManager.getPlayerByConnection(conn);
        if (!player) {
        LOG_ERROR("Player {} not found in room", playerId);
        return;
    }

    auto room = roomManager.getPlayerRoom(player);
    if (!room) {
        LOG_ERROR("Room not found for player {}", playerId);
        return;
    }

    std::string matchId = std::to_string(room->getId());
    LOG_INFO("Processing snapshot - Player: {}, match id: {}, Round: {}, Honor: {}", 
                playerId, matchId, round, honorValue);
    
    // 记录玩家快照
    if (!room->recordPlayerSnapshot(playerId, formationData, honorValue, round)) {
        LOG_ERROR("Failed to record snapshot for player {} in room {}", playerId, matchId);
        return;
    }
    
    LOG_INFO("Successfully recorded snapshot for player {} in room {}", playerId, matchId);
    
    // 检查是否所有游戏中玩家都已提交快照
    if (room->allGamingSnapshotsReceived()) {
        LOG_INFO("All snapshots received for room {}, broadcasting ALL_SNAPSHOTS", matchId);
        // room->broadcastAllGamingSnapshots();

        // 记录机器人快照
        room->recordRobotSnapshots();

        // 广播所有快照
        room->broadcastAllSnapshots();
        
        // 清理快照为下一轮准备
        room->clearSnapshots();
    }
}

// 处理战斗后的消息 {type:战斗结果，对局id，我方id，我方荣耀值，敌方id，敌方荣耀值}
// 如果是最后一轮，需要单独处理
void CppEngine::onBattleResult(const std::shared_ptr<Connection>& conn, const Message& msg) {
    LOG_INFO("Player Battle Result: {}", conn->getId());
    // 解析消息获取玩家ID
    NetworkMessage pb_msg;
    if (!msg.getBodyAsProto(pb_msg)) {
        LOG_ERROR("Failed to parse battle result message body");
        return;
    }

    if( !pb_msg.has_battle_result_report()) {
        LOG_ERROR("Message does not contain battle result data");
        return;
    }

    const auto& battleResult = pb_msg.battle_result_report();

    std::string playerId = pb_msg.player_id();
    int32_t round = battleResult.round();
    int32_t honorValue = battleResult.honor_value();
    std::string enemyId = battleResult.enemy_player_id();
    int32_t enemyHonorValue = battleResult.enemy_honor_value();
    
    // 获取房间管理器并找到对应房间
    auto& roomManager = RoomManager::getInstance();
    // auto room = roomManager.getPlayerRoom(playerId);
    auto player = roomManager.getPlayerByConnection(conn);
    if(!player) {
        LOG_ERROR("Player not found for connection {}", conn->getId());
        return;
    }

    auto room = roomManager.getPlayerRoom(player);
    if (!room) {
        LOG_ERROR("Room not found");
        return;
    }

    std::string matchId = std::to_string(room->getId());
    LOG_INFO("Processing battle result - Player: {}, Match: {}, Round: {}, Honor: {}", 
                playerId, matchId, round, honorValue);
    

    // // 更新玩家最新的荣耀值（不再使用，insertRanking会更新）
    // player->SetHonorValue(honorValue);
    // player->SetRound(round);
    
    // 记录战斗结果
    if (!room->insertRanking(playerId, honorValue)) {
        LOG_ERROR("Failed to record ranking for player {} in room {}", playerId, matchId);
        return;
    }

    // 20250806updated: 真人和机器人打，更新机器人荣耀值
    auto enemy_player = room->getPlayer(enemyId);
    if(enemy_player && enemy_player->isRobot()) {
        enemy_player->SetHonorValue(enemyHonorValue);
        enemy_player->SetRound(round);

        // 记录机器人的战斗结果
        if (!room->insertRanking(enemyId, enemyHonorValue)) {
            LOG_ERROR("Failed to record ranking for player {} in room {}", enemyId, matchId);
            return;
        }
    }

    // 20250805updated: 服务器发给每个结算的客户端 {type:对手信息，对手id，是否机器人，机器人阵容，先手id}
    if(room->getCurrentRound() < ROUND_NUM)
        room->sendEnemyFormationToPlayer(player);

    // 20250805updated: 广播排名
    // 每有玩家结算，服务器广播客户端 {type：现在排名信息，所有<玩家id，排名，荣耀值>}
    room->BroadcastCurrentRankings();
    LOG_INFO("Successfully broadcast ranking for player in room {}", matchId);
    
    // 检查是否所有在线玩家都已提交战斗结果（不包括纯机器人对战情况）
    if (room->allGamingRankingsReceived()) {
        LOG_INFO("All gaming rankings received for room {}, broadcasting rankings", matchId);

        // 20250806updated: 结算两个机器人对战的情况，再次广播排名
        room->calculateRobotHonor();
        room->BroadcastCurrentRankings();
        
        // 继续下一轮战斗
        if(room->getCurrentRound() < ROUND_NUM){
            room->resetSendResultState();  // 重置所有玩家的发送结果状态

            room->nextRound();  // 回合+1
            room->broadcastPrepareStart();  // 广播备战开始消息
            room->startBattlePrepTimer();   // 启动备战倒计时
        }
        // 战斗结束，广播结算，清理房间
        else {
            room->BroadcastResults();
            for (auto playerx : room->getPlayers()) {
                playerx->setState(PlayerState::CONNECTED);
            }
            roomManager.removeRoom(matchId);
            LOG_INFO("Game finished in room {}, broadcasting battle results", matchId);
        }
    }
}

// 玩家主动退出匹配或游戏
void CppEngine::onExit(const std::shared_ptr<Connection>& conn, const Message& msg) {
    LOG_INFO("Player Exit: {}", conn->getId());

    // 解析消息获取玩家ID
    NetworkMessage pb_msg;
    if (!msg.getBodyAsProto(pb_msg)) {
        LOG_ERROR("Failed to parse exit message body");
        return;
    }

    auto exit_msg = pb_msg.exit();
    std::string exit_player_id = exit_msg.exit_info().exit_player_id();
    if (exit_player_id.empty()) {
        LOG_ERROR("Player ID is empty in exit message");
        return;
    }
    LOG_INFO("Player {} is exiting", exit_player_id);

    // 获取房间管理器并找到对应房间
    auto& roomManager = RoomManager::getInstance();

    auto player = roomManager.getPlayerByConnection(conn);

    if (!player) {
        LOG_ERROR("游戏已结束，房间已销毁");
        return ;
    }
    
    // 玩家主动退出匹配
    if (player->getState() == PlayerState::CONNECTED) {
        LOG_INFO("Player {} is exiting match queue", exit_player_id);
        roomManager.removePlayerFromWaitQueue(player->GetPlayerId());
    }
    else if (player->getState() == PlayerState::GAMING) {
        int32_t exit_round = exit_msg.exit_info().exit_round();
        int32_t exit_honor_value = exit_msg.exit_info().exit_honor_value();
        // auto room = roomManager.getPlayerRoom(exit_player_id);
        auto room = roomManager.getPlayerRoom(player);
        if (!room) {
            LOG_ERROR("Room not found for player {}", exit_player_id);
            return;
        }

        // 关闭定时器
        // room->stopCountdownTimer();
        // player->setState(PlayerState::ROBOT);
        // player->SetConnection(nullptr);
        room->deletePlayer(player->GetPlayerId());  // 删除玩家
        // 房间处理玩家退出并广播消息
        roomManager.removePlayerConnection(conn);
        room->onPlayerExit(exit_player_id, exit_round, exit_honor_value);
        room->broadcastExitMessage();

        if (room->getGamingPlayerCount() == 0) {
            LOG_INFO("All players have exited the game and removed from room {}", room->getId());
            roomManager.removeRoom(std::to_string(room->getId()));
        }
        LOG_INFO("Player {} has exited the game and removed from room {}", exit_player_id, room->getId());
    }
}

// void CppEngine::onPlayerUpdate(const std::shared_ptr<Connection>& conn, const Message& msg) {
//     NetworkMessage pb_msg;
//     if (!msg.getBodyAsProto(pb_msg)) {
//         LOG_ERROR("Failed to parse player update message");
//         return;
//     }
    
//     if (pb_msg.has_player_update()) {
//         const auto& update = pb_msg.player_update();
//         LOG_DEBUG("Player update: pos=({}, {}, {}), rot=({}, {}, {})", 
//                      update.position_x(), update.position_y(), update.position_z(),
//                      update.rotation_x(), update.rotation_y(), update.rotation_z());
//     }
// }

// void CppEngine::onPlayerAttribute(const std::shared_ptr<Connection>& conn, const Message& msg) {
//     NetworkMessage pb_msg;
//     if (!msg.getBodyAsProto(pb_msg)) {
//         LOG_ERROR("Failed to parse player attribute message");
//         return;
//     }
    
//     if (pb_msg.has_player_attribute()) {
//         const auto& attr = pb_msg.player_attribute();
//         LOG_DEBUG("Player attribute: health={}, armor={}", attr.health(), attr.armor());
//     }
// }

// void CppEngine::onPlayerState(const std::shared_ptr<Connection>& conn, const Message& msg) {
//     NetworkMessage pb_msg;
//     if (!msg.getBodyAsProto(pb_msg)) {
//         LOG_ERROR("Failed to parse player state message");
//         return;
//     }
    
//     if (pb_msg.has_player_state()) {
//         const auto& state = pb_msg.player_state();
//         LOG_DEBUG("Player state: alive={}, team={}", state.is_alive(), state.team_id());
//     }
// }

// void CppEngine::onPlayerJoin(const std::shared_ptr<Connection>& conn, const Message& msg) {
//     LOG_INFO("Player joined: {}", conn->getId());
// }

// void CppEngine::onPlayerLeave(const std::shared_ptr<Connection>& conn, const Message& msg) {
//     LOG_INFO("Player left: {}", conn->getId());
// } 