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

        // 广播游戏开始消息 {type：对局开始，对局id，所有玩家id}
        NetworkMessage msg;
        msg.set_msg_id(MessageType::GAME_START);
        GameStartMessage* start_msg = msg.mutable_game_start();
        start_msg->set_match_id(std::to_string(room_id_));
        for(int i = 0;i < players_.size(); i++)
        {
            start_msg->add_player_ids(players_[i]->GetPlayerId());
        }
        broadcastMessage(msg);

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

void Room::BroadcastResults() {
    // 构造结算消息并广播（待实现）
    // NetworkMessage msg;
    // ...填充msg...
    // broadcastMessage(msg);
}

std::vector<RankingEntry> Room::getRankings() {
    // 计算并返回排名（待实现）
    std::vector<RankingEntry> rankings;
    // ...实现排名逻辑...
    return rankings;
}

void Room::cleanupRoom() {
    players_.clear();
    playerMap_.clear();
    currentSnapshots_.clear();
    state_ = RoomState::FINISHED;
}