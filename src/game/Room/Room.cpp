#include "game/Room/Room.h"
#include "game/Player/Player.h"
#include "log/log_macro.h"
#include "proto/Message.h"
#include "proto/NetworkMessage.pb.h"
#include <cstddef>
#include <memory>
#include <numeric>
#include <spdlog/spdlog.h>
#include <string>

// 析构函数
Room::~Room() {
    LOG_INFO_ROOM("Destroying room {}", room_id_);
    
    // 确保定时器线程正确停止
    stopCountdownTimer();
    
    // 清理所有资源
    cleanupRoom();
    
    LOG_INFO_ROOM("Room {} destroyed successfully", room_id_);
}

bool Room::isPlayerReady(const std::string& playerId) const{
    auto it = playerReadyStatus_.find(playerId);
    if (it != playerReadyStatus_.end()) {
        return it->second;
    }
    return false;  // 默认未准备
}

void Room::setReadyStatus(const std::string& playerId, bool ready){
    playerReadyStatus_[playerId] = ready;
}

bool Room::isAllPlayerReady() const{
    for(const auto& p:players_){
        if(p && !isPlayerReady(p->GetPlayerId())) {
            return false;  // 只要有一个玩家未准备就返回false
        }
    }
    return true;  // 所有玩家都已准备
}

void Room::setDataLoadStatus(const std::string& playerId, bool loaded){
    playerDataLoadStatus_[playerId] = loaded;
}

bool Room::isPlayerDataLoaded(const std::string& playerId) const{
    auto it = playerDataLoadStatus_.find(playerId);
    if (it != playerDataLoadStatus_.end()) {
        return it->second;
    }
    return false;  // 默认未加载
}

bool Room::isAllPlayerDataLoaded() const{
    for(const auto& p:players_){
        LOG_INFO_ROOM("Player {} , state{}, data load status: {}", 
            p->GetPlayerId(), static_cast<int>(p->getState()),  isPlayerDataLoaded(p->GetPlayerId()));
    }
    for(const auto& p:players_){
        if(p && p->getState()==PlayerState::GAMING 
            && !isPlayerDataLoaded(p->GetPlayerId())) {
            return false;  // 只要有一个玩家未加载就返回false
        }
    }
    return true;  // 所有玩家都已加载
}

size_t Room::getAllPlayerCount() const{
    return players_.size();
}

size_t Room::getExitPlayerCount() const{
    size_t count = 0;
    for(const auto& p:players_){
        if(p && p->getState()==PlayerState::DISCONNECTED){
            count++;
        }
    }
    return count;
}

size_t Room::getGamingPlayerCount() const{
    size_t count = 0;
    for(const auto& p:players_){
        if(p && p->getState()==PlayerState::GAMING){
            count++;
        }
    }
    return count;
}

// 添加玩家到房间
bool Room::addPlayer(std::shared_ptr<Player> player) {
    if (!player || isFull()) return false;

    players_.push_back(player);
    playerMap_[player->GetPlayerId()] = player;
    setReadyStatus(player->GetPlayerId(), false);
    setDataLoadStatus(player->GetPlayerId(), false);
    return true;
}

// 获取指定ID的玩家信息
std::shared_ptr<Player> Room::getPlayer(const std::string& playerId) {
    auto it = playerMap_.find(playerId);
    if (it != playerMap_.end()) {
        return it->second;
    }
    return nullptr;
}

// 启动倒计时seconds秒
void Room::startCountdownTimer(int32_t seconds) {
    stopCountdownTimer(); // 保证不会有多个定时器线程
    countdown_running_ = true;
    countdown_remaining_seconds_ = seconds;

    // 创建倒计时线程
    countdown_thread_ = std::thread([this]() {
        while (countdown_running_ && countdown_remaining_seconds_ >= 0) {
            std::this_thread::sleep_for(std::chrono::seconds(BROADCAST_INTERVAL));
            if (!countdown_running_)
            {
                LOG_INFO_ROOM("Countdown stopped!");
                break;
            }
            // 执行逻辑
            onCountdownTick();
        }
    });
}

