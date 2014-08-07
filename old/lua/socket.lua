local cjson = require "cjson"
local Que =  require "lua/queue"
local Sche = require "lua/scheduler"

local NET_DATA = 1
local ACCEPT_CALLBACK = 2
local CONNECT_CALLBACK = 3

local msgque = Que.Queue()
local lp_wait_on_msgque =  Que.Queue() --阻塞在提取msg上的light process

local function Quepush(ele)
	msgque:push(ele)
	local lp = lp_wait_on_msgque:pop()
	if lp then
		Sche.Schedule(lp)
		--Sche.WakeUp(lp)
	end
end

local function quepop()
	local lp = Sche.GetCurrentLightProcess()
	local ele = msgque:pop()
	while not ele do
		lp_wait_on_msgque:push(lp)
		Sche.Block()
		ele = msgque:pop()
	end
	return ele	
end


local function Close(socket)
	C.close(socket)
end

for i=1,8192 do
	Sche.Spawn(
		function() 
			while true do
				local msg = quepop()
				local type = msg[1]
				if type == NET_DATA then
					msg[2](msg[3],msg[4],msg[5])
					C.socket_subref(msg[3])
				elseif type == ACCEPT_CALLBACK then
					msg[2](msg[3])
				elseif type == CONNECT_CALLBACK then
					msg[2](msg[3],msg[4],msg[5])
				end
			end	
		end		
	  )
end	

local function Bind(socket,ondata)
	return C.bind(socket,{recvfinish = function (s,data,err)
									if data then data = cjson.decode(data) end
										--ondata(s,data,err)
										C.socket_addref(s)					
										Quepush({NET_DATA,ondata,s,data,err})
										--Sche.Schedule()
								  end})
end

return {
	Close = Close,
	Bind = Bind,
	Quepush = Quepush,
	NET_DATA = NET_DATA,
	ACCEPT_CALLBACK = ACCEPT_CALLBACK,
	CONNECT_CALLBACK = CONNECT_CALLBACK	
}
