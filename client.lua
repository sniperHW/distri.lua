local registernet = assert(package.loadlib("./luanet.so","RegisterNet"))  
registernet()
dofile("net/net.lua")

local recv_count = 0;

function connect_ok(connection)
	if connection then
		SendPacket(connection,"hello kenny")
	else
		print("connect timeout")
	end
end

function process_packet(connection,packet)
	recv_count = recv_count + 1
	SendPacket(connection,"hello kenny")
end

function _timeout(connection)
	active_close(connection)
end

tcpclient = net:new()

function tcpclient:new()
        local o = {}   
        self.__index = self
        self._process_packet = process_packet    --处理网络包
        self._on_accept = nil         --处理新到连接
        self._on_connect = connect_ok
        self._on_disconnect = nil     --处理连接关闭
        self._send_timeout = _timeout      
        self._recv_timeout = _timeout         
        setmetatable(o, self)
        return o
end        


function mainloop()
	local client = tcpclient:new()
	for i=1,arg[3] do
		client:connect(arg[1],arg[2],500)
	end
	local lasttick = GetSysTick()
	while client:run(50) == 0 do
		local tick = GetSysTick()
		if tick - 1000 >= lasttick then
			print("recv_count:" .. recv_count)
			lasttick = tick
			recv_count = 0
		end
	end
	client = nil
	print("main loop end")
end	
mainloop() 