// 停止倒计时
void Room::stopCountdownTimer() {
    LOG_DEBUG_ROOM("Stopping countdown timer for room {}", room_id_);
    
    // 设置停止标志
    countdown_running_ = false;
    
    // 等待定时器线程结束
    if (countdown_thread_.joinable()) {
        LOG_DEBUG_ROOM("Waiting for countdown thread to join for room {}", room_id_);
        countdown_thread_.join();
        LOG_DEBUG_ROOM("Countdown thread joined successfully for room {}", room_id_);
    }
    
    countdown_remaining_seconds_ = PREPARE_TIME_SECONDS[0];
    LOG_DEBUG_ROOM("Countdown timer stopped for room {}", room_id_);
}

// 获取当前剩余时间
int32_t Room::getRemainingTime() const {
    return countdown_remaining_seconds_;
}

// 对局开始，广播 {type：对局开始，对局id，总回合数，所有<玩家id，玩家name，头像id>}
void Room::broadcastPlayerInfo() {
    if (isFull()) {
        setState(RoomState::LOADING);

        NetworkMessage msg;
        msg.set_msg_id(MessageType::GAME_START);
        GameStartMessage* start_msg = msg.mutable_game_start();
        start_msg->set_match_id(std::to_string(room_id_));
        start_msg->set_total_rounds(ROUND_NUM);

        for(const auto& player : players_) {
            if (player) {
                // 使用add_players_info()添加玩家信息
                PlayerBasicInfo* info = start_msg->add_players_info();
                info->set_player_id(player->GetPlayerId());
                info->set_player_name(player->GetPlayerName());
                info->set_icon_id(player->GetIconId());
                LOG_INFO_ROOM("ttttt Broadcasting player info: {} in room {}", 
                        player->GetPlayerId(), room_id_);
            }else {
                LOG_WARN_ROOM("Player info is null in room {}", room_id_);
            }
        }
        
        broadcastMessage(msg);
        LOG_INFO_ROOM("Broadcasted player info for room {}", room_id_);
    } else {
        LOG_WARN_ROOM("Cannot broadcast player inf, room {} is not full", room_id_);
    }
}

// 开始游戏主函数
void Room::startGame() {
    if (isFull()) {
        setState(RoomState::GAMING);

        initSendResultState();
        nextRound();  // 回合+1
        sendEnemyFormationToAll();       // 发送对手阵容
        broadcastPrepareStart();    // 广播备战开始消息
        startBattlePrepTimer();     // 启动备战倒计时

        LOG_INFO_ROOM("Game started in room {}", room_id_);
    } else {
        LOG_WARN_ROOM("Cannot start game, room {} is not full", room_id_);
    }
}

// 广播消息给房间所有Gaming状态玩家
void Room::broadcastMessage(const NetworkMessage& msg,bool haslog) {
    // 将NetworkMessage转换为Message
    Message body;
    body.setBodyFromProto(msg);

    // 遍历所有玩家并发送消息
    for (const auto& player : players_) {
        if (player && player->getState() == PlayerState::GAMING) {
            auto conn = player->GetConnection();
            if (conn) {
                bool success = conn->sendMessage(body);
                if (!success)
                    LOG_ERROR_ROOM("Failed to send message to player: {}", player->GetPlayerId());
                else{
                    if (haslog) LOG_INFO_ROOM("Message sent to player: {}", player->GetPlayerId());
                    // body.logMessage();  // 打印消息详情
                }
            }
        }
    }
}

// 广播消息给除指定玩家外的所有玩家
void Room::broadcastToOthers(const std::string& excludePlayerId, const NetworkMessage& msg) {
    Message body;
    body.setBodyFromProto(msg);

    for (const auto& player : players_) {
        if (player && player->getState() == PlayerState::GAMING && player->GetPlayerId() != excludePlayerId) {
            auto conn = player->GetConnection();
            if(conn)
            {
                bool success = conn->sendMessage(body);
                if (!success)
                    LOG_ERROR_ROOM("Failed to send message to player: {}", player->GetPlayerId());
                else{
                    LOG_INFO_ROOM("Message sent to player: {}", player->GetPlayerId());
                    // body.logMessage();  // 打印消息详情
                }
            }
        }
    }
}

