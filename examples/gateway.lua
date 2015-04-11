--[[
一个简单的网关转发服务,将收到的来自客户端的数据包转发到服务hello2.lua,
hello2.lua回射给网关服务,网关接收到之后再回射给相应的客户端

测试方法,首先在8001端口启动hello2.lua
之后再启动gateway.lua
]]--

local sche = require "lua.sche"
local TcpServer = require "lua.tcpserver"
local App = require "lua.application"
local Timer = require "lua.timer"
local Socket = require "lua.socket"

--首先建立到127.0.0.1:8001的连接
local toserver = Socket.Stream.New(CSocket.AF_INET)
local socketmap = {}
if toserver:Connect("127.0.0.1",8001) then
	print("connect to 127.0.0.1:8000 error")
else
	local count = 0
	local toserver_app = App.New()
	function on_server_packet(s,rpk)
		--收到来自服务器的数据包，将数回射给相应的客户端
		local str = rpk:Read_string()
		local sock_str = rpk:Read_string()
		local client = socketmap[sock_str]
		if client then
			count = count + 1
			client:Send(Socket.RawPacket(str))	
		end
	end
	toserver_app:Add(toserver:Establish(Socket.Stream.RDecoder()),on_server_packet)
	local toclient_app = App.New()
	local function on_client_packet(s,rpk)
		local str = rpk:Read_rawbin()
		local wpk = Socket.WPacket(64)
		wpk:Write_string(str)
		wpk:Write_string(s:tostring())
		toserver:Send(wpk)	
	end
	--连接服务器成功,启动对客户端的监听
	--local success
	--toclient_app:Run(function ()
	local success = not TcpServer.Listen("127.0.0.1",8000,function (client)
			client:Establish(Socket.Stream.RawDecoder())
			socketmap[client:tostring()] = client
			toclient_app:Add(client,on_client_packet,function (s,err)
						socketmap[s:tostring()] = nil				
			end
			)		
		end)

	if success then
		print("gateway start on 127.0.0.1:8000")
		local last = C.GetSysTick()
		local timer = Timer.New():Register(function ()
			local now = C.GetSysTick()
			print("count:" .. count*1000/(now-last) .. " " .. now-last)
			count = 0
			last = now
		end,1000):Run()
	else
		print("gateway start error\n")
	end
end


