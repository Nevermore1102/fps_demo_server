-- 设置 Lua 模块搜索路径（可选，lua-protobuf 安装后一般不需要）
-- local script_dir = debug.getinfo(1, "S").source:match("@?(.*/)")
-- package.cpath = script_dir .. "?.so;" .. package.cpath

local pb = require "pb"

-- 加载消息描述文件
local f = assert(io.open("scripts/message.desc", "rb"))
local buffer = f:read "*a"
f:close()
assert(pb.load(buffer))


--有个全局变量map[id] = PlayerData
local player_data_map = {}

-- 测试 ，创建id为test的player
local test_player = PlayerData.new("test")

-- 消息处理函数
function handle_player_update(msg)
    print("Lua handling player update message start")

    -- 先解码 NetworkMessage
    local success,network_msg = pcall(pb.decode, "NetworkMessage", msg.body)
    if not success then
        print("消息解码失败")
        return false
    end

    -- 从 data 字段获取 player_update
    local data = network_msg.player_update

    if not data then
        print("无效的player_update数据")
        return false
    end

    
    --测试打印
    -- print("Player ID type:", type(network_msg.player_id))
    -- print("Player ID:", network_msg.player_id)
    -- 强制转字符串
    local player_id_str = tostring(network_msg.player_id)
    -- print("Player ID (as string):", player_id_str, "type:", type(player_id_str))

    -- print("Position:", data.position_x, data.position_y, data.position_z)
    -- print("Rotation:", data.rotation_x, data.rotation_y, data.rotation_z)
    -- print("Velocity:", data.velocity_x, data.velocity_y, data.velocity_z)
    -- print("Is Grounded:", data.is_grounded)
    -- print("Health:", data.health)
    

    -- 获取或创建PlayerData实例
    local player_data = player_data_map[player_id_str]
    if not player_data then
        player_data = PlayerData.new(player_id_str)
        player_data_map[player_id_str] = player_data
        -- 尝试加载已保存的数据
        if not player_data:load() then
            print("首次创建PlayerData:", player_id_str)
        end
    end

    
    -- 更新PlayerData
    local success,err=pcall(function()
        player_data:update_position(data.position_x, data.position_y, data.position_z)
        player_data:update_rotation(data.rotation_x, data.rotation_y, data.rotation_z)
        player_data:update_velocity(data.velocity_x, data.velocity_y, data.velocity_z)
        player_data:update_is_grounded(data.is_grounded)
        player_data:update_health(data.health)
    end)

    if not success then
        print("PlayerData更新失败:", err)
    else
        print("PlayerData已更新",network_msg.player_id)    
    end

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