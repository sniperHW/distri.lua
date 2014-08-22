local Socket = require "lua/socket"
local Sche = require "lua/sche"

local function listen(ip,port,maxpacket_size,decoder,process)
	local server = Socket.New(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
	local err = server:Listen(ip,port)
	if err then
		return err
	end
	Sche.Spawn(function ()	
		while true do
			Sche.Spawn(process,server:Accept(maxpacket_size,decoder))
		end
	end)
	return nil
end

return {
	Listen = listen
}
