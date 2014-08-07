local netaddr = require "lua/netaddr"
local cjson = require "cjson"
local Socket = require "lua/socket"
--for ipv4
local function Listen4(addr,on_newclient)
	if addr then 
		addr.type = AF_INET
		if not addr.ip or not addr.port then
			return nil,"ip or port is nil"
		end
		return C.stream_listen(addr,{onaccept=function (s)
									if data then data = cjson.decode(data) end				
										Socket.Quepush({Socket.ACCEPT_CALLBACK,on_newclient,s})
								  end})
	else
		return nil,"addr is nil"
	end
end

local function Connect4(remoteaddr,localaddr,connect_callback,timeout)
	if remoteaddr then
		remoteaddr.type =AF_INET
		if not remoteaddr.ip or not remoteaddr.port then
			return nil,"remoteaddr ip or port is nil"
		end		
		if localaddr then
			localaddr.type =AF_INET
			if not localaddr.ip or not localaddr.port then
				return nil,"localaddr ip or port is nil"
			end					
		end
		return C.connect(SOCK_STREAM,remoteaddr,localaddr,{onconnected = function (s,remote_addr,err)
									if data then data = cjson.decode(data) end				
										Socket.Quepush({Socket.CONNECT_CALLBACK,connect_callback,s,remote_addr,err})
								  end},timeout)
	else
		return nil,"remoteaddr is nil"
	end
end

local function Send(socket,data)
	if not socket or not data then
		return false,"socket or data is nil"
	end
	return C.send(socket,cjson.encode(data),nil)
end

return {
	Listen4 = Listen4,
	Connect4 = Connect4,
	Send = Send,
}
