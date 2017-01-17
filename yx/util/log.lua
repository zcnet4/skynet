--定义log输出接口
local core = require "skynet.core"

----------------------------------------------------------------------------------------
local _TO_STRING = nil
local _SESSION = 0
local function log_tostring(logLevel, tag, ...)
	if _TO_STRING == nil then
		_TO_STRING = require "util/pprint".pformat
	end
	local args = {...}
	-- select will get an accurate count of array len, counting trailing nils
	local len = select('#', ...)
	local str = {}
	_SESSION = _SESSION + 1
	table.insert(str, string.format("[%s] [%s] [%d] ", logLevel, os.date("%Y-%m-%d %H:%M:%S"), _SESSION))
	table.insert(str, tag)
	table.insert(str, " ==> ")
	for ix = 1,len do
		table.insert(str, _TO_STRING(args[ix], nil, nil))
		table.insert(str, ' ')
	end
	return table.concat(str)
end
----------------------------------------------------------------------------------------
local LOG = {}
local _LEVEL = {
	ERROR = 0, 
	WARNING = 1, 
	INFO = 2, 
	DEBUG = 3, 
}

local _LOG_LEVEL = _LEVEL.INFO
function LOG.SetLevel(logLevel)
	_LOG_LEVEL = logLevel
end

function LOG.Error(tag, ...)
	if _LOG_LEVEL >= _LEVEL.ERROR then
		core.send(".logger", 0, 0, log_tostring("ERROR", tag, ...))
	end
end

function LOG.Warning(tag, ...)
	if _LOG_LEVEL >= _LEVEL.WARNING then
		core.send(".logger", 0, 0, log_tostring("WARNING", tag, ...))
	end
end

function LOG.Info(tag, ...)
	if _LOG_LEVEL >= _LEVEL.INFO then
		core.send(".logger", 0, 0, log_tostring("INFO", tag, ...))
	end
end

function LOG.Debug(tag, ...)
	if _LOG_LEVEL >= _LEVEL.DEBUG then
		core.send(".logger", 0, 0, log_tostring("DEBUG", tag, ...))
	end
end

----------------------------------------------------------------------------------------
return LOG
