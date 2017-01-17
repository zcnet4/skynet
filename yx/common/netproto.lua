-- yx_proto.lua
----------------------------------------------------------------------------------------
local M = {}
-- 相关的协议
M.C2S = {
    -- 访问S的协议
    CONNECT = 0x0001,
    CHALLENGE_RESPONSE = 0x0002,
    JOIN_SERVER = 0x0003,
    REQ_CONN_GATEWAY = 0x0004,
}
M.S2C = {
    -- 访问C的协议
    CONNECT = 0x1001,
    CHALLENGE = 0x1002,
    CONNECT_ACCEPT = 0x1003,
    CONNECT_REJECT = 0x1004,
    JOIN_SERVER_RESPONSE = 0x1005,
    REQ_CONN_GATEWAY_RESPONSE = 0x1006,
}
M.G2G = {
    GF2G_FORWARD = 0x2001,
    GF2G_FORWARD_CLOSE = 0x2002,
    G2GF_FORWARD = 0x2003,
}
----------------------------------------------------------------------------------------
return M
