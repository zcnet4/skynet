local skynet = require "skynet"

----------------------------------------------------------------------------------------
skynet.start(function()
	LOG.Info("HUB", "HUB Server start...")
	local info = string.format("harbor_host:%s harbor_hub:%s", skynet.getenv("harbor_host"), skynet.getenv("harbor_hub"))
	LOG.Info("HUB", info)
	info = nil
	--skynet.exit()
end)