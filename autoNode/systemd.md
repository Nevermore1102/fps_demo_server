
# systemd 服务 自动重启服务相关命令

## 查看服务状态
### 创建/修改服务文件
```
# 创建新的服务文件
sudo vim /etc/systemd/system/game-server.service
```
### 服务配置模板



```
[Unit]
Description=Game Server
After=network.target

[Service]
Type=simple
User=hxy
Group=hxy
WorkingDirectory=/home/hxy/card_zzy/node_test
ExecStart=/home/hxy/card_zzy/node_test/game_server

# 重启策略
Restart=always
RestartSec=5
StartLimitBurst=5
StartLimitIntervalSec=10

# 日志配置
StandardOutput=journal
StandardError=journal

# 资源限制
LimitNOFILE=65536
LimitCORE=infinity
MemoryMax=1G

[Install]
WantedBy=multi-user.target
```



## 🔧 服务管理命令

### 基本操作



```
# 重载systemd配置（修改服务文件后必须执行）
sudo systemctl daemon-reload

# 启动服务
sudo systemctl start game-server

# 停止服务
sudo systemctl stop game-server

# 重启服务
sudo systemctl restart game-server

# 启用自动启动（开机启动）
sudo systemctl enable game-server

# 禁用自动启动
sudo systemctl disable game-server

# 查看服务状态
sudo systemctl status game-server

# 查看服务是否启用
sudo systemctl is-enabled game-server

# 查看服务是否运行中
sudo systemctl is-active game-server
```


### 强制操作



```
# 强制停止服务
sudo systemctl kill game-server

# 发送特定信号（如SEGV用于测试core dump）
sudo systemctl kill -s SEGV game-server

# 重新加载服务配置（不重启服务）
sudo systemctl reload game-server
```

## 📋 日志查看命令

### 实时日志


```
# 实时查看服务日志（最常用）
sudo journalctl -u game-server -f

# 实时查看最近100行日志
sudo journalctl -u game-server -n 100 -f
```

### 历史日志



```
# 查看最近N行日志
sudo journalctl -u game-server -n 50

# 查看今天的日志
sudo journalctl -u game-server --since today

# 查看指定时间段的日志
sudo journalctl -u game-server --since "2024-01-20 10:00:00" --until "2024-01-20 12:00:00"

# 查看最近1小时的日志
sudo journalctl -u game-server --since "1 hour ago"

# 查看最近1周的日志
sudo journalctl -u game-server --since "1 week ago"
```

### 日志过滤



```
# 只看错误日志
sudo journalctl -u game-server -p err

# 搜索特定内容
sudo journalctl -u game-server | grep "ERROR|WARN"

# 查看启动相关日志
sudo journalctl -u game-server | grep -i "start|restart"
```

### 日志导出



```
# 导出今天的日志到文件
sudo journalctl -u game-server --since today > /tmp/game-server.log

# 导出所有日志
sudo journalctl -u game-server --no-pager > /tmp/game-server-all.log

# 以JSON格式导出
sudo journalctl -u game-server -o json-pretty > /tmp/game-server.json
```

## 📊 日志管理命令

### 日志空间查看



```
# 查看日志占用空间
sudo journalctl --disk-usage

# 查看日志文件列表
sudo journalctl --list-boots

# 查看systemd配置
sudo journalctl --show-config
```

### 日志清理



```
# 清理旧日志（保留最近1周）
sudo journalctl --vacuum-time=1w

# 清理日志（保留最多1GB）
sudo journalctl --vacuum-size=1G

# 清理日志（保留最多100个文件）
sudo journalctl --vacuum-files=100
```

## 🐛 调试和故障排除

### 服务调试



```
# 查看服务启动失败原因
sudo systemctl status game-server -l

# 查看服务依赖关系
sudo systemctl list-dependencies game-server

# 测试服务配置语法
sudo systemd-analyze verify /etc/systemd/system/game-server.service

# 查看systemd启动性能
sudo systemd-analyze blame
```



## 🔄 快速操作组合

### 更新服务后的标准流程



```
# 1. 修改服务文件
sudo vim /etc/systemd/system/game-server.service

# 2. 重载配置并重启
sudo systemctl daemon-reload
sudo systemctl restart game-server

# 3. 查看状态和日志
sudo systemctl status game-server
sudo journalctl -u game-server -f
```

### 故障排除流程


```
# 1. 查看服务状态
sudo systemctl status game-server -l

# 2. 查看最近的错误日志
sudo journalctl -u game-server -n 50 -p err

# 3. 查看完整的启动日志
sudo journalctl -u game-server --since "10 minutes ago"
```

### 日常监控



```
# 查看服务状态（简短）
sudo systemctl is-active game-server && echo "✅ 服务运行中" || echo "❌ 服务已停止"

# 查看最近重启次数
sudo systemctl show game-server --property=NRestarts

# 查看服务运行时间
sudo systemctl show game-server --property=ActiveEnterTimestamp
```
