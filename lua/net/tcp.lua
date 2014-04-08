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
	return casyn_connect(remote_addr,local_addr,IPPROTO_TCP,SOCK_STREAM,callbackobj,timeout)
end

local function tcp_connect(remote_addr,local_addr)
	return cconnect(remote_addr,local_addr,IPPROTO_TCP,SOCK_STREAM)
end

return {
	tcp_listen = tcp_listen,
	tcp_asynconnect = tcp_asynconnect,
	tcp_connect = tcp_connect
}





