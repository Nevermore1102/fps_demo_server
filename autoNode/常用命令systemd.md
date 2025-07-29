# systemd 服务常用命令

## 🚀 最常用核心命令

### 服务管理（必备）



```
# 1. 修改服务配置后重载
sudo systemctl daemon-reload

# 2. 启动服务
sudo systemctl start game-server

# 3. 重启服务
sudo systemctl restart game-server

# 4. 停止服务
sudo systemctl stop game-server

# 5. 查看服务状态
sudo systemctl status game-server
```

### 日志查看（必备）


```
# 6. 实时查看日志（最常用）
sudo journalctl -u game-server -f

# 7. 查看最近50行日志
sudo journalctl -u game-server -n 50

# 8. 查看今天的日志
sudo journalctl -u game-server --since today
```

### 开机启动设置



```
# 9. 启用开机自启动
sudo systemctl enable game-server

# 10. 禁用开机自启动
sudo systemctl disable game-server
```

## 📝 典型使用流程

### 首次部署



```
# 创建服务文件
sudo vim /etc/systemd/system/game-server.service

# 重载配置
sudo systemctl daemon-reload

# 启用并启动
sudo systemctl enable game-server
sudo systemctl start game-server

# 查看状态
sudo systemctl status game-server

# 查看实时日志
sudo journalctl -u game-server -f
```

### 日常维护


```
# 查看服务状态
sudo systemctl status game-server

# 重启服务
sudo systemctl restart game-server

# 查看实时日志
sudo journalctl -u game-server -f

# 查看最近日志
sudo journalctl -u game-server -n 100
```

### 故障排查



```
# 查看服务状态（详细信息）
sudo systemctl status game-server

# 查看最近错误日志
sudo journalctl -u game-server -p err

# 查看启动失败日志
sudo journalctl -u game-server --since "10 minutes ago"

# 查看最近重启次数
sudo systemctl show game-server --property=NRestarts
```