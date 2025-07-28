#include "game/Room/Room.h"
#include "game/Player/Player.h"
#include "proto/Message.h"
#include "proto/NetworkMessage.pb.h"
#include <cstddef>
#include <spdlog/spdlog.h>
#include <string>

// 析构函数
Room::~Room() {
    spdlog::info("Destroying room {}", room_id_);
    
    // 确保定时器线程正确停止
    stopCountdownTimer();
    
    // 清理所有资源
    cleanupRoom();
    
    spdlog::info("Room {} destroyed successfully", room_id_);
}

size_t Room::getAllPlayerCount() const{
    return players_.size();
}

size_t Room::getExitPlayerCount() const{
    size_t count = 0;
    for(const auto& p:players_){
        if(p && p->getState()==PlayerState::DISCONNECTED){
            count++;
        }
    }
    return count;
}

size_t Room::getGamingPlayerCount() const{
    size_t count = 0;
    for(const auto& p:players_){
        if(p && p->getState()==PlayerState::GAMING){
            count++;
        }
    }
    return count;
}

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
        while (countdown_running_ && countdown_remaining_seconds_ >= 0) {
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
    spdlog::debug("Stopping countdown timer for room {}", room_id_);
    
    // 设置停止标志
    countdown_running_ = false;
    
    // 等待定时器线程结束
    if (countdown_thread_.joinable()) {
        spdlog::debug("Waiting for countdown thread to join for room {}", room_id_);
        countdown_thread_.join();
        spdlog::debug("Countdown thread joined successfully for room {}", room_id_);
    }
    
    countdown_remaining_seconds_ = 0;
    spdlog::debug("Countdown timer stopped for room {}", room_id_);
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

        nextRound();  // 回合+1
        startBattlePrepTimer();  // 启动备战倒计时

        spdlog::info("Game started in room {}", room_id_);
    } else {
        spdlog::warn("Cannot start game, room {} is not full", room_id_);
    }
}

