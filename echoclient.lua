local net = require "lua/netaddr"
local cjson = require "cjson"
local Sche = require "lua/scheduler"
local count = 0

function on_data(s,data,err)
	if not data then
		print("a client disconnected")
		C.close(s)
	else
		count = count + 1
		local tb = cjson.decode(data)
		C.send(s,cjson.encode(tb),nil)
	end
end

function on_connected(s,remote_addr,err)
	print("on_connected")
	if s then
		if not C.bind(s,{recvfinish = on_data}) then
			print("bind error")
			C.close(s)
		else
			print("bind success")
			C.send(s,cjson.encode({"hahaha"}),nil)
		end
	end	
end
print("echoclient")
for i=1,1 do
	C.connect(IPPROTO_TCP,SOCK_STREAM,net.netaddr_ipv4("127.0.0.1",8010),nil,{onconnected = on_connected},3000)
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
