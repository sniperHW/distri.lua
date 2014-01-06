socket = {
	type = nil,   -- data or listen
}

function socket:new()
    local o = {}
    self.__gc = socket.finalize
    self.__index = self
    setmetatable(o, self)
    o.type = 1
    return o
end

function socket:show()
	print("hello socket " .. self.type)
end

function testObj()
	local sock1 = socket:new()
	sock1.type = 2
	local sock2 = socket:new()
	sock2.type = 3
	return sock1,sock2
end

function hello(arg)
	print("hello " .. arg[1] .. arg[2][1] .. arg[2][2] )
	--local sock1 = socket:new()
	--sock1.type = 2
	--local sock2 = socket:new()
	--sock2.type = 3
	--return sock1,sock2
end

function testArray()
	return {1,2,3,4,5}
end