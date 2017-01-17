-- ywc.lua yw客户端
local class = require "util/class"
local sc = require "socketchannel"
local P = require "protobuf"
----------------------------------------------------------------------------------------
-- 创建ywc类。
local YWClient = class("ywc");

P.register_file("d:\\yx_code\\yx\\build\\proto_client.pb");

local function NetPack(content)
	local content_size = string.len(content)
	return string.pack(">I2c"..content_size, content_size, content)
end

local function ProtoPack(type, msg_data)
	local content = P.encode("Message", {type=type,msg_data=msg_data})
	local content_size = string.len(content)
	local xor = {}
	for i = 1, content_size do
		table.insert(xor, string.char(string.byte(content, i) ~ 165));
	end
	content = table.concat(xor)
	return string.pack(">I2c"..content_size, content_size, content)
end

local function ProtoUnPack(response, sz)
	local xor = {}
	for i = 1, sz do
		table.insert(xor, string.char(string.byte(response, i) ~ 165));
	end
	return P.decode("Message", table.concat(xor), sz);
end

local function Response(sock)
	local sz = sock:read(2)
	sz = string.unpack(">I2", sz)
	print("read -- sz:", sz)
	local buf = sock:read(sz)
	return true, ProtoUnPack(buf, sz)
end

function YWClient:ctor(identId)
	print("YWClient ctor")
end

local TOSERVER_LOGIN = 2
function YWClient:connect_gateway()
	self.channel = sc.channel {host="127.0.0.1",port=1767}
	self.channel:connect(true)
end

function YWClient:login()
	--
	local c2s_login = P.encode("c2s_login", {uid=1767,world_id=1})
	--
	local response = self.channel:request(ProtoPack(TOSERVER_LOGIN, c2s_login), Response)
	pp.print(response);
end

function YWClient:send(content)
	local response = self.channel:request(ProtoPack(3, content), Response);
	pp.print(response);
end

function YWClient:sendtest(content)
	self.channel:request(NetPack(content));
end

----------------------------------------------------------------------------------------
return YWClient

