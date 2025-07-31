#!/bin/bash
# /home/hxy/card_zzy/autoNode/save-crash-log.sh

TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="/home/hxy/card_zzy/autoNode/crash_logs/crash_${TIMESTAMP}.log"

# 创建日志目录
mkdir -p /home/hxy/card_zzy/autoNode/crash_logs

# 简单保存关键信息
echo "=== 游戏服务器崩溃日志 - $TIMESTAMP ===" > "$LOG_FILE"
echo "" >> "$LOG_FILE"

# 保存服务状态
systemctl status game-server --no-pager >> "$LOG_FILE" 2>&1
echo "" >> "$LOG_FILE"

# 保存最近错误日志
echo "=== 最近错误日志 ===" >> "$LOG_FILE"
journalctl -u game-server -p err --since "10 minutes ago" --no-pager >> "$LOG_FILE" 2>&1
echo "" >> "$LOG_FILE"

# 保存重启次数
echo "=== 重启次数 ===" >> "$LOG_FILE"
systemctl show game-server --property=NRestarts >> "$LOG_FILE"

echo "崩溃日志已保存到: $LOG_FILE"