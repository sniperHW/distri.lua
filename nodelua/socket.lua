dofile("queue.lua")
dofile("light_process.lua")

socket = {
        type = nil,   -- data or listen
        msgque = nil,
        sock = nil,
        lprocess = nil,
}

function socket:accept()
    if self.type == "data"
        return nil,"accept error"
    else
        if self.msgque:is_empty() then
            if ~self.lprocess then
                self.lprocess = GetCurrentLightProcess()
            end
            --block
            Block()
        else

        end
    end
end


function socket:recv(timeout)
    if self.type ~= "data" then
        if self.msgque:is_empty() then
            if ~self.lprocess then
                self.lprocess = GetCurrentLightProcess()
            end
            --block
            Block(timeout)
        else

        end
    else
        return nil,"not data socket"
    end
end

function socket:send(data)
    if self.type ~= "data" then
        SendPacket(self.sock,data)
    else
        return nil,"not data socket"
    end
end

function socket:pushmsg(msg)
    self.msgque:push(msg)
    if self.lprocess and self.lprocess.status == "block" then
        WakeUp(self.lprocess)
    end
end

function socket:finalize()

end

function socket:new()
    local o = {}
    self.__gc = socket.finalize
    self.__index = self
    setmetatable(o, self)
    o.msgque = queue:new()
    return o
end
