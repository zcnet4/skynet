-- main.lua 机器人主服务
local skynet = require "skynet"
require "skynet.manager"	-- import manager apis
local dc = require "datacenter"
----------------------------------------------------------------------------------------
-- 服务名。
local _console_name 		= "console"
local _httpd_name 			= "archit/httpd"
----------------------------------------------------------------------------------------
local M = {}
M._robots = {}

local function base_start()
	skynet.newservice(_console_name)
	-- 启动数据中心。
	local datacenter = skynet.newservice "datacenterd"
	skynet.name("DATACENTER", datacenter)
	-- 注册robot主服务，方便机器间数据访问。
	local harbor2 = require "skynet.harbor2"
	harbor2.register("/robot/main", skynet.self())
	-- 启动httpd服务容器
	skynet.newservice(_httpd_name, 7255, "robot/httpd_cmd")
end

function M.Init()
	base_start()
	--
	dc.set("robot_count", 0)
end

function M.AddRobot(source, robot_id)
	M._robots[tostring(robot_id)] = robot_id
	--
	local robot_count = dc.get("robot_count") + 1
	dc.set("robot_count", robot_count)
	--
	skynet.error(string.format("Robot[%u] Added! Robot Count:%u", robot_id, robot_count))
	pp.print(M._robots)
end
----------------------------------------------------------------------------------------
skynet.start(function()
	print("Robot Main Server start...")
	M.Init()

	skynet.dispatch("lua", function (_, source, cmd, ...)
		print("Robot Main Server: dispatch CMD.....")
		local f = M[cmd]
		if f then
			skynet.ret(skynet.pack(f(source, ...)))
		else
			error(string.format("Robot Main Server CMD %s", tostring(cmd)))
		end
	end)
end)

