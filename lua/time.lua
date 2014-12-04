local function SysTick()
	return C.GetSysTick()
end

return {
	SysTick = SysTick
}