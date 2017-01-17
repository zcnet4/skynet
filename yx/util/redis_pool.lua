-- redis_pool.lua,redis连接池。
local redis = require "redis"
local class = require "util/class"
local LOG = LOG
----------------------------------------------------------------------------------------
-- 创建Pool类。
local Pool = class("Pool");

function Pool:ctor(minconn, maxconn)
	self.conns = {}
	self.current = 0
	self.minconn = minconn or 1
	self.maxconn = maxconn or 2
	self._LOCK = require "skynet.queue"()
end

function Pool:connect(conf)
	self.database = conf.host
	--
	for i=1,self.minconn do
		rdb = redis.connect {
        	host = conf.host,
        	port = conf.port,
        	db   = conf.db,
        	auth = conf.auth
    		}
		if not rdb then
			LOG.Info("redis pool", "failed to connect to redis server:", self.database)
			error("failed to connect to redis server:"..self.database)	
		else
			LOG.Info("redis pool", "success to connect to redis server:", self.database)
			table.insert(self.conns, rdb)
		end
	end
end

local function getIndex(self)
	local current = self.current + 1
	if current > self.minconn then
		current = 1
	end
	self.current = current
	return current
end

function Pool:getConn()
	local current = self._LOCK(getIndex, self)
	return self.conns[current]
end

local null_result = {}
function Pool:pipeline(ops)
	local conn = assert(self:getConn(), "")
	local errno = 0;
	local ok, res = pcall(conn.pipeline, conn, ops, {})
	if not ok then
		LOG.Warning("redis pool", "reconnect:", self.database)
		conn:reconnect()
		resp = {}
		ok, res = pcall(conn.pipeline, conn, ops, {})
	end
	if not ok then
		LOG.Error("redis pool", "pipeline:", self.database, " ops:", ops)
		return null_result, 400
	end
	--
	return res, 0
end

function Pool:keeplive()
	-- redis.ping不是并发的。。
	LOG.Info("redis pool", "keeplive:", self.database, " size:", #self.conns)
	for k,v in pairs(self.conns) do
		local ok, ret = pcall(v.ping, v)
		if not ok or not ret then
			LOG.Warning("redis pool", "keeplive reconnect:", self.database, " index:", k)
			v:reconnect()
		end
	end
end
----------------------------------------------------------------------------------------
return Pool