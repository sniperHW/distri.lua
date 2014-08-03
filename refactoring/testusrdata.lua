local u = ud.ud()
print(u)
local t = u:hello()
t:hello()

u = nil
t = nil

collectgarbage(collect)
