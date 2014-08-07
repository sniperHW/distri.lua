local netaddr = require "lua/netaddr"
local Sche = require "lua/scheduler"
local luanet = require "lua/luanet"

--启动本地服务
luanet.StartLocalService("PlusClient",SOCK_STREAM,netaddr.netaddr_ipv4("127.0.0.1",8012))
count = 0
while not luanet.Register2Name(netaddr.netaddr_ipv4("127.0.0.1",8010)) do
	print("连接名字服务失败,1秒后尝试")
	Sche.Sleep(1000)
end
print("register sucess")
--启动一组lightprocess执行远程调用
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
						end
					end	
				end		
			  )
end

local tick = C.GetSysTick()
local now = C.GetSysTick()
while true do 
	now = C.GetSysTick()
	if now - tick >= 1000 then
		print(count*1000/(now-tick) .. " " .. now-tick)
		tick = now
		count = 0
	end
	Sche.Sleep(5)
end
