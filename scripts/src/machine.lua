-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

---------------------------------------------------------------------------
--
--   machine.lua
--
--   Rules for building machine cores
--
----------------------------------------------------------------------------

files {
	MAME_DIR .. "src/devices/machine/bcreader.cpp",
	MAME_DIR .. "src/devices/machine/bcreader.h",
--	MAME_DIR .. "src/devices/machine/buffer.cpp",
--	MAME_DIR .. "src/devices/machine/buffer.h",
	MAME_DIR .. "src/devices/machine/clock.cpp",
	MAME_DIR .. "src/devices/machine/clock.h",
	MAME_DIR .. "src/devices/machine/keyboard.cpp", -- NGP
	MAME_DIR .. "src/devices/machine/keyboard.h", -- NGP
	MAME_DIR .. "src/devices/machine/keyboard.ipp", -- NGP
	MAME_DIR .. "src/devices/machine/laserdsc.cpp", -- NGP
	MAME_DIR .. "src/devices/machine/laserdsc.h", -- NGP
	MAME_DIR .. "src/devices/machine/nvram.cpp",
	MAME_DIR .. "src/devices/machine/nvram.h",
	MAME_DIR .. "src/devices/machine/ram.cpp",
	MAME_DIR .. "src/devices/machine/ram.h",
	MAME_DIR .. "src/devices/machine/legscsi.cpp", -- NGP
	MAME_DIR .. "src/devices/machine/legscsi.h", -- NGP
--	MAME_DIR .. "src/devices/machine/sdlc.cpp",
--	MAME_DIR .. "src/devices/machine/sdlc.h",
	MAME_DIR .. "src/devices/machine/terminal.cpp", -- NGP
	MAME_DIR .. "src/devices/machine/terminal.h", -- NGP
	MAME_DIR .. "src/devices/machine/timer.cpp",
	MAME_DIR .. "src/devices/machine/timer.h",
}
files {
--	MAME_DIR .. "src/devices/imagedev/bitbngr.cpp",
--	MAME_DIR .. "src/devices/imagedev/bitbngr.h",
	MAME_DIR .. "src/devices/imagedev/cassette.cpp",
	MAME_DIR .. "src/devices/imagedev/cassette.h",
	MAME_DIR .. "src/devices/imagedev/chd_cd.cpp", -- NGP
	MAME_DIR .. "src/devices/imagedev/chd_cd.h", -- NGP
--	MAME_DIR .. "src/devices/imagedev/diablo.cpp",
--	MAME_DIR .. "src/devices/imagedev/diablo.h",
	MAME_DIR .. "src/devices/imagedev/flopdrv.cpp", -- NGP
	MAME_DIR .. "src/devices/imagedev/flopdrv.h", -- NGP
	MAME_DIR .. "src/devices/imagedev/floppy.cpp",
	MAME_DIR .. "src/devices/imagedev/floppy.h",
	MAME_DIR .. "src/devices/imagedev/harddriv.cpp", -- NGP
	MAME_DIR .. "src/devices/imagedev/harddriv.h", -- NGP
--	MAME_DIR .. "src/devices/imagedev/mfmhd.cpp",
--	MAME_DIR .. "src/devices/imagedev/mfmhd.h",
--	MAME_DIR .. "src/devices/imagedev/microdrv.cpp",
--	MAME_DIR .. "src/devices/imagedev/microdrv.h",
--	MAME_DIR .. "src/devices/imagedev/midiin.cpp",
--	MAME_DIR .. "src/devices/imagedev/midiin.h",
--	MAME_DIR .. "src/devices/imagedev/midiout.cpp",
--	MAME_DIR .. "src/devices/imagedev/midiout.h",
--	MAME_DIR .. "src/devices/imagedev/picture.cpp",
--	MAME_DIR .. "src/devices/imagedev/picture.h",
--	MAME_DIR .. "src/devices/imagedev/printer.cpp",
--	MAME_DIR .. "src/devices/imagedev/printer.h",
	MAME_DIR .. "src/devices/imagedev/snapquik.cpp",
	MAME_DIR .. "src/devices/imagedev/snapquik.h",
--	MAME_DIR .. "src/devices/imagedev/wafadrive.cpp",
--	MAME_DIR .. "src/devices/imagedev/wafadrive.h",
--	MAME_DIR .. "src/devices/imagedev/avivideo.cpp",
--	MAME_DIR .. "src/devices/imagedev/avivideo.h",
}

---------------------------------------------------
--
--@src/devices/machine/upd1990a.h,MACHINES["UPD1990A"] = true
---------------------------------------------------

if (MACHINES["UPD1990A"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/upd1990a.cpp",
		MAME_DIR .. "src/devices/machine/upd1990a.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/gen_latch.h,MACHINES["GEN_LATCH"] = true
---------------------------------------------------

if (MACHINES["GEN_LATCH"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/gen_latch.cpp",
		MAME_DIR .. "src/devices/machine/gen_latch.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/watchdog.h,MACHINES["WATCHDOG"] = true
---------------------------------------------------

if (MACHINES["WATCHDOG"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/watchdog.cpp",
		MAME_DIR .. "src/devices/machine/watchdog.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/z80daisy.h,MACHINES["Z80DAISY"] = true
---------------------------------------------------

if (MACHINES["Z80DAISY"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/z80daisy.cpp",
		MAME_DIR .. "src/devices/machine/z80daisy.h",
		MAME_DIR .. "src/devices/machine/z80daisy_generic.cpp",
		MAME_DIR .. "src/devices/machine/z80daisy_generic.h",
	}
end

-- SYSTEM (MEGADRIVE)
---------------------------------------------------
--
--@src/devices/machine/i2cmem.h,MACHINES["I2CMEM"] = true
---------------------------------------------------

if (MACHINES["I2CMEM"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/i2cmem.cpp",
		MAME_DIR .. "src/devices/machine/i2cmem.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/lc89510.h,MACHINES["LC89510"] = true
---------------------------------------------------

if (MACHINES["LC89510"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/lc89510.cpp",
		MAME_DIR .. "src/devices/machine/lc89510.h",
	}
end

-- SYSTEM (GBA)
---------------------------------------------------
--
--@src/devices/machine/intelfsh.h,MACHINES["INTELFLASH"] = true
---------------------------------------------------

if (MACHINES["INTELFLASH"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/intelfsh.cpp",
		MAME_DIR .. "src/devices/machine/intelfsh.h",
	}
end

-- SYSTEM (PGM)
---------------------------------------------------
--
--@src/devices/machine/v3021.h,MACHINES["V3021"] = true
---------------------------------------------------

if (MACHINES["V3021"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/v3021.cpp",
		MAME_DIR .. "src/devices/machine/v3021.h",
	}
end

-- SYSTEM (M92)
---------------------------------------------------
--
--@src/devices/machine/pic8259.h,MACHINES["PIC8259"] = true
---------------------------------------------------

if (MACHINES["PIC8259"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/pic8259.cpp",
		MAME_DIR .. "src/devices/machine/pic8259.h",
	}
end
