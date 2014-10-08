local handler = {}

local function regHandler(cmd,f)
	handler[cmd] = f
end

local function onMsg(sock,rpacket)
	local cmd = rpacket:Read_uint16()
	local f = handler[cmd]
	if f then
		f(sock,rpacket)
	end
end

return {
	RegHandler = regHandler,
	OnMsg = onMsg
}
