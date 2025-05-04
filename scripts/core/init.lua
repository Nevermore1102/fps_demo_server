-- 游戏服务器初始化脚本
print("Lua environment initialized")

-- 定义消息处理函数
function on_message(msg)
    print("Received message:", msg)
    return "Message received"
end

-- 注册消息处理函数
_G.on_message = on_message 