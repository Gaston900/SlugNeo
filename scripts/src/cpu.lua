-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

---------------------------------------------------------------------------
--
--   cpu.lua
--
--   Rules for building CPU cores
--
---------------------------------------------------------------------------

--------------------------------------------------
-- Dynamic recompiler objects
--------------------------------------------------

DRC_CPUS = { "E1", "SH", "MIPS3", "POWERPC", "RSP", "ARM7", "ADSP21062", "MB86235", "DSP16", "UNSP" }
CPU_INCLUDE_DRC = false
for i, v in ipairs(DRC_CPUS) do
	if (CPUS[v]~=null) then
		CPU_INCLUDE_DRC = true
		break
	end
end

-- SYSTEM (GB)
if (CPU_INCLUDE_DRC) then
	files {
		MAME_DIR .. "src/devices/cpu/drcbec.cpp",
		MAME_DIR .. "src/devices/cpu/drcbec.h",
		MAME_DIR .. "src/devices/cpu/drcbeut.cpp",
		MAME_DIR .. "src/devices/cpu/drcbeut.h",
		MAME_DIR .. "src/devices/cpu/drccache.cpp",
		MAME_DIR .. "src/devices/cpu/drccache.h",
		MAME_DIR .. "src/devices/cpu/drcfe.cpp",
		MAME_DIR .. "src/devices/cpu/drcfe.h",
		MAME_DIR .. "src/devices/cpu/drcuml.cpp",
		MAME_DIR .. "src/devices/cpu/drcuml.h",
		MAME_DIR .. "src/devices/cpu/uml.cpp",
		MAME_DIR .. "src/devices/cpu/uml.h",
		MAME_DIR .. "src/devices/cpu/x86log.cpp",
		MAME_DIR .. "src/devices/cpu/x86log.h",
		MAME_DIR .. "src/devices/cpu/drcumlsh.h",
	}
	if not _OPTIONS["FORCE_DRC_C_BACKEND"] then
		files {
			MAME_DIR .. "src/devices/cpu/drcbex64.cpp",
			MAME_DIR .. "src/devices/cpu/drcbex64.h",
			MAME_DIR .. "src/devices/cpu/drcbex86.cpp",
			MAME_DIR .. "src/devices/cpu/drcbex86.h",
		}
	end
end

--------------------------------------------------
-- Motorola 68000 series
--@src/devices/cpu/m68000/m68000.h,CPUS["M680X0"] = true
--------------------------------------------------

if (CPUS["M680X0"]~=null) then
	files {
		MAME_DIR .. "src/devices/cpu/m68000/m68kcpu.cpp",
		MAME_DIR .. "src/devices/cpu/m68000/m68kcpu.h",
		MAME_DIR .. "src/devices/cpu/m68000/m68kops.cpp",
		MAME_DIR .. "src/devices/cpu/m68000/m68kops.h",
		MAME_DIR .. "src/devices/cpu/m68000/m68000.h",
		MAME_DIR .. "src/devices/cpu/m68000/m68kfpu.hxx",
		MAME_DIR .. "src/devices/cpu/m68000/m68kmmu.h",
	}
end

if (CPUS["M680X0"]~=null or _OPTIONS["with-tools"]) then
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/m68000/m68kdasm.cpp")
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/m68000/m68kdasm.h")
end

--------------------------------------------------
-- Zilog Z80
--@src/devices/cpu/z80/z80.h,CPUS["Z80"] = true
--------------------------------------------------

if (CPUS["Z80"]~=null) then
	files {
		MAME_DIR .. "src/devices/cpu/z80/z80.cpp",
		MAME_DIR .. "src/devices/cpu/z80/z80.h",
		MAME_DIR .. "src/devices/cpu/z80/tmpz84c011.cpp",
		MAME_DIR .. "src/devices/cpu/z80/tmpz84c011.h",
		MAME_DIR .. "src/devices/cpu/z80/tmpz84c015.cpp",
		MAME_DIR .. "src/devices/cpu/z80/tmpz84c015.h",
		MAME_DIR .. "src/devices/cpu/z80/kc82.cpp",
		MAME_DIR .. "src/devices/cpu/z80/kc82.h",
		MAME_DIR .. "src/devices/cpu/z80/kl5c80a12.cpp",
		MAME_DIR .. "src/devices/cpu/z80/kl5c80a12.h",
		MAME_DIR .. "src/devices/cpu/z80/kl5c80a16.cpp",
		MAME_DIR .. "src/devices/cpu/z80/kl5c80a16.h",
		MAME_DIR .. "src/devices/cpu/z80/kp69.cpp",
		MAME_DIR .. "src/devices/cpu/z80/kp69.h",
		MAME_DIR .. "src/devices/cpu/z80/ky80.cpp",
		MAME_DIR .. "src/devices/cpu/z80/ky80.h",
		MAME_DIR .. "src/devices/cpu/z80/lz8420m.cpp",
		MAME_DIR .. "src/devices/cpu/z80/lz8420m.h",
	}
