#pragma once
#include "TestBase.h"
#include <thread>
#include <chrono>
#include <iostream>

class LuaMessageTest : public TestBase {
public:
    LuaMessageTest(const std::string& host, uint16_t port)
        : TestBase("Lua Message Routing Test")
        , client_(host, port, this)
        , player_update_received_(false)
        , player_shoot_received_(false)
        , player_hit_received_(false) {}

    bool run() override {
        if (!client_.connect()) {
            return false;
        }

        std::cout << "\n=== 开始测试 Lua 消息处理 ===\n" << std::endl;

        // 发送玩家更新消息
        std::cout << "发送玩家更新消息..." << std::endl;
        std::vector<uint8_t> update_data = {'p', 'o', 's', '1'};
        Message update_msg(MessageType::PLAYER_UPDATE, update_data);
        client_.sendMessage(update_msg);

        // 等待处理
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 发送玩家射击消息
        std::cout << "\n发送玩家射击消息..." << std::endl;
        std::vector<uint8_t> shoot_data = {'s', 'h', 'o', 't'};
        Message shoot_msg(MessageType::PLAYER_SHOOT, shoot_data);
        client_.sendMessage(shoot_msg);

        // 等待处理
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 发送玩家受击消息
        std::cout << "\n发送玩家受击消息..." << std::endl;
        std::vector<uint8_t> hit_data = {'h', 'i', 't', '1'};
        Message hit_msg(MessageType::PLAYER_HIT, hit_data);
        client_.sendMessage(hit_msg);

        // 运行事件循环
        client_.run();

        // 验证所有消息是否都被Lua处理
        bool success = player_update_received_ && 
                      player_shoot_received_ && 
                      player_hit_received_;

        std::cout << "\n=== 测试结果 ===\n" << std::endl;
        if (success) {
            std::cout << "✓ 所有消息都被Lua成功处理" << std::endl;
        } else {
            std::cout << "✗ 部分消息未被Lua处理:" << std::endl;
            if (!player_update_received_) std::cout << "  - 玩家更新消息" << std::endl;
            if (!player_shoot_received_) std::cout << "  - 玩家射击消息" << std::endl;
            if (!player_hit_received_) std::cout << "  - 玩家受击消息" << std::endl;
        }

        return success;
    }

    void sendMessage(const Message& msg) override {
        client_.sendMessage(msg);
    }

private:
    class TestClient : public TestClientBase {
    public:
        TestClient(const std::string& host, uint16_t port, LuaMessageTest* test)
            : TestClientBase(host, port), test_(test) {}

    protected:
        void onMessageReceived(const Message& msg) override {
            std::cout << "收到服务器响应: 类型=" << static_cast<int>(msg.getType());
            
            // 打印消息体内容
            const auto& body = msg.getBody();
            if (!body.empty()) {
                std::string body_str(body.begin(), body.end());
                std::cout << ", 内容=" << body_str;
            }
            std::cout << std::endl;
            
            switch (msg.getType()) {
                case MessageType::PLAYER_UPDATE:
                    test_->player_update_received_ = true;
                    break;
                case MessageType::PLAYER_SHOOT:
                    test_->player_shoot_received_ = true;
                    break;
                case MessageType::PLAYER_HIT:
                    test_->player_hit_received_ = true;
                    break;
                default:
                    std::cout << "未知消息类型: " << static_cast<int>(msg.getType()) << std::endl;
                    break;
            }
        }

        void onConnected() override {
            std::cout << "已连接到服务器" << std::endl;
        }

        void onDisconnected() override {
            std::cout << "与服务器断开连接" << std::endl;
        }

        void onError(const std::string& error) override {
            std::cerr << "错误: " << error << std::endl;
        }

    private:
        LuaMessageTest* test_;
    };

    TestClient client_;
    bool player_update_received_;
    bool player_shoot_received_;
    bool player_hit_received_;

    friend class TestClient;
}; 