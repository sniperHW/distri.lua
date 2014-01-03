dofile("timer.lua")
scheduler =
{
	pending_add,  --等待添加到活动列表中的coObject
	timer,
	CoroCount,
	current_co
}

function scheduler:new(o)
  o = o or {}   
  setmetatable(o, self)
  self.__index = self
  return o
end

function scheduler:init()
	self.m_timer = timer:new()
	self.pending_add = {}
	self.current_co = nil
	self.CoroCount = 0
end

--添加到活动列表中
function scheduler:Add2Active(sock)
	if sock.status == "actived" then
		return
	end
	
	sock.status = "actived"
	table.insert(self.pending_add,sock)
end

function scheduler:Block(sock,ms)
	if ms and ms > 0 then
		local nowtick = GetSysTick()
		sock.timeout = nowtick + ms
		if sock.index == 0 then
			self.m_timer:Insert(sock)
		else
			self.m_timer:Change(sock)
		end
	end
	sock.status = "block"
	coroutine.yield(sock.co)
	--被唤醒了，如果还有超时在队列中，这里需要将这个结构放到队列头，并将其删除
	if sock.index ~= 0 then
		sock.timeout = 0
		self.m_timer:Change(sock)
		self.m_timer:PopMin()
	end
end


--睡眠ms
function scheduler:Sleep(sock,ms)
	if ms and ms > 0 then
		sock.timeout = GetSysTick() + ms
		if sock.index == 0 then
			self.m_timer:Insert(sock)
		else
			self.m_timer:Change(sock)
		end
		sock.status = "sleep"
	else
		sock.status = "yield"
	end
	coroutine.yield(sock.co)
end

--暂时释放执行权
function scheduler:Yield(sock)
	self:Sleep(sock)
end


--主调度循环
function scheduler:Schedule()
	local runlist = {}
	--将pending_add中所有coObject添加到活动列表中
	for k,v in pairs(self.pending_add) do
			table.insert(runlist,v)
	end
	
	self.pending_add = {}
	local now_tick = GetSysTick()
	for k,v in pairs(runlist) do
		coroutine.resume(v.co,v)
		if v.status == "yield" then
			self:Add2Active(v)
		end
	end
	runlist = {}
	--看看有没有timeout的纤程	
	local now = GetSysTick()
	while self.m_timer:Min() ~=0 and self.m_timer:Min() <= now do
		local sock = self.m_timer:PopMin()
		if sock.status == "block" or sock.status == "sleep" then
			self:Add2Active(sock)
		end
	end	
end

global_sc = scheduler:new()
global_sc:init()

function node_spwan(sock,mainfun)
	sock.co = coroutine.create(mainfun)
	global_sc:Add2Active(sock)
end

function node_process_msg(msg)
	
end

function node_loop()
	global_sc:Schedule()
	node_process_msg(PeekMsg(50))
end

function node_listen(ip,port)
	return Listen(ip,port)
end

