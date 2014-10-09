local Cjson = require "cjson"
local Que = require "lua/queue"
local Gate = require "Survive/groupserver/gate"

local freeidx = Que.New()

for i=1,65535 do
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

local createcha  = 1
local loading    = 2
local playing    = 3
local releasing  = 4

local player = {}
local actname2player ={}
local id2player = {}

function player:new(actname)
  local id = GetIdx()
  if not id then
	return nil
  end
  local o = {}   
  setmetatable(o, self)
  self.__index = self
  o.groupsession = id
  return o
end

local function NewPlayer(actname)
	local ply = player:new()
	if ply then
		ply.actname = actname
		id2player[ply.groupsession] = ply
		actname2player[actname] = ply		
	end
	return ply	
end

local function ReleasePlayer(ply)
	if ply.groupsession then
		id2player[ply.groupsession] = nil
		actname2player[ply.actname] = nil
		ReleaseIdx(ply.groupsession)
		ply.groupsession = nil
	end
end

local function GetPlayerById(id)
	return id2player[id]
end

local function GetPlayerByActname(actname)
	return actname2player[actname]
end

local function player:NotifyBeginPlay()
	
end

local function player:NotifyCreate()
	self.status = createcha
	return true,self.groupsession,"create"	
end

local function RegRpcService(app)
	app:RPCService("PlayerLogin",function (sock,actname,chaid,sessionid)
		local ply = GetPlayerByActname(actname)
		--local gatesession = {sock=sock,id = sessionid}
		if ply then 
			if ply.gatesession then
				return false,"invaild login"
			else
				--断线重连
				if ply.status == createcha then
					return ply:NotifyCreate()
				elseif ply.status == playing then
					Gate.Bind(GetGateBySock(sock),ply,sessionid)
					ply:NotifyBeginPlay()					
					if ply.gamesession then
						--通知gameserver断线重连
					end					
					return true,ply.groupsession					
				else
					ReleasePlayer(ply)
					return false,"group busy"				
				end				 
			end
		else
			ply = NewPlayer(actname)
		    if not ply then
				return false,"group busy"
		    end
		    Gate.Bind(GetGateBySock(sock),ply,sessionid)
		    if chaid == 0 then
				return ply:NotifyCreate()
		    else
				ply.status = loading
				ply.chaid = chaid
				--"hmget chaid:" .. chaid .. " chaname avatarid chainfo bag skills"
				local err,result = Db.Command("hmget chaid:" .. chaid .. " nickname avatarid")
				if err then
					ReleasePlayer(ply)
					return false,"group busy"
				end
				if not result then
					return ply:NotifyCreate()
				else
					ply.status   = playing
					ply.nickname = result[1]
					ply.avatarid = result[2]
					ply:NotifyBeginPlay()
					return true,ply.groupsession
				end  				
		    end	
		end		
	end)
end




