local skynet = require "skynet"
local pp = require "util/pprint"
local dc = require "datacenter"
local invoke = require "common/invoke"
----------------------------------------------------------------------------------------
local CMD = {}

local function http_call(session, call_type, cmd, ...)
	LOG.Info("http_call", session, call_type, cmd, ...)
	local ret = invoke.call(call_type, cmd, ...)
	LOG.Info("http_call", session, call_type, cmd, ..., "call finished")
	return ret
end

function CMD.call(h, ...)
	-- 仅供测试用！
	local args = {...}
	local cmd = args[1] .. "/" .. args[2]
	print("http call cmd:", cmd)
	print("http call args:", table.unpack(args, 3))

	local ret = http_call(h.session, invoke.CALL_TYPE_ADMIN, cmd, table.unpack(args, 3))
 	h:response(pp.tostring(ret))
end

----------------------------------------------------------------------------------------

function CMD.echo(h, ...)
	h:response("loveMMbyZC")
end

function CMD.listGates(h, ...)
	h:response(dc.get("listGates"))
end

local function robot_call(i, dst, method, ...)
	skynet.send("/robot/robot"..i, "lua", "Invoke", dst, method, ...)
end

function CMD.gateway(h, ...)
	local robot_count = dc.get("robot_count")
	pp.print(robot_count)
	for i = 1, robot_count do
		robot_call(i, "gateway", select(1, ...))
	end
	h:response("Ok")
end

----------------------------------------------------------------------------------------
return CMD