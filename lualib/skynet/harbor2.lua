local skynet = require "skynet"


skynet.register_protocol {
	name = "harbor",
	id = skynet.PTYPE_HARBOR,
	pack = function(...)
		local count = select("#", ...)
		local fmt = ""
		for i=1,count do
		 	fmt = fmt .. "s2"
		end
		return string.pack(">I2"..fmt, count, ...)
	end,
	unpack = skynet.tostring,
}

local harbor = {}

function harbor.globalname(name, handle)
	handle = handle or skynet.self()
	assert(#name <= 32)
	skynet.send(".harbor", "harbor", "REGISTER", name, handle)
end

function harbor.register(name, handle)
	handle = handle or skynet.self()
	assert(#name <= 32)
	skynet.send(".harbor", "harbor", "REGISTER", name, handle)
end

function harbor.queryname(name)
	assert(#name <= 32)
	return skynet.call(".harbor", "harbor", "QUERYNAME", name)
end

function harbor.link(id)
	skynet.call(".harbor", "harbor", "LINK", id)
end

function harbor.connect(id)
	skynet.call(".harbor", "harbor", "CONNECT", id)
end

function harbor.linkmaster()
	skynet.call(".harbor", "harbor", "LINKMASTER")
end

return harbor
