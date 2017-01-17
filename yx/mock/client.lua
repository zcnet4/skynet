-- client.lua
local skynet = require "skynet"
local class = require "util/class"
local sc = require "mock/sc2" 
----------------------------------------------------------------------------------------
local M = class("mock_client");

function M:ctor(uid)
	self.uid = uid
	self.session = 0
	self.channel = nil
end

local function channel_response(sock)
	local sz = sock:read(2)
	sz = string.unpack(">I2", sz)
	print("read -- sz:", sz)
	local buf = sock:read(sz)
	if sz >= 2 + 4 + 4 then
		local content_size = sz - (2 + 4 +4)
		local cmd, session, uid, content = nil, nil, nil, nil
		if content_size == 0 then
			cmd, session, uid = string.unpack(">I2I4I4", buf)
		else
			cmd, session, uid, content = string.unpack(">I2I4I4c"..content_size, buf)
		end
		return session, true, content
	else
		return nil, false
	end
end


function M:connect(addr)
	if self.channel and self.channel:check_connection() then
		skynet.error("socket had connected")
		return
	end
	print("sc2 connect to: " .. addr .. "uid=" .. self.uid)
	local host, port = string.match(addr, "([^:]+):(.+)$")
	self.channel = sc.channel {
		host = host,
		port = port,
		response = channel_response,
	}
	self.channel:connect(true)
	print("sc2 connect to: " .. addr .. "uid=" .. self.uid .. "!!!!!!!!!!!!end")

	math.randomseed(tostring(os.time()):reverse():sub(1, 6)) 
end

function M:close()
	self.channel:close()
end

function M:request(session, content)
--[len][cmd][session][uid][content]:包长度+命令+会话+uid+内容。
--sizeof(uint16_t),sizeof(uint16_t),sizeof(uint32_t),sizeof(uint32_t)
    local data = nil
	local cmd = 1767
	local uid = self.uid
	local content_size = content and string.len(content) or 0
    if content_size > 0 then
	    data = string.pack(">I2I2I4I4c"..content_size, 2 + 4 + 4 + content_size, 1767, session, uid, content)
    else
        data = string.pack(">I2I2I4I4", 2 + 4 + 4, cmd, session, uid)
    end
	if #data <= 10 then
		skynet.error("ssssssssssssssssss-----")
	end
	local psz = 2 + 4 + 4 + content_size 
	print("send - session:"..self.session.." sz:"..#data.." psz:"..psz)
    local content = self.channel:request(data, session)
	return content
end

function M:send(package_size)
	if self.channel and self.channel:check_connection() then
		self.session = self.session + 1
		local data_size = math.random(12,64*1024 - 24 - 24)
		--local data_size = math.random(64*1024 - 100,64*1024 - 24)
		local N = math.ceil(data_size / 10)
		local data = string.rep("loveMMbyZC", N)
		self:request(self.session, "["..data.."]")
	else
		skynet.error("socket not connect, not send")
	end
end

----------------------------------------------------------------------------------------
return M
