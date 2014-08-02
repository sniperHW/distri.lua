
function co_mainfunc()
	local co = coroutine.running()
	print(co.status)
	print("co_mainfunc()")
end

local co = coroutine.create(co_mainfunc)
co.status = "actived"

coroutine.resume(co)