--[[
local function wpk_write_agentsession(wpk,agent)
	wpk_write_uint32(wpk,agent.id.high)
	wpk_write_uint32(wpk,agent.id.low)
end

local function wpk_write_agentidx(wpk,agent)
	wpk_write_uint32(wpk,agent.id.high)
end

local function rpk_read_agentsession(rpk)
	local agentsession = {}
	agentsession.high = rpk_read_uint32(rpk)
	agentsession.low = rpk_read_uint32(rpk)	
	return agentsession
end

function player:OnBegPly(wpk)
	self.attr:OnBegPly(wpk)
	self.bag:OnBegPly(wpk)
	self.skills:OnBegPly(wpk)
end

function player:Send2Client(wpk)
	local agent = self.agent
	if not agent then
		return
	end	
	wpk_write_agentsession(wpk,agent)
	wpk_write_uint32(wpk,1)
	wpk_write_agentidx(wpk,agent)
	C.send(agent.conn,wpk)	
end

function Send2Agent(agent,wpk)
	wpk_write_agentidx(wpk,agent)
	C.send(agent.conn,wpk)	
end


function player:notifycreate()
	local agent = self.agent
	self.status = stat_normal 
	local wpk = new_wpk()
	wpk_write_uint16(wpk,CMD_GA_CREATE)
	wpk_write_uint16(wpk,self.groupid)
	wpk_write_agentsession(wpk,agent)	
	Send2Agent(agent,wpk)
end

function player:notifybegply()
	self.status = stat_playing
	local wpk = new_wpk()
	wpk_write_uint16(wpk,CMD_GC_BEGINPLY)
	wpk_write_uint16(wpk,self.avatarid)
	wpk_write_string(wpk,self.nickname)
	self:OnBegPly(wpk)
	wpk_write_uint16(wpk,self.groupid)
	self:Send2Client(wpk)	
end

function player:notifybusy()
	self.status = stat_normal 
	local wpk = new_wpk()
	wpk_write_uint16(wpk,CMD_GA_BUSY)
	wpk_write_agentsession(wpk,self.agent)	
	Send2Agent(self.agent,wpk)
end

local function cb_updateacdb(self,err,result)
	local ply = self.ply
	if err then
		ply.chaid = 0
		ply:notifybusy()	
		return
	end
	ply:create_character(ply.nickname)
end

local function get_id_callback(self,err,result)
	local ply = self.ply
	if err or not result then
		ply:notifybusy()
	end
	local chaid = result
	ply.chaid = chaid
	GroupApp.grouplog(LOG_INFO,"get_id_callback chaid:" .. chaid)

	local cmd = "set " .. ply.actname .. " " .. chaid
	err = Dbmgr.DBCmd(chaid,cmd,{callback = cb_updateacdb,ply=ply})
	if err then
		ply:notifybusy()
	end
end

local function db_create_callback(self,error,result)
	local ply = self.ply
	if error then
		ply:notifybusy()
	else
		ply:notifybegply()
	end
end

function player:create_character(nickname)
	self.nickname = nickname
	if self.chaid == 0 then
		local cmd = "incr chaid"
		local err = Dbmgr.DBCmd("global",cmd,{callback = get_id_callback,ply=self})
		if err then
			self:notifybusy()
		end	
	else
		local attr={
			[Name2idx.idx("level")] = 1,
			[Name2idx.idx("exp")] = 0,
			[Name2idx.idx("power")] = 0,
			[Name2idx.idx("endurance")] = 0,
			[Name2idx.idx("constitution")] = 0,
			[Name2idx.idx("agile")] = 0,
			[Name2idx.idx("lucky")] = 0,
			[Name2idx.idx("accurate")] = 0,
			[Name2idx.idx("movement_speed")] = 0,
			[Name2idx.idx("shell")] = 0,
			[Name2idx.idx("pearl")] = 0,
			[Name2idx.idx("soul")] = 0,
			[Name2idx.idx("action_force")] = 0,
			
			[Name2idx.idx("attack")] = 100,
			[Name2idx.idx("defencse")] = 100,
			[Name2idx.idx("life")] = 100,
			[Name2idx.idx("maxlife")] = 100,			
			[Name2idx.idx("dodge")] = 100,
			[Name2idx.idx("crit")] = 100,
			[Name2idx.idx("hit")] = 100,
			[Name2idx.idx("anger")] = 0,
			[Name2idx.idx("combat_power")] = 0,
		}	
		self.attr = Attr.New():Init(attr)
		self.bag = Bag.New():Init()
		self.skills = Skill.New():Init()
		
		if self.weapon then
		end		
		local cmd = string.format("hmset chaid:%u chaname %s avatarid %u chainfo %s bag %s skills %s",self.chaid,self.nickname,self.avatarid,self.attr:DbStr(),
								  self.bag:DbStr(),self.skills:DbStr())
		local err = Dbmgr.DBCmd(self.chaid,cmd,{callback = db_create_callback,ply=self})
		if err then
			self:notifybusy()
		end
		self.status = stat_creating			
	end
end

local function initfreeidx()
	local que = Que.Queue()
	for i=1,65535 do
		que:push({v=i,__next=nil})
	end
	return que
end 


local playermgr = {
	freeidx = initfreeidx(),
	players = {},
	actname2player ={},
}

function playermgr:new_player(actname)
	if not actname or actname == '' then
		return nil
	end
	if self.freeidx:is_empty() then
		return nil
	else
		local newply = player:new()
		newply.actname = actname
		newply.groupid = self.freeidx:pop().v
		self.players[newply.groupid] = newply
		self.actname2player[actname] = newply
		GroupApp.grouplog(LOG_INFO,actname .. " new_player groupid:" .. newply.groupid)
		return newply
	end
end

function playermgr:release_player(ply)
	if ply.groupid and ply.groupid >= 1 and ply.groupid <= 65535 then
		self.freeidx:push({v=ply.groupid,__next=nil})
		self.players[ply.groupid] = nil
		self.actname2player[ply.actname] = nil
		ply.groupid = nil
	end
end

function playermgr:getplybyid(groupid)
	return self.players[groupid]
end

function playermgr:getplybyactname(actname)
	if not actname or actname == '' then
		return nil
	end
	return self.actname2player[actname]
end


function load_chainfo_callback(self,error,result)
	local ply = self.ply
	if error then
		ply:notifybusy()
		return
	end
	
	if not result then
		ply:notifycreate()
		return 
	end
	ply.nickname = result[1]
	ply.avatarid = result[2]
	ply.attr =  Attr.New():Init(Cjson.decode(result[3]))
	ply.bag = Bag.New():Init(Cjson.decode(result[4]))
	ply.skills = Skill.New():Init(Cjson.decode(result[5]))
	ply.attr:Set("attack",100)
	ply.attr:Set("defencse",100)
	ply.attr:Set("life",100)
	ply.attr:Set("maxlife",100)	
	ply.attr:Set("dodge",100)
	ply.attr:Set("crit",100)
	ply.attr:Set("hit",100)
	ply.attr:Set("anger", 0)
	ply.attr:Set("combat_power",0)		
	ply:notifybegply()
end

------网络消息入口处理函数

local player_net_handler = {}

player_net_handler[CMD_AG_PLYLOGIN] = function (rpk,conn)
	local actname = rpk_read_string(rpk)
	local chaid = rpk_read_uint32(rpk)	
	local agentsession = rpk_read_agentsession(rpk)
	local ply = playermgr:getplybyactname(actname)
	local agent = {id = agentsession,conn = conn}
	if ply then
		if ply.agent then
			GroupApp.grouplog(LOG_INFO,actname .. " alread have gate")
			--此帐号已经连上服务器,本次登录请求非法
			local wpk = new_wpk()
			wpk_write_uint16(wpk,CMD_GA_PLY_INVAILD)
			wpk_write_agentsession(wpk,agent)
			wpk_write_agentidx(wpk,agent)
			C.send(conn,wpk)	
		else
			--客户端的断线重连	
			GroupApp.grouplog(LOG_INFO,actname .. " already in game")
			ply.agent = agent
			Gate.InsertGatePly(ply,agent)			
			if ply.status == stat_playing then
				ply:notifybegply()
				if ply.game then
					local gate = Gate.GetGateByConn(conn)	
					local param = {ply.game.id,{name=gate.name,id=agent.id}}
					local r = Rpc.RPCCall(ply.game.conn,"CliReConn",param,{OnRPCResponse=function (_,ret,err)
							if err then	
								GroupApp.grouplog(LOG_INFO,actname .. " CliReConn error")
							end
							end})
					if not r then
						GroupApp.grouplog(LOG_INFO,actname .. " CliReConn error")
					end
				end
			elseif ply.status == stat_normal then
				if not ply.bag or not ply.attr or not ply.skill then
					ply:notifycreate()
				end
			end	
		end
		return
	end
	ply = playermgr:new_player(actname)
	if not ply then
		--到达groupserver的容量限制，拒绝本次登录请求
		local wpk = new_wpk()
		wpk_write_uint16(wpk,CMD_GA_BUSY)
		wpk_write_agentsession(wpk,agent)
		wpk_write_agenidx(wpk,agent)
		C.send(conn,wpk)
	else
		ply.agent = agent
		Gate.InsertGatePly(ply,agent)
		if chaid == 0 then
			ply:notifycreate()
		else
			ply.chaid = chaid
			local cmd = "hmget chaid:" .. chaid .. " chaname avatarid chainfo bag skills"
			local err = Dbmgr.DBCmd(chaid,cmd,{callback = load_chainfo_callback,ply=ply})
			if err then
				ply:notifybusy()
			end
			ply.status = stat_loading
		end
	end
end

player_net_handler[CMD_CG_CREATE] = function (rpk,conn)
	local avatarid = rpk_read_uint8(rpk)
	local nickname = rpk_read_string(rpk)
	local weapon = rpk_read_uint8(rpk)
	local groupid = rpk_reverse_read_uint16(rpk)	
	local ply = playermgr:getplybyid(groupid)	
	if ply then
		if ply.status == stat_creating then
			return
		end
		ply.avatarid = avatarid
		ply.weapon = weapon	
		ply:create_character(nickname);
		GroupApp.grouplog(LOG_INFO,ply.actname .. " CG_CREATE:" .. nickname)
	else
	end
end

player_net_handler[CMD_AG_CLIENT_DISCONN] = function (rpk,conn)
	local groupid = rpk_reverse_read_uint16(rpk)	
	local ply = playermgr:getplybyid(groupid)
	if ply then
		Gate.RemoveGatePly(ply.agent,ply)
		ply.agent = nil
		if ply.game then
			local wpk = new_wpk()
			wpk_write_uint16(wpk,CMD_GGAME_CLIDISCONNECTED)
			wpk_write_uint32(wpk,ply.game.id)
			C.send(ply.game.conn,wpk)	
		end
	end
end


player_net_handler[CMD_CG_ENTERMAP] = function (rpk,conn)
	local groupid = rpk_reverse_read_uint16(rpk)
	local ply = playermgr:getplybyid(groupid)
	if ply and ply.status == stat_playing then
		if not ply.game then	
			local type = rpk_read_uint8(rpk)
			if MapMgr.EnterMap(ply,type) then
				ply.status = stat_enteringmap
			end
		end
	end
end

player_net_handler[DUMMY_ON_CHAT_CONNECTED] = function (rpk,conn)

end

local function reg_cmd_handler()
	C.reg_cmd_handler(CMD_AG_PLYLOGIN,player_net_handler)
	C.reg_cmd_handler(CMD_CG_CREATE,player_net_handler)
	C.reg_cmd_handler(CMD_AG_CLIENT_DISCONN,player_net_handler)
	C.reg_cmd_handler(CMD_CG_ENTERMAP,player_net_handler)
	C.reg_cmd_handler(DUMMY_ON_CHAT_CONNECTED,player_net_handler)
end

return {
	RegHandler = reg_cmd_handler,
}
]]--
