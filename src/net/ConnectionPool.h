#pragma once

#include "Connection.h"
#include <unordered_map>
#include <memory>
#include <mutex>

class ConnectionPool {
public:
    static ConnectionPool& getInstance();

    // 添加新连接
    void addConnection(const std::shared_ptr<Connection>& conn);
    
    // 移除连接
    void removeConnection(const std::string& id);
    
    // 获取连接
    std::shared_ptr<Connection> getConnection(const std::string& id);
    
    // 获取所有连接
    std::vector<std::shared_ptr<Connection>> getAllConnections();
    
    // 获取连接数量
    size_t getConnectionCount() const;

private:
    ConnectionPool() = default;
    ~ConnectionPool() = default;
    
    // 禁止拷贝和赋值
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    std::unordered_map<std::string, std::shared_ptr<Connection>> connections_;
    mutable std::mutex mutex_;
}; 