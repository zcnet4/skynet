local skynet = require "skynet"
local util = {}

util.MASTER = ".MASTER"
util.MYSQL = ".MYSQL"
util.ECONOMIC = ".ECONOMIC"
util.MALL = ".MALL"
util.USER = ".USER"
util.CHARGE = "CHARGE"
util.RELAYORDER = "RELAYORDER"
util.CHARGESERVE = "CHARGESERVER"
util.RESIMPORT = "RESIMPORT"

function util.file_load(filename)
	local file
	if filename == nil then
		file = io.stdin
	else
		local err
		file, err = io.open(filename, "rb")
		if file == nil then
			error(("Unable to read '%s': %s"):format(filename, err))
		end
	end
	local data = file:read("*a")

	if filename ~= nil then
		file:close()
	end

	if data == nil then
		error("Failed to read " .. filename)
	end

	return data
end

return util