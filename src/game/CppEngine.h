/**
 * @file CppEngine.h
 * @brief C++直接业务逻辑处理模块
 * 
 * 该模块负责：
 * - 处理C++业务逻辑
 * @author Nevermore1102
 * @date 2025-05-18
 */


#pragma once
#include <memory>
#include "proto/Message.h"
#include "net/Connection.h"

class CppEngine {
public:
    CppEngine();
    ~CppEngine() = default;

    // 处理消息
    bool handleMessage(const std::shared_ptr<Connection>& conn, const Message& msg);

    // 可扩展的C++处理函数
    void onHeartbeat(const std::shared_ptr<Connection>& conn, const Message& msg);
    void onLogin(const std::shared_ptr<Connection>& conn, const Message& msg);
    // ... 其他C++处理函数可在此声明 ...
}; 