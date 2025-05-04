#include "EventLoop.h"
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
    event_base_dispatch(base_);
}

void EventLoop::stop() {
    if (base_) {
        event_base_loopbreak(base_);
        event_base_free(base_);
        base_ = nullptr;
    }
    running_ = false;
} 