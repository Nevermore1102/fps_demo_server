#include "ConnectionPool.h"
#include <spdlog/spdlog.h>

ConnectionPool& ConnectionPool::getInstance() {
static ConnectionPool instance;
return instance;
}

void ConnectionPool::addConnection(const std::shared_ptr<Connection>& conn) {
if (!conn) {
    return;
}

std::lock_guard<std::mutex> lock(mutex_);
connections_[conn->getId()] = conn;
spdlog::info("New connection added: {}, total connections: {}", 
                conn->getId(), connections_.size());
}

void ConnectionPool::removeConnection(const std::string& id) {
std::lock_guard<std::mutex> lock(mutex_);
if (connections_.erase(id) > 0) {
    spdlog::info("Connection removed: {}, total connections: {}", 
                    id, connections_.size());
}
}

std::shared_ptr<Connection> ConnectionPool::getConnection(const std::string& id) {
std::lock_guard<std::mutex> lock(mutex_);
auto it = connections_.find(id);
if (it != connections_.end()) {
    return it->second;
}
return nullptr;
}

std::vector<std::shared_ptr<Connection>> ConnectionPool::getAllConnections() {
std::lock_guard<std::mutex> lock(mutex_);
std::vector<std::shared_ptr<Connection>> result;
result.reserve(connections_.size());
for (const auto& pair : connections_) {
    result.push_back(pair.second);
}
return result;
}

size_t ConnectionPool::getConnectionCount() const {
std::lock_guard<std::mutex> lock(mutex_);
return connections_.size();
}