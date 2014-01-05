light_process =
{
    index = 0,
    status = nil,
    croutine = nil,
    timeout = 0,
    ud = nil
}

function light_process:new(o)
  o = o or {}
  setmetatable(o, self)
  self.__index = self
  return o
end
