#!/bin/bash

# 设置工作目录为脚本所在目录
cd "$(dirname "$0")"

echo "🚀 开始部署 game_server..."

# 检查构建文件
if [ ! -f "../build/game_server" ] || [ ! -f "../build/test/test_client" ]; then
    echo "❌ 构建文件不存在，请先编译项目"
    exit 1
fi

# 创建必要目录
mkdir -p data

# 停止服务
echo "⏹️  正在停止服务..."
sudo systemctl stop game-server 2>/dev/null || true
sleep 2

# 更新文件
echo "📦 正在更新文件..."
cp -f "../build/game_server" . && echo "✅ game_server 已更新"
cp -f "../build/test/test_client" . && echo "✅ test_client 已更新"

# 设置权限
chmod +x game_server test_client

# 重启服务
echo "🔄 正在重启服务..."
sudo systemctl start game-server

# 检查服务状态
echo "📊 服务状态:"
sudo systemctl status game-server --no-pager -l

echo "🎉 部署完成!"