end

if (CPUS["Z80"]~=null or _OPTIONS["with-tools"]) then
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/z80/z80dasm.cpp")
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/z80/z80dasm.h")
end

-- SYSTEM (NPG)
--------------------------------------------------
-- Toshiba TLCS-900 Series
--@src/devices/cpu/tlcs900/tlcs900.h,CPUS["TLCS900"] = true
--------------------------------------------------

if (CPUS["TLCS900"]~=null) then
	files {
		MAME_DIR .. "src/devices/cpu/tlcs900/tlcs900.cpp",
		MAME_DIR .. "src/devices/cpu/tlcs900/tlcs900.h",
		MAME_DIR .. "src/devices/cpu/tlcs900/900tbl.hxx",
		MAME_DIR .. "src/devices/cpu/tlcs900/900htbl.hxx",
		MAME_DIR .. "src/devices/cpu/tlcs900/tmp95c061.cpp",
		MAME_DIR .. "src/devices/cpu/tlcs900/tmp95c061.h",
		MAME_DIR .. "src/devices/cpu/tlcs900/tmp95c063.cpp",
		MAME_DIR .. "src/devices/cpu/tlcs900/tmp95c063.h",
		MAME_DIR .. "src/devices/cpu/tlcs900/tmp96c141.cpp",
		MAME_DIR .. "src/devices/cpu/tlcs900/tmp96c141.h",
	}
end

if (CPUS["TLCS900"]~=null or _OPTIONS["with-tools"]) then
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/tlcs900/dasm900.cpp")
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/tlcs900/dasm900.h")
end

-- SYSTEM (MEGADRIVE)
--------------------------------------------------
-- Hitachi SuperH series (SH1/SH2/SH3/SH4)
--@src/devices/cpu/sh/sh2.h,CPUS["SH"] = true
--@src/devices/cpu/sh/sh4.h,CPUS["SH"] = true
--------------------------------------------------

if (CPUS["SH"]~=null) then
	files {
		MAME_DIR .. "src/devices/cpu/sh/sh.cpp",
		MAME_DIR .. "src/devices/cpu/sh/sh.h",
		MAME_DIR .. "src/devices/cpu/sh/sh2.cpp",
		MAME_DIR .. "src/devices/cpu/sh/sh2.h",
		MAME_DIR .. "src/devices/cpu/sh/sh2comn.cpp",
		MAME_DIR .. "src/devices/cpu/sh/sh2comn.h",
		MAME_DIR .. "src/devices/cpu/sh/sh_fe.cpp",
		MAME_DIR .. "src/devices/cpu/sh/sh2fe.cpp",
		MAME_DIR .. "src/devices/cpu/sh/sh4fe.cpp",
		MAME_DIR .. "src/devices/cpu/sh/sh7604_bus.cpp",
		MAME_DIR .. "src/devices/cpu/sh/sh7604_bus.h",
		MAME_DIR .. "src/devices/cpu/sh/sh7604_sci.cpp",
		MAME_DIR .. "src/devices/cpu/sh/sh7604_sci.h",
		MAME_DIR .. "src/devices/cpu/sh/sh7604_wdt.cpp",
		MAME_DIR .. "src/devices/cpu/sh/sh7604_wdt.h",
		MAME_DIR .. "src/devices/cpu/sh/sh4.cpp",
		MAME_DIR .. "src/devices/cpu/sh/sh4.h",
		MAME_DIR .. "src/devices/cpu/sh/sh4comn.cpp",
		MAME_DIR .. "src/devices/cpu/sh/sh4comn.h",
		MAME_DIR .. "src/devices/cpu/sh/sh3comn.cpp",
		MAME_DIR .. "src/devices/cpu/sh/sh3comn.h",
		MAME_DIR .. "src/devices/cpu/sh/sh4tmu.cpp",
		MAME_DIR .. "src/devices/cpu/sh/sh4tmu.h",
		MAME_DIR .. "src/devices/cpu/sh/sh4dmac.cpp",
		MAME_DIR .. "src/devices/cpu/sh/sh4dmac.h",
		MAME_DIR .. "src/devices/cpu/sh/sh4regs.h",
	}
