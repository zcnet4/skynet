-- YWC_MGR.lua ywc管理服务
local skynet = require "skynet"
local _NEW_YWC = require "mock/ywc".new
----------------------------------------------------------------------------------------
local YWC_MGR = {}
-- 无返回参数定义
local NORET = {}

function YWC_MGR.Init()
	local ywc1 = _NEW_YWC()
	ywc1:connect_gateway()
	ywc1:login()
	ywc1:send("loveMM")
	--ywc1:sendtest("loveMM")
end



----------------------------------------------------------------------------------------
skynet.start(function()
	print("YWC_MGR Server start...")
	YWC_MGR.Init()

	skynet.dispatch("lua", function (_, source, cmd, ...)
		print("YWC_MGR service: dispatch CMD.....")
		local f = YWC_MGR[cmd]
		if f then
			local ret = f(source, ...)
			if ret ~= NORET then
				skynet.ret(skynet.pack(ret))
			end
		else
			error(string.format("Unknown Test service CMD %s", tostring(cmd)))
		end
	end)
end)