// 广播开始准备的消息 {type：备战开始，备战时间，先手玩家id}
void Room::broadcastPrepareStart(){
    NetworkMessage msg;
    msg.set_msg_id(MessageType::PREPARE_START);

    BattlePrepStartMessage* prep_msg = msg.mutable_battle_prep_start();
    prep_msg->set_prepare_time_seconds(PREPARE_TIME_SECONDS[getCurrentRound()]);
    
    // 轮数奇数，先加入的玩家先手
    // if (getCurrentRound() % 2)
    //     prep_msg->set_first_player_id(players_.front()->GetPlayerId());
    // else
    //     prep_msg->set_first_player_id(players_.back()->GetPlayerId());
    
    broadcastMessage(msg);
}

// 开启备战倒计时
void Room::startBattlePrepTimer() {
    startCountdownTimer(PREPARE_TIME_SECONDS[getCurrentRound()]);
}

// 定时器回调：每x秒调用一次，处理倒计时逻辑
void Room::onCountdownTick() {
    if (countdown_remaining_seconds_ > 0) {
        countdown_remaining_seconds_-=BROADCAST_INTERVAL;

        // 剩余时间消息 {type：剩余时间广播，对局id，轮次，备战剩余时间}
        NetworkMessage msg;
        msg.set_msg_id(MessageType::BATTLE_PREP_TIMER);

        BattlePrepTimerMessage* timer_msg = msg.mutable_battle_prep_timer();
        timer_msg->set_match_id(std::to_string(room_id_));
        timer_msg->set_round(currentRound_);
        timer_msg->set_remaining_time_seconds(countdown_remaining_seconds_);
        
        broadcastMessage(msg,false);
    }
    if (countdown_remaining_seconds_ == 0) {
        onCountdownFinished();
    }
}

void Room::onCountdownFinished() {
    // 处理倒计时结束逻辑，如自动准备、结算等（待实现）
}

// 检查所有Gaming玩家是否都已发送结果
bool Room::allGamingRankingsReceived() {
    if(isSendResult_.size() != getAllPlayerCount()){
        LOG_WARN_ROOM("Not all players have sent their results, current size: {}", isSendResult_.size());
        return false;
    }

    for(const auto& [pid,flag]:isSendResult_){
        auto player = getPlayer(pid);
        if(player && player->getState()==PlayerState::GAMING && !flag){
            LOG_WARN_ROOM("Gaming player {} has not sent their result", pid);
            return false;
        }
    }
    
    return true;
}

// 检查所有玩家是否都已发送结果
bool Room::allRankingsReceived() {
    if(isSendResult_.size() != getAllPlayerCount()){
        LOG_WARN_ROOM("Not all players have sent their results, current size: {}", isSendResult_.size());
        return false;
    }

    for(const auto& [pid,flag]:isSendResult_){
        if(!flag){
            LOG_WARN_ROOM("Player {} has not sent their result", pid);
            return false;
        }
    }
    
    return true;
}

// 清理排名，真删除allHonorValue_，暂时不用
void Room::clearRankings() {
    allHonorValue_.clear();
    LOG_INFO_ROOM("Cleared all rankings for room {}", room_id_);
}

// 广播排名给所有玩家
void Room::BroadcastCurrentRankings() {
    auto rankings = getRankings();

    NetworkMessage msg;
    msg.set_msg_id(MessageType::CURRENT_RANK_INFO);
    CurrentRankInfoMessage* rank_msg = msg.mutable_current_rank_info();
    
    std::string log_rank_info;

    for(const auto& entry : rankings) {
        // 使用add_rankings()添加新的排名条目
        RankingEntry* new_entry = rank_msg->add_rankings();
        new_entry->CopyFrom(*entry);  // 复制entry的内容到新条目
        log_rank_info += "Player ID: " + entry->player_id() + ", Honor Value: " + std::to_string(entry->honor_value()) + "\n";
    }

    // 广播消息
    broadcastMessage(msg);
    LOG_INFO_ROOM("Broadcasted current rankings for room {}: \n{}", room_id_, log_rank_info);
}

