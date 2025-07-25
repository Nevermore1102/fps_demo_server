#pragma once
#include "TestBase.h"
#include <thread>
#include <chrono>
#include <iostream>
#include "proto/NetworkMessage.pb.h"

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
        sendHeartbeat();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        while (!disconnected_) {
            client_.runOnce();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        client_.cleanup();
        return disconnected_;
    }
    void sendMessage(const Message& msg) override {
        client_.sendMessage(msg);
    }
private:
    void sendHeartbeat() {
        NetworkMessage net_msg;
        net_msg.set_msg_id(MessageType::HEARTBEAT);
        HeartbeatMessage* heartbeat = net_msg.mutable_heartbeat();
        (void)heartbeat;
        net_msg.set_player_id("test_player");
        Message msg;
        msg.setBodyFromProto(net_msg);
        msg = Message(MessageType::HEARTBEAT, msg.getBody());
        client_.sendMessage(msg);
    }
    class TestClient : public TestClientBase {
    public:
        TestClient(const std::string& host, uint16_t port, ServerShutdownTest* test)
            : TestClientBase(host, port), test_(test) {}
    protected:
        void onMessageReceived(const Message& msg) override {
            NetworkMessage net_msg;
            if (!msg.getBodyAsProto(net_msg)) {
                std::cout << "收到无法解析的protobuf消息" << std::endl;
                return;
            }
            if (net_msg.msg_id() == MessageType::HEARTBEAT) {
                test_->heartbeat_received_ = true;
                std::cout << "收到心跳响应" << std::endl;
            }
        }
        void onConnected() override {
            std::cout << "已连接服务器" << std::endl;
        }
        void onDisconnected() override {
            std::cout << "与服务器断开连接" << std::endl;
            test_->disconnected_ = true;
        }
        void onError(const std::string& error) override {
            std::cerr << "错误: " << error << std::endl;
        }
    private:
        ServerShutdownTest* test_;
    };
    TestClient client_;
    bool heartbeat_received_;
    bool disconnected_;
    friend class TestClient;
}; 