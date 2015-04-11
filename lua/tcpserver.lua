local Socket = require "lua.socket"
local Sche = require "lua.sche"

local function listen(ip,port,process)
	local server = Socket.Stream(CSocket.AF_INET)
	local err = server:Listen(ip,port)
	if err then
		return err
	end
	Sche.Spawn(function ()	
		while true do
			Sche.Spawn(process,server:Accept())
		end
	end)
	return nil
end

return {
	Listen = listen
}
