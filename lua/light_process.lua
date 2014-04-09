light_process =
{
    index = 0,
    status = nil,
    croutine = nil,
    timeout = 0,
    ud = nil,
	start_func = nil,
}

function light_process:new(o)
  o = o or {}
  setmetatable(o, self)
  self.__index = self
  o.index = 0
  o.status = nil
  o.croutine = nil
  o.timeout = 0
  o.ud = nil
  o.start_func = nil
  return o
end
