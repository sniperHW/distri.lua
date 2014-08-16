local socket = require "lua/socket"
local sche = require "lua/sche"
local Packet = require "lua/packet"

local server = socket.new(luasocket.AF_INET,luasocket.SOCK_STREAM,luasocket.IPPROTO_TCP)
if not server:listen("127.0.0.1",8010) then
		print("server listen on 127.0.0.1 8010")
else
	print("create server on 127.0.0.1 8010 error")
	return
end

local count = 0

sche.Spawn(function ()
	local tick = GetSysTick()
	local now = GetSysTick()
	while true do 
		now = GetSysTick()
		if now - tick >= 1000 then
			print(count*1000/(now-tick) .. " " .. now-tick)
			tick = now
			count = 0
		end
		sche.Sleep(10)
	end
end)

while true do
	local client = server:accept()
	sche.Spawn(function ()	
		local unpack_buffer = bytebuffer.bytebuffer(65535)
		local buf_size = 0
		while true do
			local packet,err = client:recv()
			if err then
				print("client disconnected err:" .. err)			
				client:close()
				return
			end
			local len = string.len(packet)
			if buf_size + len > 65535 then
				print("close here a:" .. buf_size + len)
				client:close()
				return
			end						
			unpack_buffer:write_raw(buf_size,packet)
			buf_size = buf_size + len
			if buf_size >= 4 then
				local packet_len = unpack_buffer:read_uint32(0)
				packet_len = packet_len + 4 --加上包头字段
				if packet_len > 65535 then
					print("buff_size:" .. buf_size)
					print("close here b:" .. packet_len)
					client:close()
					return				
				end							
				if packet_len <= buf_size then
					--有完整的包
					buf_size = buf_size - packet_len
					if buf_size > 0 then
						--调整unpack_buffer中的数据
						unpack_buffer:move(packet_len,buf_size)
					end
					local rpacket = Packet.RPacket(bytebuffer.bytebuffer(unpack_buffer:read_raw(0,packet_len)))
					count = count + 1
					client:send(rpacket.bytebuffer)
				end
			end
		end
	end)
end


