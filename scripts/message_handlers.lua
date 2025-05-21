-- 设置 Lua 模块搜索路径（可选，lua-protobuf 安装后一般不需要）
-- local script_dir = debug.getinfo(1, "S").source:match("@?(.*/)")
-- package.cpath = script_dir .. "?.so;" .. package.cpath

local pb = require "pb"

-- 加载消息描述文件
local f = assert(io.open("scripts/message.desc", "rb"))
local buffer = f:read "*a"
f:close()
assert(pb.load(buffer))

-- 消息处理函数
function handle_player_update(msg)
    print("Lua handling player update message")
    -- 先解码 NetworkMessage
    local network_msg = pb.decode("NetworkMessage", msg.body)
    -- 从 data 字段获取 player_update
    local data = network_msg.player_update
    
    print("Player ID:", network_msg.player_id)
    print("Position:", data.position_x, data.position_y, data.position_z)
    print("Rotation:", data.rotation_x, data.rotation_y, data.rotation_z)
    print("Velocity:", data.velocity_x, data.velocity_y, data.velocity_z)
    print("Is Grounded:", data.is_grounded)
    print("Health:", data.health)
    
    -- -- 发送响应消息
    -- local response = {
    --     msg_id = MessageType.PLAYER_UPDATE,
    --     player_id = network_msg.player_id,
    --     timestamp = network_msg.timestamp,
    --     player_update = {
    --         position_x = data.position_x,
    --         position_y = data.position_y,
    --         position_z = data.position_z,
    --         rotation_x = data.rotation_x,
    --         rotation_y = data.rotation_y,
    --         rotation_z = data.rotation_z,
    --         velocity_x = data.velocity_x,
    --         velocity_y = data.velocity_y,
    --         velocity_z = data.velocity_z,
    --         is_grounded = data.is_grounded,
    --         health = data.health
    --     }
    -- }
    -- local encoded = pb.encode("NetworkMessage", response)
    -- local success = send_response(MessageType.PLAYER_UPDATE, encoded)
    -- print("Send response result:", success)
    return true
end

function handle_player_shoot(msg)
    print("Lua handling player shoot message")
    -- 解析消息
    local data = pb.decode("PlayerShootMessage", msg.body)
    print("Player ID:", data.player_id)
    print("Bullet ID:", data.bullet_id)
    print("Direction:", data.direction_x, data.direction_y, data.direction_z)
    
    -- 发送响应消息
    local response = {
        player_id = data.player_id,
        bullet_id = data.bullet_id,
        status = "shot"
    }
    local encoded = pb.encode("PlayerShootMessage", response)
    local success = send_response(MessageType.PLAYER_SHOOT, encoded)
    print("Send response result:", success)
    return true
end

function handle_player_hit(msg)
    print("Lua handling player hit message")
    -- 解析消息
    local data = pb.decode("PlayerHitMessage", msg.body)
    print("Attacker ID:", data.attacker_id)
    print("Target ID:", data.target_id)
    print("Damage:", data.damage)
    
    -- 发送响应消息
    local response = {
        attacker_id = data.attacker_id,
        target_id = data.target_id,
        damage = data.damage,
        status = "hit"
    }
    local encoded = pb.encode("PlayerHitMessage", response)
    local success = send_response(MessageType.PLAYER_HIT, encoded)
    print("Send response result:", success)
    return true
end

-- 注册消息处理函数
_G.handle_player_update = handle_player_update
_G.handle_player_shoot = handle_player_shoot
_G.handle_player_hit = handle_player_hit

print("Message handlers registered") 