--名字到连接的映射
socket_map = {

}

rpk_skeleton_map = {
	
}


function block_connect(remote_addr,timeout)

end

function on_disconnect(s)
	local name = get_name(s)
	socket_map[name] = nil
	close(s)
end


function rpc_call(remote,name,arguments)
	local skeleton = rpk_skeleton_map[name]
	if skeleton then
		return skeleton_GetAddr(remote,arguments)
	else
		return nil,"unknow remote function"
	end
end

--远程调用的本地代理
function skeleton_GetAddr(remote,arguments)
	
	
end


function sendmsg(name,msg)
	--首先检查连接是否已经存在
	local s = socket_map[name]
	if not s then
		--没有建立连接,向nameservice发出查询信息,lua5.2以后无法通过arg获得不定参数，只能使用table
		local remote_addr,err = rpc_call(nameservice,"GetAddr",{name})	
		if err ~= nil then
			return -1
		end
		if remote_addr then
			local s = block_connect(remote_addr,3000)
			if s then
				socket_map[name] = s
				set_name(s,name);
			end
		else
			return -1
		end	
	end
	tcp.send(s,msg)
	return 0
end