// 广播结果给所有玩家
void Room::BroadcastResults() {
    auto rankings = getRankings();

    NetworkMessage msg;
    msg.set_msg_id(MessageType::SETTLEMENT);
    SettlementMessage* settlement_msg = msg.mutable_settlement();
    settlement_msg->set_match_id(std::to_string(room_id_));

    std::string log_rank_info;
    for(const auto& entry : rankings) {
        // 使用add_rankings()添加新的排名条目
        RankingEntry* new_entry = settlement_msg->add_rankings();
        new_entry->CopyFrom(*entry);  // 复制entry的内容到新条目
        log_rank_info += "Player ID: " + entry->player_id() + ", Honor Value: " + std::to_string(entry->honor_value()) + "\n";
    }

    // 广播消息
    broadcastMessage(msg);
    LOG_INFO_ROOM("Broadcasted final results for room {}: \n{}", room_id_, log_rank_info);
}

// 初始化发送结果状态
void Room::initSendResultState(){
    for (const auto&p: players_){
        isSendResult_[p->GetPlayerId()] = false;
    }
}

// 重置所有玩家的发送结果状态
void Room::resetSendResultState(){
    for (auto&[pid,flag]: isSendResult_) {
        auto player = getPlayer(pid);

        // 重置玩家的发送结果状态
        if (player) {
            flag = false;
            LOG_INFO_ROOM("Reset send result state for player {}", pid);
        }
    }
}

// 插入在线玩家/机器人荣耀值
bool Room::insertRanking(const std::string& playerId, int32_t honorValue) {
    // 插入或更新排名
    auto player = getPlayer(playerId);
    if (!player) {
        LOG_ERROR_ROOM("11111111111 Player {} not found or not in gaming state or robot state", playerId);
        return false;
    }

    player->SetHonorValue(honorValue);
    isSendResult_[playerId] = true;  // 标记该玩家已发送结果
    allHonorValue_[playerId] = honorValue;

    LOG_INFO_ROOM("Inserted ranking for player {} with honor value {}, allHonorValue_ size ", playerId, honorValue, allHonorValue_.size());
    return true;
}

// 插入退出玩家荣耀值（新版本不再使用，所有机器人玩家和真人玩家荣耀值都要更新）
// bool Room::insertExitRanking(const std::string& playerId, int32_t honorValue) {
//     // 插入或更新退出玩家的荣耀值
//     auto player = getPlayer(playerId);
//     if (!player || player->getState() != PlayerState::DISCONNECTED) {
//         LOG_ERROR_ROOM("Player {} not found or not in disconnected state", playerId);
//         return false;
//     }
    
//     allHonorValue_[playerId] = honorValue;

//     LOG_INFO_ROOM("Inserted exit ranking for player {} with honor value {}", playerId, honorValue);
//     return true;
// }

// 根据荣耀值计算房间内玩家的排名
std::vector<std::shared_ptr<RankingEntry>> Room::getRankings() {
    if(allHonorValue_.size() != getAllPlayerCount()) {
        LOG_WARN_ROOM("Not all players have submitted their rankings, cannot generate complete rankings, allHonorValue_ size:", allHonorValue_.size());
        return {};
    }

    std::vector<std::shared_ptr<RankingEntry>> rankings;
    rankings.reserve(getAllPlayerCount());

    // 收集所有玩家的排名信息
    for (const auto&[p,h]: allHonorValue_) {
        auto player = getPlayer(p);
        if (!player) {
            LOG_WARN_ROOM("Player {} not found in room {}", p, room_id_);
            continue;
        }

        auto rankInfo = std::make_shared<RankingEntry>();
        rankInfo->set_player_id(player->GetPlayerId());
        rankInfo->set_honor_value(player->GetHonorValue());
        rankInfo->set_rank(0);  // 初始排名为0

        rankings.push_back(rankInfo);
    }

    // 根据荣耀值降序排序
    std::sort(rankings.begin(), rankings.end(), 
        [](const std::shared_ptr<RankingEntry>& a, const std::shared_ptr<RankingEntry>& b) {
            return a->honor_value() > b->honor_value();  // 降序排序
    });

    // 设置排名
    for (size_t i = 0; i < rankings.size(); ++i) {
        rankings[i]->set_rank(i + 1);  // 排名从1开始
    }

    return rankings;
}

