local Lock = require "lua/lock"
local Sche = require "lua/sche"

local mtx = Lock.New()

Sche.Spawn(function ()
	while true do
		mtx:Lock()
		print("1")	
		Sche.Sleep(1000)
		mtx:Unlock()
	end
end)

Sche.Spawn(function ()
	while true do
		mtx:Lock()
		print("3")	
		Sche.Sleep(1000)
		mtx:Unlock()
	end
end)

Sche.Spawn(function ()
	while true do
		mtx:Lock()
		print("4")	
		Sche.Sleep(1000)
		mtx:Unlock()
	end
end)


while true do
	mtx:Lock()
	print("2")	
	Sche.Sleep(1000)
	mtx:Unlock()	
end


