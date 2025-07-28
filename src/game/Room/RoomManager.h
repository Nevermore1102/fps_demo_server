#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include "Room.h"
#include "game/Player/Player.h"

class RoomManager {
public:
    static RoomManager& getInstance();
    
    // 匹配系统
    void joinWaitRoom(const std::shared_ptr<Player>& player);
    
    // 房间管理
    std::shared_ptr<Room> getRoom(const std::string& roomId);
    void removeRoom(const std::string& roomId);
    
    // 玩家房间查询
    std::shared_ptr<Room> getPlayerRoom(const std::string& playerId);
    
    // 玩家管理辅助方法
    bool removePlayerFromWaitQueue(const std::string& playerId);
    bool removePlayerFromRoom(const std::string& playerId);

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
    ~RoomManager() = default;
    
    // 禁用拷贝
    RoomManager(const RoomManager&) = delete;
    RoomManager& operator=(const RoomManager&) = delete;
    
    // 匹配逻辑
    void processMatching();
    std::shared_ptr<Room> createRoom();
    // std::string generateRoomId();
    
    // 清理逻辑
    void cleanupDisconnectedPlayers();
    
    // 数据成员
    std::vector<std::shared_ptr<Player>> wait_rooms_; // 等待匹配的玩家队列
    std::unordered_map<int32_t, std::shared_ptr<Room>> rooms_; // 房间ID到房间对象的映射

    // 线程安全
    mutable std::mutex playersMutex_;
    mutable std::mutex roomsMutex_;
    mutable std::mutex matchingMutex_;
    
    // 配置
    static constexpr int MAX_PLAYERS_PER_ROOM = MAX_PLAYERS;
    
    // 房间ID生成
    std::atomic<int32_t> roomIdCounter_;
};