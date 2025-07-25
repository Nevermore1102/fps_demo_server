#pragma once
#include "TestBase.h"
#include <thread>
#include <chrono>
#include <iostream>
#include "proto/NetworkMessage.pb.h"

class MatchStartTest : public TestBase {
public:
    MatchStartTest(const std::string& host, uint16_t port)
        : TestBase("Match Start Test")
        , client_(host, port, this)
        , match_start_received_(false) {}

    bool run() override {
        if (!client_.connect()) {
            return false;
        }
        // 等待服务器推送匹配开始（模拟等待）
        std::cout << "等待服务器推送匹配开始..." << std::endl;
        for (int i = 0; i < 50 && !match_start_received_; ++i) {
            client_.runOnce();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return match_start_received_;
    }
    void sendMessage(const Message& msg) override {
        client_.sendMessage(msg);
    }
private:
    class TestClient : public TestClientBase {
    public:
        TestClient(const std::string& host, uint16_t port, MatchStartTest* test)
            : TestClientBase(host, port), test_(test) {}
    protected:
        void onMessageReceived(const Message& msg) override {
            NetworkMessage net_msg;
            if (!msg.getBodyAsProto(net_msg)) {
                std::cout << "收到无法解析的protobuf消息" << std::endl;
                return;
            }
            if (net_msg.msg_id() == MessageType::START_MATCH) {
                test_->match_start_received_ = true;
                std::cout << "收到匹配开始消息" << std::endl;
            }
        }
        void onConnected() override {
            std::cout << "已连接服务器" << std::endl;
        }
        void onDisconnected() override {
            std::cout << "与服务器断开连接" << std::endl;
        }
        void onError(const std::string& error) override {
            std::cerr << "错误: " << error << std::endl;
        }
    private:
        MatchStartTest* test_;
    };
    TestClient client_;
    bool match_start_received_;
    friend class TestClient;
}; 