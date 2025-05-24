#include "PlayerData.h"
#include <spdlog/spdlog.h>

PlayerData::PlayerData(const std::string& playerId) : playerId_(playerId) {
    initState();
}

void PlayerData::initState() {
    state_.health = 100;
    state_.ammo = 30;
    state_.x = 0.0f;
    state_.y = 0.0f;
    state_.z = 0.0f;
    state_.rotation_x = 0.0f;
    state_.rotation_y = 0.0f;
    state_.rotation_z = 0.0f;
    state_.velocity_x = 0.0f;
    state_.velocity_y = 0.0f;
    state_.velocity_z = 0.0f;
    state_.is_grounded = true;
}

bool PlayerData::load() {
    if (playerId_.empty()) {
        spdlog::error("Player ID is empty");
        return false;
    }
    std::string data = Storage::getInstance().loadPlayerData(playerId_);
    if (data.empty()) {
        spdlog::info("No saved data found for player {}", playerId_);
        return false;
    }

    try {
        nlohmann::json json = nlohmann::json::parse(data);
        return fromJson(json);
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse player data for {}: {}", playerId_, e.what());
        return false;
    }
}

bool PlayerData::save() {
    try {
        nlohmann::json json = toJson();
        std::string data = json.dump();
        return Storage::getInstance().savePlayerData(playerId_, data);
    } catch (const std::exception& e) {
        spdlog::error("Failed to save player data for {}: {}", playerId_, e.what());
        return false;
    }
}

void PlayerData::updateHealth(int health) {
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

void PlayerData::updateRotation(float x, float y, float z) {
    state_.rotation_x = x;
    state_.rotation_y = y;
    state_.rotation_z = z;
}

void PlayerData::updateVelocity(float x, float y, float z) {
    state_.velocity_x = x;
    state_.velocity_y = y;
    state_.velocity_z = z;

}

void PlayerData::updateIsGrounded(bool is_grounded) {
    state_.is_grounded = is_grounded;
}

bool PlayerData::fromJson(const nlohmann::json& json) {
    try {
        state_.health = json["health"].get<int>();
        state_.ammo = json["ammo"].get<int>();
        state_.x = json["x"].get<float>();
        state_.y = json["y"].get<float>();
        state_.z = json["z"].get<float>();
        state_.rotation_x = json["rotation_x"].get<float>();
        state_.rotation_y = json["rotation_y"].get<float>();
        state_.rotation_z = json["rotation_z"].get<float>();
        state_.velocity_x = json["velocity_x"].get<float>();
        state_.velocity_y = json["velocity_y"].get<float>();
        state_.velocity_z = json["velocity_z"].get<float>();
        state_.is_grounded = json["is_grounded"].get<bool>();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse player state from JSON: {}", e.what());
        return false;
    }
}

nlohmann::json PlayerData::toJson() const {
    nlohmann::json json;
    json["health"] = state_.health;
    json["ammo"] = state_.ammo;
    json["x"] = state_.x;
    json["y"] = state_.y;
    json["z"] = state_.z;
    json["rotation_x"] = state_.rotation_x;
    json["rotation_y"] = state_.rotation_y;
    json["rotation_z"] = state_.rotation_z;
    json["velocity_x"] = state_.velocity_x;
    json["velocity_y"] = state_.velocity_y;
    json["velocity_z"] = state_.velocity_z;
    json["is_grounded"] = state_.is_grounded;
    return json;
}