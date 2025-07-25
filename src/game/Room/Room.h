#pragma once
#include <string>
#include "game/Player/Player.h"
#include <thread>
#include <atomic>

#define MAX_PLAYERS 2           // 房间最大玩家数
#define PREPARE_TIME 30         // 准备时间（秒）
#define BROADCAST_INTERVAL 5    // 广播间隔（秒）

enum class RoomState {
    WAITING,    // 等待玩家
    FULL,       // 房间玩家已满，准备开始
    GAMING,     // 游戏中
    FINISHED    // 已结束
};

class Room{
public:
    Room(const int32_t room_id = 0):room_id_(room_id),
                                    max_players_(MAX_PLAYERS){}
    ~Room() = default;

    // 基础信息
    const int32_t& getId() const { return room_id_; }
    RoomState getState() const { return state_; }
    void setState(RoomState state) { state_ = state; }
    bool isFull() const { return players_.size() >= max_players_; }
    bool isEmpty() const { return players_.empty(); }
    size_t getPlayerCount() const { return players_.size(); }
    
    // 玩家管理
    bool addPlayer(std::shared_ptr<Player> player);
    std::shared_ptr<Player> getPlayer(const std::string& playerId);
    const std::vector<std::shared_ptr<Player>>& getPlayers() const { return players_; }

    // 游戏开始
    void startGame();

    // 获取当前回合
    int32_t getCurrentRound() const { return currentRound_; }

private:
    // 基础信息
    int32_t room_id_;
    RoomState state_;
    int32_t max_players_;
    int32_t currentRound_;

    // 玩家相关数据：房间内玩家信息、玩家ID映射表、玩家快照
    std::vector<std::shared_ptr<Player>> players_;
    std::unordered_map<std::string, std::shared_ptr<Player>> playerMap_;
    std::unordered_map<std::string, PlayerSnapshot> currentSnapshots_;

    // 定时器相关：剩余倒计时秒数，定时器线程
    int32_t countdown_remaining_seconds_;
    std::thread countdown_thread_;
    std::atomic<bool> countdown_running_{false};

    // 内部方法
    void startCountdownTimer(int32_t seconds);  // 启动倒计时，参数为总秒数
    void stopCountdownTimer();                  // 停止倒计时
    int32_t getRemainingTime() const;           // 获取当前剩余时间（秒）
    void broadcastMessage(const NetworkMessage& msg);
    void broadcastToOthers(const std::string& excludePlayerId, const NetworkMessage& msg);
    void startBattlePrepTimer();                // 启动备战倒计时
    void onCountdownTick();                     // 定时器回调：每秒调用一次，处理倒计时逻辑
    void onCountdownFinished();                 // 定时器回调：备战倒计时结束时调用
    void BroadcastResults();                    // 广播游戏结果
    std::vector<RankingEntry> getRankings();    // 获取排名
    void cleanupRoom();                         // 清理房间
};
