-- robot.lua 机器人服务
local skynet = require "skynet"
----------------------------------------------------------------------------------------
-- 服务名。
local _console_name 		= "console"
----------------------------------------------------------------------------------------
local M = {}
M._units = {
	gateway = require "robot/robot_gateway",
}

function M.Init()
	-- 注册自己
	local harbor_id = skynet.getenv("harbor")
	M.robot_id = harbor_id
	assert(tonumber(harbor_id or 0) ~= 0)
	local harbor2 = require "skynet.harbor2"
	harbor2.register("/robot/robot"..harbor_id, skynet.self())
	skynet.error("Robot["..harbor_id.."] Server start...")
	-- 通知主服务，机器人已到位。by ZC. 2017-1-10 17:18
	skynet.send("/robot/main", "lua", "AddRobot", M.robot_id)
end

function M.Invoke(source, dst, method, ...)
	skynet.error("Robot["..M.robot_id.."] Invoke dst="..dst.." method="..method)
	local unit = M._units[tostring(dst)]
	if unit then
		unit[tostring(method)](...)
	else
		return "error"	
	end
end

----------------------------------------------------------------------------------------
skynet.start(function()
	M.Init()

	skynet.dispatch("lua", function (_, source, cmd, ...)
		print("Robot Server: dispatch CMD.....")
		local f = M[cmd]
		if f then
			skynet.ret(skynet.pack(f(source, ...)))
		else
			error(string.format("Robot Server CMD %s", tostring(cmd)))
		end
	end)
end)

