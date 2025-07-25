#pragma once
#include "TestBase.h"
#include <thread>
#include <chrono>
#include <iostream>
#include "proto/NetworkMessage.pb.h"

extern std::string g_playerName;


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
                // 发送心跳消息
        sendHeartbeat();
        
        // 处理心跳响应
        for (int i = 0; i < 10; ++i) {
            client_.runOnce();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (heartbeat_received_) break;
        }

        // 发送开始匹配消息
        std::cout << "发送开始匹配消息..." << std::endl;
        sendStartMatch();   


        // 等待服务器推送匹配开始（模拟等待）
        std::cout << "等待服务器推送匹配开始..." << std::endl;
        for (int i = 0; i < 500 && !match_start_room_; ++i) {
            client_.runOnce();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return match_start_room_;
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
        net_msg.set_player_id("test_player111");
        
        Message msg(MessageType::HEARTBEAT);
        msg.setBodyFromProto(net_msg);
        client_.sendMessage(msg);
        std::cout << "发送心跳消息" << std::endl;
    }


    void sendStartMatch() {
        NetworkMessage net_msg;
        net_msg.set_msg_id(MessageType::START_MATCH);
        HeartbeatMessage* heartbeat = net_msg.mutable_heartbeat();
        (void)heartbeat;
        net_msg.set_player_id(g_playerName);
        
        Message msg(MessageType::START_MATCH);
        msg.setBodyFromProto(net_msg);
        client_.sendMessage(msg);
        std::cout << "开始匹配消息消息，id：" <<net_msg.player_id()<< std::endl;
    }



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
            switch (net_msg.msg_id()) {
                case MessageType::HEARTBEAT:
                    test_->heartbeat_received_ = true;
                    std::cout << "收到心跳响应" << std::endl;
                    break;
                case MessageType::CONNECT:
                    test_->connect_ack_received_ = true;
                    std::cout << "收到连接响应" << std::endl;
                    break;
                case MessageType::START_MATCH:
                    test_->match_start_received_ = true;
                    std::cout << "收到开始匹配请求？？" << std::endl;
                    break;
                case MessageType::GAME_START:
                    test_->match_start_room_ = true;
                    std::cout << "收到游戏开局请求" << std::endl;
                    break;
                default:
                    std::cout << "收到未知类型: " << static_cast<int>(net_msg.msg_id()) << std::endl;
                    break;
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
    bool heartbeat_received_;
    bool connect_ack_received_;
    bool match_start_room_;
    friend class TestClient;
    
}; 