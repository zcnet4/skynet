local skynet = require "skynet"
local socket = require "socket"
local table = table
local string = string

local mode, http_cmd = ...

if mode == "agent" then

local httpd = require "http.httpd"
local sockethelper = require "http.sockethelper"
local urllib = require "http.url"
local class = require "util/class"

local HANDLE_CMD = require(http_cmd)
-- 创建Handle类。
local Handle = class("Handle");
function Handle:ctor(id, url, code, method, header, body)
	self.id = id
	self.path, self.query = urllib.parse(url)
	self.code = code
	self.method = method
	self.header = header
	self.body = body
	self.response_statuscode = code
	self.response_header = nil
	self.session = 0
end

function Handle:dispatch(request_body)
	if self.code ~= 200 then
		self:response(nil)
	else
		--根据不同的path派发到不同的服务去
		local paths = string.split(self.path, '/');
		if #paths < 2 then
			body = "error"
			self:response("error")
		else
			local f = HANDLE_CMD[paths[2]];
			if f then
				f(self, table.unpack(paths, 3));
			else
				self:response("unprocess......")
			end
		end
	end
end


function Handle:response(body)
	local ok, err = httpd.write_response(sockethelper.writefunc(self.id), self.response_statuscode, body, self.response_header)
	if not ok then
		-- if err == sockethelper.socket_error , that means socket closed.
		skynet.error(string.format("fd = %d, %s", self.id, err))
	end
end

skynet.start(function()
	skynet.dispatch("lua", function (_,_,id)
		socket.start(id)
		-- limit request body size to 8192 (you can pass nil to unlimit)
		local code, url, method, header, body = httpd.read_request(sockethelper.readfunc(id), 8192)
		if code then
			--http request info
			local infos = "method = " .. method .. "     code = " .. code .. "   url = " .. url
			LOG.Info("httpagent httprequest", infos)

			local h = Handle.new(id, url, code, method, header, body)
			local ok, ret = pcall(Handle.dispatch, h, body)
			if ok then
				--
			else
				LOG.Info("Http Handle.dispatch", "!Ok")
				h:response("!Ok")
			end
			
			h = nil
		else
			if url == sockethelper.socket_error then
				skynet.error("socket closed")
			else
				skynet.error(url)
			end
		end
		socket.close(id)
	end)
end)

else

skynet.start(function()
	assert(type(mode) == "string" and type(http_cmd) == "string")
	local agent = {}
	for i= 1, 20 do
		agent[i] = skynet.newservice(SERVICE_NAME, "agent", http_cmd)
	end
	local balance = 1
	local id = socket.listen("0.0.0.0", tonumber(mode))
	skynet.error("Listen web port "..mode)
	socket.start(id , function(id, addr)
		skynet.error(string.format("%s connected, pass it to agent :%08x", addr, agent[balance]))
		skynet.send(agent[balance], "lua", id)
		balance = balance + 1
		if balance > #agent then
			balance = 1
		end
	end)
end)

end