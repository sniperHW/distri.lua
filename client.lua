local netaddr = require "lua/netaddr"
local table2str = require "lua/table2str"
local Sche = require "lua/scheduler"
local luanet = require "lua/luanet"
--启动本地服务
luanet.StartLocalService("PlusClient",SOCK_STREAM,netaddr.netaddr_ipv4("127.0.0.1",8012))

count = 0

--注册到NameService
if luanet.Register2Name(netaddr.netaddr_ipv4("127.0.0.1",8010)) then
	print("register success")
	--启动100个lightprocess执行远程调用
	for i=1,5000 do	
		Sche.Spawn(
					function() 
						while true do
							--print("call Plus")
							local ret,err = luanet.RPCCall("PlusServer","Plus",{1,2})
							if err then
								print(err)
								return
							else
								count = count + 1
								--print(ret)
							end
						end	
					end		
				  )
	end
	
	local tick = C.GetSysTick()
	local now = C.GetSysTick()
	while true do 
		now = C.GetSysTick()
		if now - tick > 1000 then
			print(count)
			tick = now
			count = 0
		end
		Sche.Sleep(50)
	end	
end			  



