# Game Server

基于 libevent 和 Lua 的游戏服务器框架。

## 项目结构

```
game-server/
├── CMakeLists.txt          # 构建配置文件
├── config/                 # 配置文件目录
├── Dockerfile             # Docker配置文件
├── node/                  # 节点运行目录
├── scripts/               # Lua脚本目录
│   ├── core/             # 核心脚本
│   ├── handlers/         # 消息处理脚本
│   ├── message.desc      # 消息描述文件
│   └── message_handlers.lua # 消息处理主文件
├── src/                  # 源代码目录
│   ├── core/            # 核心功能模块
│   ├── data/            # 数据管理模块
│   ├── game/            # 游戏逻辑模块
│   ├── main.cpp         # 主程序入口
│   ├── net/             # 网络模块
│   ├── proto/           # Protocol Buffers 相关
│   ├── script/          # 脚本引擎模块
│   ├── test/            # 测试代码，提供了存储模块的测试
│   └── util/            # 工具类模块
├── test/                # 测试目录以及测试客户端代码（用来测试基础网络模块，未更新后续消息类型，已无法兼容）
└── build/               # 构建输出目录
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
- 位置：`src/net/`
- 功能：TCP 服务器实现
- 主要组件：
  - TcpServer：服务器核心实现
  - Connection：连接管理
  - ConnectionPool：连接池管理
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
  - 管理脚本生命周期

### 4. 游戏引擎 (Game)
- 位置：`src/game/`
- 功能：游戏核心逻辑实现
- 主要组件：
  - GameServer：游戏服务器核心
  - CppEngine：C++ 游戏逻辑引擎
  - LuaEngine：Lua 游戏逻辑引擎
  - MessageProcessor：消息处理器
- 职责：
  - 游戏逻辑处理
  - 消息分发和处理
  - 游戏状态管理
  - 双引擎（C++/Lua）支持

### 5. 协议模块 (Proto)
- 位置：`src/proto/`
- 功能：网络消息协议定义和处理
- 主要组件：
  - NetworkMessage.proto：协议定义文件
  - Message：消息处理基类
- 职责：
  - 定义网络通信协议
  - 提供消息序列化/反序列化
  - 管理消息类型和格式
  - 生成协议相关代码

### 6. 数据模块 (Data)
- 位置：`src/data/`
- 功能：游戏数据管理
- 主要组件：
  - Storage：数据存储接口
  - PlayerData：玩家数据管理
- 职责：
  - 管理游戏数据存储
  - 处理玩家数据持久化
  - 提供数据访问接口

## 构建与运行

### 依赖项
- CMake 3.10+
- libevent
- Lua 5.3
- C++17 编译器
- spdlog
- SQLite3
- Protocol Buffers
- uuid

### 构建步骤

1. 安装依赖（Ubuntu/Debian 系统示例）
sudo apt-get update
sudo apt-get install cmake libevent-dev liblua5.3-dev libssl-dev libspdlog-dev libsqlite3-dev protobuf-compiler libprotobuf-dev uuid-dev

2. 构建项目
mkdir build
cd build
cmake ..
make

### 运行服务器
```bash
cd node

#此脚本会自动拷贝lua脚本以及编译后build目录下文件
./start.sh

```

## 配置说明

### 服务器配置
- 默认端口：8888

### Lua脚本及其相关序列化文件
- 均在scripts/下


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