// 广播消息给房间所有Gaming状态玩家
void Room::broadcastMessage(const NetworkMessage& msg) {
    // 将NetworkMessage转换为Message
    Message body;
    body.setBodyFromProto(msg);

    // 遍历所有玩家并发送消息
    for (const auto& player : players_) {
        if (player && player->getState() == PlayerState::GAMING) {
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
        if (player && player->getState() == PlayerState::GAMING && player->GetPlayerId() != excludePlayerId) {
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

// 定时器回调：每x秒调用一次，处理倒计时逻辑
void Room::onCountdownTick() {
    if (countdown_remaining_seconds_ > 0) {
        countdown_remaining_seconds_-=BROADCAST_INTERVAL;

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

bool Room::allGamingRankingsReceived() const{
    // 检查是否所有游戏中玩家都已提交排名
    if (allHonorValue_.size() != getGamingPlayerCount()) {
        spdlog::warn("Not all gaming players have submitted their rankings, current size: {}", allHonorValue_.size());
        return false;
    }
    
    return true;
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

// 插入在线玩家荣耀值
bool Room::insertRanking(const std::string& playerId, int32_t honorValue) {
    // 插入或更新排名
    auto player = getPlayer(playerId);
    if (!player || player->getState() != PlayerState::GAMING) {
        spdlog::error("Player {} not found or not in gaming state", playerId);
        return false;
    }

    allHonorValue_[playerId] = honorValue;

    spdlog::info("Inserted ranking for player {} with honor value {}, allHonorValue_ size ", playerId, honorValue, allHonorValue_.size());
    return true;
}

// 插入退出玩家荣耀值
bool Room::insertExitRanking(const std::string& playerId, int32_t honorValue) {
    // 插入或更新退出玩家的荣耀值
    auto player = getPlayer(playerId);
    if (!player || player->getState() != PlayerState::DISCONNECTED) {
        spdlog::error("Player {} not found or not in disconnected state", playerId);
        return false;
    }
    
    allHonorValue_[playerId] = honorValue;

    spdlog::info("Inserted exit ranking for player {} with honor value {}", playerId, honorValue);
    return true;
}

// 根据荣耀值计算房间内玩家的排名
std::vector<std::shared_ptr<RankingEntry>> Room::getRankings() {
    if(allHonorValue_.size() != getAllPlayerCount()) {
        spdlog::warn("Not all players have submitted their rankings, cannot generate complete rankings, allHonorValue_ size:", allHonorValue_.size());
        return {};
    }

    std::vector<std::shared_ptr<RankingEntry>> rankings;
    rankings.reserve(getAllPlayerCount());

    // 收集所有玩家的排名信息
    for (const auto&[p,h]: allHonorValue_) {
        auto player = getPlayer(p);
        if (!player) {
            spdlog::warn("Player {} not found in room {}", p, room_id_);
            continue;
        }

        auto rankInfo = std::make_shared<RankingEntry>();
        rankInfo->set_player_id(player->GetPlayerId());
        rankInfo->set_honor_value(player->GetHonorValue());
        rankInfo->set_rank(0);  // 初始排名为0

        rankings.push_back(rankInfo);
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

// 房间清理
void Room::cleanupRoom() {
    spdlog::info("Cleaning up room {}", room_id_);
    
    // 先停止定时器
    stopCountdownTimer();
    
    // 清理所有容器
    players_.clear();
    playerMap_.clear();
    currentSnapshots_.clear();
    allHonorValue_.clear();
    exitPlayers_.clear();
    
    // 设置状态为已结束
    state_ = RoomState::FINISHED;
    
    spdlog::info("Room {} cleanup completed", room_id_);
}

// 记录玩家快照，并更新到Player的最新快照中
bool Room::recordPlayerSnapshot(const std::string& playerId, const std::string& formationData, int32_t honorValue, int32_t round) {
    // 检查玩家是否在房间中
    auto player = getPlayer(playerId);
    if (!player) {
        spdlog::warn("Player {} not found in room {}", playerId, room_id_);
        return false;
    }
    
    // 创建快照
    PlayerSnapshot snapshot;
    snapshot.set_player_id(playerId);
    snapshot.set_match_id(std::to_string(room_id_));
    snapshot.set_round(round);
    snapshot.set_formation_data(formationData);
    snapshot.set_honor_value(honorValue);

    // 更新最新的玩家快照
    player->SetLatestSnapshot(std::make_shared<PlayerSnapshot>(snapshot));
    
    // 存储快照
    currentSnapshots_[playerId] = snapshot;
    
    spdlog::info("Recorded snapshot for player {} in room {}, round {} (honor: {})", 
                playerId, room_id_, round, honorValue);
    return true;
}

// 检查是否所有玩家的快照都已收到
bool Room::allGamingSnapshotsReceived() const {
    if (currentSnapshots_.size() != getGamingPlayerCount()) {
        spdlog::warn("Not all gaming players have submitted their snapshots, current size: {}, gaming player: ", currentSnapshots_.size(), getGamingPlayerCount());
        return false;
    }
    
    // 确保所有玩家都有快照
    for (const auto& player : players_) {
        if (player->getState() != PlayerState::GAMING && 
            currentSnapshots_.find(player->GetPlayerId()) == currentSnapshots_.end()) {
            return false;
        }
    }
    
    return true;
}

// 广播所有快照
void Room::broadcastAllGamingSnapshots() {
    if (!allGamingSnapshotsReceived()) {
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
    all_snapshots->set_round(getCurrentRound());
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

// 玩家退出处理
void Room::onPlayerExit(const std::string& playerId, int32_t exit_round, int32_t honorValue) {
    spdlog::info("Player {} has exited the room {}", playerId, room_id_);
    // 检查玩家是否在房间中
    auto player = getPlayer(playerId);
    if (!player) {
        spdlog::warn("Player {} not found in room {}", playerId, room_id_);
        return;
    }

    // 更新玩家状态
    player->setState(PlayerState::DISCONNECTED);

    // 放入退出玩家信息
    ExitPlayerInfo exitInfo;
    exitInfo.set_exit_player_id(playerId);
    exitInfo.set_exit_round(exit_round);
    exitInfo.set_exit_honor_value(honorValue);
    exitPlayers_[playerId] = exitInfo;
    spdlog::info("Player {} info inserted into exitPlayers_ with round {} and honor value {}", 
                playerId, exit_round, honorValue);

    // 插入退出玩家的荣耀值
    if (!insertExitRanking(playerId, honorValue)) {
        spdlog::error("Failed to insert exit ranking for player {}", playerId);
        return;
    }

    spdlog::info("Player {} marked as disconnected in room {}", playerId, room_id_);
}

// 广播退出消息
void Room::broadcastExitMessage() {
    NetworkMessage msg;
    msg.set_msg_id(MessageType::DISCONNECTED);
    ExitBroadcastMessage* exit_msg = msg.mutable_exit_broadcast();

    for(const auto& exitPair : exitPlayers_) {
        const ExitPlayerInfo& exitInfo = exitPair.second;
        ExitPlayerInfo* info = exit_msg->add_exit_players_info();
        info->set_exit_player_id(exitInfo.exit_player_id());
        info->set_exit_round(exitInfo.exit_round());
        info->set_exit_honor_value(exitInfo.exit_honor_value());
    }

    // 广播消息
    broadcastMessage(msg);
    spdlog::info("Broadcasted exit message for room {}", room_id_);
}