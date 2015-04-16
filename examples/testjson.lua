local cjson = require "cjson"


local t = {0xfffffffffffffff,2,3,4,5}
local str = cjson.encode(t)
print(str)
t = cjson.decode(str)
print(table.unpack(t))



--[[a=1
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
end]]--
--[[
local ab = {
    person = {
        {
            name = "Alice",
            id = 10000,
            phone = {
                {
                    number = "123456789",
                    type = 1,
                },
                {
                    number = "87654321",
                    type = 2,
                },
            }
        },
        {
            name = "Bob",
            id = 20000,
            phone = {
                {
                    number = "01234567890",
                    type = 3,
                }
            }
        }

    }
}


t = os.clock()

for i = 1,1000000 do
	cjson.encode(ab)
end

print(os.clock() - t)
]]--