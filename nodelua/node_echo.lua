--设计方向 node.lua使用方式
--echoserver范例

function doio(s)
	while true do
		local data,err = s:recv()
		if err == "disconnect" then
			return
		else
			s:send(data)
		end
	end
end

function listen_fun(l)
	while true do
		local s,err = l:accept()
		if s then
			node_spwan(s,doio) --spwan a light process to do io
		elseif err == "stop" then
			return
		end
	end
end

function main()		
	local l,err = node_listen("127.0.0.1",8010)
	if l then
		node_spwan(l,listen_fun) --spwan a light process to do accept
	end
	node_loop()
end

main()
