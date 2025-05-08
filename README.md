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


