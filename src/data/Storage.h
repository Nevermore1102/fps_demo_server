#pragma once
#include <memory>
#include <string>
#include <sqlite3.h>
#include <spdlog/spdlog.h>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @brief 存储管理类
 * 
 * 负责管理数据库连接和基本操作
 * 使用单例模式确保全局只有一个数据库连接
 */
class Storage {
public:
    static Storage& getInstance() {
        static Storage instance;
        return instance;
    }

    // 使用默认配置初始化
    bool init() {
        return init("game.db");
    }

    bool init(const std::string& dbPath) {
        // 确保数据目录存在
        fs::path dataDir = "data";
        if (!fs::exists(dataDir)) {
            fs::create_directory(dataDir);
        }

        // 构建完整的数据库路径
        fs::path fullPath = dataDir / dbPath;
        
        // 打开数据库连接
        if (sqlite3_open(fullPath.string().c_str(), &db_) != SQLITE_OK) {
            spdlog::error("Failed to open database: {}", sqlite3_errmsg(db_));
            return false;
        }

        // 初始化数据库表
        if (!initTables()) {
            spdlog::error("Failed to initialize tables");
            return false;
        }

        spdlog::info("Database initialized successfully at {}", fullPath.string());
        return true;
    }

    // 保存玩家数据
    bool savePlayerData(const std::string& playerId, const std::string& data) {
        const char* sql = "INSERT OR REPLACE INTO player_data (player_id, data, update_time) "
                         "VALUES (?, ?, CURRENT_TIMESTAMP)";
        
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db_));
            return false;
        }

        // 绑定参数
        if (sqlite3_bind_text(stmt, 1, playerId.c_str(), -1, SQLITE_STATIC) != SQLITE_OK ||
            sqlite3_bind_text(stmt, 2, data.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) {
            spdlog::error("Failed to bind parameters: {}", sqlite3_errmsg(db_));
            sqlite3_finalize(stmt);
            return false;
        }

        // 执行语句
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        if (!success) {
            spdlog::error("Failed to execute statement: {}", sqlite3_errmsg(db_));
        }

        sqlite3_finalize(stmt);
        return success;
    }

    // 加载玩家数据
    std::string loadPlayerData(const std::string& playerId) {
        const char* sql = "SELECT data FROM player_data WHERE player_id = ?";
        
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db_));
            return "";
        }

        // 绑定参数
        if (sqlite3_bind_text(stmt, 1, playerId.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) {
            spdlog::error("Failed to bind parameters: {}", sqlite3_errmsg(db_));
            sqlite3_finalize(stmt);
            return "";
        }

        // 执行查询
        std::string result;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (data) {
                result = data;
            }
        }

        sqlite3_finalize(stmt);
        return result;
    }

private:
    Storage() = default;
    ~Storage() {
        if (db_) {
            sqlite3_close(db_);
        }
    }
    
    // 禁止拷贝
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;

    // 初始化数据库表
    bool initTables() {
        const char* sql = 
            "CREATE TABLE IF NOT EXISTS player_data ("
            "player_id TEXT PRIMARY KEY, "
            "data TEXT NOT NULL, "
            "update_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ")";

        char* errMsg = nullptr;
        if (sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            spdlog::error("Failed to create table: {}", errMsg);
            sqlite3_free(errMsg);
            return false;
        }

        return true;
    }

    sqlite3* db_ = nullptr;
}; 