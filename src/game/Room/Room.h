#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include "game/Player/Player.h"
#include "proto/NetworkMessage.pb.h"
#include <thread>
#include <atomic>
#include <unordered_map>

#define MAX_PLAYERS 2           // 房间最大玩家数
#define BROADCAST_INTERVAL 1    // 广播间隔（秒）
#define ROUND_NUM 7             // 游戏总轮次
// // 备战时间（秒）常数组
// const int PREPARE_TIME_SECONDS[] = {
//     -1,  // 第0回合
//     40,  // 第一回合
//     25,  // 第二回合
//     30,  // 第三回合
//     30,  // 第四回合
//     35,  // 第五回合
//     35,  // 第六回合
//     40,  // 第七回合
// };

// 备战时间（秒）常数组
const int PREPARE_TIME_SECONDS[] = {
    -1,  // 第0回合
    40,  // 第一回合
    25,  // 第二回合
    25,  // 第三回合
    30,  // 第四回合
    30,  // 第五回合
    35,  // 第六回合
    35,  // 第七回合
};

 
enum class RoomState {
    WAITING,    // 等待玩家
    FULL,       // 房间玩家已满，准备开始
    LOADING,    // 加载中
    GAMING,     // 游戏中
    FINISHED    // 已结束
};

class Room{
public:
    Room(const int32_t room_id = 0):room_id_(room_id),
                                    max_players_(MAX_PLAYERS),
                                    currentRound_(0){}
    ~Room();

    // 基础信息
    const int32_t& getId() const { return room_id_; }
    RoomState getState() const { return state_; }
    void setState(RoomState state) { state_ = state; }
    bool isFull() const { return players_.size() >= max_players_; }
    bool isEmpty() const { return players_.empty(); }
    void setReadyStatus(const std::string& playerId, bool ready);
    bool isPlayerReady(const std::string& playerId) const;
    bool isAllPlayerReady() const;
    void setDataLoadStatus(const std::string& playerId, bool loaded);
    bool isPlayerDataLoaded(const std::string& playerId) const;
    bool isAllPlayerDataLoaded() const;
    bool isPlayerInRoom(const std::shared_ptr<Player>& player) const{
        for(const auto& p:players_){
            if(p == player){
                return true;
            }
        }
        return false;
    }

    // 房间内玩家数量
    size_t getAllPlayerCount() const;       // 所有玩家数量
    size_t getExitPlayerCount() const;      // 退出的的玩家数量
    size_t getGamingPlayerCount() const;    // 游戏中的玩家数量
    
    // 玩家管理
    bool addPlayer(std::shared_ptr<Player> player);
    std::shared_ptr<Player> getPlayer(const std::string& playerId);
    const std::vector<std::shared_ptr<Player>>& getPlayers() const { return players_; }

    // 游戏开始
    void broadcastPlayerInfo();
    void startGame();

    // 获取当前回合
    int32_t getCurrentRound() const { return currentRound_; }

    // 回合+1
    void nextRound() {++currentRound_; };

    // 备战倒计时管理
    void broadcastPrepareStart();
    void startBattlePrepTimer();
    
    // 快照管理
    bool recordPlayerSnapshot(const std::string& playerId, const std::string& formationData, int32_t honorValue, int32_t round);
    bool allGamingSnapshotsReceived() const;
    void broadcastAllGamingSnapshots();
    void clearSnapshots();

    // 战斗结果管理
    bool insertRanking(const std::string& playerId, int32_t honorValue); // 插入游戏中玩家荣耀值
    bool insertExitRanking(const std::string& playerId, int32_t honorValue); // 插入退出玩家荣耀值
    bool allGamingRankingsReceived() const; // 检查是否所有游戏中玩家都已提交排名
    void clearRankings();
    void BroadcastResults();    // 广播游戏结果

    // 玩家断线或退出
    void onPlayerExit(const std::string& playerId, int32_t exit_round, int32_t honorValue);
    void broadcastExitMessage();
    std::unordered_map<std::string, ExitPlayerInfo> exitPlayers_;

    // 房间清理
    void cleanupRoom();

    // 定时器管理
    void setCountdownRunning(bool running) { countdown_running_ = running; }
    bool getCountdownRunning() const { return countdown_running_; }
    bool isCountdownThreadRunning() const { return countdown_thread_.joinable(); }
    void stopCountdownTimer();

    // 房间内回合匹配逻辑
    // 获取此轮敌人id
    std::string getCurrentEnemyId(const std::string& playerId);
    std::vector<std::pair<int, int>> generateRoundMatches(int playerCount, int round);
private:
    // 基础信息
    int32_t room_id_;
    RoomState state_;
    int32_t max_players_;
    int32_t currentRound_;

    // 玩家相关数据：房间内所有玩家信息、玩家ID映射表、玩家快照、结算数据、退出的玩家数据
    // 注：当玩家退出，不会删除玩家，只是将Player状态设置为DISCONNECTED
    std::vector<std::shared_ptr<Player>> players_;
    std::unordered_map<std::string, bool> playerReadyStatus_;       // 玩家准备状态
    std::unordered_map<std::string, bool> playerDataLoadStatus_;    // 玩家数据加载状态
    std::unordered_map<std::string, std::shared_ptr<Player>> playerMap_;
    std::unordered_map<std::string, PlayerSnapshot> currentSnapshots_;
    std::unordered_map<std::string, int32_t> allHonorValue_;

    // 定时器相关：剩余倒计时秒数，定时器线程
    int32_t countdown_remaining_seconds_;
    std::thread countdown_thread_;
    std::atomic<bool> countdown_running_{false};

    // 内部方法
    void startCountdownTimer(int32_t seconds);  // 启动倒计时，参数为总秒数
    int32_t getRemainingTime() const;           // 获取当前剩余时间（秒）
    void broadcastMessage(const NetworkMessage& msg);
    void broadcastToOthers(const std::string& excludePlayerId, const NetworkMessage& msg);
    void onCountdownTick();                     // 定时器回调：每秒调用一次，处理倒计时逻辑
    void onCountdownFinished();                 // 定时器回调：备战倒计时结束时调用
    std::vector<std::shared_ptr<RankingEntry>> getRankings();    // 获取总排名
};