// 房间清理
void Room::cleanupRoom() {
    LOG_INFO_ROOM("Cleaning up room {}", room_id_);
    
    // 先停止定时器
    stopCountdownTimer();
    
    // 清理所有容器
    players_.clear();
    playerReadyStatus_.clear();
    playerDataLoadStatus_.clear();
    playerMap_.clear();
    currentSnapshots_.clear();
    allHonorValue_.clear();
    exitPlayers_.clear();
    
    // 设置状态为已结束
    state_ = RoomState::FINISHED;
    
    LOG_INFO_ROOM("Room {} cleanup completed", room_id_);
}

// 记录玩家快照，并更新到Player的最新快照中
bool Room::recordPlayerSnapshot(const std::string& playerId, const std::string& formationData, int32_t honorValue, int32_t round) {
    // 检查玩家是否在房间中
    auto player = getPlayer(playerId);
    if (!player) {
        LOG_WARN_ROOM("Player {} not found in room {}", playerId, room_id_);
        return false;
    }
    // if(round<=hassnapshotclearedRound_){
    //     LOG_WARN_ROOM("Player {} round {} snapshot has been cleared, cannot record", playerId, round);
    //     return false;
    // }
    // //记录第一个快照
    // bool firstSnapshot = snapshotPlayers_.empty();
    // snapshotPlayers_.insert(playerId);


    // 创建快照
    PlayerSnapshot snapshot;
    snapshot.set_player_id(playerId);
    snapshot.set_match_id(std::to_string(room_id_));
    snapshot.set_round(round);
    snapshot.set_formation_data(formationData);
    snapshot.set_honor_value(honorValue);
    snapshot.set_is_robot(player->isRobot());

    // 更新最新的玩家快照
    player->SetLatestSnapshot(std::make_shared<PlayerSnapshot>(snapshot));
    
    // 存储快照
    currentSnapshots_[playerId] = snapshot;
    
    // //第一个启动快照计时器
    // if (firstSnapshot) {
    //     tryStartSnapshotTimer();
    // }
    LOG_INFO_ROOM("Recorded snapshot for player {} in room {}, round {} (honor: {})", 
                playerId, room_id_, round, honorValue);
    return true;
}

void Room::deletePlayer(const std::string& playerId){
    auto player = getPlayer(playerId);
    if (!player) {
        LOG_WARN_ROOM("Player {} not found in room {}", playerId, room_id_);
        return;
    }
    player->setState(PlayerState::ROBOT);
    //设定机器人阵容 ，0，1，2
    player->SetFormationData(std::to_string(get_robot_formation_data()));
    auto coon=player->GetConnection();
    player->SetConnection(nullptr);
}

void Room::deletePlayerWithConn(const std::string& playerId){
    auto player = getPlayer(playerId);
    if (!player) {
        LOG_WARN_ROOM("Player {} not found in room {}", playerId, room_id_);
        return;
    }
    player->setState(PlayerState::ROBOT);
    //设定机器人阵容 ，0，1，2
    player->SetFormationData(std::to_string(get_robot_formation_data()));
    auto coon=player->GetConnection();
    player->SetConnection(nullptr);
    if (!coon) {
        LOG_WARN_ROOM("Player {} conn has del  in room {}", playerId, room_id_);
        // return;
    }
    else coon->close();
}

int Room::get_robot_formation_data(){
    robot_formation_data++;
    if(robot_formation_data>=3)
    {
        LOG_ERROR_ROOM("robot_formation_data error");
        return 0;
    }
    return robot_formation_data;
}

