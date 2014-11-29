local inputstr = "fasfasfasfe"
local encode_str = CBase64.encode(inputstr)
print(inputstr)
print(encode_str)
print(CBase64.decode(encode_str))
Exit()