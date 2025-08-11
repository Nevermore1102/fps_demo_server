#include "RoomManager.h"
#include "game/Player/Player.h"
#include "proto/Message.h"
#include "proto/NetworkMessage.pb.h"
#include <cstdlib>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <string>

// 单例模式实现
RoomManager& RoomManager::getInstance() {
    static RoomManager instance;
    return instance;
}

// 析构函数确保线程安全退出
RoomManager::~RoomManager() {
    stopMatchingCountdown();
}

void RoomManager::broadcastwaitRoom(const NetworkMessage& msg) {
    Message body;
    body.setBodyFromProto(msg);

    for (auto playerx : wait_rooms_) {
        auto conn = playerx->GetConnection();
        if (conn) {
            bool success = conn->sendMessage(body);
            if (!success)
                LOG_ERROR("Failed to send message to player: {}", playerx->GetPlayerId());
            else{
                LOG_INFO("Message sent to player: {}", playerx->GetPlayerId());
                // body.logMessage();  // 打印消息详情
            }
        }
    }
}

// 玩家加入等待队列
void RoomManager::joinWaitRoom(const std::shared_ptr<Player>& player) {
    std::lock_guard<std::mutex> lock(matchingMutex_);
    
    LOG_INFO("Player {} joining wait room", player->GetPlayerId());
    
    // 设置玩家状态
    player->setState(PlayerState::CONNECTED);
    
    // 添加连接映射
    auto conn = player->GetConnection();
    if (conn) {
        addPlayerConnection(conn, player);
    }
    
    // 检查是否是第一个玩家加入
    bool was_empty = wait_rooms_.empty();
    
    // 添加到等待队列
    wait_rooms_.push_back(player);

    // 如果是第一个玩家，启动匹配倒计时
    if (was_empty) {
        LOG_INFO("First player joined, starting matching countdown");
        startMatchingCountdown();
    }

    NetworkMessage msg;
    msg.set_msg_id(MessageType::WAITING_PLAYER);

    WaitingPlayerMessage* msg_waiting = msg.mutable_waiting_player();
    msg_waiting->set_waiting_player_count(wait_rooms_.size());
    msg_waiting->set_room_capacity(MAX_PLAYERS_PER_ROOM);

    broadcastwaitRoom(msg);
    
    // 检查是否达到满员条件
    if (wait_rooms_.size() >= MAX_PLAYERS_PER_ROOM) {
        LOG_INFO("Room capacity reached, stopping countdown and creating room immediately");
        stopMatchingCountdown();
        processMatching();
    }
}

// 获取房间
std::shared_ptr<Room> RoomManager::getRoom(const std::string& roomId) {
    std::lock_guard<std::mutex> lock(roomsMutex_);
    
    try {
        int32_t id = std::stoi(roomId);
        auto it = rooms_.find(id);
        return (it != rooms_.end()) ? it->second : nullptr;
    } catch (const std::exception& e) {
        LOG_ERROR("Invalid room ID format: {}", roomId);
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
            LOG_INFO("Removing room {}", roomId);
            
            auto room = it->second;
            
            // 清理房间内玩家的房间ID
            for (auto& player : room->getPlayers()) {
                if (player && player->getState() == PlayerState::DISCONNECTED) {
                    player->SetRoomId("");
                    // player->setState(PlayerState::DISCONNECTED);
                    auto conn = player->GetConnection();
                    Connections_player_.erase(conn);
                }
            }
            
            // 确保房间资源完全清理
            room->cleanupRoom();
            
            // 从容器中移除
            rooms_.erase(it);
            
            LOG_INFO("Room {} removed and cleaned up successfully", roomId);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Invalid room ID format when removing: {}", roomId);
    }
}

// 获取玩家所在房间
// std::shared_ptr<Room> RoomManager::getPlayerRoom(const std::string& playerId) {
//     std::lock_guard<std::mutex> lock(roomsMutex_);
    
