local NetCmd = require "Survive/netcmd/netcmd"

local sock2game = {}
local name2game = {}

local function RegRpcService(app)
	--gameserver登录到groupserver
	app:RPCService("GameLogin",function (sock,name,ip,port)
		if sock2game[sock] == nil and name2game[name] == nil then
			local game = {sock = sock,name = name,players={},plycount=0,ip=ip,port=port}
			sock.type = "game"
			sock2game[sock] = game
			name2game[name] = game
			print(name .. " Login Success")
			--通知gate有gameserver连上group	
			local wpk = CPacket.NewWPacket(128)
			wpk:Write_uint16(NetCmd.CMD_GA_NOTIFY_GAME)
			wpk:Write_string(name)
			wpk:Write_string(ip)
			wpk:Write_uint16(port)
			BoradCast2Gate(wpk)
			
			--[[
			for test
			local wpk1 = CPacket.NewWPacket(128)
			wpk1:Write_uint16(NetCmd.CMD_GA_NOTIFY_GAME)
			wpk1:Write_string(name)
			wpk1:Write_string(ip)
			wpk1:Write_uint16(port)
			
			local wpk2 = CPacket.NewWPacket(64)
			wpk2:Write_wpk(wpk1)
			local rpk = CPacket.NewRPacket(wpk2) 
			local wpk3 = CPacket.NewWPacket(rpk:Read_string())
			
			
			--local rpk2 = CPacket.NewRPacket(wpk3)
			
			--print(rpk2:Read_uint16())
			--print(rpk2:Read_string())
			
			
			BoradCast2Gate(wpk3)]]--
			
			return "Login Success"
		else
			return "Login failed"
		end
	end)
end

local function GetGames()
	local ret = {}
	for k,v in pairs(name2game) do
		table.insert(ret,{v.name,v.ip,v.port})
	end
	return ret
end

local function OnGameDisconnected(sock,errno)
	local game = sock2game[sock]
	if game then
		print(game.name .. " disconnected")
		for k,v in pairs(game.players) do
			v.gamesession = nil
		end
		sock2game[sock] = nil
		name2game[game.name] = nil
	end	
end

local function Bind(game,player,sessionid)
	 game.players[player] = player
	 game.plycount = game.plycount + 1
	 player.gamesession = {game=game,sessionid=sessionid}
end

local function UnBind(player)
	local game = player.gamesession
	if game then
		game.players[player] = nil
		game.plycount = game.plycount - 1
		player.gamesession = nil
	end
end

return {
	Bind = Bind,
	UnBind = UnBind,
	OnGameDisconnected = OnGameDisconnected,
	RegRpcService = RegRpcService,
	GetGames   = GetGames,
}


--[[

local gamemgr = {
	con2game  = {},
	name2game = {},
	size = 0,
}


function BoradCast2Game(wpk)
	for k,_ in pairs(gamemgr.con2game) do
		local l_wpk = C.new_wpk_by_wpk(wpk)
		C.send(k,l_wpk)
	end
	destroy_wpk(wpk)
end

--gate登陆上group之后将连上group的game信息发送给gate
local function on_gate_login(gate)
	local wpk = new_wpk(4096)
	wpk_write_uint16(wpk,CMD_GA_NOTIFYGAME)
	wpk_write_uint8(wpk,gamemgr.size)	
	for k,v in pairs(gamemgr.con2game) do
		wpk_write_string(wpk,v.ip)
		wpk_write_uint16(wpk,v.port)
	end
	C.send(gate.conn,wpk)
end


local game_net_handler = {}

game_net_handler[CMD_GAMEG_LOGIN] =  function (rpk,conn)
	C.set_conn_type(conn,GAMESERVER)
	local name = rpk_read_string(rpk)
	if gamemgr.con2game[conn] == nil and gamemgr.name2game[name] == nil then
		--game监听gate的ip和port
		local ip = rpk_read_string(rpk)
		local port = rpk_read_uint16(rpk)
		local game = {name=name,conn=conn,name=name,ip=ip,port=port,gameplys={},plycount=0}
		gamemgr.con2game[conn] = game
		gamemgr.name2game[name] = game	
		gamemgr.size = gamemgr.size + 1
		
		--通知所有gate新的game加入系统		
		local wpk = new_wpk(64)
		wpk_write_uint16(wpk,CMD_GA_NOTIFYGAME)
		wpk_write_uint8(wpk,1)
		wpk_write_string(wpk,game.ip)
		wpk_write_uint16(wpk,game.port)
		BoradCast2Gate(wpk)
		GroupApp.grouplog(LOG_INFO,"game:" ..name .. " login success")
	end
end

game_net_handler[DUMMY_ON_GAME_DISCONNECTED] =  function (rpk,conn)
	if gamemgr.con2game[conn] then
		local game = gamemgr.con2game[conn]
		gamemgr.con2game[conn] = nil
		gamemgr.name2game[game.name] = nil
		GroupApp.grouplog(LOG_INFO,"gameserver: " .. game.name .. " disconnected")		
		for k,v in pairs(game.gameplys) do
			v.game = nil
		end
		gamemgr.size = gamemgr.size - 1
		if game.onGameDisconnect then
			game.onGameDisconnect(game)
		end		
	end
end

local function reg_cmd_handler()
	C.reg_cmd_handler(CMD_GAMEG_LOGIN,game_net_handler)
	C.reg_cmd_handler(DUMMY_ON_GAME_DISCONNECTED,game_net_handler)
end


local function insertGamePly(ply,game)
	local t = gamemgr.con2game[game.conn]
	if t then
		t.gameplys[ply] = ply
		t.plycount = t.plycount + 1
	end
end

local function removeGamePly(ply,game)
	local t = gamemgr.con2game[game.conn]
	if t then
		t.gameplys[ply] = nil
		t.plycount = t.plycount - 1
	end
end

local function getGameByName(name)
	return gamemgr.name2game[name]
end


local function getMinGame()
	local min = 65535
	local game = nil
	for k,v in pairs(gamemgr.con2game) do
		if v.plycount < min then
			min = v.plycount
			game = v
		end
	end
	return game
end


return {
	RegHandler = reg_cmd_handler,
	InsertGamePly = insertGamePly,
	RemoveGamePly = removeGamePly,
	GetGameByName = getGameByName,
	OnGateLogin   = on_gate_login,
	GetMinGame    = getMinGame,
}
]]--
