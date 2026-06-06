local dat = {}
local ver, info

local datread = require("data/load_dat")
datread, ver = datread.open("gameinit.dat", "# .-GAMEINIT.DAT")

function dat.check(set, softlist)
	if softlist or not datread then
		return nil
	end
	local status
	status, info = pcall(datread, "mame", "info", set)
------------------------------------- 缘来是你 ------------------------------------------>>>			
	-- 若当前游戏无信息，则引用主游戏
	if not status or not info then
		local driver = emu.driver_find(set)
		if driver then
			local parent = driver.parent
			if parent and parent ~= set and parent ~= "" then
				status, info = pcall(datread, "mame", "info", parent)
				if status and info then
	               return _("Gameinit")
				end
			end
		end
---------------------------------------------------------------------------------------->>>

		return nil
	end
	return _("Gameinit")
end

function dat.get()
	return info
end

function dat.ver()
	return ver
end

return dat
