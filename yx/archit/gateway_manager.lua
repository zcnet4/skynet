-- gateway_manager.lua，网关管理服务
local skynet = require "skynet"
local core = require "skynet.core"
local np = require "common/netproto"
local crypt = require "crypt"
local socket = require "socket"
local cjson = require "cjson"
----------------------------------------------------------------------------------------
local GATEWAY = {}

-- 网关与网关转发器配置。
local gates_conf = {
	g1 = {mid = 1, pwd='defalut', },
	g2 = {mid = 2, pwd='defalut', },
	g3 = {mid = 3, pwd='defalut', },
	g4 = {mid = 4, pwd='defalut', },
	g5 = {mid = 5, pwd='defalut', },
	g6 = {mid = 6, pwd='defalut', },
	g7 = {mid = 7, pwd='defalut', },
	g8 = {mid = 8, pwd='defalut', },
	g9 = {mid = 9, pwd='defalut', },
	g10 = {mid = 10, pwd='defalut', },
	g11 = {mid = 11, pwd='defalut', },
	g12 = {mid = 12, pwd='defalut', },
	--
	f1 = {mid = 21, pwd='defalut', link = "g1", n = 5},
	f2 = {mid = 22, pwd='defalut', link = "g2", n = 5},
	f3 = {mid = 23, pwd='defalut', link = "g3", n = 5},
}

local name2ends = {}
local fd2ends = {}
local fd2secret = {}

local function client_send(fd, cmd, session, uid, content)
	local data, data_sz = core.proto_pack2(cmd, session, uid, content)
	socket.write(fd, data, data_sz)
end

function GATEWAY.Init()
	print(pp.tostring(gates_conf))
end

function GATEWAY.connect(fd, secret)
	LOG.Warning("GM", "connect fd:", fd)
	fd2secret[tostring(fd)] = secret
	-- 为了能发送数据。
	socket.start(fd)
end

function GATEWAY.disconnect(fd)
	LOG.Warning("GM", "disconnect fd:", fd)
	fd2secret[tostring(fd)] = nil
	local ends = fd2ends[tostring(fd)]
	if ends then
		name2ends[ends.name] = nil
		fd2ends[tostring(fd)] = nil
	end
	GATEWAY.pushGatesList();
end

function GATEWAY.Join(fd, content)
	LOG.Warning("GM", "fd:", fd)
	local secret = fd2secret[tostring(fd)];
	if not secret then
		LOG.Error("GM", "fd:"..fd.." not secret")
		client_send(fd, np.S2C.JOIN_SERVER_RESPONSE, 0, 0)
		return
	end

	local ok, json = pcall(cjson.decode, crypt.xor_str(content, secret))
	if not ok or not json.join then
		LOG.Error("GM", "fd:"..fd.." Bad join message")
		client_send(fd, np.S2C.JOIN_SERVER_RESPONSE, 0, 0)
		return
	end 
	local join_client = json.join
	local gate_name = join_client['name']
	local gate_conf = gates_conf[gate_name]
	if not gate_conf or gate_conf.mid ~= join_client.mid or gate_conf.pwd ~= join_client.pwd then
		LOG.Error("GM", "fd:"..fd.." Mid or wrong password", join_client)
		client_send(fd, np.S2C.JOIN_SERVER_RESPONSE, 0, 0)
		return
	end

	local old_client = name2ends[gate_name]
	if old_client and old_client.mid ~= join_client.mid then
		LOG.Error("GM", "fd:"..fd.." Mid already exists", join_client)
		client_send(fd, np.S2C.JOIN_SERVER_RESPONSE, 0, 0)
	end

	local ends = {
		fd = fd,
		name = join_client.name, 
		pwd = join_client.pwd, 
		mid = math.tointeger(join_client.mid),
		frontend_host = join_client.frontend_host,
		backend_host = join_client.backend_host,
		proxy_tcp = join_client.proxy_tcp,
		proxy_udp = join_client.proxy_udp,
	}
	name2ends[gate_name] = ends
	fd2ends[tostring(fd)] = ends
	--
	pp.print(name2ends)
	LOG.Warning("GM", "fd:"..fd.." join GM Ok")
	client_send(fd, np.S2C.JOIN_SERVER_RESPONSE, 0, 1)
	GATEWAY.pushGatesList();
end

function GATEWAY.RegConnGatewayImpl(fd, content)
	--pp.print(fd2secret)
	local secret = fd2secret[tostring(fd)]
	if not secret then
		LOG.Error("GM", "fd:"..fd.." not secret")
		error("not secret")
	end
	--
	local gate_name = crypt.xor_str(content, secret)
	local ends = name2ends[gate_name]
	local ends_fd = fd2ends[tostring(fd)]
	if not ends or not ends_fd or ends.mid ~= ends_fd.mid then
		LOG.Error("GM", "fd:"..fd.." gate_name:"..gate_name.."not join GM")
		error("not join GM")
	end
	--
	local link_node_name = gates_conf[gate_name].link
	if not link_node_name then
		LOG.Error("GM", "fd:"..fd.." gate_name:"..gate_name.."No connection points are configured")
		error("offline")
	end
	--
	local conn_info = nil
	local link_node = name2ends[link_node_name]
	if link_node then
		conn_info = {name=link_node.name,proxy_tcp=link_node.proxy_tcp,proxy_udp=link_node.proxy_udp,n=gates_conf[gate_name].n}	
	else
		conn_info = {}
		LOG.Error("GM", "fd:"..fd.." gate_name:"..gate_name.." link gate_node:"..link_node_name.." offline")
	end
	--
	return crypt.xor_str(cjson.encode(conn_info), secret)	
end

function GATEWAY.RegConnGateway(fd, content)
	local ok, data = pcall(GATEWAY.RegConnGatewayImpl, fd, content)
	if ok then
		client_send(fd, np.S2C.REQ_CONN_GATEWAY_RESPONSE, 0, 1, data)
	else
		client_send(fd, np.S2C.REQ_CONN_GATEWAY_RESPONSE, 0, 0, data)
	end	
	print("GATEWAY.RegConnGateway:", ok)
end

function GATEWAY.pushGatesListImpl()
	local list = {}
	for k,v in pairs(name2ends) do
		table.insert( list, {name=k,host=v.frontend_host})
	end
	local json_string = cjson.encode(list)
	pp.print(json_string)
	skynet.send("/public/datacenter", "lua", "UPDATE", "listGates", json_string)
end

function GATEWAY.pushGatesList()
	skynet.send(skynet.self(), "lua", "pushGatesListImpl")	
end
----------------------------------------------------------------------------------------
skynet.register_protocol {
	name = "client",
	id = skynet.PTYPE_CLIENT,
	unpack = core.proto_unpack2,
	dispatch = function(_, fd, cmd, session, uid, content)
		if np.C2S.JOIN_SERVER == cmd then
			GATEWAY.Join(fd, content)
		elseif np.C2S.REQ_CONN_GATEWAY == cmd then
			GATEWAY.RegConnGateway(fd, content)
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
