local Sche = require "lua/scheduler"

--启动一个light process运行主文件
Sche.Spawn(dofile,C.startfile)
--运行消息循环
local ms = 5
while C.run(ms) do
	if Sche.Schedule() > 0 then
		ms = 0
	end
end
