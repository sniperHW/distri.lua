local Sche = require "lua.sche"
local Actor = require "examples.actor"

for i=1,10 do
	Actor.Spawn(i,function (self)
		while true do
			local msg = self:Recv()
			print(msg,self.identity)
		end	
	end)
end

while true do
--[[	for i=1,10 do
		local a = Actor.GetActor(i)
		if not a then
			print("can not get actor " .. i)
		else
			a:Send("hello")
		end
	end
]]--
	--test boardcast
	Actor.BoardCast("hello",{1,3,5,7,9})
	Sche.Sleep(1000)
end	
