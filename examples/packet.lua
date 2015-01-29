
local wpk = CPacket.NewWPacket()
wpk:Write_uint32(100)
wpk:Write_uint16(10)
wpk:Write_string("hello")
local rpk = CPacket.NewRPacket(wpk)

print(rpk:Read_uint32())
print(rpk:Peek_uint16())
print(rpk:Read_uint16())
print(rpk:Read_string())

--[[
100
10
10
hello
]]--

wpk = CPacket.NewWPacket()
local wpos = wpk:Get_write_pos()
wpk:Write_uint8(0)
wpk:Write_uint32(100)
wpk:Rewrite_uint8(wpos,255)
rpk = CPacket.NewRPacket(wpk)

print(rpk:Reverse_read_uint32())
print(rpk:Read_uint8())
print(rpk:Read_uint32())

--[[
100
255
100
]]--


local rawpk = CPacket.NewRawPacket("hello")
print(rawpk:Read_rawbin())

--[[
hello
]]--





