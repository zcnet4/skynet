-- main.lua 主服务
local skynet = require "skynet"
----------------------------------------------------------------------------------------
-- 服务名。
local _console_name 		= "console"
----------------------------------------------------------------------------------------
local M = {}

function M.Init()
	skynet.newservice(_console_name)
	skynet.newservice("mock/gateway")
	--skynet.newservice("mock/test_gf")
end
----------------------------------------------------------------------------------------
skynet.start(function()
	print("Main Mock Server start...")
	M.Init()

	skynet.dispatch("lua", function (_, source, cmd, ...)
		print("Main Mock service: dispatch CMD.....")
		local f = M[cmd]
		if f then
			skynet.ret(skynet.pack(f(source, ...)))
		else
			error(string.format("Main Mock service CMD %s", tostring(cmd)))
		end
	end)
end)

