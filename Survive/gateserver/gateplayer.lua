local Que = require "lua/queue"

local freeidx = Que.New()

local id2player = {}
local sock2player = {}

for i=1,4096 do
	freeidx:Push({v=i})
end

local function GetIdx()
	local n = freeidx:Pop()
	if n then 
		return n.v
	else
		return nil
end

local function ReleaseIdx(idx)
	freeidx:Push({v=idx})
end

local player = {}

function player:new()
  local idx = GetIdx()
  if not idx then
	return nil
  end
  local o = {}   
  setmetatable(o, self)
  self.__index = self
  o.idx = idx
  o.timestamp = GetSysTick()
  return o
end


local function GetPlayerById(id,timestamp)
	local ply = id2player[id]
	if ply and ply.timestamp == timestamp then
		return ply
	else
		return nil
	end
end

local function GetPlayerBySock(sock)
	return sock2player[sock]
end

local function NewGatePly(sock)
	local ply = player:new()
	if ply then
		ply.sock = sock
		id2player[ply.idx] = ply 
		sock2player[sock] = ply
	end
	return ply
end

local function PlayerDisconnected(ply)
	if ply.idx then
		id2player[ply.idx] = nil
		sock2player[ply.sock] = nil
		ReleaseIdx(ply.idx)
		ply.idx = nil
	end
end


return {
	NewGatePly = NewGatePly,
	PlayerDisconnected = PlayerDisconnected,
	GetPlayerBySock = GetPlayerBySock,
	GetPlayerById = GetPlayerById,
}
