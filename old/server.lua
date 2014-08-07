local netaddr = require "lua/netaddr"
local luanet = require "lua/luanet"
local Sche = require "lua/scheduler"

luanet.RegRPCFunction("Plus",
function (arg)
	--print("Plus")
	return nil,arg[1] + arg[2]
end
)

local function service_disconnected(name)
	print("service_disconnected " .. name)
end



luanet.StartLocalService("PlusServer",SOCK_STREAM,netaddr.netaddr_ipv4("127.0.0.1",8011),service_disconnected)
while not luanet.Register2Name(netaddr.netaddr_ipv4("127.0.0.1",8010)) do
	print("连接名字服务失败,1秒后尝试")
	Sche.Sleep(1000)
end




