#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "Storage.h"

struct PlayerState {
    int health;
    int ammo;
    float x, y, z;  // 位置
    float rotation_x, rotation_y, rotation_z;  // 旋转
    float velocity_x, velocity_y, velocity_z;  // 速度
    bool is_grounded;  // 是否在地面上
};

class PlayerData {
public:
    PlayerData(const std::string& playerId);
    
    // 数据操作
    bool load();
    bool save();
    
    // 状态更新
    void updateHealth(int health);
    void updateAmmo(int ammo);
    void updatePosition(float x, float y, float z);
    void updateRotation(float x, float y, float z);
    void updateVelocity(float x, float y, float z);
    void updateIsGrounded(bool is_grounded);
    
    // 状态获取
    const PlayerState& getState() const { return state_; }
    const std::string& getPlayerId() const { return playerId_; }
    
private:
    void initState();
    bool fromJson(const nlohmann::json& json);
    nlohmann::json toJson() const;

    std::string playerId_;
    PlayerState state_;
}; 