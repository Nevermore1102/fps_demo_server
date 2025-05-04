/**
 * @file EventLoop.h
 * @brief 事件循环系统的核心实现
 * 
 * 该模块基于 libevent 实现事件循环系统，用于处理：
 * - 网络事件（连接、数据收发）
 * - 定时器事件
 * - 信号处理
 * 
 * @author Nevermore1102
 * @date 2025-05-05
 */

#pragma once

#include <event2/event.h>
#include <memory>
#include <string>

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    bool init();
    void run();
    void stop();
    
    struct event_base* getBase() const { return base_; }

private:
    struct event_base* base_;
    bool running_;
}; 