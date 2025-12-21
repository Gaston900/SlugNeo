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
CPUS["TLCS900"] = true -- NPG
CPUS["SH"] = true -- MEGADRIVE
CPUS["SSP1601"] = true -- MEGADRIVE
CPUS["I386"] = true  -- MEGADRIVE
CPUS["LR35902"] = true -- GB
CPUS["ARM7"] = true -- GB

--------------------------------------------------
-- specify available sound cores
--------------------------------------------------

SOUNDS["SPEAKER"] = true -- NEOGEO
SOUNDS["YM2610"] = true -- NEOGEO
SOUNDS["DAC"] = true -- NPG
SOUNDS["T6W28"] = true -- NPG
SOUNDS["VOLT_REG"] = true -- NPG
SOUNDS["YM2203"] = true -- MEGADRIVE
SOUNDS["YM2608"] = true -- MEGADRIVE
SOUNDS["YM2612"] = true -- MEGADRIVE
SOUNDS["AY8910"] = true -- MEGADRIVE
SOUNDS["SN76496"] = true -- MEGADRIVE
SOUNDS["CDDA"] = true -- MEGADRIVE
SOUNDS["RF5C68"] = true -- MEGADRIVE
SOUNDS["SN76496"] = true -- MEGADRIVE
SOUNDS["GB_SOUND"] = true -- GBA

--------------------------------------------------
-- specify available machine cores
--------------------------------------------------

MACHINES["GEN_LATCH"] = true -- NEOGEO
MACHINES["UPD1990A"] = true -- NEOGEO
MACHINES["WATCHDOG"] = true -- NEOGEO
MACHINES["Z80DAISY"] = true -- NEOGEO
MACHINES["INTELFLASH"] = true -- GBA

--------------------------------------------------
-- specify available bus cores
--------------------------------------------------

BUSES["GENERIC"] = true -- NPG
BUSES["MEGADRIVE"] = true -- MEGADRIVE
BUSES["GBA"] = true -- GBA
BUSES["GAMEBOY"] = true -- GB

--------------------------------------------------
-- specify available video cores
--------------------------------------------------

VIDEOS["SEGA315_5124"] = true -- MEGADRIVE
VIDEOS["SEGA315_5313"] = true -- MEGADRIVE
VIDEOS["GBA_LCD"] = true -- GBA
VIDEOS["GB_LCD"] = true -- GB

--------------------------------------------------
-- this is the list of driver libraries that
-- comprise MAME plus mamedriv.o which contains
-- the list of drivers
--------------------------------------------------

function linkProjects_mame_arcade(_target, _subtarget)
	links {
		"neogeo",
		"nintendo",
		"sega",
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
--		MAME_DIR .. "src/lib/netlist",
		MAME_DIR .. "3rdparty",
		MAME_DIR .. "3rdparty/zlib",
		GEN_DIR  .. "mame/layout",
--		GEN_DIR  .. "hbmame/layout",
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
	MAME_DIR .. "src/hbmame/drivers/neogeo_noslot.cpp",
	MAME_DIR .. "src/hbmame/drivers/neogeo_noslothb.cpp",
	MAME_DIR .. "src/hbmame/drivers/neogeo_noslotdd.cpp",
	MAME_DIR .. "src/hbmame/drivers/mslug.cpp",
	MAME_DIR .. "src/hbmame/drivers/mslug2.cpp",
	MAME_DIR .. "src/hbmame/drivers/mslug3.cpp",
	MAME_DIR .. "src/hbmame/drivers/mslug4.cpp",
	MAME_DIR .. "src/hbmame/drivers/mslug5.cpp",
	MAME_DIR .. "src/hbmame/drivers/mslugx.cpp",
	MAME_DIR .. "src/hbmame/drivers/homebrew.cpp",
	MAME_DIR .. "src/hbmame/video/neogeo.cpp",
	MAME_DIR .. "src/hbmame/video/neogeo_spr.cpp",
	MAME_DIR .. "src/hbmame/machine/ng_memcard.cpp",
	MAME_DIR .. "src/hbmame/bus/neogeo/prot.cpp",
	MAME_DIR .. "src/hbmame/bus/neogeo/banked_cart.cpp",
	MAME_DIR .. "src/hbmame/bus/neogeo_ctrl/ctrl.cpp",
	MAME_DIR .. "src/hbmame/bus/neogeo_ctrl/joystick.cpp",
	MAME_DIR .. "src/hbmame/bus/neogeo_ctrl/mahjong.cpp",
	MAME_DIR .. "src/hbmame/bus/neogeo_ctrl/dial.cpp",
	MAME_DIR .. "src/hbmame/bus/neogeo_ctrl/irrmaze.cpp",
	MAME_DIR .. "src/hbmame/bus/neogeo_ctrl/kizuna4p.cpp",
}

createMAMEProjects(_target, _subtarget, "nintendo")
files {
	MAME_DIR .. "src/mame/drivers/gb.cpp",
	MAME_DIR .. "src/mame/includes/gb.h",
	MAME_DIR .. "src/mame/machine/gb.cpp",
	MAME_DIR .. "src/mame/drivers/gba.cpp",
	MAME_DIR .. "src/mame/includes/gba.h",
}

createMAMEProjects(_target, _subtarget, "sega")
files {
	MAME_DIR .. "src/mame/drivers/megadriv.cpp",
	MAME_DIR .. "src/mame/includes/megadriv.h",
	MAME_DIR .. "src/mame/machine/megadriv.cpp",
	MAME_DIR .. "src/mame/machine/mega32x.cpp",
	MAME_DIR .. "src/mame/machine/mega32x.h",
	MAME_DIR .. "src/mame/machine/megacd.cpp",
	MAME_DIR .. "src/mame/machine/megacd.h",
	MAME_DIR .. "src/mame/machine/megacdcd.cpp",
	MAME_DIR .. "src/mame/machine/megacdcd.h",
}

createMAMEProjects(_target, _subtarget, "snk")
files {
	MAME_DIR .. "src/mame/drivers/ngp.cpp",
	MAME_DIR .. "src/mame/video/k1ge.cpp",
	MAME_DIR .. "src/mame/video/k1ge.h",
}

end
