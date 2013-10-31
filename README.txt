为lua提供简单的网络访问接口,只支持字符串协议

echo.lua

local registernet = assert(package.loadlib("./luanet.so","RegisterNet"))
registernet()
dofile("net/net.lua") 

function process_packet(connection,packet)
        send2all(connection_set,packet)
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
        setmetatable(o, self)
        o._process_packet = process_packet,    --处理网络包
        o._on_accept = client_come,         --处理新到连接
        o._on_connect = nil,
        o._on_disconnect = client_go,     --处理连接关闭
        o._on_send_finish = nil,
        o._send_timeout = _timeout,      
        o._recv_timeout = _timeout, 
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
