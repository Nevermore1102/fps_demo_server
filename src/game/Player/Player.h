#pragma once
#include "net/Connection.h"
#include <cstdint>
#include <memory>
#include <string>

enum class PlayerState {
    CONNECTED,      // 已连接
    GAMING,         // 游戏中
    FINISHED,       // 游戏结束
    DISCONNECTED    // 断线
};

class Player {
public:
    Player(const std::string& player_id, const std::shared_ptr<Connection>& conn)
        : player_id_(player_id), connection_(conn) {}

    ~Player() = default;

    // Setters
    void SetRoomId(const std::string& room_id) { room_id_ = room_id; }
    void SetPlayerId(const std::string& player_id) { player_id_ = player_id; }
    void SetFormationData(const std::string& formation_data) { formation_data_ = formation_data; }
    void SetPlayerImageId(const std::string& player_imageid) { player_imageid_ = player_imageid; }
    void SetConnection(const std::shared_ptr<Connection>& connection) { connection_ = connection; }
    void setState(PlayerState state) { state_ = state; }

    // Getters
    const std::string& GetRoomId() const { return room_id_; }
    const std::string& GetPlayerId() const { return player_id_; }
    const std::string& GetFormationData() const { return formation_data_; }
    const std::string& GetPlayerImageId() const { return player_imageid_; }
    PlayerState getState() const { return state_; }
    std::shared_ptr<Connection> GetConnection() const { return connection_; }

private:
    // 基础信息
    std::string player_id_;
    std::string room_id_;
    PlayerState state_;
    std::shared_ptr<Connection> connection_;

    // 游戏数据
    int32_t honor_value_;
    std::string formation_data_;
    std::string player_imageid_;
};