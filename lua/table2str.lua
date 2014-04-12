--table<->string之间的转换，主要用于网络传输和数据存储

local function tostr(key,val)
	if type(key) == "userdata" then
		return nil
	end
	
	local str
	if type(key) == "number" then
		str = "[" .. tostring(key) .. "]="
	else
		str = tostring(key) .. "="
	end
	if type(val) == "table" then
		str = str .. "{"
		for k,v in pairs(val) do
			str = str .. tostr(k,v) .. ","
		end
		str = str .. "}"
	elseif type(val) == "string" then
		str = str .. "'" .. val .. "'"
	else
		str = str .. val 
	end
	return str
end

local function Table2Str(tb)
	if type(tb) ~= "table" then
		return nil
	end
	local str = "return {"
	for k,v in pairs(tb) do
		str = str .. tostr(k,v) .. ","
	end
	return str .. "}"
end

local function Str2Table(str)
	return assert(loadstring(str)())
end


local tb = {name = "sniperHW",age = 32,address={"shanghai","shaoguan"}}
local str = Table2Str(tb)
print(str)
local tb = Str2Table(str)

for k,v in pairs(tb) do
	if type(v) ~= "table" then
		print(k .. ":" .. v)
	end
end

return {
	Table2Str = Table2Str,
	Str2Table = Str2Table
}



