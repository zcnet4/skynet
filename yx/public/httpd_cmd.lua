local pp = require "util/pprint"
local urllib = require "http.url"
local util = require "util/util"
local invoke = require "common/invoke"
local cjson = require "cjson"
local httpc = require "http.httpc"
local md5 = require "md5"
local date = require "util/date"
local dc = require "datacenter"
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

----------------------------------------------------------------------------------------
return CMD