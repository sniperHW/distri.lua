local Avatar = require "Survive/gameserver/avatar"
local Player = require "Survive/gameserver/player"
local Que = require "lua/queue"
local Cjson = require "cjson"
local Gate = require "Survive/gameserver/gate"
local Attr = require "Survive/gameserver/attr"
local Skill = require "Survive/gameserver/skill"
local Aoi = require "Survive/aoi"
local Astar = require "Survive/astar"
local Timer = require "lua/timer"
local NetCmd = require "Survive/netcmd/netcmd"
local MsgHandler = require "Survive/netcmd/msghandler"
local IdMgr = require "Survive/common/idmgr"

local mapdef = {
	[1] = {
		gridlength = 100,          --管理格大小
		xcount,
		ycount,
		radius = 100,              --视距大小
		coli   = "./fightMap.meta",   --寻路碰撞文件
		astar  = nil,
	},
}

for k,v in ipairs(mapdef) do
	v.astar,v.xcount,v.ycount = Astar.create(v.coli)
	if not v.astar then
		print("astar init error")
	end
end

local function getDefByType(type)
	return mapdef[type]
end

local maps = {} --所有的地图实例
local mapidx = IdMgr.New(65535)

local function GetMapById(id)
	id = bit32.rshift(id,16)
	return maps[id]
end

local map = {
	maptype,
	mapid,
	astar,
	aoi,
	avatars,
	freeidx,
	plycount,      
	movingavatar, 
	movtimer,
}

function map:new(mapid,maptype)
	local o = {}   
	setmetatable(o, self)
	self.__index = self
	o.mapid = mapid
	o.maptype = maptype
	o.movingavatar = {}
	o.avatars = {}
	o.freeidx = IdMgr.New(4096)
	local mapdef = GetDefByType(maptype)	
	o.astar = mapdef.astar
	o.aoi = Aoi.create_map(mapdef.gridlength,mapdef.radius,0,0,mapdef.xcount-1,mapdef.ycount-1)
	o.movtimer = Timer.New():Register(function () o:process_mov() return true end)
	Sche.Spawn(function () o.movtimer:Run() end)
	maps[mapid] = o  
	return o
end

function map:GetAvatar(id)
	return self.avatars[id]
end

function map:entermap(plys)
	if self.freeidx:Len() < #plys then
		return nil
	else
		local gameids = {}
		local useidx  = {}
		for _,v in pairs(plys) do
			local avatid = v.avatid
			local gate = Gate.GetGateByName(v.gatesession.name)
			if not gate then
				for k1,v1 in pairs(useidx)
					self.freeidx:Push(v1)
				end
				return nil
			end
			local gateid = v.gatesession.id
			local id = self.freeidx:Pop()
			table.insert(useidx,id)
			local ply = Player:New(id,avatid)
			ply.gatesession = {sock=gate.sock,sessionid=v.gatesession.id}
			ply.nickname = v.nickname
			ply.actname = v.actname
			ply.groupsession = v.groupsession
			ply.attr = Attr.New():Init(ply,v.attr)
			ply.skillmgr = Skill.New()
			ply.id = bit32.lshift(self.mapid,16) + ply.id
			table.insert(gameids,ply.id)
			ply.pos[1] = 220
			ply.pos[2] = 120
			ply.dir = 5
			local wpk = CPacket.NewWPacket(64)
			wpk:Write_uint16(NetCmd.CMD_SC_ENTERMAP)
			wpk:Write_uint16(self.maptype)
			ply:on_entermap(wpk)	
			self.avatars[id] = ply
			ply.map = self
			Aoi.enter_map(self.aoi,ply.aoi_obj,ply.pos[1],ply.pos[2])
			print(ply.actname .. " enter map")
		end 
		return gameids
	end
end

function map:leavemap(id)
	local ply = self:GetAvatar(id)
	if ply then
		ply:Release()
		self.avatars[id] = nil
		return true
	end
	return false
end

function map:findpath(from,to)
	return Astar.findpath(self.astar,from[1],from[2],to[1],to[2])
end

function map:beginMov(avatar)
	if not self.movingavatar[avatar.id] then
		self.movingavatar[avatar.id] = avatar
	end
end

function map:Release()
	for k,v in ipairs(self.avatars) do
		v:Release()
	end
	Aoi.destroy_map(self.aoi)
	self.movtimer:Stop()
	maps[self.mapid] = nil