void Room::onSnapshotTimeout() {
    LOG_WARN_ROOM("Room {} snapshot timeout, now handle unsubmitted players", getId());

    // 检查哪些玩家未提交快照
    for (const auto& player : players_) {
        if (player->getState() == PlayerState::GAMING && 
            snapshotPlayers_.find(player->GetPlayerId()) == snapshotPlayers_.end()) {
            //删除玩家
            deletePlayerWithConn(player->GetPlayerId());
            // recordPlayerSnapshot(player->GetPlayerId(), "default_formation", 0, getCurrentRound());
        }
    }

    // 正常继续广播和清理
    recordRobotSnapshots();
    broadcastAllSnapshots();
    clearSnapshots();
    // stopSnapshotTimer();
}


void Room::tryStartSnapshotTimer() {
    // 若timer正在运行，先停一下再开启
    stopSnapshotTimer();
    snapshotTimer_.start(snapshotTimeoutMs_, [this]() {
        this->onSnapshotTimeout();
    });
}
void Room::stopSnapshotTimer() {
    snapshotTimer_.stop();
}


// 检查是否所有玩家的快照都已收到
bool Room::allGamingSnapshotsReceived() const {
    // if (currentSnapshots_.size() != getGamingPlayerCount()) {
    //     LOG_WARN_ROOM("Not all gaming players have submitted their snapshots, current size: {}, gaming player: ", currentSnapshots_.size(), getGamingPlayerCount());
    //     return false;
    // }
    
    // 确保所有Gaming玩家都有快照
    for (const auto& player : players_) {
        if (player->getState() == PlayerState::GAMING ){
            if (currentSnapshots_.find(player->GetPlayerId()) == currentSnapshots_.end())
                return false;
        }
    }
    
    return true;
}

// 广播所有快照
void Room::broadcastAllSnapshots() {
    if (!allGamingSnapshotsReceived()) {
        LOG_WARN_ROOM("Cannot broadcast snapshots, not all players have submitted");
        return;
    }
    
    // 生成随机种子
    int32_t seed = static_cast<int32_t>(std::time(nullptr)) + room_id_;
    
    // 构造 ALL_SNAPSHOTS 消息
    NetworkMessage msg;
    msg.set_msg_id(MessageType::ALL_SNAPSHOTS);
    
    AllSnapshotsMessage* all_snapshots = msg.mutable_all_snapshots();
    all_snapshots->set_match_id(std::to_string(room_id_));
    all_snapshots->set_round(getCurrentRound());
    all_snapshots->set_seed(seed);
    
    // 添加所有玩家快照
    for (const auto& snapshotPair : currentSnapshots_) {
        PlayerSnapshot* snapshot = all_snapshots->add_snapshots();
        auto player = getPlayer(snapshotPair.first);
        auto s = snapshotPair.second;
        if(player->isRobot()){
            s.set_is_robot(true);
        }
        else {
            s.set_is_robot(false);
        }
        *snapshot = s;
    }
    
    // 广播消息
    broadcastMessage(msg);
    
    LOG_INFO_ROOM("Broadcasted all snapshots for room {}, round {} with seed {}", 
                room_id_, currentRound_, seed);
}

// 清理快照
void Room::clearSnapshots() {
    currentSnapshots_.clear();
    hassnapshotclearedRound_ = currentRound_;
    stopSnapshotTimer(); // 防止残留timer
    LOG_INFO_ROOM("Cleared snapshots for room {}", room_id_);
}

// 记录机器人快照
void Room::recordRobotSnapshots(){
    for(const auto& player : players_) {
        if (player->getState() == PlayerState::ROBOT) {
            currentSnapshots_[player->GetPlayerId()] = player->GetPlayerSnapshot();
        }
    }
    LOG_INFO_ROOM("Recorded robot snapshots for room {}", room_id_);
}

