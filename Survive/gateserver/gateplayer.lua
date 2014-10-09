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

local function DestroyPlayer(ply)
	if ply.idx then		
		--通知group和game连接断开
		id2player[ply.idx] = nil
		sock2player[ply.sock] = nil
		ReleaseIdx(ply.idx)
		ply.idx = nil
	end
end

local function OnPlayerDisconnected(sock,errno)
	local ply = GetPlayerBySock(sock)
	if not ply then
		return
	end	
	if ply.status == verifying or ply.status == login2group then
		ply.status = "closing"	
	else
		DestroyPlayer(ply)
	end
end

local function IsVaild(ply)
	return ply.status ~= "closing"
end

return {
	NewGatePly = NewGatePly,
	GetPlayerBySock = GetPlayerBySock,
	GetPlayerById = GetPlayerById,
	DestroyPlayer = DestroyPlayer,
	OnPlayerDisconnected = OnPlayerDisconnected,
	IsVaild = IsVaild,
	verifying = 1,
	login2group = 2,
	createcha = 3,
	playing = 4,
}