end

function map:process_mov()
	local stops = {}
	for k,v in pairs(self.movingavatar) do
		if v:process_mov() then
			table.insert(stops,k)
		end
	end
	
	for k,v in pairs(stops) do
		self.movingavatar[v] = nil
	end
	return 1 
end

--注册RPC服务
local function RegRpcService(app)
	app:RPCService("EnterMap",function (id,type,plys)
		local plyids
		local m
		if id == 0 then
			id = mapidx:Get()
			if not id then
				return {false,"game busy"}
			end
			m = map:new(id,maptype)
		else
			m = maps[mapid] 
			if not m then
				return {false,"invaild mapid"}				
			end
		end
		plyids = m:entermap(plys)
		if not plyids and id == 0 then
			m:Release()
		end
		return {not plyids,plyids} 			
	end)
	
	app:RPCService("LeaveMap",function (sock,actname,chaid,sessionid)
	end)
	
	app:RPCService("CliReConn",function (sock,actname,chaid,sessionid)
	end)
)		

--[[

Rpc.RegisterRpcFunction("LeaveMap",function (rpcHandle)
	local param = rpcHandle.param
	local mapid = param[1]
	local map = game.maps[mapid]
	if map then
		local plyid = rpk_read_uint16(rpk)
		if map:leavemap(plyid) then
			Rpc.RPCResponse(rpcHandle,mapid,nil)
			if map.plycount == 0 then
				map:clear()
				game.que:push({v=mapid,__next=nil})
				game.maps[mapid] = nil				
			end
		else
			Rpc.RPCResponse(rpcHandle,nil,"failed")
		end
	else
		Rpc.RPCResponse(rpcHandle,nil,"failed")
	end	
end)


Rpc.RegisterRpcFunction("CliReConn",function (rpcHandle)
	local param = rpcHandle.param
	local gameid = param[1]
	local mapid = bit32.rshift(gameid,16)
	local map = game.maps[mapid]
	if map then
		local plyid = bit32.band(gameid,0xFFFF)
		local ply = map.avatars[plyid]
		if ply and ply.avattype == Avatar.type_player then
			local gate = Gate.GetGateByName(param[2].name)
			if not gate then
				Rpc.RPCResponse(rpcHandle,nil,"failed")
				return
			end
			ply.gate = {conn=gate.conn,id=param[2].id}
			ply:reconn()
			Rpc.RPCResponse(rpcHandle,nil,nil)	
		else
			Rpc.RPCResponse(rpcHandle,nil,"failed")
		end
	
	else
		Rpc.RPCResponse(rpcHandle,nil,"failed")
	end
end)
]]--

--[[
local game_net_handler = {}

game_net_handler[CMD_CS_MOV] = function (rpk,conn)
	local gameid = rpk_reverse_read_uint32(rpk)
	local mapid = bit32.rshift(gameid,16)
	local map = game.maps[mapid]
	if map then
		local plyid = bit32.band(gameid,0xFFFF)
		local ply = map.avatars[plyid]
		if ply and ply.avattype == Avatar.type_player then
			local x = rpk_read_uint16(rpk)
			local y = rpk_read_uint16(rpk)
			ply:Mov(x,y)
		end
	end
end

game_net_handler[CMD_CS_USESKILL] = function (rpk,conn)
	print("CMD_CS_USESKILL")
	local gameid = rpk_reverse_read_uint32(rpk)
	local mapid = bit32.rshift(gameid,16)
	local map = game.maps[mapid]
	if map then
		local plyid = bit32.band(gameid,0xFFFF)
		local ply = map.avatars[plyid]
		if ply and ply.avattype == Avatar.type_player then
			ply:UseSkill(rpk)
		end
	end
end

game_net_handler[CMD_GGAME_CLIDISCONNECTED] =  function (rpk,conn)
	local gameid = rpk_reverse_read_uint32(rpk)
	local mapid = bit32.rshift(gameid,16)
	local map = game.maps[mapid]
	if map then
		local plyid = bit32.band(gameid,0xFFFF)
		--map:leavemap(plyid)
		local ply = map.avatars[plyid]
		if ply and ply.avattype == Avatar.type_player then
			ply.gate = nil
			print(ply.actname .. " disconnected")
		end		
	end	
end 
]]--
--return {
--	NewMap = function (mapid,maptype) return map:new():init(mapid,maptype) end,
--}
