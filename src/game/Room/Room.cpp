#include "game/Room/Room.h"
#include "game/Player/Player.h"
#include "proto/Message.h"
#include "proto/NetworkMessage.pb.h"
#include <spdlog/spdlog.h>
#include <string>

// 添加玩家到房间
bool Room::addPlayer(std::shared_ptr<Player> player) {
    if (!player || isFull()) return false;

    players_.push_back(player);
    playerMap_[player->GetPlayerId()] = player;
    return true;
}

// 获取指定ID的玩家信息
std::shared_ptr<Player> Room::getPlayer(const std::string& playerId) {
    auto it = playerMap_.find(playerId);
    if (it != playerMap_.end()) {
        return it->second;
    }
    return nullptr;
}

// 启动倒计时seconds秒
void Room::startCountdownTimer(int32_t seconds) {
    stopCountdownTimer(); // 保证不会有多个定时器线程
    countdown_running_ = true;
    countdown_remaining_seconds_ = seconds;

    // 创建倒计时线程
    countdown_thread_ = std::thread([this]() {
        while (countdown_running_ && countdown_remaining_seconds_ > 0) {
            std::this_thread::sleep_for(std::chrono::seconds(BROADCAST_INTERVAL));
            if (!countdown_running_)
            {
                spdlog::info("Countdown stopped!");
                break;
            }
            // 执行逻辑
            onCountdownTick();
        }
    });
}

// 停止倒计时
void Room::stopCountdownTimer() {
    countdown_running_ = false;
    // 等待定时器线程结束
    if (countdown_thread_.joinable()) {
        countdown_thread_.join();
    }
    countdown_remaining_seconds_ = 0;
}

// 获取当前剩余时间
int32_t Room::getRemainingTime() const {
    return countdown_remaining_seconds_;
}

// 开始游戏主函数
void Room::startGame() {
    if (isFull()) {
        setState(RoomState::GAMING);

        // 广播游戏开始消息 {type：对局开始，对局id，所有玩家信息}
        NetworkMessage msg;
        msg.set_msg_id(MessageType::GAME_START);
        GameStartMessage* start_msg = msg.mutable_game_start();
        start_msg->set_match_id(std::to_string(room_id_));

        for(const auto& player : players_) {
            if (player) {
                // 使用add_players_info()添加玩家信息
                PlayerBasicInfo* info = start_msg->add_players_info();
                info->set_player_id(player->GetPlayerId());
                info->set_player_name(player->GetPlayerName());
                info->set_icon_id(player->GetIconId());
            }
        }
        broadcastMessage(msg);

        startBattlePrepTimer();  // 启动备战倒计时

        spdlog::info("Game started in room {}", room_id_);
    } else {
        spdlog::warn("Cannot start game, room {} is not full", room_id_);
    }
}

// 广播消息给房间所有玩家
void Room::broadcastMessage(const NetworkMessage& msg) {
    // 将NetworkMessage转换为Message
    Message body;
    body.setBodyFromProto(msg);

    // 遍历所有玩家并发送消息
    for (const auto& player : players_) {
        if (player){
            auto conn = player->GetConnection();
            if (conn) {
                bool success = conn->sendMessage(body);
                if (!success)
                    spdlog::error("Failed to send message to player: {}", player->GetPlayerId());
                else{
                    spdlog::info("Message sent to player: {}", player->GetPlayerId());
                    body.logMessage();  // 打印消息详情
                }
            }
        }
    }
}

// 广播消息给除指定玩家外的所有玩家
void Room::broadcastToOthers(const std::string& excludePlayerId, const NetworkMessage& msg) {
    Message body;
    body.setBodyFromProto(msg);

    for (const auto& player : players_) {
        if (player && player->GetPlayerId() != excludePlayerId) {
            auto conn = player->GetConnection();
            if(conn)
            {
                bool success = conn->sendMessage(body);
                if (!success)
                    spdlog::error("Failed to send message to player: {}", player->GetPlayerId());
                else{
                    spdlog::info("Message sent to player: {}", player->GetPlayerId());
                    body.logMessage();  // 打印消息详情
                }
            }
        }
    }
}

// 开启备战倒计时
void Room::startBattlePrepTimer() {
    startCountdownTimer(PREPARE_TIME);
}

// 定时器回调：每秒调用一次，处理倒计时逻辑
void Room::onCountdownTick() {
    if (countdown_remaining_seconds_ > 0) {
        --countdown_remaining_seconds_;

        // 剩余时间消息 {type：剩余时间广播，对局id，轮次，备战剩余时间}
        NetworkMessage msg;
        msg.set_msg_id(MessageType::BATTLE_PREP_TIMER);

        BattlePrepTimerMessage* timer_msg = msg.mutable_battle_prep_timer();
        timer_msg->set_match_id(std::to_string(room_id_));
        timer_msg->set_round(currentRound_);
        timer_msg->set_remaining_time_seconds(countdown_remaining_seconds_);
        
        broadcastMessage(msg);
    }
    if (countdown_remaining_seconds_ == 0) {
        onCountdownFinished();
    }
}

