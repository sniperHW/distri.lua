local Sche = require "lua/scheduler"
local Thread = require "lua/thread"



function channel_msg_callback(from,msg)
	Thread.On_channel_msg(from,msg)	
end
--启动一个light process运行主文件
local main = loadfile(C.startfile)
Sche.Spawn(function ()
				Thread.Setup()
				main()
			end)
--运行消息循环
local ms = 5
while C.run(ms) do
	if Sche.Schedule() > 0 then
		ms = 0
	else
		ms = 5
	end
end
