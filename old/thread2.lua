local Thread = require "lua/thread"
local Sche = require "lua/scheduler"
print("thread2 begin")
Thread.RegThdFunction("Plus",
function (arg)
	return nil,arg[1] + arg[2]
end
)
