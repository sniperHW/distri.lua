local queue = {
	filed1,
	filed2,
}

function queue:new(o)
  o = o or {}   
  setmetatable(o, self)
  self.__index = self
  self.filed1 = 1
  self.filed2 = 2
  return o
end


function queue:show()
	print(self.filed1)
	print(self.filed2)
	testfun({1,2,3},2000)
	return 100
end

function newQue()
	return queue:new()
end
