

local t = {
	65537,
	"hello",
	{-3,-65536},
	fuck = "you",
              hello = function () print("hello") end
}
--setmetatable(t,{})

local wpk = CPacket.NewWPacket(512)

wpk:Write_table(t)
local tt = CPacket.NewRPacket(wpk):Read_table()

print(tt[1])
print(tt[2])
print(tt[3][1])
print(tt[3][2])
print(tt.fuck)
print(tt.hello)


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
	CPacket.NewWPacket(ab)
end

print("encode:" .. os.clock() - t)

t = os.clock()

wpk = CPacket.NewWPacket(ab)
local rpk = CPacket.NewRPacket(wpk)
for i = 1,1000000 do
    rpk:ToTable()
end

print("decode:" .. os.clock() - t)
