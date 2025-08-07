// Timer.h
#pragma once
#include <functional>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

class Timer {
public:
    Timer() : active_(false) {}

    ~Timer() {
        stop();
    }

    // 启动定时器，delay毫秒后触发cb。可重复启动
    void start(uint32_t delay_ms, std::function<void()> cb) {
        stop();
        active_ = true;
        thread_ = std::thread([this, delay_ms, cb]() {
            std::unique_lock<std::mutex> lck(m_);
            if(cv_.wait_for(lck, std::chrono::milliseconds(delay_ms), [this](){ return !active_; }))
                return; // 提前被stop
            if (active_ && cb) cb();
        });
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lck(m_);
            active_ = false;
        }
        cv_.notify_one();
        if(thread_.joinable())
        {
            if(thread_.get_id() == std::this_thread::get_id()) {
                // 如果是当前线程，直接返回
                return;
            }
            else thread_.join();
        }
    }

private:
    std::atomic<bool> active_;
    std::thread thread_;
    std::mutex m_;
    std::condition_variable cv_;
};