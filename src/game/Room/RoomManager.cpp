#include "RoomManager.h"
#include <spdlog/spdlog.h>
#include <algorithm>

// 单例模式实现
RoomManager& RoomManager::getInstance() {
    static RoomManager instance;
    return instance;
}

// 玩家加入等待队列
void RoomManager::joinWaitRoom(const std::shared_ptr<Player>& player) {
    std::lock_guard<std::mutex> lock(matchingMutex_);
    
    spdlog::info("Player {} joining wait room", player->GetPlayerId());
    
    // 设置玩家状态
    player->setState(PlayerState::CONNECTED);
    
    // 添加到等待队列
    wait_rooms_.push_back(player);
    
    // 触发匹配处理
    processMatching();
}

// 获取房间
std::shared_ptr<Room> RoomManager::getRoom(const std::string& roomId) {
    std::lock_guard<std::mutex> lock(roomsMutex_);
    
    try {
        int32_t id = std::stoi(roomId);
        auto it = rooms_.find(id);
        return (it != rooms_.end()) ? it->second : nullptr;
    } catch (const std::exception& e) {
        spdlog::error("Invalid room ID format: {}", roomId);
        return nullptr;
    }
}

// 移除房间
void RoomManager::removeRoom(const std::string& roomId) {
    std::lock_guard<std::mutex> lock(roomsMutex_);
    
    try {
        int32_t id = std::stoi(roomId);
        auto it = rooms_.find(id);
        if (it != rooms_.end()) {
            spdlog::info("Removing room {}", roomId);
            
            auto room = it->second;
            
            // 清理房间内玩家的房间ID
            for (auto& player : room->getPlayers()) {
                if (player) {
                    player->SetRoomId("");
                    player->setState(PlayerState::DISCONNECTED);
                }
            }
            
            // 确保房间资源完全清理
            room->cleanupRoom();
            
            // 从容器中移除
            rooms_.erase(it);
            
            spdlog::info("Room {} removed and cleaned up successfully", roomId);
        }
    } catch (const std::exception& e) {
        spdlog::error("Invalid room ID format when removing: {}", roomId);
    }
}

// 获取玩家所在房间
std::shared_ptr<Room> RoomManager::getPlayerRoom(const std::string& playerId) {
    std::lock_guard<std::mutex> lock(roomsMutex_);
    
    // 遍历所有房间查找玩家
    for (const auto& roomPair : rooms_) {
        const auto& room = roomPair.second;
        auto player = room->getPlayer(playerId);
        if (player) {
            return room;
        }
    }
    
    return nullptr;
}

// 获取等待匹配的玩家数量
size_t RoomManager::getMatchingPlayers() const {
    std::lock_guard<std::mutex> lock(matchingMutex_);
    return wait_rooms_.size();
}

// 获取总房间数量
size_t RoomManager::getTotalRooms() const {
    std::lock_guard<std::mutex> lock(roomsMutex_);
    return rooms_.size();
}

// 匹配逻辑处理
void RoomManager::processMatching() {
    // 注意：调用此函数时已经持有 matchingMutex_ 锁
    
    // 如果等待队列中玩家数量足够创建房间
    while (wait_rooms_.size() >= MAX_PLAYERS_PER_ROOM) {
        spdlog::info("Processing matching with {} waiting players", wait_rooms_.size());
        
        // 创建新房间
        auto room = createRoom();
        if (!room) {
            spdlog::error("Failed to create room");
            break;
        }
        
        // 将玩家分配到房间
        for (int i = 0; i < MAX_PLAYERS_PER_ROOM && !wait_rooms_.empty(); ++i) {
            auto player = wait_rooms_.front();
            wait_rooms_.erase(wait_rooms_.begin());
            
            // 设置玩家房间ID
            player->SetRoomId(std::to_string(room->getId()));
            
            // 添加到房间
            if (room->addPlayer(player)) {
                spdlog::info("Player {} assigned to room {}", 
                            player->GetPlayerId(), room->getId());
            } else {
                spdlog::error("Failed to add player {} to room {}", 
                             player->GetPlayerId(), room->getId());
            }
        }
        
        // 检查房间是否满员，如果满员则设置为 FULL 状态
        if (room->isFull()) {
            room->setState(RoomState::FULL);
            spdlog::info("Room {} is full with {} players", 
                        room->getId(), room->getAllPlayerCount());
        }
    }
}

// 创建房间
std::shared_ptr<Room> RoomManager::createRoom() {
    int32_t id = roomIdCounter_.fetch_add(1, std::memory_order_relaxed);
    
    auto room = std::make_shared<Room>(id);
    room->setState(RoomState::WAITING);
    
    {
        std::lock_guard<std::mutex> lock(roomsMutex_);
        rooms_[id] = room;
    }
    
    spdlog::info("Created new room with ID: {}", id);
    return room;
}

// 生成房间ID
// std::string RoomManager::generateRoomId() {
//     int32_t id = roomIdCounter_.fetch_add(1, std::memory_order_relaxed);
//     return std::to_string(id);
// }

