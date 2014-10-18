local Channel = require "lua.channel"
local Sche = require "lua.sche"

local actor = {}

function actor:new(identity)
  local o = {}
  self.__index = self      
  setmetatable(o,self)
  o.chan = Channel.New()
  o.identity = identity
  return o
end

function actor:Send(msg)
	self.chan:Send(msg)
end

function actor:Recv()
	return self.chan:Recv()
end

local id2actor = {}

local function actor_fun(identity,startfunc,a,...)
    local ret,err = pcall(startfunc,a,...)
    if not ret then
        print("error on actor start_fun:" .. err)
    end
    id2actor[identity] = nil
end


local function Spawn(identity,startfunc,...)
	if not identity or not startfunc then
		return nil
	end
	local a = actor:new(identity)
	Sche.Spawn(actor_fun,identity,startfunc,a,...)
	id2actor[identity] = a
	return a 	
end

local function GetActor(identity)
	return id2actor[identity]
end

local function BoardCast(msg,exclude)
	local function isInExclude(a)
		if not exclude then
			return false
		end
		for k,v in pairs(exclude) do
			if a == v then
				return true
			end
		end
		return false
	end
	for k,v in pairs(id2actor) do
		if not isInExclude(k) then
			v:Send(msg)
		end
	end	
end

return {
	Spawn = Spawn,
	GetActor = GetActor,
	BoardCast = BoardCast,
}




