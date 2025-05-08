#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

struct PlayerState {
    float health;
    int ammo;
    float x, y, z;  // 位置
    float rotation; // 朝向
};

class PlayerData {
public:
    PlayerData(const std::string& playerId);
    
    // 数据操作
    bool load();
    bool save();
    
    // 状态更新
    void updateHealth(float health);
    void updateAmmo(int ammo);
    void updatePosition(float x, float y, float z);
    void updateRotation(float rotation);
    
    // 状态获取
    const PlayerState& getState() const { return state_; }
    const std::string& getPlayerId() const { return playerId_; }
    
private:
    std::string playerId_;
    PlayerState state_;
    nlohmann::json data_;
    
    static constexpr const char* SAVE_DIR = "save/players/";
}; 