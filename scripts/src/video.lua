-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

---------------------------------------------------------------------------
--
--   video.lua
--
--   Rules for building video cores
--
---------------------------------------------------------------------------

files {
	MAME_DIR .. "src/devices/video/poly.h",
	MAME_DIR .. "src/devices/video/sprite.cpp",
	MAME_DIR .. "src/devices/video/sprite.h",
	MAME_DIR .. "src/devices/video/vector.cpp",
	MAME_DIR .. "src/devices/video/vector.h",
}

--------------------------------------------------
--
--@src/devices/video/mos6566.h,VIDEOS["MOS6566"] = true
--------------------------------------------------

--if (VIDEOS["MOS6566"]~=null) then
--	files {
--		MAME_DIR .. "src/devices/video/mos6566.cpp",
--		MAME_DIR .. "src/devices/video/mos6566.h",
--	}
--end


files {
	MAME_DIR .. "src/devices/video/cgapal.cpp",
	MAME_DIR .. "src/devices/video/cgapal.h",
}

-- SYSTEM (MEGADRIVE)
--------------------------------------------------
--
--@src/devices/video/315_5124.h,VIDEOS["SEGA315_5124"] = true
--------------------------------------------------

if (VIDEOS["SEGA315_5124"]~=null) then
	files {
		MAME_DIR .. "src/devices/video/315_5124.cpp",
		MAME_DIR .. "src/devices/video/315_5124.h",
	}
end

--------------------------------------------------
--
--@src/devices/video/315_5313.h,VIDEOS["SEGA315_5313"] = true
--------------------------------------------------

if (VIDEOS["SEGA315_5313"]~=null) then
	files {
		MAME_DIR .. "src/devices/video/315_5313.cpp",
		MAME_DIR .. "src/devices/video/315_5313.h",
	}
end

-- SYSTEM (GBA)
--------------------------------------------------
--
--@src/devices/video/gba_lcd.h,VIDEOS["GBA_LCD"] = true
--------------------------------------------------

if (VIDEOS["GBA_LCD"]~=null) then
	files {
		MAME_DIR .. "src/devices/video/gba_lcd.cpp",
		MAME_DIR .. "src/devices/video/gba_lcd.h",
	}
end

-- SYSTEM (GB)
--------------------------------------------------
--
--@src/devices/video/gb_lcd.h,VIDEOS["GB_LCD"] = true
--------------------------------------------------

if (VIDEOS["GB_LCD"]~=null) then
	files {
		MAME_DIR .. "src/devices/video/gb_lcd.cpp",
		MAME_DIR .. "src/devices/video/gb_lcd.h",
	}
end

-- SYSTEM (M92)
--------------------------------------------------
--
--@src/devices/video/bufsprite.h,VIDEOS["BUFSPRITE"] = true
--------------------------------------------------

if (VIDEOS["BUFSPRITE"]~=null) then
	files {
		MAME_DIR .. "src/devices/video/bufsprite.cpp",
		MAME_DIR .. "src/devices/video/bufsprite.h",
	}
end
