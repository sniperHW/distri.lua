local netaddr = require "lua/netaddr"
local table2str = require "lua/table2str"
local luanet = require "lua/luanet"

local function Plus(arg)
	--print("Plus")
	return nil,arg[1] + arg[2]
end

local function service_disconnected(name)
	print("service_disconnected " .. name)
end


luanet.RegRPCFunction("Plus",Plus)
luanet.StartLocalService("PlusServer",SOCK_STREAM,netaddr.netaddr_ipv4("127.0.0.1",8011),service_disconnected)
if luanet.Register2Name(netaddr.netaddr_ipv4("127.0.0.1",8010)) then
	print("register sucess")
end
