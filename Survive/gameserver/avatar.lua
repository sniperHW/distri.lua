local Aoi = require "Survive/aoi"

local avatar ={
	id,            
	avatid,        
	pos,
	aoi_obj,
	see_radius,   
	view_obj,      
	watch_me,      
	gatesession,
	groupsession,
	map,          
	path,
	speed,         
	lastmovtick,   
	movmargin,     
	dir,           
	nickname,      
	skillmgr,
	attr,                 
}

function avatar:new()
  local o = {}   
  setmetatable(o, self)
  self.__index = self
  return o
end

function avatar:Init(id,avatid)
	self.id = id
	self.avatid = avatid
	self.avattype = 0
	self.see_radius = 5
	self.view_obj = {}
	self.watch_me = {}
	self.gate = nil
	self.map =  nil
	self.path = nil
	self.speed = 20
	self.pos = nil
	self.nickname = ""
	self.aoi_obj = Aoi.create_aoi_obj(o)
	return self
end

function avatar:Send2Client(wpk)
end


function avatar:Send2view(wpk,exclude) --exclude排除列表
	--将玩家分组,同gateserver的玩家为一组,发送一个统一的包	
	exclude = exclude or {}	
	local gates = {}
	for k,v in pairs(self.watch_me) do
		if v.gatesession and (not exclude[v]) then
			local t = gates[v.gatesession]
			if not t then
				t = {}
				gates[v.gatesession] = t
			end
			table.insert(t,v)
		end
	end
	
	for k,v in pairs(gates) do
		local w = CPacket.NewWPacket(512)
		local rpk = CPacket.NewRPacket(wpk)
		w:Write_uint16(rpk:Read_uint16())
		w:Write_wpk(wpk)
		w:Write_uint16(#v)
		for k1,v1 in pairs(v) do
			w:Write_uint32(v1.gatesession.sessionid)
		end
		k:Send(w)
	end
end

function avatar:Release()
	if self.aoi_obj then
		Aoi.destroy_obj(self.aoi_obj)
	end
end

return {
	New = function () return avatar:new() end
}
