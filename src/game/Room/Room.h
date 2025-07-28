#pragma once
#include <cstdint>
#include <string>
#include "game/Player/Player.h"
#include "proto/NetworkMessage.pb.h"
#include <thread>
#include <atomic>
#include <unordered_map>

#define MAX_PLAYERS 2           // 房间最大玩家数
#define PREPARE_TIME 30         // 准备时间（秒）
#define BROADCAST_INTERVAL 5    // 广播间隔（秒）
#define ROUND_NUM 7             // 游戏总轮次

enum class RoomState {
    WAITING,    // 等待玩家
    FULL,       // 房间玩家已满，准备开始
    GAMING,     // 游戏中
    FINISHED    // 已结束
};

class Room{
public:
    Room(const int32_t room_id = 0):room_id_(room_id),
                                    max_players_(MAX_PLAYERS),
                                    currentRound_(0){}
    ~Room() = default;

    // 基础信息
    const int32_t& getId() const { return room_id_; }
    RoomState getState() const { return state_; }
    void setState(RoomState state) { state_ = state; }
    bool isFull() const { return players_.size() >= max_players_; }
    bool isEmpty() const { return players_.empty(); }

    // 房间内玩家数量
    size_t getAllPlayerCount() const;       // 所有玩家数量
    size_t getExitPlayerCount() const;      // 退出的的玩家数量
    size_t getGamingPlayerCount() const;    // 游戏中的玩家数量
    
    // 玩家管理
    bool addPlayer(std::shared_ptr<Player> player);
    std::shared_ptr<Player> getPlayer(const std::string& playerId);
    const std::vector<std::shared_ptr<Player>>& getPlayers() const { return players_; }

    // 游戏开始
    void startGame();

    // 获取当前回合
    int32_t getCurrentRound() const { return currentRound_; }

    // 回合+1
    void nextRound() {++currentRound_; };

    // 启动备战倒计时
    void startBattlePrepTimer();
    
    // 快照管理
    bool recordPlayerSnapshot(const std::string& playerId, const std::string& formationData, int32_t honorValue, int32_t round);
    bool allGamingSnapshotsReceived() const;
    void broadcastAllSnapshots();
    void clearSnapshots();

    // 战斗结果管理
    bool insertRanking(const std::string& playerId, int32_t honorValue); // 插入游戏中玩家荣耀值
    bool insertExitRanking(const std::string& playerId, int32_t honorValue); // 插入退出玩家荣耀值
    bool allGamingRankingsReceived() const; // 检查是否所有游戏中玩家都已提交排名
    void BroadcastResults();    // 广播游戏结果

    // 玩家断线或退出
    void onPlayerExit(const std::string& playerId, int32_t exit_round, int32_t honorValue);
    void broadcastExitMessage();
    std::unordered_map<std::string, ExitPlayerInfo> exitPlayers_;

    // 房间清理
    void cleanupRoom();

private:
    // 基础信息
    int32_t room_id_;
    RoomState state_;
    int32_t max_players_;
    int32_t currentRound_;

    // 玩家相关数据：房间内所有玩家信息、玩家ID映射表、玩家快照、结算数据、退出的玩家数据
    // 注：当玩家退出，不会删除玩家，只是将Player状态设置为DISCONNECTED
    std::vector<std::shared_ptr<Player>> players_;
    std::unordered_map<std::string, std::shared_ptr<Player>> playerMap_;
    std::unordered_map<std::string, PlayerSnapshot> currentSnapshots_;
    std::unordered_map<std::string, int32_t> allHonorValue_;

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
    void onCountdownTick();                     // 定时器回调：每秒调用一次，处理倒计时逻辑
    void onCountdownFinished();                 // 定时器回调：备战倒计时结束时调用
    std::vector<std::shared_ptr<RankingEntry>> getRankings();    // 获取总排名
};
