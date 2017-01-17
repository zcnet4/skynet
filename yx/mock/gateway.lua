-- gateway.lua 网关模拟服务
local skynet = require "skynet"
local sc = require "socketchannel"
local socket = require "socket"
local crypt = require "crypt"
local np = require "common/netproto"
local cjson = require "cjson"
----------------------------------------------------------------------------------------
local MOCK = {}
-- 无返回参数定义
local NORET = {}

local function write_package(channel, cmd, session, uid, content)
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
    local content = channel:request(data, function(sock)
		local sz = sock:read(2)
		sz = string.unpack(">I2", sz)
		local content = sock:read(sz)
		if sz >= 2 + 4 + 4 then
			return true, content
		else
			return false 
		end
	end)
	local content_size = #content - (2 + 4 +4)
	if content_size == 0 then
		return string.unpack(">I2I4I4", content)
	else
		return string.unpack(">I2I4I4c"..content_size, content)
	end
end

local function connect_ap(channel, server)
	local secret = nil
	local ckey = crypt.randomkey()
	local ckey_change = crypt.dhexchange(ckey)
	local cmd, session, uid, s1 = write_package(channel, np.C2S.CONNECT, 0, 0, ckey_change)
	if np.S2C.CHALLENGE == cmd and #s1 == 16 then
		local challenge, skey_change = string.unpack("c8c8", s1)
		secret = crypt.dhsecret(skey_change, ckey)
		--
		local cr = string.pack("c8z", crypt.hmac64(challenge, secret), crypt.xor_str(server, secret))
		local cmd, session, uid, s1 = write_package(channel, np.C2S.CHALLENGE_RESPONSE, 0, 0, cr)
		if np.S2C.CONNECT_ACCEPT == cmd then
			return secret
		else
			return nil
		end
	else
		return nil
	end
end 

local function connect_gateway_manager(channel, gateway_name, gateway_pwd, gateway_uid, frontend_host, backend_host, proxy_tcp, proxy_udp)
	local secret = connect_ap(channel, "gateway")
	if secret then
		skynet.sleep(100)
		local login = {join={name=gateway_name,pwd=gateway_pwd,mid=gateway_uid,frontend_host=frontend_host,backend_host=backend_host,proxy_tcp=proxy_tcp,proxy_udp=proxy_udp}}
		local cmd, session, uid, content = write_package(channel, np.C2S.JOIN_SERVER, 0, 0, crypt.xor_str(cjson.encode(login), secret))
		if np.S2C.JOIN_SERVER_RESPONSE == cmd and 1 == uid then
			skynet.error("Connect Gateway Manager Ok")
			return secret
		end
	end
	skynet.error("Connect Gateway Manager Failed")
end

--local host = "183.6.232.33"
--local host = "172.20.55.39"
local host = "127.0.0.1"
local gateway_config = {
	gateway_name = "g3",
	gateway_pwd = "defalut",
	gateway_uid = 3,
	frontend_host = host..":3020",
	backend_host = host..":3021",
	proxy_tcp = host..":3022",
}


function MOCK.Init()
	pp.print(gateway_config)
	local host, port = skynet.call("master_ap", "lua", "host")
	MOCK.mockConnectGM(host, port);
	MOCK.mockFrontend();
end

local _CHANNEL_AP = nil
function MOCK.mockConnectGM(host, port)
	skynet.error("connect Gateway Manager:"..host..":"..port)
	_CHANNEL_AP = sc.channel {host=host,port=port}
	_CHANNEL_AP:connect(true)
	local secret = connect_gateway_manager(_CHANNEL_AP, gateway_config.gateway_name, gateway_config.gateway_pwd, gateway_config.gateway_uid,
	gateway_config.frontend_host, gateway_config.backend_host, gateway_config.frontend_proxy_host)
end
local i = 0
local function fork_proxy_tcp(fd)
	socket.start(fd)
	--
	while true do
		local sz = socket.read(fd, 2)
		--assert(sz, "closed")
		if not sz then
			skynet.error("close fd:".. fd)
			break
		end
		sz = string.unpack(">I2", sz)
		if sz >= 2 + 4 + 4 then
			local buf = socket.read(fd, sz)
			local content_size = sz - (2 + 4 +4)
			local cmd, session, uid, content = nil, nil, nil, nil
			if content_size == 0 then
				cmd, session, uid = string.unpack(">I2I4I4", buf)
				skynet.error("content_size == 0", fd)
			else
				cmd, session, uid, content = string.unpack(">I2I4I4c"..content_size, buf)
			end
			if np.G2G.GF2G_FORWARD == cmd then
				local data = nil
				if content_size > 0 then
	    			data = string.pack(">I2I2I4I4c"..content_size, 2 + 4 + 4 + content_size, np.G2G.G2GF_FORWARD, session, uid, content)
					local raw_cs = content_size - 10
					local cmd1, session1, uid1, content1 = string.unpack(">I2I4I4c"..raw_cs, content)
					skynet.error("write data session:"..session1)
    			else
        			data = string.pack(">I2I2I4I4", 2 + 4 + 4, np.G2G.G2GF_FORWARD, session, uid)
    			end
				socket.write(fd, data)
				i = i + 1
				skynet.error("write data:"..i.." size:"..#data)
			elseif np.G2G.GF2G_FORWARD_CLOSE == cmd then
				-- nothing
				local agnet_id = uid << 32 | session
				skynet.error("close agnet id:".. agnet_id)
			else
				skynet.error("disconnect -1 fd:", fd)
				socket.close(fd)
				return
			end		
		else
			skynet.error("disconnect -2 fd: sz", fd, sz)
			socket.close(fd)
			return	
		end
	end
end

function MOCK.mockFrontend()
	skynet.error("listen proxy_tcp:", gateway_config.proxy_tcp)
	local fph_fd = socket.listen(gateway_config.proxy_tcp)
	socket.start(fph_fd , function(fd, addr)
		skynet.error("connect from " .. addr .. " " .. fd)
		skynet.fork(fork_proxy_tcp, fd)
	end)
end
----------------------------------------------------------------------------------------
skynet.start(function()
	print("Gateway Mock Server start...")
	MOCK.Init()

	skynet.dispatch("lua", function (_, source, cmd, ...)
		print("Gateway Mock service: dispatch CMD.....")
		local f = MOCK[cmd]
		if f then
			local ret = f(source, ...)
			if ret ~= NORET then
				skynet.ret(skynet.pack(ret))
			end
		else
			error(string.format("Gateway Mock service CMD %s", tostring(cmd)))
		end
	end)
end)