// 玩家退出处理
void Room::onPlayerExit(const std::string& playerId, int32_t exit_round, int32_t honorValue) {
    LOG_INFO_ROOM("Player {} has exited the room {}", playerId, room_id_);
    // 检查玩家是否在房间中
    auto player = getPlayer(playerId);
    if (!player) {
        LOG_WARN_ROOM("Player {} not found in room {}", playerId, room_id_);
        return;
    }

    // 更新玩家状态
    // player->setState(PlayerState::DISCONNECTED);

    // 放入退出玩家信息
    ExitPlayerInfo exitInfo;
    exitInfo.set_exit_player_id(playerId);
    exitInfo.set_exit_round(exit_round);
    exitInfo.set_exit_honor_value(honorValue);
    exitPlayers_[playerId] = exitInfo;
    LOG_INFO_ROOM("Player {} info inserted into exitPlayers_ with round {} and honor value {}", 
                playerId, exit_round, honorValue);

    // // 插入退出玩家的荣耀值
    // if (!insertExitRanking(playerId, honorValue)) {
    //     LOG_ERROR_ROOM("Failed to insert exit ranking for player {}", playerId);
    //     return;
    // }

    LOG_INFO_ROOM("Player {} marked as disconnected in room {}", playerId, room_id_);
}

// 广播退出消息
void Room::broadcastExitMessage() {
    NetworkMessage msg;
    msg.set_msg_id(MessageType::DISCONNECTED);
    ExitBroadcastMessage* exit_msg = msg.mutable_exit_broadcast();

    for(const auto& exitPair : exitPlayers_) {
        const ExitPlayerInfo& exitInfo = exitPair.second;
        ExitPlayerInfo* info = exit_msg->add_exit_players_info();
        info->set_exit_player_id(exitInfo.exit_player_id());
        info->set_exit_round(exitInfo.exit_round());
        info->set_exit_honor_value(exitInfo.exit_honor_value());
    }

    // 广播消息
    broadcastMessage(msg);
    LOG_INFO_ROOM("Broadcasted exit message for room {}", room_id_);
}

int Room::getCurrentEnemyIdx(const std::string& playerId) {
    int n = players_.size();
    if (n < 2 || n % 2 != 0) {
        LOG_ERROR_ROOM("Invalid player count: {}, must be even and at least 2", n);
        return -1;  // 无法配对
    }

    // 查找玩家索引
    auto it = std::find_if(players_.begin(), players_.end(),
                          [&](const auto& p){ return p->GetPlayerId() == playerId; });
    if (it == players_.end()) 
    {
        LOG_ERROR_ROOM("Player {} not found in room {}", playerId, room_id_);
        return -1;
    }
    int idx = it - players_.begin();

    // 计算本轮配对方案
    int round = (currentRound_ - 1) % (n - 1);
    std::vector<int> pos(n);
    std::iota(pos.begin(), pos.end(), 0);
    if (round > 0) std::rotate(pos.begin() + 1, pos.begin() + 1 + round, pos.end());

    int half = n / 2;
    int pairIdx = -1; // 本玩家在左还是右？
    for (int i = 0; i < half; ++i) {
        if (pos[i] == idx) pairIdx = n - 1 - i;
        else if (pos[n - 1 - i] == idx) pairIdx = i;
        if (pairIdx != -1) break;
    }
    if (pairIdx == -1) return -1;

    // return players_[pos[pairIdx]]->GetPlayerId();
    return pos[pairIdx];  // 返回配对的玩家ID

}

// 发送对手阵容
void Room::sendEnemyFormationToAll(){
    for(const auto& player : players_){
        if(player && player->getState() == PlayerState::GAMING)
            sendEnemyFormationToPlayer(player);
    }
}