// 从等待队列中移除玩家
bool RoomManager::removePlayerFromWaitQueue(const std::string& playerId) {
    std::lock_guard<std::mutex> lock(matchingMutex_);
    
    auto it = std::find_if(wait_rooms_.begin(), wait_rooms_.end(),
        [&playerId](const std::shared_ptr<Player>& player) {
            return player->GetPlayerId() == playerId;
        });
    
    if (it != wait_rooms_.end()) {
        wait_rooms_.erase(it);
        spdlog::info("Player {} removed from wait queue", playerId);
        return true;
    }
    
    return false;
}

// 从房间中移除玩家
bool RoomManager::removePlayerFromRoom(const std::string& playerId) {
    std::lock_guard<std::mutex> lock(roomsMutex_);
    
    for (auto& roomPair : rooms_) {
        auto& room = roomPair.second;
        auto player = room->getPlayer(playerId);
        
        if (player) {
            // 从房间的玩家列表中移除（需要修改Room类来支持移除玩家）
            // 这里由于Room类没有removePlayer方法，我们需要手动操作
            auto& players = const_cast<std::vector<std::shared_ptr<Player>>&>(room->getPlayers());
            auto it = std::find(players.begin(), players.end(), player);
            if (it != players.end()) {
                players.erase(it);
                player->SetRoomId("");
                player->setState(PlayerState::DISCONNECTED);
                
                spdlog::info("Player {} removed from room {}", playerId, roomPair.first);
                
                // 如果房间变空，删除房间
                if (room->isEmpty()) {
                    spdlog::info("Room {} is empty, will be removed", roomPair.first);
                    rooms_.erase(roomPair.first);
                } else {
                    // 更新房间状态
                    room->setState(RoomState::WAITING);
                }
                
                return true;
            }
        }
    }
    
    return false;
}

// 检查房间是否已满
bool RoomManager::isRoomFull(const std::string& roomId) const {
    auto room = const_cast<RoomManager*>(this)->getRoom(roomId);
    return room && room->isFull();
}

// 获取房间中的玩家数量
size_t RoomManager::getRoomPlayerCount(const std::string& roomId) const {
    auto room = const_cast<RoomManager*>(this)->getRoom(roomId);
    return room ? room->getAllPlayerCount() : 0;
}

// 获取所有房间信息（用于调试和监控）
std::vector<std::pair<std::string, size_t>> RoomManager::getAllRoomsInfo() const {
    std::lock_guard<std::mutex> lock(roomsMutex_);
    
    std::vector<std::pair<std::string, size_t>> roomsInfo;
    roomsInfo.reserve(rooms_.size());
    
    for (const auto& roomPair : rooms_) {
        roomsInfo.emplace_back(std::to_string(roomPair.first), 
                              roomPair.second->getAllPlayerCount());
    }
    
    return roomsInfo;
}

// 开始房间内的游戏
bool RoomManager::startGameInRoom(const std::string& roomId) {
    auto room = getRoom(roomId);
    if (!room) {
        spdlog::warn("Room {} not found for game start", roomId);
        return false;
    }
    
    if (!room->isFull()) {
        spdlog::warn("Room {} is not full, cannot start game", roomId);
        return false;
    }
    
    // 设置所有玩家状态为游戏中
    for (const auto& player : room->getPlayers()) {
        player->setState(PlayerState::GAMING);
    }
    
    // 启动游戏
    room->startGame();
    
    spdlog::info("Game started in room {}", roomId);
    return true;
}

// 清理空房间
void RoomManager::cleanupEmptyRooms() {
    std::lock_guard<std::mutex> lock(roomsMutex_);
    
    auto it = rooms_.begin();
    while (it != rooms_.end()) {
        if (it->second->isEmpty()) {
            spdlog::info("Cleaning up empty room {}", it->first);
            it = rooms_.erase(it);
        } else {
            ++it;
        }
    }
}

// 清理已结束的房间
void RoomManager::cleanupFinishedRooms() {
    std::lock_guard<std::mutex> lock(roomsMutex_);
    
    auto it = rooms_.begin();
    while (it != rooms_.end()) {
        if (it->second->getState() == RoomState::FINISHED) {
            spdlog::info("Cleaning up finished room {}", it->first);
            it = rooms_.erase(it);
        } else {
            ++it;
        }
    }
}

// 清理断线玩家
void RoomManager::cleanupDisconnectedPlayers() {
    std::lock_guard<std::mutex> lock(matchingMutex_);
    
    // 清理等待队列中的断线玩家
    auto it = wait_rooms_.begin();
    while (it != wait_rooms_.end()) {
        if ((*it)->getState() == PlayerState::DISCONNECTED) {
            spdlog::info("Removing disconnected player {} from wait queue", 
                        (*it)->GetPlayerId());
            it = wait_rooms_.erase(it);
        } else {
            ++it;
        }
    }
}