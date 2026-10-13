-- license:BSD-3-Clause
-- copyright-holders:Gaston90
---------------------------------------------------
--   SLUGNEO target makefile
---------------------------------------------------

---------------------------------------------------
-- specify available CPU cores
---------------------------------------------------

CPUS["Z80"] = true -- NEOGEO
CPUS["M680X0"] = true -- NEOGEO
--------------------------------------------------
-- specify available sound cores
--------------------------------------------------

SOUNDS["SPEAKER"] = true -- NEOGEO
SOUNDS["YM2610"] = true -- NEOGEO
SOUNDS["YM2203"] = true -- NEOGEO CD
SOUNDS["YM2608"] = true -- NEOGEO CD
SOUNDS["YM2610B"] = true -- NEOGEO CD
SOUNDS["YM2612"] = true -- NEOGEO CD
SOUNDS["CDDA"] = true -- NEOGEO CD

--------------------------------------------------
-- specify available video cores
--------------------------------------------------

VIDEOS["MOS6566"] = true -- NEOGEO CD

--------------------------------------------------
-- specify available machine cores
--------------------------------------------------

MACHINES["GEN_LATCH"] = true -- NEOGEO
MACHINES["UPD1990A"] = true -- NEOGEO
MACHINES["WATCHDOG"] = true -- NEOGEO
MACHINES["Z80DAISY"] = true -- NEOGEO
MACHINES["TTL74259"] = true -- NEOGEO CD
MACHINES["INPUT_MERGER"] = true -- NEOGEO CD
MACHINES["LC89510"] = true -- NEOGEO CD

--------------------------------------------------
-- this is the list of driver libraries that
-- comprise MAME plus mamedriv.o which contains
-- the list of drivers
--------------------------------------------------

function linkProjects_mame_arcade(_target, _subtarget)
	links {
		"neogeo",
		"snk",
	}
end

function createMAMEProjects(_target, _subtarget, _name)
	project (_name)
	targetsubdir(_target .."_" .. _subtarget)
	kind (LIBTYPE)
	uuid (os.uuid("drv-" .. _target .."_" .. _subtarget .. "_" .._name))
	addprojectflags() -- no idea if this is needed, seems to work fine without it
	precompiledheaders()

	includedirs {
		MAME_DIR .. "src/osd",
		MAME_DIR .. "src/emu",
		MAME_DIR .. "src/devices",
		MAME_DIR .. "src/hbmame/includes",
		MAME_DIR .. "src/hbmame",
		MAME_DIR .. "src/mame",
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/util",
		MAME_DIR .. "3rdparty",
		MAME_DIR .. "3rdparty/zlib",
		GEN_DIR  .. "mame/layout",
	}
	includedirs {
		ext_includedir("flac"),
		ext_includedir("glm"),
		ext_includedir("jpeg"),
		ext_includedir("rapidjson"),
	}
end

function createProjects_mame_arcade(_target, _subtarget)
--------------------------------------------------
-- the following files are general components and
-- shared across a number of drivers
--------------------------------------------------

createMAMEProjects(_target, _subtarget, "neogeo")
files {
	MAME_DIR .. "src/hbmame/drivers/neogeo.cpp",
	MAME_DIR .. "src/hbmame/drivers/mslug.cpp",
	MAME_DIR .. "src/hbmame/drivers/mslug2.cpp",
	MAME_DIR .. "src/hbmame/drivers/mslug3.cpp",
	MAME_DIR .. "src/hbmame/drivers/mslug4.cpp",
	MAME_DIR .. "src/hbmame/drivers/mslug5.cpp",
	MAME_DIR .. "src/hbmame/drivers/mslugx.cpp",
	MAME_DIR .. "src/hbmame/drivers/homebrew.cpp",
	MAME_DIR .. "src/hbmame/video/neogeo.cpp",
	MAME_DIR .. "src/hbmame/video/neogeo_spr.cpp",
	MAME_DIR .. "src/hbmame/machine/nghb_memcard.cpp",
	MAME_DIR .. "src/hbmame/bus/neogeo/prot.cpp",
	MAME_DIR .. "src/hbmame/bus/neogeo/banked_cart.cpp",
	MAME_DIR .. "src/hbmame/bus/neogeo_ctrl/ctrl.cpp",
	MAME_DIR .. "src/hbmame/bus/neogeo_ctrl/joystick.cpp",
	MAME_DIR .. "src/hbmame/bus/neogeo_ctrl/mahjong.cpp",
	MAME_DIR .. "src/hbmame/bus/neogeo_ctrl/dial.cpp",
	MAME_DIR .. "src/hbmame/bus/neogeo_ctrl/irrmaze.cpp",
	MAME_DIR .. "src/hbmame/bus/neogeo_ctrl/kizuna4p.cpp",
}

createMAMEProjects(_target, _subtarget, "snk")
files {
	MAME_DIR .. "src/mame/snk/neogeocd.cpp",
	MAME_DIR .. "src/mame/snk/neogeo_spr.cpp",
	MAME_DIR .. "src/mame/snk/neogeo.cpp",
	MAME_DIR .. "src/mame/snk/neogeo_v.cpp",
	MAME_DIR .. "src/devices/bus/neogeo/slot.cpp",
	MAME_DIR .. "src/mame/snk/ng_memcard.cpp",
	MAME_DIR .. "src/mame/machine/megacdcd.cpp",
	MAME_DIR .. "src/mame/machine/megacdcd.h",
}

end
