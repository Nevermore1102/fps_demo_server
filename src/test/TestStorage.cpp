// #include "TestStorage.h"

// namespace test {

// bool TestStorage::runAllTests() {
//     LOG_INFO("开始测试存储模块...");

//     // 使用默认配置初始化数据库
//     if (!Storage::getInstance().init()) {
//         LOG_ERROR("数据库初始化失败");
//         return false;
//     }
//     LOG_INFO("数据库初始化成功");

//     if (!testPlayerDataSaveLoad()) {
//         return false;
//     }

//     if (!testPlayerStateValidation()) {
//         return false;
//     }

//     LOG_INFO("存储模块测试完成");
//     return true;
// }

// bool TestStorage::testPlayerDataSaveLoad() {
//     // 创建测试玩家数据
//     PlayerData player("test_player_001");
    
//     // 设置一些测试数据
//     player.updateHealth(80);
//     player.updateAmmo(45);
//     player.updatePosition(10.5f, 20.5f, 30.5f);
//     player.updateRotation(90.0f, 0.0f, 0.0f);

//     // 保存数据
//     if (!player.save()) {
//         LOG_ERROR("保存玩家数据失败");
//         return false;
//     }
//     LOG_INFO("保存玩家数据成功");

//     // 创建新的玩家对象来测试加载
//     PlayerData loadedPlayer("test_player_001");
//     if (!loadedPlayer.load()) {
//         LOG_ERROR("加载玩家数据失败");
//         return false;
//     }
//     LOG_INFO("加载玩家数据成功");

//     return true;
// }

// bool TestStorage::testPlayerStateValidation() {
//     PlayerData loadedPlayer("test_player_001");
//     if (!loadedPlayer.load()) {
//         LOG_ERROR("加载玩家数据失败");
//         return false;
//     }

//     // 验证加载的数据
//     const PlayerState& state = loadedPlayer.getState();
//     if (state.health != 80 || state.ammo != 45 || 
//         state.x != 10.5f || state.y != 20.5f || state.z != 30.5f || 
//         state.rotation_x != 90.0f || state.rotation_y != 0.0f || state.rotation_z != 0.0f) {
//         LOG_ERROR("加载的数据与保存的数据不匹配");
//         return false;
//     }

//     LOG_INFO("数据验证成功");
//     return true;
// }

// } // namespace test 