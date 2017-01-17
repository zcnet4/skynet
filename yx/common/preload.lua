-- preload.lua
--print("preload!!!")
require "util/stdext"
pp  = require "util/pprint"
LOG = require "util/log"
-- 将系统的print重定向到LOG.Info中。
--sprint = print
--print = LOG.Info