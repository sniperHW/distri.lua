local Sche = require "lua/sche"

local function co_fun(name)
	while true do
		print(name)
		Sche.Sleep(1000)
	end
end

Sche.Spawn(co_fun,"co1")
Sche.Spawn(co_fun,"co2")
Sche.Spawn(co_fun,"co3")
Sche.Spawn(co_fun,"co4")

while true do
	Sche.Schedule()
end	
