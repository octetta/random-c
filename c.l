debug = print

if true then
	_snmp_ver = "-v 2c"
	_snmp_opts = "-On -Ot -Oq"
	_snmp_capture = "(%d+)%.(%d+)%s(.*)"
else
	_snmp_opts = "-On -Ot"
	_snmp_capture = "(%d+)%.(%d+) = (.*)"
end

_snmp_stderr = ""
--_snmp_stderr = "2> /dev/null"

function snmpbulkwalk_new(host, community, mib, timeout)
	local sto = timeout or 30
	local cmd = string.format("snmpbulkwalk -t %d %s %s %s -c %s %s %s",
		sto,
		_snmp_opts,
		_snmp_ver,
		host,
		community,
		mib,
		_snmp_stderr)
	--debug(cmd)
	local p = io.popen(cmd, "r")
	return p
end

function snmpbulkwalk_call(p)
	if p then
		local s = p:read("*l")
		if not s then
			p:close()
			return nil
		end
		return s
	end
	return nil
end

port_status_mib = ".1.3.6.1.2.1.10.127.1.3.3.1"
port_status_filter = {
	["2"] = "MACaddr",
	["3"] = "IPaddr",
	["4"] = "DownstreamPortNumber",
	["5"] = "UpstreamPortNumber",
	["6"] = "UpstreamPowerLevel",
	["9"] = "ConnectionState",
	["13"] = "SignalNoiseRatio",
}

hostlist = {
	{"192.168.47.147", 59, "public", 33},
	{"192.168.40.120", 61, "public", 22},
}

proc = {}

for i=1,table.getn(hostlist) do
	host = hostlist[i][1]
	proc[host] = {}
	proc[host].community = hostlist[i][3]
	debug(host, community, proc[host])
	proc[host].fork = snmpbulkwalk_new(host, proc[host].community, port_status_mib)
	proc[host].time = socket.gettime()
	proc[host].ttl = 0
	--
	proc[host].name = host
	proc[host].getfd = function(self)
		return jlib.getfd(self.fork)
	end
	--
end

while true do
	
	readlist = {}
	
	for i=1,table.getn(hostlist) do
		host = hostlist[i][1]
		if proc[host].fork then
			table.insert(readlist, proc[host])
		end
	end

	if table.getn(readlist) <= 0 then
		break
	end

	r,w,e = socket.select(readlist, nil, nil)

	for i,v in ipairs(r) do
		if type(v) == "table" then
			local s = snmpbulkwalk_call(v.fork)
			if s then
				local tr, ndx, val
				_,_,tr,ndx,val = string.find(s, _snmp_capture)
				if tr and port_status_filter[tr] then
					debug(v.name, tr, ndx, val)
					if not v.get then v.get = {} end
					if not v.get[ndx] then v.get[ndx] = {} end
					if not v.get[ndx][tr] then v.get[ndx][tr] = {} end
					v.get[ndx][tr] = val
					v.ttl =  v.ttl + 1
				end
			else
				print("total for " .. v.name .. " is " .. v.ttl)
				for key,val in ipairs(v.get) do
					print(key,val)
				end
				v.fork = nil
				--[[
				v.fork = snmpbulkwalk_new(v.name, v.community, port_status_mib)
				v.time = socket.gettime()
				v.getfd = function(self) return jlib.getfd(self.fork) end
				v.ttl = 0
				--]]
			end
		end
	end
end

