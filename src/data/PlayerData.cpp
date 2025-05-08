#include "data/PlayerData.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

PlayerData::PlayerData(const std::string& playerId)
    : playerId_(playerId) {
    // 初始化玩家状态
    state_.health = 100.0f;
    state_.ammo = 30;
    state_.x = 0.0f;
    state_.y = 0.0f;
    state_.z = 0.0f;
    state_.rotation = 0.0f;
}

bool PlayerData::load() {
    try {
        fs::path savePath = SAVE_DIR;
        savePath /= playerId_ + ".json";
        
        if (!fs::exists(savePath)) {
            return false;
        }
        
        std::ifstream file(savePath);
        if (!file.is_open()) {
            return false;
        }
        
        file >> data_;
        
        // 从 JSON 加载数据
        state_.health = data_["health"];
        state_.ammo = data_["ammo"];
        state_.x = data_["position"]["x"];
        state_.y = data_["position"]["y"];
        state_.z = data_["position"]["z"];
        state_.rotation = data_["rotation"];
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool PlayerData::save() {
    try {
        // 准备保存目录
        fs::path savePath = SAVE_DIR;
        fs::create_directories(savePath);
        
        // 更新 JSON 数据
        data_["health"] = state_.health;
        data_["ammo"] = state_.ammo;
        data_["position"]["x"] = state_.x;
        data_["position"]["y"] = state_.y;
        data_["position"]["z"] = state_.z;
        data_["rotation"] = state_.rotation;
        
        // 保存到文件
        savePath /= playerId_ + ".json";
        std::ofstream file(savePath);
        if (!file.is_open()) {
            return false;
        }
        
        file << data_.dump(4);
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void PlayerData::updateHealth(float health) {
    state_.health = health;
}

void PlayerData::updateAmmo(int ammo) {
    state_.ammo = ammo;
}

void PlayerData::updatePosition(float x, float y, float z) {
    state_.x = x;
    state_.y = y;
    state_.z = z;
}

void PlayerData::updateRotation(float rotation) {
    state_.rotation = rotation;
} 