//     // 遍历所有房间查找玩家
//     for (const auto& roomPair : rooms_) {
//         const auto& room = roomPair.second;
//         auto player = room->getPlayer(playerId);
//         if (player) {
//             return room;
//         }
//     }
    
//     return nullptr;
// }

// 获取玩家所在房间
std::shared_ptr<Room> RoomManager::getPlayerRoom(const std::shared_ptr<Player>& player) {
    std::lock_guard<std::mutex> lock(roomsMutex_);
    
    // 遍历所有房间查找玩家
    for (const auto& roomPair : rooms_) {
        const auto& room = roomPair.second;
        if (room->isPlayerInRoom(player)) {
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
    
    // 如果等待队列中玩家数量足够创建满员房间，立即创建
    while (wait_rooms_.size() >= MAX_PLAYERS_PER_ROOM) {
        LOG_INFO("Processing matching with {} waiting players (capacity reached)", wait_rooms_.size());
        
        // 创建新房间
        auto room = createRoom();
        if (!room) {
            LOG_ERROR("Failed to create room");
            break;
        }
        
        // 将满员数量的玩家分配到房间
        for (int i = 0; i < MAX_PLAYERS_PER_ROOM && !wait_rooms_.empty(); ++i) {
            auto player = wait_rooms_.front();
            wait_rooms_.erase(wait_rooms_.begin());
            
            // 设置玩家房间ID
            player->SetRoomId(std::to_string(room->getId()));
            
            // 添加到房间
            if (room->addPlayer(player)) {
                LOG_INFO("Player {} assigned to room {}", 
                            player->GetPlayerId(), room->getId());
            } else {
                LOG_ERROR("Failed to add player {} to room {}", 
                             player->GetPlayerId(), room->getId());
            }
        }
        
        // 满员房间设置为FULL状态
        room->setState(RoomState::FULL);
        LOG_INFO("Room {} is full with {} players", 
                    room->getId(), room->getAllPlayerCount());
        
        // 更新等待队列广播
        // NetworkMessage msg;
        // msg.set_msg_id(MessageType::WAITING_PLAYER);
        // WaitingPlayerMessage* msg_waiting = msg.mutable_waiting_player();
        // msg_waiting->set_waiting_player_count(wait_rooms_.size());
        // msg_waiting->set_room_capacity(MAX_PLAYERS_PER_ROOM);
        // broadcastwaitRoom(msg);
    }
    
    // 处理剩余玩家：如果还有玩家但不足满员，检查是否需要重新启动倒计时
    // if (!wait_rooms_.empty() && !has_first_player_joined_) {
    //     LOG_INFO("Remaining {} players in queue, restarting countdown", wait_rooms_.size());
    //     startMatchingCountdown();
    // }
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
    
    LOG_INFO("Created new room with ID: {}", id);
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
        // 移除连接映射
        auto conn = (*it)->GetConnection();
        if (conn) {
            removePlayerConnection(conn);
        }
        
        wait_rooms_.erase(it);
        
        // 如果队列变空，停止倒计时
        if (wait_rooms_.empty()) {
            LOG_INFO("Wait queue is empty, stopping matching countdown");
            stopMatchingCountdown();
            return true;
        }
        
        NetworkMessage msg;
        msg.set_msg_id(MessageType::WAITING_PLAYER);
    
        WaitingPlayerMessage* msg_waiting = msg.mutable_waiting_player();
        msg_waiting->set_waiting_player_count(wait_rooms_.size());
        msg_waiting->set_room_capacity(MAX_PLAYERS_PER_ROOM);
        broadcastwaitRoom(msg);
        
        LOG_INFO("Player {} removed from wait queue, remaining: {}", playerId, wait_rooms_.size());
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
            // 移除连接映射
            auto conn = player->GetConnection();
            if (conn) {
                removePlayerConnection(conn);
            }
            
            // 从房间的玩家列表中移除（需要修改Room类来支持移除玩家）
            // 这里由于Room类没有removePlayer方法，我们需要手动操作
            auto& players = const_cast<std::vector<std::shared_ptr<Player>>&>(room->getPlayers());
            auto it = std::find(players.begin(), players.end(), player);
            if (it != players.end()) {
                players.erase(it);
                player->SetRoomId("");
                player->setState(PlayerState::DISCONNECTED);
                
                LOG_INFO("Player {} removed from room {}", playerId, roomPair.first);
                
                // 如果房间变空，删除房间
                if (room->isEmpty()) {
                    LOG_INFO("Room {} is empty, will be removed", roomPair.first);
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
        LOG_WARN("Room {} not found for game start", roomId);
        return false;
    }
    
    if (!room->isFull()) {
        LOG_WARN("Room {} is not full, cannot start game", roomId);
        return false;
    }
    
    // 设置所有真人玩家状态为游戏中
    for (const auto& player : room->getPlayers()) {
        if(!player->isRobot())
            player->setState(PlayerState::GAMING);
    }

    // 生成随机玩家名字和头像id
    room->generateRandomPlayerNames();
    room->generateRandomPlayerIcons();

    // 启动游戏（先广播玩家信息，等待所有客户端渲染完毕信息后的消息再开始）
    // room->startGame();
    room->broadcastPlayerInfo();
    
    LOG_INFO("Game started in room {}", roomId);
    return true;
}

// 清理空房间
void RoomManager::cleanupEmptyRooms() {
    std::lock_guard<std::mutex> lock(roomsMutex_);
    
    auto it = rooms_.begin();
    while (it != rooms_.end()) {
        if (it->second->isEmpty()) {
            LOG_INFO("Cleaning up empty room {}", it->first);
            it = rooms_.erase(it);
        } else {
            ++it;
        }
    }
}

// 清理已结束的房间
// void RoomManager::cleanupFinishedRooms() {
//     std::lock_guard<std::mutex> lock(roomsMutex_);
    
//     auto it = rooms_.begin();
//     while (it != rooms_.end()) {
//         if (it->second->getState() == RoomState::FINISHED) {
//             LOG_INFO("Cleaning up finished room {}", it->first);
//             it = rooms_.erase(it);
//         } else {
//             ++it;
//         }
//     }
// }

// 清理已结束的房间
void RoomManager::cleanupFinishedRooms() {
    std::vector<std::string> roomsToRemove;
    
    {
        std::lock_guard<std::mutex> lock(roomsMutex_);
        for (const auto& roomPair : rooms_) {
            if (roomPair.second->getState() == RoomState::FINISHED) {
                roomsToRemove.push_back(std::to_string(roomPair.first));
            }
        }
    }
    
    // 使用安全删除方法
    for (const auto& roomId : roomsToRemove) {
        removeRoom(roomId);
    }
}

// 清理断线玩家
void RoomManager::cleanupDisconnectedPlayers() {
    std::lock_guard<std::mutex> lock(matchingMutex_);
    
    // 清理等待队列中的断线玩家
    auto it = wait_rooms_.begin();
    while (it != wait_rooms_.end()) {
        if ((*it)->getState() == PlayerState::DISCONNECTED) {
            LOG_INFO("Removing disconnected player {} from wait queue", 
                        (*it)->GetPlayerId());
            it = wait_rooms_.erase(it);
        } else {
            ++it;
        }
    }
}

// 启动匹配倒计时
void RoomManager::startMatchingCountdown() {
    // 停止之前的倒计时（如果有）
    stopMatchingCountdown();
    
    matching_timer_running_ = true;
    stop_signal_ = std::promise<void>();
    auto future = stop_signal_.get_future();
    first_player_join_time_ = std::chrono::steady_clock::now();
    has_first_player_joined_ = true;
    
    matching_timer_thread_ = std::thread([this, future = std::move(future)]() {
        LOG_INFO("Matching countdown started - waiting {} seconds", MATCHING_COUNTDOWN_SECONDS);
        
        // 等待超时或停止信号
        if (future.wait_for(std::chrono::seconds(MATCHING_COUNTDOWN_SECONDS)) 
            == std::future_status::timeout) {
            // 超时，正常结束
            if (matching_timer_running_) {
                LOG_INFO("Matching countdown finished, creating room with current players");
                onMatchingCountdownFinished();
            }
        } else {
            // 收到停止信号
            LOG_INFO("Matching countdown was stopped");
        }
    });
}

// 停止匹配倒计时
void RoomManager::stopMatchingCountdown() {
    if (matching_timer_running_) {
        matching_timer_running_ = false;
        stop_signal_.set_value();  // 立即唤醒
        has_first_player_joined_ = false;
        
        LOG_INFO("Matching countdown stopped");
    }
    if (matching_timer_thread_.joinable()) {
        matching_timer_thread_.join();
    }
}

// 匹配倒计时结束处理
void RoomManager::onMatchingCountdownFinished() {
    std::lock_guard<std::mutex> lock(matchingMutex_);
    
    if (!matching_timer_running_) {
        // 倒计时已被取消
        return;
    }
    
    matching_timer_running_ = false;
    has_first_player_joined_ = false;
    
    if (!wait_rooms_.empty()) {
        LOG_INFO("Countdown finished - creating room with {} players (capacity: {})", 
                    wait_rooms_.size(), MAX_PLAYERS_PER_ROOM);
        
        // 不管人数多少，直接创建房间
        createRoomWithCurrentPlayers();
    } else {
        LOG_INFO("No players in queue when countdown finished");
    }
}

// 用当前队列中的玩家创建房间
void RoomManager::createRoomWithCurrentPlayers() {
    if (wait_rooms_.empty()) {
        LOG_WARN("Cannot create room - no players in queue");
        return;
    }
    
    // 创建新房间
    auto room = createRoom();
    if (!room) {
        LOG_ERROR("Failed to create room");
        return;
    }
    
    // 更新等待队列广播
    NetworkMessage msg;
    msg.set_msg_id(MessageType::WAITING_PLAYER);
    WaitingPlayerMessage* msg_waiting = msg.mutable_waiting_player();
    msg_waiting->set_waiting_player_count(MAX_PLAYERS_PER_ROOM); // 队列已清空
    msg_waiting->set_room_capacity(MAX_PLAYERS_PER_ROOM);
    broadcastwaitRoom(msg);

    // 将等待队列中的所有玩家分配到房间
    int players_added = 0;
    while (!wait_rooms_.empty()) {
        auto player = wait_rooms_.front();
        wait_rooms_.erase(wait_rooms_.begin());
        
        // 设置玩家房间ID
        player->SetRoomId(std::to_string(room->getId()));
        
        // 添加到房间
        if (room->addPlayer(player)) {
            players_added++;
            LOG_INFO("Player {} assigned to room {} (forced by countdown)", 
                        player->GetPlayerId(), room->getId());
        } else {
            LOG_ERROR("Failed to add player {} to room {}", 
                         player->GetPlayerId(), room->getId());
        }
    }

    // 检查是否需要添加机器人
    int robot_nums = MAX_PLAYERS_PER_ROOM - players_added;
    int robot_index = 0;
    while (robot_nums--) {
        auto robot = std::make_shared<Player>();
        robot->SetPlayerId(std::to_string(room->getId()) + "_robot_" + std::to_string(robot_index++));
        robot->SetRoomId(std::to_string(room->getId()));
        robot->setState(PlayerState::ROBOT);
        robot->SetFormationData(std::to_string(room->get_robot_formation_data()));
        // robot->SetIconId(robot_nums);
        // robot->SetPlayerName(robot_names_[robot_nums]);
        room->addPlayer(robot);
    }
    
    // 设置房间状态
    if (room->isFull()) {
        room->setState(RoomState::FULL);
        LOG_INFO("Room {} is full with {} players", room->getId(), players_added);
    } else {
        room->setState(RoomState::WAITING);
        LOG_INFO("Room {} created with {} players (not full)", room->getId(), players_added);
    }
    
    LOG_INFO("Room {} created by countdown with {} players", room->getId(), players_added);

    // 尝试开始游戏
    if (room) {
        LOG_INFO("Room {} is full, attempting to start game", room->getId());
        
        // 尝试开始游戏
        if (startGameInRoom(std::to_string(room->getId()))) {
            LOG_INFO("Game successfully started in room {}", room->getId());
        } else {
            LOG_WARN("Failed to start game in room {}", room->getId());
        }
    } 
}

// 添加连接-玩家映射
void RoomManager::addPlayerConnection(const std::shared_ptr<Connection>& conn, const std::shared_ptr<Player>& player) {
    std::lock_guard<std::mutex> lock(playersMutex_);
    
    if (!conn || !player) {
        LOG_ERROR("Invalid connection or player pointer");
        return;
    }
    
    // 检查连接是否已存在
    auto it = Connections_player_.find(conn);
    if (it != Connections_player_.end()) {
        LOG_WARN("Connection {} already exists, updating player mapping", conn->getId());
    }
    
    Connections_player_[conn] = player;
    LOG_INFO("Added connection mapping: {} -> {}", conn->getId(), player->GetPlayerId());
}

// 移除连接-玩家映射
void RoomManager::removePlayerConnection(const std::shared_ptr<Connection>& conn) {
    std::lock_guard<std::mutex> lock(playersMutex_);
    
    if (!conn) {
        LOG_ERROR("Invalid connection pointer");
        return;
    }
    
    auto it = Connections_player_.find(conn);
    if (it != Connections_player_.end()) {
        LOG_INFO("Removing connection mapping: {} -> {}", 
                    conn->getId(), it->second->GetPlayerId());
        Connections_player_.erase(it);
    } else {
        LOG_WARN("Connection {} not found in mapping", conn->getId());
    }
}

// 通过连接获取玩家
std::shared_ptr<Player> RoomManager::getPlayerByConnection(const std::shared_ptr<Connection>& conn) {
    std::lock_guard<std::mutex> lock(playersMutex_);
    
    if (!conn) {
        LOG_ERROR("Invalid connection pointer");
        return nullptr;
    }
    
    auto it = Connections_player_.find(conn);
    if (it != Connections_player_.end()) {
        return it->second;
    }
    
    LOG_DEBUG("No player found for connection {}", conn->getId());
    return nullptr;
}

// 通过玩家ID获取连接
std::shared_ptr<Connection> RoomManager::getConnectionByPlayerId(const std::string& playerId) {
    std::lock_guard<std::mutex> lock(playersMutex_);
    
    if (playerId.empty()) {
        LOG_ERROR("Empty player ID");
        return nullptr;
    }
    
    for (const auto& pair : Connections_player_) {
        if (pair.second && pair.second->GetPlayerId() == playerId) {
            return pair.first;
        }
    }
    
    LOG_DEBUG("No connection found for player {}", playerId);
    return nullptr;
}

// 检查连接是否活跃
bool RoomManager::isConnectionActive(const std::shared_ptr<Connection>& conn) const {
    std::lock_guard<std::mutex> lock(playersMutex_);
    
    if (!conn) {
        return false;
    }
    
    auto it = Connections_player_.find(conn);
    if (it != Connections_player_.end()) {
        // 检查连接是否仍然有效
        return conn->isConnected() && it->second->getState() != PlayerState::DISCONNECTED;
    }
    
    return false;
}

// 清理无效连接
void RoomManager::cleanupInactiveConnections() {
    std::lock_guard<std::mutex> lock(playersMutex_);
    
    auto it = Connections_player_.begin();
    while (it != Connections_player_.end()) {
        const auto& conn = it->first;
        const auto& player = it->second;
        
        // 检查连接是否无效或玩家已断线
        if (!conn || !player || !conn->isConnected() || player->getState() == PlayerState::DISCONNECTED) {
            LOG_INFO("Cleaning up inactive connection: {} -> {}", 
                        conn ? conn->getId() : "null", 
                        player ? player->GetPlayerId() : "null");
            
            // 如果玩家存在，设置为断线状态
            if (player) {
                player->setState(PlayerState::DISCONNECTED);
            }
            
            it = Connections_player_.erase(it);
        } else {
            ++it;
        }
    }
}

// 处理连接断开
void RoomManager::handleConnectionDisconnect(const std::shared_ptr<Connection>& conn) {
    if (!conn) {
        LOG_ERROR("Invalid connection pointer in handleConnectionDisconnect");
        return;
    }
    
    LOG_INFO("Handling connection disconnect: {}", conn->getId());
    
    // 获取对应的玩家
    auto player = getPlayerByConnection(conn);
    if (!player) {
        LOG_WARN("No player found for disconnected connection: {}", conn->getId());
        // 仍然从连接池中移除
        removePlayerConnection(conn);
        return;
    }
    
    // 使用 use_count 检查 shared_ptr 是否有效
    if (player.use_count() == 0) {
        LOG_ERROR("Player object has been destroyed");
        return;
    }

    std::string playerId = player->GetPlayerId();
    // PlayerState currentState = player->getState();
    PlayerState currentState;
    try {
        currentState = player->getState();
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to get player state: {}", e.what());
        return;
    }
    
    LOG_INFO("Player {} (state: {}) disconnected", playerId, static_cast<int>(currentState));
    
    // 设置玩家状态为断线
    // player->setState(PlayerState::DISCONNECTED);
    
    // 根据玩家当前状态进行不同处理
    switch (currentState) {
        case PlayerState::CONNECTED: {
            // 玩家在等待匹配状态，从等待队列中移除

            player->setState(PlayerState::DISCONNECTED);
            if (removePlayerFromWaitQueue(playerId)) {
                LOG_INFO("Player {} removed from wait queue due to disconnect", playerId);
            }
            break;
        }
        
        case PlayerState::GAMING: {
            // 玩家在游戏中，需要通知房间内其他玩家
            // auto room = getPlayerRoom(playerId);
            // player->setState(PlayerState::ROBOT);
            // player->SetConnection(nullptr);
            
            auto room = getPlayerRoom(player);
            room->deletePlayer(player->GetPlayerId()) ;
            if (room) {
                LOG_INFO("Notifying room {} about player {} disconnect", room->getId(), playerId);
                
                // room->stopCountdownTimer();
                // 房间处理玩家退出并广播消息
                room->onPlayerExit(playerId, room->getCurrentRound(), player->GetHonorValue());
                room->broadcastExitMessage();
                
                // 从房间中移除玩家
                // removePlayerFromRoom(playerId);
                
                // 检查房间是否需要结束或继续游戏
                if (room->getGamingPlayerCount() == 0) {
                    LOG_INFO("Room {} is empty after disconnect, marking as finished", room->getId());
                    room->setState(RoomState::FINISHED);
                    removeRoom(std::to_string(room->getId()));
                } else {
                    LOG_INFO("Room {} continues with {} players", room->getId(), room->getGamingPlayerCount());
                    // 可以在这里添加其他逻辑，比如暂停游戏等
                }
            }
            break;
        }
        
        case PlayerState::FINISHED: {
            // 游戏已结束，只需要清理
            // removePlayerFromRoom(playerId);
            LOG_INFO("Player {} disconnected after game finished", playerId);
            break;
        }
        
        case PlayerState::DISCONNECTED: {
            // 已经是断线状态，只需要清理
            LOG_INFO("Player {} was already disconnected", playerId);
            break;
        }
        case ::PlayerState::ROBOT: {  
            break;  
        }
    }
    
    // 移除连接映射
    removePlayerConnection(conn);
    
    LOG_INFO("Connection disconnect handling completed for player {}", playerId);
}
