local Sche = require "lua/scheduler"
local Socket = require "lua/socket"
local Tcp = require "lua/tcp"
local count = 0

function on_data(s,data,err)
	if not data then
		--print("a client disconnected")
		Socket.Close(s)
	else
		--print(data[1])
		count = count + 1
		--Tcp.Connect4({ip="127.0.0.1",port=8010},nil,on_connected,3000)
		Tcp.Send(s,data)
	end
end


function on_connected(s,remote_addr,err)
	--print("on_connected")
	if s then
		if not Socket.Bind(s,on_data) then
			--print("bind error")
			Socket.Close(s)
		else
			--print("bind success")
			Tcp.Send(s,{"hahahha"})
		end
	end
end
print("echoclient")
for i=1,100 do
	Tcp.Connect4({ip="127.0.0.1",port=8010},nil,on_connected,3000)
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
	Sche.Sleep(50)
end

