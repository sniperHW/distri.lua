local Thread = require "lua/thread"
local Sche = require "lua/scheduler"

local thd = Thread.Fork("thread2.lua") 


count = 0
--启动一组lightprocess执行远程调用
for i=1,20000 do	
	Sche.Spawn(
				function() 
					while true do
						local ret,err = Thread.Call(thd,"Plus",{1,2})
						if err then
							print(err)
							return
						else
							if ret ~= 3 then
								print("error")
							end
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
