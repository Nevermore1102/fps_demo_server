#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <future>
#include "Room.h"
#include "game/Player/Player.h"
#include "net/Connection.h"
#include "proto/NetworkMessage.pb.h"

class RoomManager {
public:
    static RoomManager& getInstance();
    
    // 匹配系统
    void joinWaitRoom(const std::shared_ptr<Player>& player);
    
    // 房间管理
    std::shared_ptr<Room> getRoom(const std::string& roomId);
    void removeRoom(const std::string& roomId);
    void broadcastwaitRoom(const NetworkMessage& msg);
    
    // 玩家房间查询
    // std::shared_ptr<Room> getPlayerRoom(const std::string& playerId);
    std::shared_ptr<Room> getPlayerRoom(const std::shared_ptr<Player>& player);
    
    // 玩家管理辅助方法
    bool removePlayerFromWaitQueue(const std::string& playerId);
    bool removePlayerFromRoom(const std::string& playerId);

    // 连接-玩家映射管理
    void addPlayerConnection(const std::shared_ptr<Connection>& conn, const std::shared_ptr<Player>& player);
    void removePlayerConnection(const std::shared_ptr<Connection>& conn);
    std::shared_ptr<Player> getPlayerByConnection(const std::shared_ptr<Connection>& conn);
    std::shared_ptr<Connection> getConnectionByPlayerId(const std::string& playerId);
    bool isConnectionActive(const std::shared_ptr<Connection>& conn) const;
    void cleanupInactiveConnections();
    
    // 连接断开处理
    void handleConnectionDisconnect(const std::shared_ptr<Connection>& conn);

    // 房间状态查询
    bool isRoomFull(const std::string& roomId) const;
    size_t getRoomPlayerCount(const std::string& roomId) const;
    std::vector<std::pair<std::string, size_t>> getAllRoomsInfo() const;

    // 统计信息
    size_t getMatchingPlayers() const;
    size_t getTotalRooms() const;
    
    // 游戏控制
    bool startGameInRoom(const std::string& roomId);
    
    // 清理任务
    void cleanupEmptyRooms();
    void cleanupFinishedRooms();
    void safeRemoveRoom(const std::string& roomId);  // 安全删除房间
    
    
private:
    RoomManager() : roomIdCounter_(1) {}
    ~RoomManager();
    
    // 禁用拷贝
    RoomManager(const RoomManager&) = delete;
    RoomManager& operator=(const RoomManager&) = delete;
    
    // 匹配逻辑
    void processMatching();
    std::shared_ptr<Room> createRoom();
    void createRoomWithCurrentPlayers(); // 用当前队列创建房间
    // std::string generateRoomId();
    
    // 清理逻辑
    void cleanupDisconnectedPlayers();
    
    // 匹配倒计时相关方法
    void startMatchingCountdown();
    void stopMatchingCountdown();
    void onMatchingCountdownFinished();
    
    // 数据成员
    std::vector<std::shared_ptr<Player>> wait_rooms_; // 等待匹配的玩家队列
    std::unordered_map<int32_t, std::shared_ptr<Room>> rooms_; // 房间ID到房间对象的映射
    std::unordered_map<std::shared_ptr<Connection>, std::shared_ptr<Player>> Connections_player_; // 玩家连接到玩家对象的映射

    // 线程安全
    mutable std::mutex playersMutex_;
    mutable std::mutex roomsMutex_;
    mutable std::mutex matchingMutex_;
    
    // 配置
    static constexpr int MAX_PLAYERS_PER_ROOM = MAX_PLAYERS;
    
    // 房间ID生成
    std::atomic<int32_t> roomIdCounter_;
    
    // 匹配倒计时机制
    std::thread matching_timer_thread_;
    std::atomic<bool> matching_timer_running_{false};
    std::chrono::steady_clock::time_point first_player_join_time_;
    bool has_first_player_joined_ = false;
    static constexpr int MATCHING_COUNTDOWN_SECONDS = 10;
    std::promise<void> stop_signal_;

    std::vector<std::string> robot_names_ = {"温柔的瓦力", "美丽的伊芙", "勤劳的萝丝"};
};