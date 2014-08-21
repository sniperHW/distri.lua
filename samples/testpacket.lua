local packet = require "lua/packet"

print("--------------test rawpacket--------------------")

local rawpacket1 = packet.RawPacket("hahaha")
print(rawpacket1:read_rawbin())

local rawpacket2 = packet.RawPacket(rawpacket1.bytebuffer)
print(rawpacket2:read_rawbin())

rawpacket1:write_rawbin("fuck you")
print(rawpacket1:read_rawbin())
print(rawpacket2:read_rawbin())

print("--------------test rpacket----------------------")

local wpacket = packet.WPacket(64)
wpacket:write_uint8(1)
wpacket:write_uint16(2)
wpacket:write_uint32(3)
wpacket:write_float(4.0)
wpacket:write_double(5.0)
wpacket:write_string("hello")
wpacket:write_string("hello123456789123456789123456789")
print("wpacket datasize:" .. wpacket:size())
print(wpacket.bytebuffer:cap())

local rpacket = packet.RPacket(wpacket.bytebuffer)
print("rpacket datasize:" .. rpacket:size())
print(rpacket:read_uint8())
print(rpacket:read_uint16())
print(rpacket:read_uint32())
print(rpacket:read_float())
print(rpacket:read_double())
print(rpacket:read_string())
print(rpacket:read_string())
