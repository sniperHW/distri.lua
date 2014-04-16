local net = require "lua/net"
local table2str = require "lua/table2str"

local function Plus(arg)
	return nil,arg.a + arg.b
end
luanet.RegRPCFunction("Plus") = Plus
luanet.StartLocalService("PlusServer",SOCK_STREAM,net.netaddr_ipv4("127.0.0.1",8011))
luanet.Register2Name(net.netaddr_ipv4("127.0.0.1",8010))
