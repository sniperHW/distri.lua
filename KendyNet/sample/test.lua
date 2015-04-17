function fun0()
	print("fun0")
end

function fun1(arg1,arg2)
	print("fun1")
	print(arg1,arg2)
	return 10
end

function fun2(arg1,arg2)
	print("fun2")
	print(arg1)
	print(arg2)
	return "fuck"		
end

function fun3(arg1,arg2,arg3)
	print("fun3")
	print(arg1)
	print(arg2)
	print(arg3)	
	return "abc","abcd"			
end

function fun4(arg1,arg2,arg3,arg4)
	print("fun4")
	print(arg1)
	print(arg2)
	print(arg3)
	print(arg4)					
end

function fun5()
	return function () print("haha") end
end

local obj = {
	f1 = 10,
	f2 = "function",
}

obj.hello = function ()
	print("obj.hello")
end

function fun6()
	return obj,100
end

function fun7()
	print("obj.f1",obj.f1)
	if obj.f1 == nil then
		print("true")
	end
end

ttab = {}

function ttab:fun0()
	a()
	print("ttab:fun0")
end

function ttab:fun1(arg1)
	print("ttab:fun1")
	print(arg1)
end

function ttab:fun2(arg1,arg2)
	print("ttab:fun2")
	print(arg1)
	print(arg2)		
end

function ttab:fun3(arg1,arg2,arg3)
	print("ttab:fun3")
	print(arg1)
	print(arg2)
	print(arg3)				
end

function ttab:fun4(arg1,arg2,arg3,arg4)
	print("ttab:fun4")
	print(arg1)
	print(arg2)
	print(arg3)
	print(arg4)					
end

function ttab:fun5(arg1,arg2,arg3,arg4,arg5)
	print("ttab:fun5")
	print(arg1)
	print(arg2)
	print(arg3)
	print(arg4)
	print(arg5)	
end

function ttab:fun6(arg1,arg2,arg3,arg4,arg5,arg6)
	print("ttab:fun6")
	print(arg1)
	print(arg2)
	print(arg3)
	print(arg4)
	print(arg5)
	print(arg6)			
end

ttab2 = {
			key1={1,2,3},
			key2={4,5,6},
			key3={7,8,9}}
			
			
function callFuncRef(luafunc)
	print("callFuncRef")
	luafunc(1,2,3,4)
end

ttab3 = {
	ip = "127.0.0.1",
	port = 8010
}

ttab4 = {
	ip1 = "127.0.0.1",
	ip2 = "127.0.0.2",
	ip3 = "127.0.0.3"	
}

function fun8(wpk)
	print("fun8")
	C.packet_pack(wpk,{1,2,3,4,5})
end

function fun9(rpk)
	print("fun9")
	local t = C.packet_unpack(rpk)
	for k,v in pairs(t) do
		print(v)
	end
end
