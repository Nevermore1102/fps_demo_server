-- 消息处理函数
function handle_player_update(msg)
    print("Lua handling player update message")
    print("Message type:", msg.type)
    print("Message body:", msg.body)
    -- 发送响应消息
    local success = send_response(MessageType.PLAYER_UPDATE, "update_ack")
    print("Send response result:", success)
    return true
end

function handle_player_shoot(msg)
    print("Lua handling player shoot message")
    print("Message type:", msg.type)
    print("Message body:", msg.body)
    -- 发送响应消息
    local success = send_response(MessageType.PLAYER_SHOOT, "shoot_ack")
    print("Send response result:", success)
    return true
end

function handle_player_hit(msg)
    print("Lua handling player hit message")
    print("Message type:", msg.type)
    print("Message body:", msg.body)
    -- 发送响应消息
    local success = send_response(MessageType.PLAYER_HIT, "hit_ack")
    print("Send response result:", success)
    return true
end

-- 注册消息处理函数
_G.handle_player_update = handle_player_update
_G.handle_player_shoot = handle_player_shoot
_G.handle_player_hit = handle_player_hit

print("Message handlers registered") 