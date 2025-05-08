# 游戏服务器测试说明

## 测试用例列表

### 1. 连接测试
- **功能**: 测试服务器连接、心跳和登录
- **命令**: `./build/test/test_client connection`
- **测试内容**:
  - 连接服务器
  - 心跳消息收发
  - 登录消息收发

### 2. 服务器关闭测试
- **功能**: 测试服务器关闭时的客户端行为
- **命令**: `./build/test/test_client shutdown`
- **测试内容**:
  - 连接服务器
  - 发送心跳消息
  - 关闭服务器
  - 验证断开连接事件

## 使用方法

1. 先启动服务器：
```bash
./build/game_server
```

2. 运行测试：
```bash
./build/test/test_client connection  # 测试连接
./build/test/test_client shutdown   # 测试服务器关闭
```

## 注意事项
- 确保服务器已启动
- 默认连接地址：127.0.0.1:8888
- 服务器关闭测试需要手动关闭服务器 