local Sche = require "lua.sche"
AddTopFilter("ssdb-server")
while true do
	print(Top())
	Sche.Sleep(1000)
end
