local skynet = require "skynet"
local util = require "util/util"
local control = require "common/control"
local socket = require "socket"
local core = require "skynet.core"
require "skynet.manager"

----------------------------------------------------------------------------------------

local WATCHDOG
local CLIENT_FD
local Agent = {}
-- 无返回参数定义
local NORET = {}
local _chatagent = "chatagent"

local function client_send(session, ...)
	local package, package_size = core.proto_pack(session, ...);
	if package_size >= 64 * 1024 then
		error("Length is not more than 64k")
	end
	socket.write(CLIENT_FD, package, package_size)
end

local function client_response(session, ok, ...)
	LOG.Info("client_response:"..session)
	if ok then
		if ... then
			client_send(session, "Ok", ...)
		else
			client_send(session, "Ok")
		end
	else
		if session then
			client_send(session, "!Ok")
		end
	end
end

function Agent.start(conf)
	CLIENT_FD = conf.client_fd
	WATCHDOG = conf.watchdog
	-- 是否需要发心跳包?
	-- 注册推送服务。by ZC 2016-4-14
	skynet.call(util.MASTER, "lua", "addPush", conf.push_server)
end

function Agent.disconnect()
	-- todo: do something before exit
	-- 注册推送服务。by ZC 2016-4-14
	skynet.call(util.MASTER, "lua", "addPush", 0)
	skynet.exit()
end

function Agent.push(identId, ...)
	LOG.Info("Agent.push", identId, select(1, ...))
	-- 参数校验。
	local identId_type = type(identId)
	if "string" == identId_type then
		identId = math.tointeger(identId)
	elseif "number" ~= identId_type then
		error(string.format("FAULT IDENT ID:%s Type:%s", tostring(identId), identId_type), 2)
	end
	identId_type = nil
	-- 设置为PUSH指令
	local session = identId | 0xC0000000;

	client_response(session, "Ok", ...)

	return NORET
end

----------------------------------------------------------------------------------------

local model = ...;

if model == _chatagent then

local function chat_call(cmd, chatId, ...)
	--
	LOG.Info("Agent", "chat cmd:", cmd, "possible identId:", select(1, ...))

	if type(cmd) == "string" then
		return control.call(control.CALL_TYPE_CHATSERVER, cmd, ...)
	else
		error("err_msg")
	end
end

skynet.register_protocol {
	name = "client",
	id = skynet.PTYPE_CLIENT,
	unpack = core.proto_unpack,
	dispatch = function(_, _, session, cmd, chatId, ...)
		-- 检测是否是控制命令。
		if session & 0x80000000 == 0x80000000 then
			print("0x80000000")
		elseif cmd == "chat/ping" then
			client_send(session, "Ok")
		else
			LOG.Info("client session:", session)
			client_response(session, pcall(chat_call, cmd, chatId, ...))
		end
	end
}

skynet.start(function()
	skynet.dispatch("lua", function(_,_, cmd, ...)
		print("Agent service: dispatch CMD.....")
		local f = Agent[cmd]
		if f then
			local ret = f(...)
			if ret ~= NORET then
				skynet.retpack(ret)
			end
		else
			error(string.format("Unknown Master service CMD %s", tostring(cmd)))
		end
	end)
end)

else

----------------------------------------------------------------------------------------
local function launch_chatagent()
	local chat_agent = skynet.newservice(SERVICE_NAME, _chatagent)
	
	skynet.call(chat_agent, "lua", "start", { client_fd = CLIENT_FD, watchdog = WATCHDOG, push_server = chat_agent})
	skynet.call(WATCHDOG, "lua", "forward", CLIENT_FD, chat_agent)
end

local function client_call(cmd, identId, ...)
	if cmd == "chat/ping" then
		return "ok"
	end
	--
	LOG.Info("Agent", "client cmd:", cmd, "identId:", identId)

	if type(cmd) == "string" then
		return control.call(control.CALL_TYPE_CLIENT, cmd, identId, ...)
	else
		error("err_msg")
	end
end
	
skynet.register_protocol {
	name = "client",
	id = skynet.PTYPE_CLIENT,
	unpack = core.proto_unpack,
	dispatch = function(_, _, session, cmd, identId, ...)
		-- 检测是否是控制命令。
		if session & 0x80000000 == 0x80000000 then
			if cmd == "chatagent" then
				launch_chatagent()
				client_response(session, true)
			end
		elseif cmd == "chat/ping" then
			client_send(session, "Ok")
		else
			LOG.Info("client session:", session)
			client_response(session, pcall(client_call, cmd, identId, ...))
		end
	end
}

skynet.start(function()
	skynet.dispatch("lua", function(_,_, cmd, ...)
		print("Agent service: dispatch CMD.....")
		local f = Agent[cmd]
		if f then
			local ret = f(...)
			if ret ~= NORET then
				skynet.retpack(ret)
			end
		else
			error(string.format("Unknown Master service CMD %s", tostring(cmd)))
		end
	end)
end)

----------------------------------------------------------------------------------------
end

