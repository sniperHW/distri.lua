
function netaddr_ipv4(ip,port)
	return {type=AF_INET,ip=ip,port=port} 
end

function netaddr_ipv6(ip,port)
	return {type=AF_INET6,ip=ip,port=port}
end

function netaddr_local(path)
	return {type=AF_LOCAL,path=path}
end

