local pprint = require "util/pprint"
----------------------------------------------------------------------------------------
local INVOKE = {}

--定义接口调用的来源类型
INVOKE.CALL_TYPE_CHATSERVER 	= 1000
INVOKE.CALL_TYPE_CLIENT 		= 1001
INVOKE.CALL_TYPE_HTTP		= 1002
INVOKE.CALL_TYPE_ADMIN		= 1003
----------------------------------------------------------------------------------------
--该接口提供给 接收chatserver和client端发起的接口调用 使用；
function INVOKE.call(call_type, cmd, ...)
	LOG.Info("INVOKE.call:", cmd, ...)
	
	local args = string.split(cmd, '/');
	if #args ~= 2 then
		error("INVOKE.call: "..cmd)
	end
	--
	local require_path = string.format("common/%s_i", args[1])
	local cmd_module = assert(require (require_path))
	--优先校验调用权限
	local enabled, msg = INVOKE.checkCallPermission(call_type, cmd_module, args[2])
	if enabled then
		local fun = cmd_module[args[2]]
		if fun then
			return fun(...)
		else
			return "INVOKE.call: ".. cmd .. " not exist." 
		end
	else
		return msg
	end
end

--检查发起调用的来端，是否具有该接口的调用权限
function INVOKE.checkCallPermission(call_type, cmd_module, call_name)
	if call_type == INVOKE.CALL_TYPE_CHATSERVER then
		local chatserver = cmd_module.chatserver
		if chatserver then
			local fun = chatserver[call_name]
			if fun then
				return true, "OK"
			end
		end
	elseif call_type == INVOKE.CALL_TYPE_CLIENT then
		local client = cmd_module.client
		if client then
			local fun = client[call_name]
			if fun then
				return true, "OK"
			end
		end
	elseif call_type == INVOKE.CALL_TYPE_HTTP then
		local http = cmd_module.http
		if http then
			local fun = http[call_name]
			if fun then
				return true, "OK"
			end
		end
	elseif call_type == INVOKE.CALL_TYPE_ADMIN then
		--admin，用于开发测试
		return true
	end

	return false, "Error, Call Interface is not exist!"
end

----------------------------------------------------------------------------------------

return INVOKE