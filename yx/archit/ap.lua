-- ap.lua,yx-archit的接入点服务
local skynet = require "skynet"
local gateserver = require "snax.gateserver"
local core = require "skynet.core"
----------------------------------------------------------------------------------------
local mode = ...

local AP = {}
local _PB = nil
local _AP_AUTHS = {}
local _AP_AUTH_COUNT = 0
local _ap_auth_name = "archit/ap_auth"
local _conns = {}	-- fd -> connection : { fd , auth, client, agent , ip, mode }
local reg_services = {}

skynet.register_protocol {
	name = "client",
	id = skynet.PTYPE_CLIENT,
}

function AP.init_protobuf()
	print("AP.init protobuf")
	_PB = require "protobuf"
	local pb_config = require "conf/pb_config".AP_PB
	for k,v in pairs(pb_config) do
		_PB.register_file(v)
	end
end

local ap_conf = nil
function AP.open(source, conf)
	ap_conf = conf
	pp.print(conf)
	-- 启动N个ap_auth服务，做AP验证。
	local auths = conf.auths or 8
	assert(auths > 0)
	for i=1,auths do
		table.insert(_AP_AUTHS, skynet.newservice(_ap_auth_name))
	end
	_AP_AUTH_COUNT = #_AP_AUTHS
	--
	if mode == "protobuf" then
		--AP.init_protobuf();
	end
	--LOG.Info("AP", "listen %s on %d", conf.address or "0.0.0.0", conf.port)
end


local function unforward(c, msg, sz)
	skynet.error("drop unforward message:", fd)
end

function AP.message(fd, msg, sz)
	skynet.error("socket message:", fd)
	local c = _conns[fd]
	local agent = c.agent
	if agent then
		-- recv a package, forward it
		skynet.redirect(agent, c.fd, "client", 0, msg, sz)
	else
		local ok = pcall(unforward, c, msg, sz)
	end
end

local function auth_fd(fd, addr)
	-- 为fd获取个专门验证服务
	local ap_auth = _AP_AUTHS[ fd % _AP_AUTH_COUNT + 1]
	-- call auth
	return skynet.call(ap_auth, "lua",  "auth_fd", fd, addr)
end

function AP.connect(fd, addr)
	local c = {
		fd = fd,
		ip = addr,
		secret = nil,
	}
	_conns[fd] = c
	skynet.error("New client from : " .. addr)
	-- 对链接进行验证。
	local ok, secret, server = auth_fd(fd, addr)
	if ok then
		c.agent = reg_services[server]
		c.secret = secret
		skynet.call(c.agent, "lua", "connect", c.fd, c.secret)
		skynet.error("ap open client:", fd)
		gateserver.openclient(fd)
	else
		skynet.error("ap close client:", fd)
	end
end

function AP.disconnect(fd)
	local c = _conns[fd]
	if c.agent then
		skynet.call(c.agent, "lua", "disconnect", c.fd)
	end
	skynet.error("socket disconnect:", fd)
end

function AP.error(fd, msg)
	local c = _conns[fd]
	if c.agent then
		skynet.call(c.agent, "lua", "disconnect", c.fd)
	end
	skynet.error("socket error:", fd)
end

function AP.warning(fd, size)
	skynet.error("socket warning:", fd)
end

----------------------------------------------------------------------------------------
-- ap cmd
local CMD = {}

function CMD.reg(source, name, server)
	print("CMD.reg:", name, server)
	reg_services[name] = server
end

function CMD.host(source)
	local name = ap_conf.address or "127.0.0.1"
	local port = ap_conf.port
	print("CMD.host:", name, port)
	return name, port
end

function AP.command(cmd, source, ...)
	skynet.error("AP command:", cmd)
	local f = assert(CMD[cmd])
	return f(source, ...)
end

-- 启动AP类型网关服务。
gateserver.start(AP)
