#pragma once
#include "TestBase.h"
#include <thread>
#include <chrono>
#include <iostream>

class ConnectionTest : public TestBase {
public:
    ConnectionTest(const std::string& host, uint16_t port)
        : TestBase("Connection Test")
        , client_(host, port, this)
        , heartbeat_received_(false)
        , login_received_(false) {}

    bool run() override {
        if (!client_.connect()) {
            return false;
        }

        // 发送心跳消息
        Message heartbeat(MessageType::HEARTBEAT);
        client_.sendMessage(heartbeat);

        // 等待心跳响应
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 发送登录消息
        std::vector<uint8_t> login_data = {'t', 'e', 's', 't'};
        Message login(MessageType::LOGIN, login_data);
        client_.sendMessage(login);

        // 运行事件循环
        client_.run();

        return heartbeat_received_ && login_received_;
    }

    void sendMessage(const Message& msg) override {
        client_.sendMessage(msg);
    }

private:
    class TestClient : public TestClientBase {
    public:
        TestClient(const std::string& host, uint16_t port, ConnectionTest* test)
            : TestClientBase(host, port), test_(test) {}

    protected:
        void onMessageReceived(const Message& msg) override {
            std::cout << "Received message type: " << static_cast<int>(msg.getType()) << std::endl;
            
            switch (msg.getType()) {
                case MessageType::HEARTBEAT:
                    test_->heartbeat_received_ = true;
                    break;
                case MessageType::LOGIN:
                    test_->login_received_ = true;
                    break;
                default:
                    std::cout << "Unknown message type: " << static_cast<int>(msg.getType()) << std::endl;
                    break;
            }
        }

        void onConnected() override {
            std::cout << "Connected to server" << std::endl;
        }

        void onDisconnected() override {
            std::cout << "Disconnected from server" << std::endl;
        }

        void onError(const std::string& error) override {
            std::cerr << "Error: " << error << std::endl;
        }

    private:
        ConnectionTest* test_;
    };

    TestClient client_;
    bool heartbeat_received_;
    bool login_received_;

    friend class TestClient;
};