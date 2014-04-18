local netaddr = require "lua/netaddr"
local table2str = require "lua/table2str"

local function Plus(arg)
	return nil,arg[1] + arg[2]
end
luanet.RegRPCFunction("Plus",Plus)
luanet.StartLocalService("PlusServer",SOCK_STREAM,netaddr.netaddr_ipv4("127.0.0.1",8011))
luanet.Register2Name(netaddr.netaddr_ipv4("127.0.0.1",8010))
