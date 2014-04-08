local function local_listen(local_addr,type,cb)
	local callbackobj = {}
	if type == SOCK_STREAM then
		callbackobj.onaccept = cb
	elseif type == SOCK_DGRAM then
		callbackobj.recvfinish = cb
	else
		return nil
	end
	return clisten(local_addr,0,type,callbackobj)	
end

local function local_connect(remote_addr,local_addr,type)
	return cconnect(remote_addr,local_addr,0,type)
end

return {
	local_listen = local_listen,
	local_connect = local_connect
}

