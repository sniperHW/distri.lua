local function tcp_listen(local_addr,cb)
	local callbackobj = {
		onaccept = cb
	}
	print("tcp_listen")
	print(IPPROTO_TCP)
	print(SOCK_STREAM)
	return clisten(IPPROTO_TCP,SOCK_STREAM,local_addr,callbackobj)
end

local function tcp_asynconnect(remote_addr,local_addr,cb,timeout)
	local callbackobj = {
		onconnected = cb
	}
	return casyn_connect(IPPROTO_TCP,SOCK_STREAM,remote_addr,local_addr,callbackobj,timeout)
end

local function tcp_connect(remote_addr,local_addr)
	return cconnect(IPPROTO_TCP,SOCK_STREAM,remote_addr,local_addr)
end

local function tcp_send(s,msg)
	if type(msg) == "table" then
		return csend(s,msg.ptr,nil)
	elseif type(msg) == "string" then
		return csend(s,msg,nil)
	end
end

return {
	tcp_listen = tcp_listen,
	tcp_asynconnect = tcp_asynconnect,
	tcp_connect = tcp_connect,
	send = tcp_send
}





