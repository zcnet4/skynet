-- robot_gateway.lua 机器人测试网关
local skynet = require "skynet"
----------------------------------------------------------------------------------------
-- 服务名。
local _test_client_service = "mock/test_client_service"
----------------------------------------------------------------------------------------
local M = {}

local test_client_services = {}
local test_client_services_count = 10
local function get_test_client_service(index)
	local s = test_client_services[index]
	if not s then
		s = skynet.newservice(_test_client_service, "??", index)
		test_client_services[index] = s
	end
	return s
end

local function gen_test_client_service()
	for i = 1, test_client_services_count do
		get_test_client_service(i)
	end
end

function M.testGateway_Conncet()
	gen_test_client_service()
	--
	for i = 1, test_client_services_count do
		local s = get_test_client_service(i)
		skynet.send(s, "lua", "connect")
	end 
end

function M.testGateway_SendMsg(...)
	gen_test_client_service()
	--
	local package_num, package_size = ...
	for i = 1, test_client_services_count do
		local s = get_test_client_service(i)
		skynet.send(s, "lua", "send_msg", package_num, package_size)
	end 
end
----------------------------------------------------------------------------------------
return M

