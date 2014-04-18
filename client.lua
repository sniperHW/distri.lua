local netaddr = require "lua/netaddr"
local table2str = require "lua/table2str"
local Sche = require "lua/scheduler"

--启动本地服务
luanet.StartLocalService("PlusClient",SOCK_STREAM,netaddr.netaddr_ipv4("127.0.0.1",8012))
--注册到NameService
luanet.Register2Name(netaddr.netaddr_ipv4("127.0.0.1",8010))
--启动100个lightprocess执行远程调用
for 1,100 do
	Sche.Spawn(
				function() 
					while true do
						local ret,err = luanet.RPCCall("PlusServer","Plus",{1,2})
						if err then
							return
						else
							print(ret)
						end
					end	
				end		
			  )
end


