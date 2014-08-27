local Socket = require "lua/socket"
local Sche = require "lua/sche"
local Cjson = require "cjson"


local http_response = {}

function http_response:new()
  local o = {}
  self.__index = self      
  setmetatable(o,self)
  return o
end

function http_response:Send(connection)
	local s  = string.format("HTTP/1.1 %d %s\r\nConnection: Keep-Alive\r\n",self.status,self.phase)	
	table.insert(self.headers,"Date: Tue, 26 Aug 2014 11:40:38 GMT")	
	for k,v in pairs(self.headers) do
		s = s .. string.format("%s\r\n",v)
	end	
	if self.body then
		s = s ..  "Transfer-Encoding: chunked\r\n\r\nc\r\n"
		s = s .. string.format("%s\r\n0\r\n",self.body)
	end
	s = s .. "\r\n"
	connection:Send(CPacket.NewRawPacket(s))	
end

function http_response:WriteHead(status,phase,...)
	self.status = status
	self.phase = phase
	self.headers = table.pack(...)
end

function http_response:End(body)
	self.body = body
end

local http_server = {}

function http_server:new()
  local o = {}
  self.__index = self      
  setmetatable(o,self)
  return o
end


local function process_server(connection,on_request)
	local request
	while true do
		local packet,err = connection:Recv()
		if err then	
			connection:Close()
			return
		else
			local ev_type = packet:ToString()
			if ev_type == "ON_MESSAGE_BEGIN" then
				local t = Cjson.decode(packet:Read_rawbin())
				request = {}
				request.header = {}
				request.method = t.method
			elseif ev_type == "ON_URL" then
				request.url = packet:Read_rawbin()
			elseif ev_type == "ON_HEADER_FIELD" then
				local packet1,err1 = connection:Recv()
				if err1 then		
					connection:Close()
					return
				end
				if packet1:ToString() ~= "ON_HEADER_VALUE" then
					connection:Close()
					return					
				end
				request.header[packet:Read_rawbin()] = packet1:Read_rawbin()
			elseif ev_type == "ON_BODY" then
				request.body = packet:Read_rawbin() 
			elseif ev_type == "ON_MESSAGE_COMPLETE" then
				local response = http_response:new()
				on_request(request,response)
				response:Send(connection)
				request = nil				
			end		
		end
	end

end

function http_server:CreateServer(on_request)
	self.on_request = on_request
	return self
end

function http_server:Listen(ip,port)
	self.socket = Socket.New(CSocket.AF_INET,CSocket.SOCK_STREAM,CSocket.IPPROTO_TCP)
	local err = self.socket:Listen(ip,port)
	if err then
		return err
	else
		local s = self
		Sche.Spawn(function () 	
			while true do
				local connection = s.socket:Accept()
				connection:Establish(CHttp.http_decoder(CHttp.HTTP_REQUEST,512),65535)
				Sche.Spawn(process_server,connection,s.on_request)
			end
		end)
		return nil
	end
end

local function CreateServer(on_request)
	return http_server:new():CreateServer(on_request)
end

return {
	CreateServer = CreateServer
}
