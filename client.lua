local netaddr = require "lua/netaddr"
local table2str = require "lua/table2str"
local Sche = require "lua/scheduler"
local luanet = require "lua/luanet"

--local tick = C.GetSysTick()
--local str = table2str.Table2Str({1,2,3,4,5,6,7,8,9,10})
--for i = 1,100000 do
--	table2str.Table2Str({1,2,3,4,5,6,7,8,9,10})
	--C.tab2str({1,2,3,4,5,6,7,8,9,10})
	--table2str.Str2Table(str)
--end
--print(C.GetSysTick() - tick)


--print(C.tab2str({a=1,b=2,c=3,d=4,e={2,3}}))


--启动本地服务
luanet.StartLocalService("PlusClient",SOCK_STREAM,netaddr.netaddr_ipv4("127.0.0.1",8012))
count = 0
while not luanet.Register2Name(netaddr.netaddr_ipv4("127.0.0.1",8010)) do
	print("连接名字服务失败,1秒后尝试")
	Sche.Sleep(1000)
end
print("register sucess")
--启动一组lightprocess执行远程调用
for i=1,10000 do	
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
	if now - tick > 1000 then
		print(count*1000/(now-tick))
		tick = now
		count = 0
	end
	Sche.Sleep(50)
end

