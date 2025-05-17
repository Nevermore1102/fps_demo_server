#!/bin/bash

# 设置工作目录为脚本所在目录
cd "$(dirname "$0")"

# 检查 build 目录是否存在
if [ ! -d "../build" ]; then
    echo "错误: build 目录不存在，请先编译项目"
    exit 1
fi

# 检查 game_server 是否存在
if [ ! -f "../build/game_server" ]; then
    echo "错误: game_server 不存在，请先编译项目"
    exit 1
fi

# 创建必要的目录
mkdir -p data
mkdir -p scripts

# 拷贝 game_server 到当前目录
cp "../build/game_server" .
echo "已更新 game_server"

# 拷贝 Lua 脚本
if [ -d "../scripts" ]; then
    cp -r ../scripts/* scripts/
    echo "已更新 Lua 脚本"
fi

# 设置执行权限
chmod +x game_server

# 启动服务器
echo "正在启动服务器..."
./game_server 