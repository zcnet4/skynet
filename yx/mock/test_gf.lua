-- test_gf.lua 测试gateway-forward服务。
local skynet = require "skynet"
local _NEW_CLINET = require "mock/client".new
----------------------------------------------------------------------------------------
local MOCK = {}
local N = 32
local gateway_forward_host = "127.0.0.1:2020"
local cs = {}
function MOCK.Init()
	cs[N] = true
	for i = 1, N do
		cs[i] = skynet.newservice(SERVICE_NAME, "slave", i)
	end
	for k,v in ipairs(cs) do
		skynet.send(v, "lua", "test_gf")
	end
end
----------------------------------------------------------------------------------------
local mode, uid = ...
if mode == "slave" then
local CMD = {}
function CMD.test_gf()
	print("test_gf uid:", uid)
	local c = _NEW_CLINET(uid)
	c:connect(gateway_forward_host)
	for i = 1, 99 do
		c:send()
	end
	print("end")
end
skynet.start(function()
	skynet.dispatch("lua", function (_, source, cmd, ...)
		local f = CMD[cmd]
		if f then
			f()
		end
	end)
end)
else
skynet.start(function()
	print("Gateway Mock Server start...")
	MOCK.Init()
	skynet.dispatch("lua", function (_, source, cmd, ...)
		print("Gateway Mock service: dispatch CMD.....")
		local f = MOCK[cmd]
		if f then
			local ret = f(source, ...)
			if ret ~= NORET then
				skynet.ret(skynet.pack(ret))
			end
		else
			error(string.format("Gateway Mock service CMD %s", tostring(cmd)))
		end
	end)
end)
end
