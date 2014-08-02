function fun0()
	print("fun0")
end

function fun1(arg1)
	print("fun1")
	print(arg1)
end

function fun2(arg1,arg2)
	print("fun2")
	print(arg1)
	print(arg2)		
end

function fun3(arg1,arg2,arg3)
	print("fun3")
	print(arg1)
	print(arg2)
	print(arg3)				
end

function fun4(arg1,arg2,arg3,arg4)
	print("fun4")
	print(arg1)
	print(arg2)
	print(arg3)
	print(arg4)					
end

function fun5(arg1,arg2,arg3,arg4,arg5)
	print("fun5")
	print(arg1)
	print(arg2)
	print(arg3)
	print(arg4)
	print(arg5)	
end

function fun6(arg1,arg2,arg3,arg4,arg5,arg6)
	print("fun6")
	print(arg1)
	print(arg2)
	print(arg3)
	print(arg4)
	print(arg5)
	print(arg6)			
end

ttab = {}

function ttab:fun0()
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
