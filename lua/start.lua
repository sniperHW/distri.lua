local Sche = require "lua/scheduler"

function start(filename)
	--启动一个light process运行主文件
	Sche.Spawn(dofile,filename)
end