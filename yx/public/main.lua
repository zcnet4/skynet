local skynet = require "skynet"
require "skynet.manager"	-- import manager apis
local util = require "util/util"
local pp = pp
local LOG = LOG

----------------------------------------------------------------------------------------
-- 服务名。
local _console_name 		= "console"
local _debug_console_name 	= "debug_console"
local _httpd_name 			= "archit/httpd"

-- 启动调试服务
local function base_start()
	--
	local datacenter = skynet.newservice "datacenterd"
	skynet.name("DATACENTER", datacenter)
	local harbor2 = require "skynet.harbor2"
	harbor2.register("/public/datacenter", datacenter)
end

local function http_start()
	skynet.newservice(_httpd_name, skynet.getenv("public_http_port"), "public/httpd_cmd")
end

----------------------------------------------------------------------------------------
local Main = {}
-- 无返回参数定义
local NORET = {}

function Main.Init()
	-- 启动各类服务
	base_start();
	http_start();
	print("Main Init success")
end
----------------------------------------------------------------------------------------
skynet.start(function()
	LOG.Info("Main Service", "Strife Main Server start...")
	Main.Init()

	skynet.dispatch("lua", function (_, source, cmd, ...)
		LOG.Info("Main Service", "Main service: dispatch CMD.....", cmd)
		local f = Main[cmd]
		if f then
			local ret = f(source, ...)
			if ret ~= NORET then
				skynet.ret(skynet.pack(ret))
			end
		else
			error(string.format("Unknown Main service CMD %s", tostring(cmd)))
		end
	end)
end)
