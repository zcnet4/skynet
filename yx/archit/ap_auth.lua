-- M.lua,yx-archit的接入点验证服务
local skynet = require "skynet"
local socket = require "socket"
local crypt = require "crypt"
local np = require "common/netproto"
----------------------------------------------------------------------------------------


----------------------------------------------------------------------------------------
local M = {}
-- 无返回参数定义
local NORET = {}

local function read_package(fd)
	local sz = socket.read(fd, 2)
	assert(sz, "closed")
	sz = string.unpack(">I2", sz)
    --print("read_package size:", sz)
    local content = assert(socket.read(fd, sz), "closed")
    if sz >= 2 + 4 + 4 then
        local content_size = sz - (2 + 4 +4)
        return string.unpack(">I2I4I4c"..content_size, content)
    else
        return 0, 0, 0, nil 
    end
end

local function write_package(fd, cmd, session, uid, content)
--[len][cmd][session][uid][content]:包长度+命令+会话+uid+内容。
--sizeof(uint16_t),sizeof(uint16_t),sizeof(uint32_t),sizeof(uint32_t)
    local data = nil
	local content_size = content and string.len(content) or 0
    if content_size > 0 then
	    data = string.pack(">I2I2I4I4c"..content_size, 2 + 4 + 4 + content_size, cmd, session, uid, content)
    else
        data = string.pack(">I2I2I4I4", 2 + 4 + 4, cmd, session, uid)
    end
    --print("write_package:", #data)
    socket.write(fd, data)
end

local function read_auth(fd, auth_cmd)
    local cmd, session, uid, content = read_package(fd)
    if auth_cmd == cmd then
        return content
    else
        print("Auth Error")
        error("Auth Error")
    end
end

local function write_auth(fd, cmd, content)
    local session, uid = 1767, 1
    return write_package(fd, cmd, session, uid, content)
end

function M.Init()

end

function M.auth_fd_impl(fd)
    -- 接管socket服务。
    socket.start(fd)
    -- 从client中得到加密密钥
    skynet.error("fd:%u read - c2s connent", fd)
    local clientkey = read_auth(fd, np.C2S.CONNECT)
    -- 写入challenge与密钥
    local challenge = crypt.randomkey()
    local skey = crypt.randomkey()
    local skey_change = crypt.dhexchange(skey);
    write_auth(fd, np.S2C.CHALLENGE, challenge..skey_change)
    --　计算加密密钥 
    local secret = crypt.dhsecret(clientkey, skey)
    skynet.error("fd:%u read - c2s challenge", fd)
    --
    local content = read_auth(fd, np.C2S.CHALLENGE_RESPONSE)
    local hmac_challenge, server = string.unpack("c8z", content)
    local hmac = crypt.hmac64(challenge, secret)
    if hmac ~= hmac_challenge then
        skynet.error "challenge failed"
        error "challenge failed"
    end
    skynet.error("Auth Ok")
    server = crypt.xor_str(server, secret)
    return secret, server
end

function M.auth_fd(source, fd, msg, sz)
    local Ok, secret, server = pcall(M.auth_fd_impl, fd)
    if Ok then
        write_auth(fd, np.S2C.CONNECT_ACCEPT)
        socket.abandon(fd);
    else
        write_auth(fd, np.S2C.CONNECT_REJECT)
        socket.abandon(fd);
        socket.close(fd);
        secret = nil
    end
    --
    return Ok, secret, server;
end



----------------------------------------------------------------------------------------
skynet.start(function()
	M.Init()
	skynet.dispatch("lua", function (_, source, cmd, ...)
		local f = M[cmd]
		if f then
            skynet.ret(skynet.pack(f(source, ...)))
		else
			error(string.format("Unknown M service CMD %s", tostring(cmd)))
		end
	end)
end)