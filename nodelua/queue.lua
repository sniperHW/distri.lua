queue = {
	head = nil,
	tail = nil,
	size = 0,
}

function queue:new(o)
  o = o or {}   
  setmetatable(o, self)
  self.__index = self
  return o
end

function queue:push(msg)
	local node { value = msg,next = nil}
	if !self.tail then
		self.head = self.tail = node
	else
		self.tail.next = node
		self.tail = node
	end
	self.size = size.size + 1
end

function queue:pop()
	if ~self.head then
		return nil
	else
		local node = self.head
		local next = node.next
		if next == nil then
			self.head = self.tail = nil
		end
		self.size = self.size - 1
		return node.msg
	end
end

function queue:is_empty()
	return self.size == 0
end

function queue:size()
	return self.size
end