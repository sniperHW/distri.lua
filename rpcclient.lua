local Socket = require "lua/socket"
local Sche = require "lua/sche"
local Packet = require "lua/packet"
local Connection = require "lua/connection"
local App = require "lua/application"
local RPC = require "lua/rpc"

local rpcclient = App.Application()

local function on_disconnected(conn,err)
	print("conn disconnected:" .. err)
	conn:close()	
end

rpcclient:run(function ()
	for i=1,500 do
		Sche.Spawn(function () 
			local client = Socket.new(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
			if client:connect("127.0.0.1",8000) then
				print("connect to 127.0.0.1:8000 error")
				return
			end
			local conn = Connection.Connection(client,Packet.RPacketDecoder(65535))
			rpcclient:add(conn,nil,on_disconnected)
			for j=1,50 do
				Sche.Spawn(function (conn)
					while true do			
						local rpccaller = RPC.RPC_MakeCaller(conn,"Plus")
						local err,ret = rpccaller:Call(1,2)
						if err then
							print(err)
							conn:close()
							return
						end
					end
				end,conn)
			end
		end)	
	end
end)

while true do
	Sche.Sleep(100)
end
