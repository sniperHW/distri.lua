function tcp_listen(local_addr,cb)
	local callbackobj = {
		onaccept = cb
	}
	return clisten(local_addr,IPPROTO_TCP,SOCK_STREAM,callbackobj)
end

function tcp_asynconnect(remote_addr,local_addr,cb,timeout)
	local callbackobj = {
		onconnected = cb
	}
	return casyn_connect(remote_addr,local_addr,IPPROTO_TCP,SOCK_STREAM,callbackobj,timeout)
end

function tcp_connect(remote_addr,local_addr)
	return cconnect(remote_addr,local_addr,IPPROTO_TCP,SOCK_STREAM)
end






