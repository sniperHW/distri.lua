local Timer = require "lua.timer"

local target = C.GetSysTick() + 100

local TriggerTimer= Timer.New("runImmediate",100):Register(function ()
			print("timeout",C.GetSysTick()-target)
			target = C.GetSysTick() + 100
		    end,100)