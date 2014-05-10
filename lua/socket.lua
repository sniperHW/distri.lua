local cjson = require "cjson"

local function Close(socket)
	C.close(socket)
end

local function Bind(socket,ondata)
	return C.bind(socket,{recvfinish = function (s,data,err)
									if data then data = cjson.decode(data) end					
									ondata(s,data,err)
								  end})
end

return {
	Close = Close,
	Bind = Bind
}
