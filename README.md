# Game Server

基于 libevent 和 Lua 的游戏服务器框架。

## 项目结构

```
game-server/
├── CMakeLists.txt          # 构建配置文件
├── config/                 # 配置文件目录
├── Dockerfile             # Docker配置文件
├── scripts/               # Lua脚本目录
│   ├── core/             # 核心脚本
│   │   └── init.lua     # 初始化脚本
│   └── handlers/        # 消息处理脚本
└── src/                  # 源代码目录
    ├── core/            # 核心功能模块
    │   ├── EventLoop.cpp
    │   └── EventLoop.h
    ├── main.cpp        # 主程序入口
    ├── net/           # 网络模块
    │   ├── TcpServer.cpp
    │   └── TcpServer.h
    ├── script/       # 脚本引擎模块
    │   ├── LuaVM.cpp
    │   └── LuaVM.h
    └── util/        # 工具类模块
```

## 核心模块说明

### 1. 事件循环 (EventLoop)
- 位置：`src/core/EventLoop`
- 功能：基于 libevent 的事件循环系统
- 职责：
  - 管理网络事件
  - 处理定时器事件
  - 提供异步事件处理机制

### 2. 网络模块 (TcpServer)
- 位置：`src/net/TcpServer`
- 功能：TCP 服务器实现
- 职责：
  - 监听客户端连接
  - 处理网络消息
  - 管理连接生命周期

### 3. 脚本引擎 (LuaVM)
- 位置：`src/script/LuaVM`
- 功能：Lua 脚本管理
- 职责：
  - 初始化 Lua 环境
  - 加载游戏逻辑脚本
  - 提供 C++ 和 Lua 的交互接口

## 构建与运行

### 依赖项
- CMake 3.10+
- libevent
- Lua 5.3
- C++17 编译器

### 构建步骤
```bash
mkdir build
cd build
cmake ..
make
```

### 运行服务器
```bash
./build/game_server
```

## 配置说明

### 服务器配置
- 默认端口：8888
- 配置文件路径：`config/server.toml`（待实现）

### Lua脚本
- 初始化脚本：`scripts/core/init.lua`
- 消息处理脚本：`scripts/handlers/`（待实现）

## 待实现功能

1. 消息路由模块
   - 消息分发系统
   - 路由表管理
   - 消息处理器注册机制

2. 游戏逻辑模块
   - 实体管理
   - 玩家系统
   - 战斗系统
   - 背包系统

3. 数据持久化
   - 数据库接口
   - 存档管理
   - 异步保存机制

4. 日志系统
   - 日志分级
   - 日志轮转
   - 性能监控

## 客户端与服务端消息同步开发文档

### 1. 消息格式（Protocol Buffers）

- 所有客户端与服务端通信消息均采用 Protocol Buffers（proto3）格式。
- 消息定义示例（NetworkMessage.proto）：

```proto
syntax = "proto3";

package Unity.FPS.Game;

enum MessageType {
  HEARTBEAT = 0;
  PLAYER_UPDATE = 1;
}

message NetworkMessage {
  MessageType msg_id = 1;
  uint32 player_id = 2;
  uint32 timestamp = 3;
  oneof data {
    HeartbeatMessage heartbeat = 4;
    PlayerUpdateMessage player_update = 5;
  }
}

message HeartbeatMessage {
  // 空消息体
}

message PlayerUpdateMessage {
  float position_x = 1;
  float position_y = 2;
  float position_z = 3;
  float rotation_x = 4;
  float rotation_y = 5;
  float rotation_z = 6;
  float velocity_x = 7;
  float velocity_y = 8;
  float velocity_z = 9;
  bool is_grounded = 10;
  float health = 11;
}
```

- 客户端和服务端需保持 proto 文件完全一致。
- 使用 `protoc` 工具生成各自语言的代码。

### 2. 消息序列化与反序列化

- 发送前：将消息对象序列化为二进制流（`SerializeToArray` 或 `SerializeToString`）。
- 接收后：将收到的二进制流反序列化为消息对象（`ParseFromArray` 或 `ParseFromString`）。

### 3. 粘包/拆包（包边界）处理规范

- **protobuf 只负责内容，不负责包边界。**
- 每条消息前需加上4字节无符号整型（uint32，网络字节序）表示消息体长度。
- 发送流程：
  1. 序列化消息为二进制流 `data`。
  2. 计算长度 `len = data.size()`。
  3. 发送4字节长度（uint32_t，网络字节序）。
  4. 发送消息体内容。
- 接收流程：
  1. 先读取4字节长度字段，得到 `len`。
  2. 再读取 `len` 字节内容。
  3. 用 protobuf 反序列化。

#### C++ 伪代码示例

```cpp
// 发送
std::string data;
msg.SerializeToString(&data);
uint32_t len = htonl(data.size());
send(socket, &len, sizeof(len), 0);
send(socket, data.data(), data.size(), 0);

// 接收
uint32_t len;
recv(socket, &len, sizeof(len), 0);
len = ntohl(len);
std::vector<char> buf(len);
recv(socket, buf.data(), len, 0);
NetworkMessage msg;
msg.ParseFromArray(buf.data(), len);
```

### 4. 版本兼容与扩展

- 新增字段时，使用 proto3 的兼容性特性，避免破坏旧客户端/服务端。
- 建议所有消息都通过 `NetworkMessage` 统一封装，便于扩展。

### 5. 注意事项

- proto 文件变更后，务必同步更新客户端和服务端。
- 网络传输务必处理好包边界，防止粘包/拆包导致解析错误。
- 建议所有网络日志记录原始包长度和类型，便于排查问题。

如有疑问请联系服务端/客户端开发负责人。


