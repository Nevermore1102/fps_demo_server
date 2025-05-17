#include "CppEngine.h"
#include <spdlog/spdlog.h>

CppEngine::CppEngine() {
    // 可在此初始化需要的成员
}

bool CppEngine::handleMessage(const std::shared_ptr<Connection>& conn, const Message& msg) {
    switch (msg.getType()) {
        case MessageType::HEARTBEAT:
            onHeartbeat(conn, msg);
            return true;
        case MessageType::LOGIN:
            onLogin(conn, msg);
            return true;
        default:
            return false;
    }
}

void CppEngine::onHeartbeat(const std::shared_ptr<Connection>& conn, const Message& msg) {
    spdlog::info("Sending heartbeat response to {}", conn->getId());
    if (!conn->sendMessage(Message(MessageType::HEARTBEAT))) {
        spdlog::error("Failed to send heartbeat response");
    }
}

void CppEngine::onLogin(const std::shared_ptr<Connection>& conn, const Message& msg) {
    spdlog::info("Sending login response to {}", conn->getId());
    if (!conn->sendMessage(Message(MessageType::LOGIN))) {
        spdlog::error("Failed to send login response");
    }
} 