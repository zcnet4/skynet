-- test_gf.lua 测试gateway-forward服务。
local skynet = require "skynet"
local _NEW_CLINET = require "mock/client".new
----------------------------------------------------------------------------------------
local gateway_forward_host = "127.0.0.1:2020"
--每一个client的服务会最多建立多少个连接（UID区域）
local client_max_num = 500
--每一个client的服务会建立多少个连接
local client_total_num = 10
local client_list = {}
----------------------------------------------------------------------------------------
-- local MOCK = {}
-- local N = 32
-- local cs = {}
-- function MOCK.Init()
-- 	cs[N] = true
-- 	for i = 1, N do
-- 		cs[i] = skynet.newservice(SERVICE_NAME, "slave", i)
-- 	end
-- 	for k,v in ipairs(cs) do
-- 		skynet.send(v, "lua", "send_msg")
-- 	end
-- end
----------------------------------------------------------------------------------------
-- local uid, package_num = ...
local ss, service_idx = ...
local harbor_id = tonumber(skynet.getenv("harbor") or 0)
local uid = harbor_id * service_idx * client_max_num
	
local CMD = {}

function CMD.send_msg(...)
	-- package_size(一个包的大小big:大包, small:小包)
	-- package_num(一次发多少个包)
	local package_num, package_size = ...
	package_num = package_num or 99
	package_size = package_size or "small"
	print("send_msg service_idx:" .. service_idx .. " num:" .. package_num .. " size:" .. package_size)
	for i = 1, client_total_num do
		local client = client_list[i]
		--发送 package_num 个包，数值在调用服务的时候可以更改
		for j = 1, package_num do
			client:send(package_size)
		end
	end
	print("send_msg end service_idx:" .. service_idx)
end

function CMD.connect()
	print("connect socket service_idx:" .. service_idx)
	for i = 1, client_total_num do
		local client = client_list[i]
		client:connect(gateway_forward_host)
	end
	print("connect socket end service_idx:" .. service_idx)
end

function CMD.close()
	print("close socket service_idx:" .. service_idx)
	for i = 1, client_total_num do
		local client = client_list[i]
		client:close()
	end
	print("close socket end service_idx:" .. service_idx)
end

----------------------------------------------------------------------

local MAIN = {}

function MAIN.init()
	--初始化一次把服务的所有连接对象初始化完
	print("test client init, service_idx:" .. service_idx .. ", start uid:" .. uid)
	for i = 1, client_total_num do
		client_list[i] = _NEW_CLINET(uid + i)
	end
end

----------------------------------------------------------------------

skynet.start(function()
	MAIN.init()


	-- --测试方法
	-- skynet.timeout(10, function()
	-- 	--
	-- 	--CMD.connect();
	-- 	skynet.call(skynet.self(), "lua", "connect")
	-- end)

	skynet.dispatch("lua", function (_, source, cmd, ...)
		local f = CMD[cmd]
		if f then
			f(...)
		end
	end)
end)