void Room::onCountdownFinished() {
    // 处理倒计时结束逻辑，如自动准备、结算等（待实现）
}

// 广播结果给所有玩家
void Room::BroadcastResults() {
    auto rankings = getRankings();

    NetworkMessage msg;
    msg.set_msg_id(MessageType::BATTLE_RESULT);
    SettlementMessage* settlement_msg = msg.mutable_settlement();
    settlement_msg->set_match_id(std::to_string(room_id_));
    
    for(const auto& entry : rankings) {
        // 使用add_rankings()添加新的排名条目
        RankingEntry* new_entry = settlement_msg->add_rankings();
        new_entry->CopyFrom(*entry);  // 复制entry的内容到新条目
    }

    // 广播消息
    broadcastMessage(msg);
}

// 根据荣耀值计算房间内玩家的排名
std::vector<std::shared_ptr<RankingEntry>> Room::getRankings() {
    std::vector<std::shared_ptr<RankingEntry>> rankings;
    rankings.reserve(players_.size());

    // 收集所有玩家的排名信息
    for (const auto& player : players_) {
        if (player) {  // 确保玩家指针有效
            auto rankInfo = std::make_shared<RankingEntry>();
            rankInfo->set_player_id(player->GetPlayerId());
            rankInfo->set_honor_value(player->GetHonorValue());
            rankInfo->set_rank(0);  // 初始排名为0

            rankings.push_back(rankInfo);
        }
    }

    // 根据荣耀值降序排序
    std::sort(rankings.begin(), rankings.end(), 
        [](const std::shared_ptr<RankingEntry>& a, const std::shared_ptr<RankingEntry>& b) {
            return a->honor_value() > b->honor_value();  // 降序排序
    });

    // 设置排名
    for (size_t i = 0; i < rankings.size(); ++i) {
        rankings[i]->set_rank(i + 1);  // 排名从1开始
    }

    return rankings;
}

void Room::cleanupRoom() {
    players_.clear();
    playerMap_.clear();
    currentSnapshots_.clear();
    state_ = RoomState::FINISHED;
}

// 记录玩家快照
bool Room::recordPlayerSnapshot(const std::string& playerId, const std::string& formationData, int32_t honorValue, int32_t round) {
    // 检查玩家是否在房间中
    auto player = getPlayer(playerId);
    if (!player) {
        spdlog::warn("Player {} not found in room {}", playerId, room_id_);
        return false;
    }
    
    // 更新玩家数据
    player->SetFormationData(formationData);
    player->SetHonorValue(honorValue);
    
    // 创建快照
    PlayerSnapshot snapshot;
    snapshot.set_player_id(playerId);
    snapshot.set_match_id(std::to_string(room_id_));
    snapshot.set_round(round);
    snapshot.set_formation_data(formationData);
    snapshot.set_honor_value(honorValue);
    
    // 存储快照
    currentSnapshots_[playerId] = snapshot;
    
    spdlog::info("Recorded snapshot for player {} in room {}, round {} (honor: {})", 
                playerId, room_id_, round, honorValue);
    return true;
}

// 检查是否所有玩家的快照都已收到
bool Room::allSnapshotsReceived() const {
    if (currentSnapshots_.size() != players_.size()) {
        return false;
    }
    
    // 确保所有玩家都有快照
    for (const auto& player : players_) {
        if (currentSnapshots_.find(player->GetPlayerId()) == currentSnapshots_.end()) {
            return false;
        }
    }
    
    return true;
}

// 广播所有快照
void Room::broadcastAllSnapshots() {
    if (!allSnapshotsReceived()) {
        spdlog::warn("Cannot broadcast snapshots, not all players have submitted");
        return;
    }
    
    // 生成随机种子
    int32_t seed = static_cast<int32_t>(std::time(nullptr)) + room_id_;
    
    // 构造 ALL_SNAPSHOTS 消息
    NetworkMessage msg;
    msg.set_msg_id(MessageType::ALL_SNAPSHOTS);
    
    AllSnapshotsMessage* all_snapshots = msg.mutable_all_snapshots();
    all_snapshots->set_match_id(std::to_string(room_id_));
    all_snapshots->set_round(currentRound_);
    all_snapshots->set_seed(seed);
    
    // 添加所有玩家快照
    for (const auto& snapshotPair : currentSnapshots_) {
        PlayerSnapshot* snapshot = all_snapshots->add_snapshots();
        *snapshot = snapshotPair.second;
    }
    
    // 广播消息
    broadcastMessage(msg);
    
    spdlog::info("Broadcasted all snapshots for room {}, round {} with seed {}", 
                room_id_, currentRound_, seed);
}

// 清理快照
void Room::clearSnapshots() {
    currentSnapshots_.clear();
    spdlog::info("Cleared snapshots for room {}", room_id_);
}