// 给指定玩家发送当前回合的对手阵容信息 {type:对手信息，对手id，是否机器人，对手阵容，先手id}
void Room::sendEnemyFormationToPlayer(const std::shared_ptr<Player>& player){
    int enemy_idx = getCurrentEnemyIdx(player->GetPlayerId());
    if(enemy_idx==-1){
        LOG_ERROR_ROOM("Failed to get enemy formation for player {}", player->GetPlayerId());
        return;
    }
    std::shared_ptr<Player> enemy_player = players_[enemy_idx];

    NetworkMessage msg;
    msg.set_msg_id(MessageType::ENEMY_INFO);
    EnemyInfoMessage* enemy_info = msg.mutable_enemy_info();
    enemy_info->set_enemy_player_id(enemy_player->GetPlayerId());
    enemy_info->set_real_enemy(enemy_player->isRobot());
    if(enemy_player->isRobot())
        enemy_info->set_formation_data(std::stoi(enemy_player->GetFormationData()));

    std::string first_player_id;
    // 先后手规则：
    if(getCurrentRound()%2==0)
        first_player_id = player->GetPlayerId() > enemy_player->GetPlayerId() ? player->GetPlayerId() : enemy_player->GetPlayerId();
    else
        first_player_id = player->GetPlayerId() < enemy_player->GetPlayerId() ? player->GetPlayerId() : enemy_player->GetPlayerId();
    
    enemy_info->set_first_player(first_player_id);

    if(player->sendMessage(msg)){
        LOG_INFO_ROOM("Sent enemy formation to player {} :enemy_id={}, real_enemy={}, first_player={}",
            player->GetPlayerId(), enemy_player->GetPlayerId(), enemy_player->isRobot(), first_player_id);
    }
    else
        LOG_ERROR_ROOM("Failed to send enemy formation to player {}", player->GetPlayerId());
}

// 两机器人对战荣耀结算
void Room::calculateRobotHonor(){
    int idx = getCurrentRound()-1;
    int h1 = ROUND_ROBOT_HONOR[idx].first;
    int h2 = ROUND_ROBOT_HONOR[idx].second;

    for(const auto& p:players_){
        if(p && p->isRobot()){
            int cur_honor = p->GetHonorValue();
            int enemy_player_idx = getCurrentEnemyIdx(p->GetPlayerId());
            if(enemy_player_idx==-1){
                LOG_ERROR_ROOM("Failed to get enemy formation for player {}", p->GetPlayerId());
                return;
            }
            auto enemy_player = players_[enemy_player_idx];

            // 对手也是机器人
            if(enemy_player && enemy_player->isRobot()){
                int new_honor = cur_honor + rand() % (h2 - h1 + 1) + h1;
                insertRanking(p->GetPlayerId(), new_honor);
                LOG_INFO_ROOM("insert robot {} honor value {}", p->GetPlayerId(), new_honor);
            }
        }
    }
}

// 为房间内的每个玩家生成随机名字
void Room::generateRandomPlayerNames(){
    for(auto& p:players_){
        if(p) p->SetPlayerName(generateRandomPlayerName(p));
    }
}

std::string Room::generateRandomPlayerName(const std::shared_ptr<Player>& player){
    // 用当前时间作为随机数种子
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }
    
    // 随机选择前缀和后缀
    int preIndex = std::rand() % NAME_PRE.size();
    int postIndex = std::rand() % NAME_POST.size();
    
    // 组合名字
    std::string randomName = NAME_PRE[preIndex] + "的" + NAME_POST[postIndex];
    
    // 检查名字是否已存在，如果存在则重新生成
    while (!isPlayerNameAvailable(randomName)) {
        preIndex = std::rand() % NAME_PRE.size();
        postIndex = std::rand() % NAME_POST.size();
        randomName = NAME_PRE[preIndex] + "的" + NAME_POST[postIndex];
    }
    
    spdlog::info("Generated random player name: {}", randomName);
    return randomName;
}


bool Room::isPlayerNameAvailable(const std::string& name){
    // 遍历房间中所有玩家，检查是否有重名
    for (const auto& player : players_) {
        if (player && player->GetPlayerName() == name) {
            spdlog::info("Player name '{}' already exists in room {}", name, room_id_);
            return false;
        }
    }
    
    // 没有找到重名，名字可用
    return true;
}

// 为房间内的每个玩家生成头像id
void Room::generateRandomPlayerIcons(){
    for(auto& p:players_){
        if(p) p->SetIconId(++icon_id_counter_);
    }
}