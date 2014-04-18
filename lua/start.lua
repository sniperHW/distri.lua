local Sche = require "lua/scheduler"

--启动一个light process运行主文件
Sche.Spawn(dofile,C.startfile)
--运行消息循环
while C.run(5) do
	Sche.Schedule()
end
