local socket = require "lua/socket"
local sche = require "lua/sche"

local function listen(ip,port,process)
	local server = socket.new(luasocket.AF_INET,luasocket.SOCK_STREAM,luasocket.IPPROTO_TCP)
	local err = server:listen(ip,port)
	if err then
		return err
	end
	sche.Spawn(function ()	
		while true do
			sche.Spawn(process,server:accept())
		end
	end)
	return nil
end

return {
	Listen = listen
}
