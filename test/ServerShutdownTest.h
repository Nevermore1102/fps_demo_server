#pragma once
#include "TestBase.h"
#include <thread>
#include <chrono>
#include <iostream>

class ServerShutdownTest : public TestBase {
public:
    ServerShutdownTest(const std::string& host, uint16_t port)
        : TestBase("Server Shutdown Test")
        , client_(host, port, this)
        , heartbeat_received_(false)
        , disconnected_(false) {}

    bool run() override {
        if (!client_.connect()) {
            return false;
        }

        // 发送心跳消息
        Message heartbeat(MessageType::HEARTBEAT);
        client_.sendMessage(heartbeat);

        // 等待心跳响应
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 运行事件循环，直到断开连接
        while (!disconnected_) {
            client_.runOnce();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // 确保客户端正确清理
        client_.cleanup();

        return disconnected_;
    }

    void sendMessage(const Message& msg) override {
        client_.sendMessage(msg);
    }

private:
    class TestClient : public TestClientBase {
    public:
        TestClient(const std::string& host, uint16_t port, ServerShutdownTest* test)
            : TestClientBase(host, port), test_(test) {}

    protected:
        void onMessageReceived(const Message& msg) override {
            std::cout << "Received message type: " << static_cast<int>(msg.getType()) << std::endl;
            
            if (msg.getType() == MessageType::HEARTBEAT) {
                test_->heartbeat_received_ = true;
            }
        }

        void onConnected() override {
            std::cout << "Connected to server" << std::endl;
        }

        void onDisconnected() override {
            std::cout << "Disconnected from server" << std::endl;
            test_->disconnected_ = true;
        }

        void onError(const std::string& error) override {
            std::cerr << "Error: " << error << std::endl;
        }

    private:
        ServerShutdownTest* test_;
    };

    TestClient client_;
    bool heartbeat_received_;
    bool disconnected_;

    friend class TestClient;
}; 