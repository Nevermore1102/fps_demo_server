#include "PlayerData.h"
#include <spdlog/spdlog.h>

PlayerData::PlayerData(const std::string& playerId) 
    : playerId_(playerId) {
    initState();
}

void PlayerData::initState() {
    state_.health = 100;
    state_.ammo = 30;
    state_.x = 0.0f;
    state_.y = 0.0f;
    state_.z = 0.0f;
    state_.rotation = 0.0f;
}

bool PlayerData::load() {
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

void PlayerData::updateRotation(float rotation) {
    state_.rotation = rotation;
}

bool PlayerData::fromJson(const nlohmann::json& json) {
    try {
        state_.health = json["health"].get<int>();
        state_.ammo = json["ammo"].get<int>();
        state_.x = json["x"].get<float>();
        state_.y = json["y"].get<float>();
        state_.z = json["z"].get<float>();
        state_.rotation = json["rotation"].get<float>();
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
    json["rotation"] = state_.rotation;
    return json;
} 