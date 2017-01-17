-- client_manager.lua，客户端控制管理服务
local skynet = require "skynet"
local core = require "skynet.core"
local np = require "common/netproto"
local crypt = require "crypt"
local socket = require "socket"
----------------------------------------------------------------------------------------
local GATEWAY = {}
local forward2gate_config = {
	f1={g="g1", n=10},
	f2={g="g1", n=10},
	f3={g="g1", n=10},
	f4={g="g1", n=10},
	f5={g="g2", n=10},
	f6={g="g2", n=10},
}

local name2ends = {}
local fd2ends = {}
local fd2secret = {}

local function client_send(fd, cmd, session, uid, content)
	local data, data_sz = core.proto_pack2(cmd, session, uid, content)
	socket.write(fd, data, data_sz)
end

function GATEWAY.Init()
	print(pp.tostring(forward2gate_config))
end

function GATEWAY.connect(fd, secret)
	print("GATEWAY.connect")
	fd2secret[tostring(fd)] = secret
	-- 为了能发送数据。
	socket.start(fd)
end

function GATEWAY.disconnect(fd)
	print("GATEWAY.disconnect")
	fd2secret[tostring(fd)] = nil
	local ends = fd2ends[tostring(fd)]
	if ends then
		name2ends[ends.name] = nil
		fd2ends[tostring(fd)] = nil
	end
end

function GATEWAY.Login(fd, content)
	skynet.error("login server:", fd)
	local secret = fd2secret[tostring(fd)];
	if not secret then
		skynet.error("login server:", fd, "not secret")
		client_send(fd, np.S2C.LOGIN_SERVER_RESPONSE, 0, 0)
		return
	end

	local name, pwd, uid, frontend_host, backend_host, frontend_proxy_host = string.unpack(">s2s2I4s2s2s2", crypt.xor_str(content, secret))	
	if "defalut" == pwd then
		skynet.error("login server Ok")
		client_send(fd, np.S2C.LOGIN_SERVER_RESPONSE, 0, 1)
		local ends = {fd=fd,name=name, pwd=pwd, uid=uid,fh=frontend_host,bh=backend_host,fph=frontend_proxy_host}
		name2ends[name] = ends
		fd2ends[tostring(fd)] = ends
		pp.print(name2ends)
		return
	end

	client_send(fd, np.S2C.LOGIN_SERVER_RESPONSE, 0, 0)
end

function GATEWAY.RegLoginGatewayImpl(fd, content)
	--pp.print(fd2secret)
	local secret = fd2secret[tostring(fd)]
	if not secret then
		print("not secret")
		error("not secret")
	end
	--
	local name = crypt.xor_str(content, secret)
	local gf = name2ends[name].name
	if not gf or "string" ~= type(gf) then
		local err = "error name2ends "..name.." offline" 
		print(err)
		error(err)
	end 
	--
	local f_info = forward2gate_config[gf]
	if not f_info then
		local err = "error name2ends "..name.." no config" 
		print(err)
		error(err)
	end
	--
	local g_info = name2ends[f_info.g]
	local data = string.pack(">I2s2", f_info.n, g_info.fph)
	client_send(fd, np.S2C.REQ_LOGIN_GATEWAY_RESPONSE, 0, 0, data)
end

function GATEWAY.RegLoginGateway(fd, content)
	local ok = pcall(GATEWAY.RegLoginGatewayImpl, fd, content)
	print("GATEWAY.RegLoginGateway ok:", ok)	
end

function GATEWAY.join_gate( ... )
	
end
----------------------------------------------------------------------------------------
skynet.register_protocol {
	name = "client",
	id = skynet.PTYPE_CLIENT,
	unpack = core.proto_unpack2,
	dispatch = function(_, fd, cmd, session, uid, content)
		print("fd:", fd)
		if np.C2S.LOGIN_SERVER == cmd then
			GATEWAY.Login(fd, content)
		elseif np.C2S.REQ_LOGIN_GATEWAY == cmd then
			GATEWAY.RegLoginGateway(fd, content)
		else

		end
	end
}

skynet.start(function()
	GATEWAY.Init()
	LOG.Info("Gateway manager Service", "Gateway manager Service Start...")

	skynet.dispatch("lua", function (_, _, cmd, ...)
		local f = GATEWAY[cmd]
		if f then
			skynet.ret(skynet.pack(f(...)))
		else
			error(string.format("Unknown Gateway manager service CMD %s", tostring(cmd)))
		end
	end)
end)
----------------------------------------------------------------------------------------
