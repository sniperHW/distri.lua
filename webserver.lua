local registernet = assert(package.loadlib("./luanet.so","RegisterNet"))  
registernet()

function remove_connection(connection_set,connection)
	local idx = 0
	for k,v in pairs(connection_set) do
		if v == connection then
			idx = k
		end
	end
	if idx >= 1 then
		table.remove(connection_set,idx)
	end
end

local totalsend = 0

local str = [[
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<!-- saved from url=(0031)http://www.lua.org/contact.html -->
<html><head><meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
<title>Lua: contact</title>
<style type="text/css">
em {
	font-style: normal ;
	font-weight: bold ;
	color: red ;
}
</style>
</head>

<body>

<hr>
<h1>
Contact
</h1>

<p>
<table>
<tbody><tr>
<td width="45%">
<h2>Email</h2>
<p>
The best way to contact the <a href="http://www.lua.org/authors.html">Lua team</a> is by email,
but please
send <em>all</em> Lua questions, suggestions, and
<a href="http://www.lua.org/faq.html#2.3">bug reports</a>
to the
<a href="http://www.lua.org/lua-l.html">mailing list</a>:
you will certainly get faster and better answers,
and your message will be
<a href="http://lua-users.org/lists/lua-l/">archived</a>
for further reference.

</p><p>
For matters that require our personal attention,
send email to team at lua.org.

</p></td><td class="gutter">
</td><td width="45%">
<h2>Mail</h2>
<p>
<a href="http://www.lua.inf.puc-rio.br/">Lablua</a><br>
Departamento de Informática, PUC-Rio<br>
Rua Marquês de São Vicente, 225<br>
22451-900  Rio de Janeiro, RJ, Brazil

</p><h2>Fax</h2>
<p>
+55 21 3527-1530
<br>
attn. Roberto Ierusalimschy
</p><p>
</p></td></tr>
</tbody></table>

</p><hr>
<small class="footer">
Last update:
Tue May 22 20:28:48 BRT 2012
</small>
<!--
Last change: two column layout
-->



</body></html>

]]

function mainloop()
	local lasttick = GetSysTick()
	local netengine = CreateNet(arg[1],arg[2],1)
	--local netengine = CreateNet("127.0.0.1",8010,1)
	local connection_set = {}
	local last_check_timeout = GetSysTick()
	local stop = 0
	while stop == 0 do
		
		local msgs = PeekMsg(netengine,50)
		if msgs ~= nil then
			for k,v in pairs(msgs) do
				local type,con,rpk = v[1],v[2],v[3]
				if type == NEW_CONNECTION then
					table.insert(connection_set,con)
					print("a connection comming")
				elseif type == PROCESS_PACKET then
					print("--------------------------")
					print(PacketReadString(rpk))
					local wpkt = CreateWpacket(con,0,65536)
					PacketWriteString(wpkt,str)
					SendPacket(con,wpkt,1)
					ReleaseRpacket(rpk)
				elseif type == DISCONNECT then
					if 1 == ReleaseConnection(con) then
						remove_connection(connection_set,con)
						print("disconnect")
					else
						print("release failed")
					end
				elseif type == PACKET_SEND_FINISH then
					ActiveCloseConnection(con)
				elseif type == CONNECTION_TIMEOUT then
					ActiveCloseConnection(con)	
				else
					print("break main loop")
					stop = 1
					break
				end
				
			end 
		end
		
		local tick = GetSysTick()
		--[[if tick - 1000 >= lasttick then
			print("client:" .. #connection_set .. " packet send:" ..totalsend)
			totalsend = 0;
			lasttick = tick
		end]]--
		
	end
	DestroyNet(netengine)	
	print("main loop end")
end	

mainloop()  
