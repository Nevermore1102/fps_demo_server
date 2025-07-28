#pragma once
#include "TestBase.h"
#include <cstdint>
#include <thread>
#include <chrono>
#include <iostream>
#include <type_traits>
#include <unordered_map>
#include "proto/NetworkMessage.pb.h"

extern std::string g_playerName;


class MatchTest : public TestBase {
public:
    MatchTest(const std::string& host, uint16_t port)
        : TestBase("Match Start Test")
        , client_(host, port, this)
        
        , match_start_received_(false) {}

    bool run() override {
        init();
        if (!client_.connect()) {
            return false;
        }
        std::cout<<"开始模拟完整战斗流程..." << std::endl;
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

        //发送N伦逻辑

        while (currTurn<=MaxTurn ) {
            //备战阶段。等待接收BATTLE_PREP_TIMER倒计时广播直至倒计时为0，发送快照消息
            // int lastTime =30;
            std::cout << "等待服务器岛倒计时为0...,currTurn: " <<currTurn<< std::endl;

            for (int i = 0; i < 500 && lastTimeMap_[currTurn]!=0; ++i) {
                client_.runOnce();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            if(lastTimeMap_[currTurn]!=0)
            {
                std::cout<< "倒计时未结束，当前轮次倒计时剩余时间：" << lastTimeMap_[currTurn] << "秒" << std::endl;
                return false;
            }

            
            std::cout << "发送第" << currTurn << "轮快照消息..." << std::endl;
            // 模拟发送快照数据
            sendSnapshot(g_playerName, match_id_, currTurn, "formation_data_test" + std::to_string(currTurn), 100 + currTurn);
            
            // 等待服务器处理快照
            for (int i = 0; i < 200 && hasSnapshotMap_[currTurn]==false; ++i) {
                client_.runOnce();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            if(hasSnapshotMap_[currTurn]==false)
            {
                std::cout<< "快照未收到，当前轮次：" << currTurn << std::endl;
                return false;
            }
            currTurn++;
            std::cout << "结束一个回合：currTurn: " <<currTurn<< std::endl;

        }

        std::cout << "所有轮次快照已发送完成" << std::endl;
        return true;
        // return match_start_room_;
    }
    void sendMessage(const Message& msg) override {
        client_.sendMessage(msg);
    }
private:
    void init()  {
        for(int i=0;i<=MaxTurn;i++){
            lastTimeMap_[i] = 30; // 初始化每轮倒计时为30秒
            hasSnapshotMap_[i]=false; // 初始化每轮快照收到
        }
    }
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

    //send snapshot
    void sendSnapshot(const std::string& playerId, int32_t room_id_, int32_t round, 
                      const std::string& formationData, int32_t honorValue) {
        NetworkMessage net_msg;
        net_msg.set_msg_id(MessageType::BATTLE_PREP_SNAPSHOT);
        BattlePrepSnapshotMessage* snapshot_msg = net_msg.mutable_battle_prep_snapshot();
        net_msg.set_player_id(playerId);
        snapshot_msg->set_match_id(std::to_string(room_id_));
        snapshot_msg->set_round(round);
        snapshot_msg->set_formation_data(formationData);
        snapshot_msg->set_honor_value(honorValue);  
        
        Message msg(MessageType::BATTLE_PREP_SNAPSHOT);
        msg.setBodyFromProto(net_msg);
        client_.sendMessage(msg);

        std::cout<<"发送快照消息，id："<<net_msg.player_id()<<",轮数"<< round<<",阵容数据" <<formationData<< std::endl;
    }

    

    class TestClient : public TestClientBase {
    public:
        TestClient(const std::string& host, uint16_t port, MatchTest* test)
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
                    test_->match_id_ = std::stoi(net_msg.mutable_game_start()->match_id());
                    std::cout << "收到游戏开局请求" << std::endl;
                    break;
                case MessageType::BATTLE_PREP_TIMER:
                    // test_->match_start_room_ = true;
                    test_->lastTimeMap_[net_msg.mutable_battle_prep_timer()->round()] = net_msg.mutable_battle_prep_timer()->remaining_time_seconds();
                    std::cout << "收到倒计时" <<" ,round: "<<net_msg.mutable_battle_prep_timer()->round()<<" ,lasttime: "<<net_msg.mutable_battle_prep_timer()->remaining_time_seconds()<< std::endl;
                    break;
                case MessageType::ALL_SNAPSHOTS:
                    // test_->match_start_room_ = true;
                    test_->hasSnapshotMap_[net_msg.all_snapshots().round()] = true;
                    std::cout << "收到快照,round: " <<net_msg.all_snapshots().round()<< std::endl;
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
        MatchTest* test_;
    };
    TestClient client_;
    bool match_start_received_ = false;
    bool heartbeat_received_ = false;
    bool connect_ack_received_ = false;
    bool match_start_room_ = false;
    friend class TestClient;
    int32_t currTurn = 1;
    const int32_t MaxTurn = 7; // 最大轮数
public: 
    std::unordered_map<int, int> lastTimeMap_;
    std::unordered_map<int, bool> hasSnapshotMap_;
    int match_id_ = 0; // 当前匹配ID，可能用于跟踪或调试

}; 