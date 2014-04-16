local Sche = require "lua/scheduler"

function start(filename)
	--启动一个light process运行主文件
	Sche.Spawn(dofile,filename)
	--运行消息循环
	while C.run(5) do
		Sche.Schedule()
	end
end