end

if (CPUS["SH"]~=null or _OPTIONS["with-tools"]) then
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/sh/sh_dasm.cpp")
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/sh/sh_dasm.h")
end

--------------------------------------------------
-- SSP1601
--@src/devices/cpu/ssp1601/ssp1601.h,CPUS["SSP1601"] = true
--------------------------------------------------

if (CPUS["SSP1601"]~=null) then
	files {
		MAME_DIR .. "src/devices/cpu/ssp1601/ssp1601.cpp",
		MAME_DIR .. "src/devices/cpu/ssp1601/ssp1601.h",
	}
end

if (CPUS["SSP1601"]~=null or _OPTIONS["with-tools"]) then
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/ssp1601/ssp1601d.cpp")
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/ssp1601/ssp1601d.h")
end

--------------------------------------------------
-- Intel 80x86 series (also a dynamic recompiler target)
--@src/devices/cpu/i86/i86.h,CPUS["I86"] = true
--@src/devices/cpu/i86/i286.h,CPUS["I86"] = true
--@src/devices/cpu/i386/i386.h,CPUS["I386"] = true
--------------------------------------------------

--if (CPUS["I86"]~=null) then
--	files {
--		MAME_DIR .. "src/devices/cpu/i86/i86.cpp",
--		MAME_DIR .. "src/devices/cpu/i86/i86.h",
--		MAME_DIR .. "src/devices/cpu/i86/i186.cpp",
--		MAME_DIR .. "src/devices/cpu/i86/i186.h",
--		MAME_DIR .. "src/devices/cpu/i86/i286.cpp",
--		MAME_DIR .. "src/devices/cpu/i86/i286.h",
--		MAME_DIR .. "src/devices/cpu/i86/i86inline.h",
--	}
--end

if (CPUS["I86"]~=null or CPUS["I386"]~=null or CPU_INCLUDE_DRC or _OPTIONS["with-tools"]) then
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/i386/i386dasm.cpp")
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/i386/i386dasm.h")
end

if (CPUS["I386"]~=null) then
	files {
		MAME_DIR .. "src/devices/cpu/i386/i386.cpp",
		MAME_DIR .. "src/devices/cpu/i386/i386.h",
		MAME_DIR .. "src/devices/cpu/i386/athlon.cpp",
		MAME_DIR .. "src/devices/cpu/i386/athlon.h",
		MAME_DIR .. "src/devices/cpu/i386/cache.h",
		MAME_DIR .. "src/devices/cpu/i386/cycles.h",
		MAME_DIR .. "src/devices/cpu/i386/i386op16.hxx",
		MAME_DIR .. "src/devices/cpu/i386/i386op32.hxx",
		MAME_DIR .. "src/devices/cpu/i386/i386ops.h",
		MAME_DIR .. "src/devices/cpu/i386/i386ops.hxx",
		MAME_DIR .. "src/devices/cpu/i386/i386priv.h",
		MAME_DIR .. "src/devices/cpu/i386/i386segs.hxx",
		MAME_DIR .. "src/devices/cpu/i386/i486ops.hxx",
		MAME_DIR .. "src/devices/cpu/i386/pentops.hxx",
		MAME_DIR .. "src/devices/cpu/i386/x87ops.hxx",
		MAME_DIR .. "src/devices/cpu/i386/x87priv.h",
		MAME_DIR .. "src/devices/cpu/i386/cpuidmsrs.hxx",
	}
end

-- SYSTEM (GB)
--------------------------------------------------
-- Sharp LR35902 (Game Boy CPU)
--@src/devices/cpu/lr35902/lr35902.h,CPUS["LR35902"] = true
--------------------------------------------------

if (CPUS["LR35902"]~=null) then
	files {
		MAME_DIR .. "src/devices/cpu/lr35902/lr35902.cpp",
		MAME_DIR .. "src/devices/cpu/lr35902/lr35902.h",
		MAME_DIR .. "src/devices/cpu/lr35902/opc_cb.hxx",
		MAME_DIR .. "src/devices/cpu/lr35902/opc_main.hxx",
	}
end

