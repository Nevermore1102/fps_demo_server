#pragma once
#include "net/Connection.h"
#include <cstdint>
#include <memory>
#include <string>
#include "log/log_macro.h"

enum class PlayerState {
    CONNECTED,      // 已连接
    GAMING,         // 游戏中
    FINISHED,       // 游戏结束
    DISCONNECTED,   // 断线
    ROBOT           // 机器人
};

class Player {
public:
    Player() = default;
    Player(const std::string& player_id, const std::shared_ptr<Connection>& conn)
        : player_id_(player_id), connection_(conn) {
            latest_snapshot_ = std::make_shared<PlayerSnapshot>();
            latest_snapshot_->set_player_id(player_id);
            latest_snapshot_->set_honor_value(0);  // 初始荣耀值为0
            latest_snapshot_->set_round(0);  // 初始轮数为0
            state_ = PlayerState::CONNECTED;
            player_name_ = "Unknown";  // 默认玩家名
            icon_id_ = 0;  // 默认图标ID
            room_id_ = "";  // 初始房间ID为空
         }

    ~Player() = default;

    // Setters
    void SetRoomId(const std::string& room_id) { room_id_ = room_id; }
    void SetPlayerId(const std::string& player_id) { player_id_ = player_id; }
    void SetFormationData(const std::string& formation_data) { latest_snapshot_->set_formation_data(formation_data); }
    void SetConnection(const std::shared_ptr<Connection>& connection) { connection_ = connection; }
    void setState(PlayerState state) { state_ = state; }
    void SetHonorValue(int32_t honor_value) { latest_snapshot_->set_honor_value(honor_value); }
    void SetPlayerName(const std::string& player_name) { player_name_ = player_name; }
    void SetIconId(int32_t icon_id) { icon_id_ = icon_id; }
    void SetLatestSnapshot(const std::shared_ptr<PlayerSnapshot>& snapshot) { latest_snapshot_ = snapshot; }
    void SetRound(int32_t round) { latest_snapshot_->set_round(round); }

    // Getters
    const std::string& GetRoomId() const { return room_id_; }
    const std::string& GetPlayerId() const { return player_id_; }
    const std::string& GetFormationData() const { return latest_snapshot_->formation_data(); }
    PlayerState getState() const { return state_; }
    std::shared_ptr<Connection> GetConnection() const { return connection_; }
    int32_t GetHonorValue() const { return latest_snapshot_->honor_value(); }
    std::string GetFormationData() { return latest_snapshot_->formation_data(); }
    const std::string& GetPlayerName() const { return player_name_; }
    int32_t GetIconId() const { return icon_id_; }
    const std::shared_ptr<PlayerSnapshot>& GetLatestSnapshot() const { return latest_snapshot_; }
    bool isRobot() const { return state_ == PlayerState::ROBOT; }

    // 服务器给玩家发送消息
    bool sendMessage(const NetworkMessage& msg) const {
        Message body;
        body.setBodyFromProto(msg);
        if (connection_) {
            return connection_->sendMessage(body);
        }
        return false;  // 如果连接不存在，发送失败
    }

private:
    // 基础信息
    std::string player_id_;
    std::string player_name_;
    int32_t icon_id_;
    std::string room_id_;
    PlayerState state_;
    std::shared_ptr<Connection> connection_;

    // 最新的游戏快照
    std::shared_ptr<PlayerSnapshot> latest_snapshot_;
};