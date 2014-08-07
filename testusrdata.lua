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
  self.__gc = function () print("gc1") end
  self.__index = self
  setmetatable(o, self)
  return o
end

function socket:show()
	print("show")
end


local s = socket:new()
s:show()

--s = nil

local mt = {}
mt.__gc = function () print("gc2") end

local t = {}
setmetatable(t,mt)

--t = nil

--collectgarbage(collect)
--print("here")


