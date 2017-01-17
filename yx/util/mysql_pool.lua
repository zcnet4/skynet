-- mysql_pool.lua,mysql连接池。
local mysql = require "mysql"
local class = require "util/class"
local LOG = LOG
----------------------------------------------------------------------------------------
-- 创建Pool类。
local Pool = class("Pool");

function Pool:ctor(maxconn)
	self.conns = {}
	self.current = 0
	self.maxconn = maxconn or 2
	self._LOCK = require "skynet.queue"()
end

function Pool:connect(conf)
	self.database = conf.database
	--
	for i=1,self.maxconn do
		local db = mysql.connect({
			host=conf.host,
			port=conf.port,
			database=conf.database,
			user=conf.user,
			password=conf.password,
			max_packet_size = 1024 * 1024,
			on_connect = function (db)
				db:query("set charset utf8");
			end
			})
		if not db then
			LOG.Info("mysql pool", "failed to connect to mysql server:", self.database)
			error("failed to connect to mysql server:"..self.database)	
		else
			LOG.Info("mysql pool", "success to connect to mysql server:", self.database)
			table.insert(self.conns, db)
		end
	end
end

local function getIndex(self)
	local current = self.current + 1
	if current > self.maxconn then
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
function Pool:query(sql)
	local conn = assert(self:getConn(), "")
	local errno = 0;
	local ok, res = pcall(mysql.query, conn, sql)
	if not ok then
		LOG.Warning("mysql pool", "reconnect:", self.database)
		conn:reconnect()
		ok, res = pcall(mysql.query, conn, sql)
	end
	if not ok then
		LOG.Error("mysql pool", "query:", self.database, " sql_string=", sql, " res.err = ", res.err)
		return null_result, 400
	end

	if res.badresult then
		LOG.Error("mysql pool", "query:", self.database, " sql_string=", sql, " res.err = ", res.err)		
		--
		errno = res.errno or 404
		res = null_result
	end
	return res, errno
end

function Pool:keeplive()
	-- mysql.ping不是并发的。。
	LOG.Info("mysql pool", "keeplive:", self.database, " size:", #self.conns)
	for k,v in pairs(self.conns) do
		local ok, ret = pcall(mysql.ping, v)
		if not ok or not ret then
			LOG.Warning("mysql pool", "keeplive reconnect:", self.database, " index:", k)
			v:reconnect()
		end
	end
end
----------------------------------------------------------------------------------------
return Pool