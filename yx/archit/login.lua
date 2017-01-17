local skynet = require "skynet"
require "skynet.manager"	-- import manager apis
local util = require "util/util"
local pp = pp
local LOG = LOG

----------------------------------------------------------------------------------------
-- 服务名。
local _console_name 		= "console"
local _debug_console_name 	= "debug_console"
local _ap_name 				= "archit/ap"

-- 启动调试服务
local function start_debug()
	local skynet_console_on = tonumber(skynet.getenv("skynet_console_on") or 0)
	if skynet_console_on ~= 0 then
		skynet.newservice(_console_name)
	end 
	local skynet_debug_console_on = tonumber(skynet.getenv("skynet_debug_console_on") or 0)
	if skynet_debug_console_on ~= 0 then
		local debug_console_port = tonumber(skynet.getenv("skynet_debug_console_port") or 8000)
		skynet.newservice(_debug_console_name, debug_console_port)
	end
end

-- 启动网关服务
local function start_ap()
	local skynet_port = skynet.getenv("skynet_port")
	local skynet_max_client = tonumber(skynet.getenv("skynet_max_client"))

	local ap = skynet.newservice(_ap_name)
	skynet.call(ap, "lua", "open", {
		port = skynet_port,
		maxclient = skynet_max_client,
		nodelay = true,
	})
	LOG.Info("ap listen on ", skynet_port)
end
----------------------------------------------------------------------------------------
local Master = {}
-- 无返回参数定义
local NORET = {}

function Master.Init()
	-- 启动各类服务
	start_debug();
	start_ap();
	print("Master Init success")
end
----------------------------------------------------------------------------------------
skynet.start(function()
	LOG.Info("Master Service", "Strife Master Server start...")
	Master.Init()

	skynet.dispatch("lua", function (_, source, cmd, ...)
		LOG.Info("Master Service", "Master service: dispatch CMD.....", cmd)
		local f = Master[cmd]
		if f then
			local ret = f(source, ...)
			if ret ~= NORET then
				skynet.ret(skynet.pack(ret))
			end
		else
			error(string.format("Unknown Master service CMD %s", tostring(cmd)))
		end
	end)

	skynet.register(util.MASTER)
end)
