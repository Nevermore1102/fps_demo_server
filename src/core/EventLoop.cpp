#include "EventLoop.h"
#include "log/log_macro.h"
#include <iostream>

EventLoop::EventLoop() : base_(nullptr), running_(false) {}

EventLoop::~EventLoop() {
    stop();
}

bool EventLoop::init() {
    base_ = event_base_new();
    if (!base_) {
        std::cerr << "Failed to create event base" << std::endl;
        return false;
    }
    return true;
}

void EventLoop::run() {
    if (!base_) {
        std::cerr << "Event loop not initialized" << std::endl;
        return;
    }
    
    running_ = true;
    std::cout << "Event loop started" << std::endl;
    // event_base_dispatch(base_);
    int result = event_base_dispatch(base_);
    LOG_WARN("Event loop exited with result: {}", result);
    LOG_INFO("Active events when exiting: {}", 
                event_base_get_num_events(base_, EVENT_BASE_COUNT_ACTIVE));
    LOG_INFO("Added events when exiting: {}", 
                event_base_get_num_events(base_, EVENT_BASE_COUNT_ADDED));
}

void EventLoop::stop() {
    if (base_) {
        event_base_loopbreak(base_);
        event_base_free(base_);
        base_ = nullptr;
    }
    running_ = false;
} 