if (CPUS["LR35902"]~=null or _OPTIONS["with-tools"]) then
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/lr35902/lr35902d.cpp")
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/lr35902/lr35902d.h")
end

--------------------------------------------------
-- Acorn ARM series
--
--@src/devices/cpu/arm/arm.h,CPUS["ARM"] = true
--@src/devices/cpu/arm7/arm7.h,CPUS["ARM7"] = true
--------------------------------------------------

--if (CPUS["ARM"]~=null) then
--	files {
--		MAME_DIR .. "src/devices/cpu/arm/arm.cpp",
--		MAME_DIR .. "src/devices/cpu/arm/arm.h",
--	}
--end

--if (CPUS["ARM"]~=null or _OPTIONS["with-tools"]) then
--	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/arm/armdasm.cpp")
--	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/arm/armdasm.h")
--end

if (CPUS["ARM7"]~=null) then
	files {
		MAME_DIR .. "src/devices/cpu/arm7/arm7.cpp",
		MAME_DIR .. "src/devices/cpu/arm7/arm7.h",
		MAME_DIR .. "src/devices/cpu/arm7/arm7thmb.cpp",
		MAME_DIR .. "src/devices/cpu/arm7/arm7ops.cpp",
		MAME_DIR .. "src/devices/cpu/arm7/lpc210x.cpp",
		MAME_DIR .. "src/devices/cpu/arm7/lpc210x.h",
		MAME_DIR .. "src/devices/cpu/arm7/arm7core.h",
		MAME_DIR .. "src/devices/cpu/arm7/arm7core.hxx",
		MAME_DIR .. "src/devices/cpu/arm7/arm7drc.hxx",
		MAME_DIR .. "src/devices/cpu/arm7/arm7help.h",
		MAME_DIR .. "src/devices/cpu/arm7/arm7tdrc.hxx",
	}
end

if (CPUS["ARM7"]~=null or _OPTIONS["with-tools"]) then
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/arm7/arm7dasm.cpp")
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/arm7/arm7dasm.h")
end

-- SYSTEM (M92)
--------------------------------------------------
-- NEC V-series Intel-compatible
--@src/devices/cpu/nec/nec.h,CPUS["NEC"] = true
--@src/devices/cpu/nec/v25.h,CPUS["NEC"] = true
--@src/devices/cpu/nec/v5x.h,CPUS["NEC"] = true
--@src/devices/cpu/v30mz/v30mz.h,CPUS["V30MZ"] = true
--------------------------------------------------

if (CPUS["NEC"]~=null) then
	files {
		MAME_DIR .. "src/devices/cpu/nec/nec.cpp",
		MAME_DIR .. "src/devices/cpu/nec/nec.h",
		MAME_DIR .. "src/devices/cpu/nec/necea.h",
		MAME_DIR .. "src/devices/cpu/nec/necinstr.h",
		MAME_DIR .. "src/devices/cpu/nec/necinstr.hxx",
		MAME_DIR .. "src/devices/cpu/nec/nec80inst.hxx",
		MAME_DIR .. "src/devices/cpu/nec/necmacro.h",
		MAME_DIR .. "src/devices/cpu/nec/necmodrm.h",
		MAME_DIR .. "src/devices/cpu/nec/necpriv.ipp",
		MAME_DIR .. "src/devices/cpu/nec/v25instr.h",
		MAME_DIR .. "src/devices/cpu/nec/v25instr.hxx",
		MAME_DIR .. "src/devices/cpu/nec/v25priv.ipp",
		MAME_DIR .. "src/devices/cpu/nec/v25.cpp",
		MAME_DIR .. "src/devices/cpu/nec/v25.h",
		MAME_DIR .. "src/devices/cpu/nec/v25sfr.cpp",
		MAME_DIR .. "src/devices/cpu/nec/v5x.cpp",
		MAME_DIR .. "src/devices/cpu/nec/v5x.h",
	}
end

if (CPUS["NEC"]~=null or _OPTIONS["with-tools"]) then
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/nec/necdasm.cpp")
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/nec/necdasm.h")
end

if (CPUS["V30MZ"]~=null) then
	files {
		MAME_DIR .. "src/devices/cpu/v30mz/v30mz.cpp",
		MAME_DIR .. "src/devices/cpu/v30mz/v30mz.h",
	}
end

if (CPUS["V30MZ"]~=null or _OPTIONS["with-tools"]) then
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/nec/necdasm.cpp")
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/nec/necdasm.h")
end
