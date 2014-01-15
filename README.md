#一个简单高效的lua网络框架#

echo.lua

	local registernet = assert(package.loadlib("./luanet.so","RegisterNet"))
	registernet()
	dofile("net/net.lua")
	
	function process_packet(connection,packet)
		SendPacket(connection,packet)
	end
	
	function _timeout(connection)
		active_close(connection)
	end
	
	function client_come(connection)
		client_count = client_count + 1
	end
	
	function client_go(connection)
		client_count = client_count - 1
	end
	
	client_count = 0;
	
	tcpserver = net:new()
	
	function tcpserver:new()
	  	local o = {}
	  	self.__index = self
	  	self._process_packet = process_packet    --处理网络包
	    self._on_accept = client_come         --处理新到连接
		self._on_connect = nil
		self._on_disconnect = client_go     --处理连接关闭
		self._on_send_finish = nil
		self._send_timeout = _timeout
		self._recv_timeout = _timeout
	  	setmetatable(o, self)
		return o
	end
	
	
	function mainloop()
		local lasttick = GetSysTick()
		local server = tcpserver:new():listen(arg[1],arg[2])
		while server:run(50) == 0 do
			local tick = GetSysTick()
			if tick - 1000 >= lasttick then
				print("client:" .. client_count)
				lasttick = tick
			end
		end
		server = nil
		print("main loop end")
	end
	
	mainloop()
	
	lua echo.lua 127.0.0.1 8010
