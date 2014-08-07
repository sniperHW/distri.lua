--[[local u = ud.ud()
print(u)
local t = u:hello()
t:hello()

u = nil
t = nil
]]--

local socket = {}

function socket:new()
  o = {}   
  setmetatable(o, self)
  self.__index = self
  self.__gc = function () print("gc") end
  return o
end


local s = socket:new()

s = nil

print("here")

collectgarbage(collect)
