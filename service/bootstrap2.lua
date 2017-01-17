local skynet = require "skynet"
require "skynet.manager"	-- import skynet.launch, ...
local memory = require "memory"

skynet.start(function()
	local sharestring = tonumber(skynet.getenv "sharestring" or 4096)
	memory.ssexpand(sharestring)

	local standalone = skynet.getenv "standalone"

	local launcher = assert(skynet.launch("snlua","launcher"))
	skynet.name(".launcher", launcher)

	local harbor_id = tonumber(skynet.getenv "harbor" or 0)
	if harbor_id == 0 then
		assert(standalone ==  nil)
		standalone = true
		skynet.setenv("standalone", "true")
	else
		standalone = false
		skynet.setenv("standalone", "false")
	end
	
	if standalone then
		local datacenter = skynet.newservice "datacenterd"
		skynet.name("DATACENTER", datacenter)
	else
		assert(skynet.launch("harbor2", "master"))
	end

	skynet.newservice "service_mgr"
	pcall(skynet.newservice, skynet.getenv "start" or "main")
	skynet.exit()
end)
