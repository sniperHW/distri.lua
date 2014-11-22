local cjson = require "cjson"

a=1
b=2
c=3
d=4

local t = {[a]=1,[b]=2,[c]=3,[d]=4}
--t[a] = 1
--t[b] = 2
--t[c] = 3
--t[d] = 4
--for i=1,10 do
--	t[i] = i	
	--table.insert(t,i)
--end

local str = cjson.encode(t)
t = cjson.decode(str)

for i=1,4 do
	print(t[i])
end
