// license:BSD-3-Clause
// copyright-holders:Bryan McPhail,Ernesto Corvi,Andrew Prime,Zsolt Vasvari
// thanks-to:Fuzz
/***************************************************************************

    Neo-Geo hardware

    Cartridge colours:
    ==================

    MVS cartridges were produced in different colours.

    Known cartridge colours:
    . Black
    . Blue
    . Green
    . Grey
    . Red
    . Transparent
    . Transparent Blue
    . Transparent Green
    . White
    . Yellow

    The above listed only covers SNK / PLAYMORE / SNKPLAYMORE PCBs. There also exists a
    wide range of 'bootleg' PCBs.

*****************************************************************************

    Watchdog:
    =========

    The watchdog timer will reset the system after ~0.13 seconds.
    By cgfm's research, exactly 3,244,030 cycles (based on 24MHz clock).

    Newer games force a reset using the following code (this from kof99):
        009CDA  203C 0003 0D40             MOVE.L   #0x30D40,D0
        009CE0  5380                       SUBQ.L   #1,D0
        009CE2  64FC                       BCC.S    *-0x2 [0x9CE0]
    Note however that there is a valid code path after this loop.

    The watchdog is used as a form of protection on a number of games,
    previously this was implemented as a specific hack which locked a single
    address of SRAM.

    What actually happens is if the game doesn't find valid data in the
    backup ram it will initialize it, then sit in a loop.  The watchdog
    should then reset the system while it is in this loop.  If the watchdog
    fails to reset the system the code will continue and set a value in
    backup ram to indiate that the protection check has failed.


    Mahjong Panel notes (2009-03 FP):
    =================================

    * In Service Mode menu with mahjong panel active, controls are as
      follows:

        A = select / up (for options)
        B = down (for options)
        C = go to previous menu
        E = up (for menu entries)
        F = down (for menu entries)
        G = left (for options)
        H = right (for options)

    * These only work with Japanese BIOS, but I think it's not a bug: I
      doubt other BIOS were programmed to be compatible with mahjong panels

****************************************************************************/

#include "includes/neogeo.h"
#include "unzip.h"
#include "softlist_dev.h"
#include "neogeo.lh"


#define LOG_VIDEO_SYSTEM         (0)
#define LOG_MAIN_CPU_BANKING     (0)
#define LOG_AUDIO_CPU_BANKING    (0)


/*************************************
 *
 *  Main CPU interrupt generation
 *
 *************************************/

// The display counter is automatically reloaded with the load register contents on scanline 224,
// 1146 mclks from the rising edge of /HSYNC.
#define NEOGEO_VBLANK_RELOAD_HTIM (attotime::from_ticks(1146, NEOGEO_MASTER_CLOCK))

#define IRQ2CTRL_ENABLE             (0x10)
#define IRQ2CTRL_LOAD_RELATIVE      (0x20)
#define IRQ2CTRL_AUTOLOAD_VBLANK    (0x40)
#define IRQ2CTRL_AUTOLOAD_REPEAT    (0x80)


void neogeo_state::adjust_display_position_interrupt_timer()
{
	attotime period = attotime::from_ticks((u64)m_display_counter + 1, NEOGEO_PIXEL_CLOCK);
	if (LOG_VIDEO_SYSTEM) logerror("adjust_display_position_interrupt_timer  current y: %02x  current x: %02x   target y: %x  target x: %x\n", m_screen->vpos(), m_screen->hpos(), (m_display_counter + 1) / NEOGEO_HTOTAL, (m_display_counter + 1) % NEOGEO_HTOTAL);

	m_display_position_interrupt_timer->adjust(period);
}


void neogeo_state::neogeo_set_display_position_interrupt_control( u16  data )
{
	m_display_position_interrupt_control = data;
}


void neogeo_state::neogeo_set_display_counter_msb( u16  data )
{
	m_display_counter = (m_display_counter & 0x0000ffff) | ((u32)data << 16);

	if (LOG_VIDEO_SYSTEM) logerror("PC %06x: set_display_counter %08x\n", m_maincpu->pc(), m_display_counter);
}


void neogeo_state::neogeo_set_display_counter_lsb( u16  data )
{
	m_display_counter = (m_display_counter & 0xffff0000) | data;

	if (LOG_VIDEO_SYSTEM) logerror("PC %06x: set_display_counter %08x\n", m_maincpu->pc(), m_display_counter);

	if (m_display_position_interrupt_control & IRQ2CTRL_LOAD_RELATIVE)
	{
		if (LOG_VIDEO_SYSTEM) logerror("AUTOLOAD_RELATIVE ");
		adjust_display_position_interrupt_timer();
	}
}


void neogeo_state::update_interrupts()
{
	m_maincpu->set_input_line(3, m_irq3_pending ? ASSERT_LINE : CLEAR_LINE);
	m_maincpu->set_input_line(m_raster_level, m_display_position_interrupt_pending ? ASSERT_LINE : CLEAR_LINE);
	m_maincpu->set_input_line(m_vblank_level, m_vblank_interrupt_pending ? ASSERT_LINE : CLEAR_LINE);
}


void neogeo_state::neogeo_acknowledge_interrupt( u16  data )
{
	if (data & 0x01)
		m_irq3_pending = 0;
	if (data & 0x02)
		m_display_position_interrupt_pending = 0;
	if (data & 0x04)
		m_vblank_interrupt_pending = 0;

	update_interrupts();
}


TIMER_CALLBACK_MEMBER(neogeo_state::display_position_interrupt_callback)
{
	if (LOG_VIDEO_SYSTEM) logerror("--- Scanline @ %d,%d\n", m_screen->vpos(), m_screen->hpos());

	if (m_display_position_interrupt_control & IRQ2CTRL_ENABLE)
	{
		if (LOG_VIDEO_SYSTEM) logerror("*** Scanline interrupt (IRQ2) ***  y: %02x  x: %02x\n", m_screen->vpos(), m_screen->hpos());
		m_display_position_interrupt_pending = 1;

		update_interrupts();
	}

	if (m_display_position_interrupt_control & IRQ2CTRL_AUTOLOAD_REPEAT)
	{
		if (LOG_VIDEO_SYSTEM) logerror("AUTOLOAD_REPEAT ");
		adjust_display_position_interrupt_timer();
	}
}


TIMER_CALLBACK_MEMBER(neogeo_state::display_position_vblank_callback)
{
	if (m_display_position_interrupt_control & IRQ2CTRL_AUTOLOAD_VBLANK)
	{
		if (LOG_VIDEO_SYSTEM) logerror("AUTOLOAD_VBLANK ");
		adjust_display_position_interrupt_timer();
	}

	/* set timer for next screen */
	m_display_position_vblank_timer->adjust(m_screen->time_until_pos(NEOGEO_VBSTART) + NEOGEO_VBLANK_RELOAD_HTIM);
}


TIMER_CALLBACK_MEMBER(neogeo_state::vblank_interrupt_callback)
{
	if (LOG_VIDEO_SYSTEM) logerror("+++ VBLANK @ %d,%d\n", m_screen->vpos(), m_screen->hpos());

	m_vblank_interrupt_pending = 1;
	update_interrupts();

	/* set timer for next screen */
	m_vblank_interrupt_timer->adjust(m_screen->time_until_pos(NEOGEO_VBSTART) + NEOGEO_VBLANK_IRQ_HTIM);
}


void neogeo_state::create_interrupt_timers()
{
	m_display_position_interrupt_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(neogeo_state::display_position_interrupt_callback),this));
	m_display_position_vblank_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(neogeo_state::display_position_vblank_callback),this));
	m_vblank_interrupt_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(neogeo_state::vblank_interrupt_callback),this));
}


void neogeo_state::start_interrupt_timers()
{
	m_vblank_interrupt_timer->adjust(m_screen->time_until_pos(NEOGEO_VBSTART) + NEOGEO_VBLANK_IRQ_HTIM);
	m_display_position_vblank_timer->adjust(m_screen->time_until_pos(NEOGEO_VBSTART) + NEOGEO_VBLANK_RELOAD_HTIM);
}



/*************************************
 *
 *  Audio CPU interrupt generation
 *
 *************************************/

void neogeo_state::audio_cpu_check_nmi()
{
	m_audiocpu->set_input_line(INPUT_LINE_NMI, (m_audio_cpu_nmi_enabled && m_audio_cpu_nmi_pending) ? ASSERT_LINE : CLEAR_LINE);
}

void neogeo_state::audio_cpu_enable_nmi_w(offs_t offset, u8 data)
{
	// out ($08) enables the nmi, out ($18) disables it
	m_audio_cpu_nmi_enabled = !(offset & 0x10);
	audio_cpu_check_nmi();
}



/*************************************
 *
 *  Input ports / Controllers
 *
 *************************************/

u16 neogeo_state::in0_r()
{
	return ((m_edge->in0_r() & m_ctrl1->read_ctrl()) << 8) | m_dsw->read();
}

u16 neogeo_state::in1_r()
{
	return ((m_edge->in1_r() & m_ctrl2->read_ctrl()) << 8) | 0xff;
}

void neogeo_state::io_control_w(offs_t offset, u8 data)
{
	switch (offset)
	{
		case 0x00:
			if (m_ctrl1) m_ctrl1->write_ctrlsel(data);
			if (m_ctrl2) m_ctrl2->write_ctrlsel(data);
			if (m_edge) m_edge->write_ctrlsel(data);
			break;

		case 0x10:
			break;

		case 0x18:
			if (m_type == NEOGEO_MVS)
				set_output_latch(data);
			break;

		case 0x20:
			if (m_type == NEOGEO_MVS)
				set_output_data(data);
			break;

		case 0x28:
			if (m_type == NEOGEO_MVS)
			{
				m_upd4990a->data_in_w(data >> 0 & 1);
				m_upd4990a->clk_w(data >> 1 & 1);
				m_upd4990a->stb_w(data >> 2 & 1);
			}
			break;

//  case 0x30: break; // coin counters
//  case 0x31: break; // coin counters
//  case 0x32: break; // coin lockout
//  case 0x33: break; // coin lockout

		default:
			logerror("PC: %x  Unmapped I/O control write.  Offset: %x  Data: %x\n", machine().describe_context(), offset, data);
			break;
	}
}


/*************************************
 *
 *  Unmapped memory access
 *
 *************************************/

u16 neogeo_state::neogeo_unmapped_r(address_space &space)
{
	u16  ret;

	/* unmapped memory returns the last word on the data bus, which is almost always the opcode
	   of the next instruction due to prefetch */

	/* prevent recursion */
	if (m_recurse)
		ret = 0xffff;
	else
	{
		m_recurse = true;
		ret = space.read_word(m_maincpu->pc());
		m_recurse = false;
	}
	return ret;
}



/*************************************
 *
 *  NVRAM (Save RAM)
 *
 *************************************/

void neogeo_state::set_save_ram_unlock( u8 data )
{
	m_save_ram_unlocked = data;
}


void neogeo_state::save_ram_w(offs_t offset, u16 data, u16 mem_mask)
{
	if (m_save_ram_unlocked)
		COMBINE_DATA(&m_save_ram[offset]);
}



/*************************************
 *
 *  Memory card
 *
 *************************************/

CUSTOM_INPUT_MEMBER(neogeo_state::get_memcard_status)
{
	// D0 and D1 are memcard 1 and 2 presence indicators, D2 indicates memcard
	// write protect status (we are always write enabled)
	return (m_memcard->present() == -1) ? 0x07 : 0x00;
}


u16 neogeo_state::memcard_r(offs_t offset)
{
	m_maincpu->eat_cycles(2); // insert waitstate

	u16  ret;

	if (m_memcard->present() != -1)
		ret = m_memcard->read(offset) | 0xff00;
	else
		ret = 0xffff;

	return ret;
}


void neogeo_state::memcard_w(offs_t offset, u16 data, u16 mem_mask)
{
	m_maincpu->eat_cycles(2); // insert waitstate

	if (ACCESSING_BITS_0_7)
	{
		if (m_memcard->present() != -1)
				m_memcard->write(offset, data);
	}
}

/*************************************
 *
 *  Inter-CPU communications
 *
 *************************************/

void neogeo_state::audio_command_w(u8 data)
{
	m_soundlatch->write(data);

	m_audio_cpu_nmi_pending = true;
	audio_cpu_check_nmi();

	/* boost the interleave to let the audio CPU read the command */
	machine().scheduler().boost_interleave(attotime::zero, attotime::from_usec(50));
}


u8 neogeo_state::audio_command_r()
{
	u8 ret = m_soundlatch->read();

	m_audio_cpu_nmi_pending = false;
	audio_cpu_check_nmi();

	return ret;
}


CUSTOM_INPUT_MEMBER(neogeo_state::get_audio_result)
{
	u8 ret = m_soundlatch2->read();

	return ret;
}




void neogeo_state::neogeo_main_cpu_banking_init()
{
	m_use_cart_vectors = 0;

	if (m_type != NEOGEO_CD)
	{
		m_banked_cart->init_banks();
	}
}


/*************************************
 *
 *  Audio CPU banking
 *
 *************************************/

u8 neogeo_state::audio_cpu_bank_select_r(offs_t offset)
{
	m_bank_audio_cart[offset & 3]->set_entry(offset >> 8);

	return 0;
}


void neogeo_state::neogeo_audio_cpu_banking_init(int set_entry)
{
	if (m_type == NEOGEO_CD) return;

	int region;
	int bank;
	u8 *rgn;
	u32 address_mask;

	rgn = memregion("audiocpu")->base();

	/* audio bios/cartridge selection */
	m_bank_audio_main->configure_entry(1, memregion("audiocpu")->base());
	if (memregion("audiobios"))
		m_bank_audio_main->configure_entry(0, memregion("audiobios")->base());
	else /* on hardware with no SM1 ROM, the cart ROM is always enabled */
		m_bank_audio_main->configure_entry(0, memregion("audiocpu")->base());

	m_bank_audio_main->set_entry(m_use_cart_audio);

	/* audio banking */
	m_bank_audio_cart[0] = membank("audio_f000");
	m_bank_audio_cart[1] = membank("audio_e000");
	m_bank_audio_cart[2] = membank("audio_c000");
	m_bank_audio_cart[3] = membank("audio_8000");

	address_mask = (memregion("audiocpu")->bytes() - 0x10000 - 1) & 0x3ffff;


	for (region = 0; region < 4; region++)
	{
		for (bank = 0xff; bank >= 0; bank--)
		{
			u32 bank_address = 0x10000 + ((bank << (11 + region)) & address_mask);
			m_bank_audio_cart[region]->configure_entry(bank, &rgn[bank_address]);
		}
	}

	// set initial audio banks - THIS IS A HACK
	// Z80 banking is handled by the NEO-ZMC chip in the cartridge
	// (in later cartridges, by multifunction banking/protection chips that implement the same bank scheme)
	// On the real chip, initial banks are all 0.
	// However, early cartridges with less than 64KB of Z80 code and data don't have ROM banking at all.
	// These initial bank settings are required so non-banked games will work until we identify them
	// and use a different Z80 address map for them.
	m_bank_audio_cart[0]->set_entry(0x1e);
	m_bank_audio_cart[1]->set_entry(0x0e);
	m_bank_audio_cart[2]->set_entry(0x06);
	m_bank_audio_cart[3]->set_entry(0x02);
}



/*************************************
 *
 *  System control register
 *
 *************************************/

void neogeo_state::system_control_w(offs_t offset, u8 data)
{
	u8 bit = (offset >> 3) & 0x01;

	switch (offset & 0x07)
	{
		default:
		case 0x00:
			neogeo_set_screen_shadow(bit);
			break;

		case 0x01:
			if (m_type == NEOGEO_CD)
				printf("NeoCD: write to regular vector change address? %d\n", bit); // what IS going on with "neocdz doubledr" and why do games write here if it's hooked up to nothing?
			else
				m_use_cart_vectors = bit;
			break;

		case 0x05:
			if (m_type == NEOGEO_MVS)
			{
				m_use_cart_audio = bit;
				m_sprgen->neogeo_set_fixed_layer_source(bit);
				m_bank_audio_main->set_entry(m_use_cart_audio);
			}
			break;

		case 0x06:
			if (m_type == NEOGEO_MVS)
				set_save_ram_unlock(bit);
			break;

		case 0x07:
			neogeo_set_palette_bank(bit);
			break;

		case 0x02: // memory card 1: write enable/disable
		case 0x03: // memory card 2: write disable/enable
		case 0x04: // memory card: register select enable/set to normal (what does it mean?)
			logerror("PC: %x  Unmapped system control write.  Offset: %x  Data: %x\n", machine().describe_context(), offset & 0x07, bit);
			break;
	}

	if (LOG_VIDEO_SYSTEM && ((offset & 0x07) != 0x06)) logerror("PC: %x  System control write.  Offset: %x  Data: %x\n", machine().describe_context(), offset & 0x07, bit);
}



/*************************************
 *
 *  LEDs
 *
 *************************************/

void neogeo_state::set_outputs(  )
{
	static const u8 led_map[0x10] =
		{ 0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x58,0x4c,0x62,0x69,0x78,0x00 };

	/* EL */
	m_out_digit[0] = led_map[m_el_value];

	/* LED1 */
	m_out_digit[1] = led_map[m_led1_value >> 4];
	m_out_digit[2] = led_map[m_led1_value & 0x0f];

	/* LED2 */
	m_out_digit[3] = led_map[m_led2_value >> 4];
	m_out_digit[4] = led_map[m_led2_value & 0x0f];
}


void neogeo_state::set_output_latch( u8 data )
{
	/* looks like the LEDs are set on the
	   falling edge */
	u8 falling_bits = m_output_latch & ~data;

	if (falling_bits & 0x08)
		m_el_value = 16 - (m_output_data & 0x0f);

	if (falling_bits & 0x10)
		m_led1_value = ~m_output_data;

	if (falling_bits & 0x20)
		m_led2_value = ~m_output_data;

	if (falling_bits & 0xc7)
		logerror("%s  Unmaped LED write.  Data: %x\n", machine().describe_context(), falling_bits);

	m_output_latch = data;

	set_outputs();
}


void neogeo_state::set_output_data( u8 data )
{
	m_output_data = data;
}



/*************************************
 *
 *  Machine initialization
 *
 *************************************/

void neogeo_state::init_neogeo()
{
	m_banked_cart->install_banks(machine(), m_maincpu, m_region_maincpu->base(), m_region_maincpu->bytes());

	m_sprgen->m_fixed_layer_bank_type = 0;

	// install controllers
	m_maincpu->space(AS_PROGRAM).install_read_handler(0x300000, 0x300001, 0, 0x01ff7e, 0, read16smo_delegate(*this, FUNC(neogeo_state::in0_r)));
	m_maincpu->space(AS_PROGRAM).install_read_handler(0x340000, 0x340001, 0, 0x01fffe, 0, read16smo_delegate(*this, FUNC(neogeo_state::in1_r)));
    m_maincpu->space(AS_PROGRAM).install_read_port(0x300000, 0x300001, 0x01ff7e, "DSW");
	m_maincpu->space(AS_PROGRAM).install_read_port(0x340000, 0x340001, 0x01fffe, "P2");
    m_sprgen->set_sprite_region(m_region_sprites->base(), m_region_sprites->bytes());
	m_sprgen->set_fixed_regions(m_region_fixed->base(), m_region_fixed->bytes(), m_region_fixedbios);
}


void neogeo_state::neogeo_postload()
{
	m_bank_audio_main->set_entry(m_use_cart_audio);

	if (m_type == NEOGEO_MVS) set_outputs();
}


void neogeo_state::machine_start()
{
	m_out_digit.resolve();
	m_type = NEOGEO_MVS;

	/* set the initial main CPU bank */
	neogeo_main_cpu_banking_init();

	/* set the initial audio CPU ROM banks */
	neogeo_audio_cpu_banking_init(1);

	create_interrupt_timers();

	/* irq levels for MVS / AES */
	m_vblank_level = 1;
	m_raster_level = 2;

	/* start with an IRQ3 - but NOT on a reset */
	m_irq3_pending = 1;

	// enable rtc and serial mode
	m_upd4990a->cs_w(1);
	m_upd4990a->oe_w(1);
	m_upd4990a->c0_w(1);
	m_upd4990a->c1_w(1);
	m_upd4990a->c2_w(1);

	/* register state save */
	save_item(NAME(m_display_position_interrupt_control));
	save_item(NAME(m_display_counter));
	save_item(NAME(m_vblank_interrupt_pending));
	save_item(NAME(m_display_position_interrupt_pending));
	save_item(NAME(m_irq3_pending));
	save_item(NAME(m_audio_cpu_nmi_enabled));
	save_item(NAME(m_audio_cpu_nmi_pending));
	save_item(NAME(m_save_ram_unlocked));
	save_item(NAME(m_output_data));
	save_item(NAME(m_output_latch));
	save_item(NAME(m_el_value));
	save_item(NAME(m_led1_value));
	save_item(NAME(m_led2_value));

	save_item(NAME(m_use_cart_vectors));
	save_item(NAME(m_use_cart_audio));

	machine().save().register_postload(save_prepost_delegate(FUNC(neogeo_state::neogeo_postload), this));

	m_sprgen->set_screen(m_screen);
}



/*************************************
 *
 *  Machine reset
 *
 *************************************/

void neogeo_state::machine_reset()
{
	/* reset system control registers */
	for (offs_t offs = 0; offs < 8; offs++)
		system_control_w(offs, 0);

	// disable audiocpu nmi
	m_audio_cpu_nmi_enabled = false;
	m_audio_cpu_nmi_pending = false;
	audio_cpu_check_nmi();

	m_maincpu->reset();

	start_interrupt_timers();

	/* trigger the IRQ3 that was set by MACHINE_START */
	update_interrupts();

	m_recurse = false;

	u16 game = cpuregion[0x109] * 256 + cpuregion[0x108];
	printf("Game number = %04X\n", game);
}

u16 neogeo_state::banked_vectors_r(offs_t offset)
{
	if (!m_use_cart_vectors)
	{
		u16 * bios = (u16 *)memregion("mainbios")->base();
		return bios[offset];
	}
	else
	{
		u16 * game = (u16 *)m_region_maincpu->base();
		return game[offset];
	}

}

u16 neogeo_state::neogeo_slot_rom_low_r()
{
	return 0;
}

u16 neogeo_state::neogeo_slot_rom_low_vectors_r(offs_t offset)
{
	if (!m_use_cart_vectors)
	{
		u16 * bios = (u16 *)memregion("mainbios")->base();
		return bios[offset];
	}
	else
	{
		return 0;
	}

}

/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

void neogeo_state::neogeo_main_map(address_map &map)
{
	map(0x100000,0x10ffff).mirror(0x0f0000).ram();
	/* some games have protection devices in the 0x200000 region, it appears to map to cart space, not surprising, the ROM is read here too */
	map(0x300080,0x300081).mirror(0x01ff7e).portr("TEST");
	map(0x300000,0x300001).mirror(0x01fffe).w("watchdog",FUNC(watchdog_timer_device::reset_w)).umask16(0x00ff);
	map(0x320000,0x320001).mirror(0x01fffe).portr("AUDIO/COIN");
	map(0x320000,0x320001).mirror(0x01fffe).w(FUNC(neogeo_state::audio_command_w)).umask16(0xff00);
	map(0x360000,0x37ffff).r(FUNC(neogeo_state::neogeo_unmapped_r));
	map(0x380000,0x380001).mirror(0x01fffe).portr("SYSTEM");
	map(0x380000,0x38007f).mirror(0x01ff80).w(FUNC(neogeo_state::io_control_w)).umask16(0x00ff);
	map(0x3a0000,0x3a001f).mirror(0x01ffe0).r(FUNC(neogeo_state::neogeo_unmapped_r));
	map(0x3a0000,0x3a001f).mirror(0x01ffe0).w(FUNC(neogeo_state::system_control_w)).umask16(0x00ff);
	map(0x3c0000,0x3c0007).mirror(0x01fff8).r(FUNC(neogeo_state::neogeo_video_register_r));
	map(0x3c0000,0x3c000f).mirror(0x01fff0).w(FUNC(neogeo_state::neogeo_video_register_w));
	map(0x3e0000,0x3fffff).r(FUNC(neogeo_state::neogeo_unmapped_r));
	map(0x400000,0x401fff).mirror(0x3fe000).rw(FUNC(neogeo_state::neogeo_paletteram_r),FUNC(neogeo_state::neogeo_paletteram_w));
	map(0x800000,0x800fff).rw(FUNC(neogeo_state::memcard_r),FUNC(neogeo_state::memcard_w));
	map(0xc00000,0xc1ffff).mirror(0x0e0000).rom().region("mainbios",0);
	map(0xd00000,0xd0ffff).mirror(0x0f0000).ram().w(FUNC(neogeo_state::save_ram_w)).share("saveram");
	map(0xe00000,0xffffff).r(FUNC(neogeo_state::neogeo_unmapped_r));
}


void neogeo_state::main_map_slot(address_map &map)
{
	neogeo_main_map(map);
	map(0x000000,0x00007f).r(FUNC(neogeo_state::neogeo_slot_rom_low_vectors_r));
	map(0x000080,0x0fffff).r(FUNC(neogeo_state::neogeo_slot_rom_low_r));
	map(0x200000,0x2fffff).bankr("cartridge");
//  AM_RANGE(0x2ffff0, 0x2fffff) AM_WRITE(main_cpu_bank_select_w)
}

/*************************************
 *
 *  Audio CPU memory handlers
 *
 *************************************/

void neogeo_state::audio_map(address_map &map)
{
	map(0x0000,0x7fff).bankr("audio_main");
	map(0x8000,0xbfff).bankr("audio_8000");
	map(0xc000,0xdfff).bankr("audio_c000");
	map(0xe000,0xefff).bankr("audio_e000");
	map(0xf000,0xf7ff).bankr("audio_f000");
	map(0xf800,0xffff).ram();
}

/*************************************
 *
 *  Audio CPU port handlers
 *
 *************************************/

void neogeo_state::audio_io_map(address_map &map)
{
	map(0x00,0x00).mirror(0xff00).r(FUNC(neogeo_state::audio_command_r)).w("soundlatch",FUNC(generic_latch_8_device::clear_w));
	map(0x04,0x07).mirror(0xff00).rw("ymsnd",FUNC(ym2610_device::read),FUNC(ym2610_device::write));
	map(0x08,0x08).mirror(0xff00).select(0x0010).w(FUNC(neogeo_state::audio_cpu_enable_nmi_w));
	map(0x08,0x0b).mirror(0x00f0).select(0xff00).r(FUNC(neogeo_state::audio_cpu_bank_select_r));
	map(0x0c,0x0c).mirror(0xff00).w("soundlatch2",FUNC(generic_latch_8_device::write));
}

/*************************************
 *
 *  Standard Neo-Geo DIPs and
 *  input port definition
 *
 *************************************/

INPUT_PORTS_START( neogeo )
	PORT_START("DSW")
	PORT_DIPNAME( 0x01, 0x01, "Setting Mode" ) PORT_DIPLOCATION("SW:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) ) PORT_DIPLOCATION("SW:2")
	PORT_DIPSETTING(    0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x02, "VS Mode" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Controller ) ) PORT_DIPLOCATION("SW:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Joystick ) )
	PORT_DIPSETTING(    0x00, "Mahjong Panel" )
	PORT_DIPNAME( 0x18, 0x18, "COMM Setting (Cabinet No.)" ) PORT_DIPLOCATION("SW:4,5")
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x20, 0x20, "COMM Setting (Link Enable)" ) PORT_DIPLOCATION("SW:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Free_Play ) ) PORT_DIPLOCATION("SW:7")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" ) PORT_DIPLOCATION("SW:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_CONDITION("DSW", 0xF000, NOTEQUALS, 0x0100)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_CONDITION("DSW", 0xF000, NOTEQUALS, 0x0200)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_CONDITION("DSW", 0xF000, NOTEQUALS, 0x0400)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_CONDITION("DSW", 0xF000, NOTEQUALS, 0x0800)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("@P1 Attack @Button1") PORT_CONDITION("DSW", 0xF000, NOTEQUALS, 0x1000)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("@P1 Jump @Button2") PORT_CONDITION("DSW", 0xF000, NOTEQUALS, 0x2000)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("@P1 Bomb @Button3") PORT_CONDITION("DSW", 0xF000, NOTEQUALS, 0x4000)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("@P1 It Has No Use @Button4") PORT_CONDITION("DSW", 0xF000, NOTEQUALS, 0x8000)

	PORT_START("P2")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_CONDITION("P2", 0xF000, NOTEQUALS, 0x0100)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_CONDITION("P2", 0xF000, NOTEQUALS, 0x0200)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_CONDITION("P2", 0xF000, NOTEQUALS, 0x0400)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_CONDITION("P2", 0xF000, NOTEQUALS, 0x0800)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("@P2 Attack @Button1") PORT_CONDITION("P2", 0xF000, NOTEQUALS, 0x1000)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("@P2 Jump @Button2") PORT_CONDITION("P2", 0xF000, NOTEQUALS, 0x2000)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) PORT_NAME("@P2 Bomb @Button3") PORT_CONDITION("P2", 0xF000, NOTEQUALS, 0x4000)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2) PORT_NAME("@P2 It Has No Use @Button4") PORT_CONDITION("P2", 0xF000, NOTEQUALS, 0x8000)

	PORT_START("SYSTEM")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Next Game") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Previous Game") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x7000, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_CUSTOM_MEMBER(neogeo_state, get_memcard_status)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_CUSTOM ) /* Hardware type (AES=0, MVS=1). Some games check this and show a piracy warning screen if the hardware and BIOS don't match */

	PORT_START("AUDIO/COIN")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN3 ) /* What is this? "us-e" BIOS uses it as a coin input; Universe BIOS uses it to detect MVS or AES hardware */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_COIN4 ) /* What is this? "us-e" BIOS uses it as a coin input; Universe BIOS uses it to detect MVS or AES hardware */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_CUSTOM ) /* what is this? When ACTIVE_HIGH + IN4 bit 6 ACTIVE_LOW MVS-4 slot is detected */
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_READ_LINE_DEVICE_MEMBER("upd4990a", upd1990a_device, tp_r)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_READ_LINE_DEVICE_MEMBER("upd4990a", upd1990a_device, data_out_r)
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_CUSTOM_MEMBER(neogeo_state,get_audio_result)

	PORT_START("TEST")
	PORT_BIT( 0x003f, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_CUSTOM ) /* what is this? If ACTIVE_LOW, MVS-6 slot detected, when ACTIVE_HIGH MVS-1 slot (AES) detected */
	PORT_SERVICE_NO_TOGGLE( 0x0080, IP_ACTIVE_LOW )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( neogeo_6slot )
	PORT_INCLUDE( neogeo )

	PORT_MODIFY("TEST")
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_CUSTOM )
INPUT_PORTS_END


// Fixed
const gfx_layout charlayout =
{
	8,8,			/* 8 x 8 chars */
	RGN_FRAC(1,1),
	4,				/* 4 bits per pixel */
	{ 0, 1, 2, 3 },    /* planes are packed in a nibble */
	{ 33*4, 32*4, 49*4, 48*4, 1*4, 0*4, 17*4, 16*4 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8	/* 32 bytes per char */
};

// Sprites
const gfx_layout tilelayout =
{
	16,16,	 /* 16*16 sprites */
	RGN_FRAC(1,1),
	4,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 8*8 },	/* line modulo */
	128*8		/* char modulo */
};

GFXDECODE_START( gfx_neogeo )
	GFXDECODE_ENTRY( "fixed",     0x0000, charlayout, 0, 0x1fff )
	GFXDECODE_ENTRY( "fixedbios", 0x0000, charlayout, 0, 0x1fff )
	//GFXDECODE_ENTRY( "sprites",   0x0000, tilelayout, 0, 0x1fff )  // not working
GFXDECODE_END


/*************************************
 *
 *  Machine driver
 *
 *************************************/

void neogeo_state::neogeo_base(machine_config &config)
{
	/* basic machine hardware */
	M68000(config, m_maincpu, 24000000 / 2);
	m_maincpu->set_addrmap(AS_PROGRAM, &neogeo_state::neogeo_main_map);

	Z80(config, m_audiocpu, NEOGEO_AUDIO_CPU_CLOCK);
	m_audiocpu->set_addrmap(AS_PROGRAM, &neogeo_state::audio_map);
	m_audiocpu->set_addrmap(AS_IO, &neogeo_state::audio_io_map);

	/* video hardware */
	config.set_default_layout(layout_neogeo);

	SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
	m_screen->set_raw(NEOGEO_PIXEL_CLOCK, NEOGEO_HTOTAL, NEOGEO_HBEND, NEOGEO_HBSTART, NEOGEO_VTOTAL, NEOGEO_VBEND, NEOGEO_VBSTART);
	m_screen->set_screen_update(FUNC(neogeo_state::screen_update_neogeo));

	GFXDECODE(config, "gfxdecode", m_palette, gfx_neogeo);
	/* 4096 colors * two banks * normal and shadow */
	PALETTE(config, m_palette, palette_device::BLACK, 4096*2*2);

	NEOGEO_SPRITE(config, m_sprgen, 0).set_screen(m_screen);

	/* audio hardware */
	SPEAKER(config, "lspeaker").front_left();
	SPEAKER(config, "rspeaker").front_right();

	GENERIC_LATCH_8(config, m_soundlatch);
	GENERIC_LATCH_8(config, m_soundlatch2);

	YM2610(config, m_ym, NEOGEO_YM2610_CLOCK);
	m_ym->irq_handler().set_inputline(m_audiocpu, 0);
	m_ym->add_route(0, "lspeaker", 0.28);
	m_ym->add_route(0, "rspeaker", 0.28);
	m_ym->add_route(1, "lspeaker", 0.98);
	m_ym->add_route(2, "rspeaker", 0.98);
	NEOGEO_BANKED_CART(config, "banked_cart");
}

void neogeo_state::neogeo_arcade(machine_config &config)
{
	neogeo_base(config);
	WATCHDOG_TIMER(config, "watchdog").set_time(attotime::from_ticks(3244030, NEOGEO_MASTER_CLOCK));
	UPD4990A(config, m_upd4990a);
	NVRAM(config, "saveram", nvram_device::DEFAULT_ALL_0);
	NG_MEMCARD(config, "memcard");
}

void neogeo_state::mvs(machine_config &config)
{
	neogeo_arcade(config);
	m_maincpu->set_addrmap(AS_PROGRAM, &neogeo_state::main_map_slot);

	NEOGEO_CTRL_EDGE_CONNECTOR(config, m_edge, neogeo_arc_edge, "joy", false);

	NEOGEO_CONTROL_PORT(config, "ctrl1", neogeo_arc_pin15, nullptr, false);
	NEOGEO_CONTROL_PORT(config, "ctrl2", neogeo_arc_pin15, nullptr, false);
}

void neogeo_state::main_map_noslot(address_map &map)
{
	neogeo_main_map(map);
	map(0x000000,0x00007f).r(FUNC(neogeo_state::banked_vectors_r));
	map(0x000080,0x0fffff).rom();
}

void neogeo_state::neogeo_noslot(machine_config &config)
{
	neogeo_arcade(config); // no slot config (legacy mame)
	m_maincpu->set_addrmap(AS_PROGRAM, &neogeo_state::main_map_noslot);

	//joystick controller
	NEOGEO_CTRL_EDGE_CONNECTOR(config, m_edge, neogeo_arc_edge, "joy", true);

	//no mahjong controller
	NEOGEO_CONTROL_PORT(config, "ctrl1", neogeo_arc_pin15, nullptr, true);
	NEOGEO_CONTROL_PORT(config, "ctrl2", neogeo_arc_pin15, nullptr, true);

	MSLUGX_PROT(config, "mslugx_prot");
	SMA_PROT(config, "sma_prot");
	CMC_PROT(config, "cmc_prot");
	PCM2_PROT(config, "pcm2_prot");
	PVC_PROT(config, "pvc_prot");
	NGBOOTLEG_PROT(config, "bootleg_prot");
	KOF2002_PROT(config, "kof2002_prot");
	FATFURY2_PROT(config, "fatfury2_prot");
	KOF98_PROT(config, "kof98_prot");
	SBP_PROT(config, "sbp_prot");
}

void neogeo_state::neogeo_68kram_map(address_map &map)
{
	main_map_noslot(map);
	map(0x100000,0x1FFFFF).ram();
}

void neogeo_state::neogeo_68kram(machine_config &config)
{
	neoclock_noslot(config);
	m_maincpu->set_addrmap(AS_PROGRAM, &neogeo_state::neogeo_68kram_map);
}

void neogeo_state::neoclock_noslot(machine_config &config)
{
	neogeo_noslot(config);

	m_maincpu->set_clock(48000000 / 2);
}

void neogeo_state::neosd(machine_config &config)
{
	mvs(config);
	m_maincpu->set_addrmap(AS_PROGRAM, &neogeo_state::main_map_noslot);

	// quickload
	quickload_image_device &quickload(QUICKLOAD(config, "quickload", "neo", attotime::from_seconds(1)));
	quickload.set_load_callback(FUNC(neogeo_state::neo_q_cb));
	quickload.set_interface("neo_quik");
	SOFTWARE_LIST(config, "quik_list").set_original("neo_quik");
}

QUICKLOAD_LOAD_MEMBER(neogeo_state::neo_q_cb)
{
	if (image.length() < 0x60000)
	{
		image.seterror(IMAGE_ERROR_INVALIDIMAGE, "File too short");
		printf("File too short\n");
		image.message("File too short");
		return image_init_result::FAIL;
	}

	// check header
	u8 header[0x1000];
	image.fread( &header, 0x1000);

	if ((header[0] == 'N') && (header[1] == 'E') && (header[2] == 'O'))
	{
	}
	else
	{
		image.seterror(IMAGE_ERROR_INVALIDIMAGE, "NEO header missing");
		printf("NEO header missing\n");
		image.message("NEO header missing");
		return image_init_result::FAIL;
	}

	// Get file sizes
	u32 psize = header[4] + header[5]*0x100 + header[6]*0x10000 + header[7]*0x1000000;
	u32 ssize = header[8] + header[9]*0x100 + header[10]*0x10000 + header[11]*0x1000000;
	u32 msize = header[12] + header[13]*0x100 + header[14]*0x10000 + header[15]*0x1000000;
	u32 vsize = header[16] + header[17]*0x100 + header[18]*0x10000 + header[19]*0x1000000;
	u32 v2size = header[20] + header[21]*0x100 + header[22]*0x10000 + header[23]*0x1000000;
	u32 csize = header[24] + header[25]*0x100 + header[26]*0x10000 + header[27]*0x1000000;

	// Make sure file is big enough
	u64 total = 0x1000 + psize + ssize + msize + vsize + v2size + csize;
	if (total > image.length())
	{
		image.seterror(IMAGE_ERROR_INVALIDIMAGE, "File is corrupt");
		printf("File is corrupt.\n");
		image.message("File is corrupt");
		return image_init_result::FAIL;
	}

	// Make sure regions are big enough
	if (psize > cpuregion_size)
	{
		image.seterror(IMAGE_ERROR_INVALIDIMAGE, "CPU region in NEO file is larger than supported");
		printf("CPU size requested (%08X) is greater than available (%08X)\n",psize,cpuregion_size);
		image.message("CPU region in NEO file is larger than supported");
		return image_init_result::FAIL;
	}

	if (ssize > fix_region_size)
	{
		image.seterror(IMAGE_ERROR_INVALIDIMAGE, "FIX region in NEO file is larger than supported");
		printf("FIX size requested (%08X) is greater than available (%08X)\n",ssize,fix_region_size);
		image.message("FIX region in NEO file is larger than supported");
		return image_init_result::FAIL;
	}

	if (vsize > ym_region_size)
	{
		image.seterror(IMAGE_ERROR_INVALIDIMAGE, "ADPCMA region in NEO file is larger than supported");
		printf("ADPCMA size requested (%08X) is greater than available (%08X)\n",vsize,ym_region_size);
		image.message("ADPCMA region in NEO file is larger than supported");
		return image_init_result::FAIL;
	}

	u32 ym2_region_size = memregion("ymsnd.deltat")->bytes();
	if ((v2size > ym2_region_size) || (ym_region_size > ym2_region_size))
	{
		image.seterror(IMAGE_ERROR_INVALIDIMAGE, "ADPCMB region in NEO file is larger than supported");
		printf("ADPCMB size requested (%08X) is greater than available (%08X)\n",v2size,ym2_region_size);
		image.message("ADPCMB region in NEO file is larger than supported");
		return image_init_result::FAIL;
	}

	if (msize > (audio_region_size - 0x10000))
	{
		image.seterror(IMAGE_ERROR_INVALIDIMAGE, "AUDIO region in NEO file is larger than supported");
		printf("AUDIO region (%08X) in NEO file is larger than supported\n",msize);
		image.message("AUDIO region in NEO file is larger than supported");
		return image_init_result::FAIL;
	}

	if (csize > spr_region_size)
	{
		image.seterror(IMAGE_ERROR_INVALIDIMAGE, "SPR region in NEO file is larger than supported");
		printf("SPR size requested (%08X) is greater than available (%08X)\n",csize,spr_region_size);
		image.message("SPR region in NEO file is larger than supported");
		return image_init_result::FAIL;
	}

	// copy the data from the NEO file to the regions
	printf("psize=%X\n",psize);fflush(stdout);
	if (psize)
		image.fread(&cpuregion[0],psize);

	printf("ssize=%X\n",ssize);fflush(stdout);
	if (ssize)
		image.fread(&fix_region[0],ssize);

	printf("msize=%X\n",msize);fflush(stdout);
	if (msize)
	{
		image.fread(&audiocpu_region[0x10000],msize);
		std::copy(&audiocpu_region[0x10000], &audiocpu_region[0x1ffff], &audiocpu_region[0]);
	}

	printf("vsize=%X\n",vsize);fflush(stdout);
	if (vsize)
	{
		image.fread(&ym_region[0],vsize);
		std::copy(&ym_region[0], &ym_region[vsize-1], &memregion("ymsnd.deltat")->base()[0]); // fix totc,rotd
	}

	printf("v2size=%X\n",v2size);fflush(stdout);
	if (v2size)
		image.fread(&memregion("ymsnd.deltat")->base()[0],v2size);

	printf("csize=%X\n",csize);fflush(stdout);
	if (csize)
		image.fread(&spr_region[0],csize);

	// Prepare the system
	printf("Ready to start\n");fflush(stdout);
	init_neogeo();
	m_sprgen->set_sprite_region(m_region_sprites->base(), csize); // fix wh2
	m_sprgen->set_fixed_regions(m_region_fixed->base(), ssize, m_region_fixedbios);
	m_sprgen->optimize_sprite_data(); // fix sprites

	// Fix the 512k text with horrible game-specific stuff
	if (ssize > 0x20000)
	{
		u16 game = cpuregion[0x109] * 256 + cpuregion[0x108];
		// identify kof2000, matrim, svc, kof2003
		if ((game == 0x257) || (game == 0x266) || (game == 0x269) || (game == 0x271))
			m_sprgen->m_fixed_layer_bank_type = 2;
		else
			m_sprgen->m_fixed_layer_bank_type = 1;
	}

	m_audiocpu->reset();
	machine_reset();

	return image_init_result::PASS;
}

u32 neogeo_state::mvs_open7z(std::string zip_name, std::string filename, uint8_t *region_name, u32 region_size)
{
	u32 file_size = 0U;
	util::archive_file::error ziperr = util::archive_file::error::NONE;
	util::archive_file::ptr zip;

	// look into 7z file
	ziperr = util::archive_file::open_7z(zip_name, zip);

	if (ziperr == util::archive_file::error::NONE)
	{
		int found = zip->search(filename, false);

		if (found >= 0)
		{
			ziperr = zip->decompress(&region_name[0], region_size);

			if (ziperr == util::archive_file::error::NONE)
				file_size = zip->current_uncompressed_length();
		}

		zip.reset();
	}

	return file_size;
}

void neogeo_state::multimvs(machine_config &config)
{
	mvs(config);
	m_maincpu->set_addrmap(AS_PROGRAM, &neogeo_state::main_map_noslot);

	// quickload
	quickload_image_device &quickload(QUICKLOAD(config, "quickload", "7z", attotime::from_seconds(1)));
	quickload.set_load_callback(FUNC(neogeo_state::mvs_q_cb));
	quickload.set_interface("mvs_quik");
	//SOFTWARE_LIST(config, "quik_list").set_original("mvs_quik");    // not working
}

QUICKLOAD_LOAD_MEMBER(neogeo_state::mvs_q_cb)
{
	u32 psize = 0U, ssize = 0U, msize = 0U, vsize = 0U, csize = 0U, fsize = 0U;
	std::string fname = "prom", sstr = ".7z", zipname = image.filename();
	// assuming that first .7z is the only occurence
	std::size_t found = zipname.find(sstr);
	if (found != std::string::npos)
	{
		zipname.erase(found+3);
		//printf("%s\n",zipname.c_str());fflush(stdout);

		// The protected sets also have "prom1", but we don't support it.
		psize = mvs_open7z(zipname, fname, &cpuregion[0], cpuregion_size);
	}
	if (!psize)
	{
		image.seterror(IMAGE_ERROR_INVALIDIMAGE, "File is missing or unusable");
		printf("File is missing or unusable\n");
		image.message("File is missing or unusable");
		return image_init_result::FAIL;
	}

	fname = "srom";
	ssize = mvs_open7z(zipname, fname, &fix_region[0], fix_region_size);

	fname = "m1rom";
	msize = mvs_open7z(zipname, fname, &audiocpu_region[0x10000], audio_region_size - 0x10000);
	if (msize)
		std::copy(&audiocpu_region[0x10000], &audiocpu_region[0x1ffff], &audiocpu_region[0]);

	fname = "vroma0";
	vsize = mvs_open7z(zipname, fname, &ym_region[0], ym_region_size);
	if (vsize)
		std::copy(&ym_region[0], &ym_region[ym_region_size-1], &memregion("ymsnd.deltat")->base()[0]);
	// Assume that the ADPCMB rom would be called "vromb0", but no current sets use it.

	fname = "crom0";
	csize = mvs_open7z(zipname, fname, &spr_region[0], spr_region_size);
	if (csize)
		for (u32 i = 0; i < csize; i+=4)
			std::swap(spr_region[i+1], spr_region[i+2]);

	init_neogeo();
	m_sprgen->set_sprite_region(m_region_sprites->base(), csize);
	m_sprgen->set_fixed_regions(m_region_fixed->base(), ssize, m_region_fixedbios);
	m_sprgen->optimize_sprite_data();

	// Fix the 512k text with horrible game-specific stuff
	if (ssize > 0x20000)
	{
		u16 game = cpuregion[0x109] * 256 + cpuregion[0x108];
		// identify kof2000, matrim, svc, kof2003
		if ((game == 0x257) || (game == 0x266) || (game == 0x269) || (game == 0x271))
			m_sprgen->m_fixed_layer_bank_type = 2;
		else
			m_sprgen->m_fixed_layer_bank_type = 1;
	}

	// This file is optionally included, perhaps to signify how to handle protection?
	// The contents are an ascii number, so for now just print it out.
	fname = "fpga";
	unsigned char buffer[8] = {};
	fsize = mvs_open7z(zipname, fname, &buffer[0], 4);
	if (fsize)
		printf("FPGA code = %s\n",buffer);

	m_audiocpu->reset();
	machine_reset();

	return image_init_result::PASS;
}

void neogeo_state::neogno(machine_config &config)
{
	mvs(config);
	m_maincpu->set_addrmap(AS_PROGRAM, &neogeo_state::main_map_noslot);

	// quickload
	quickload_image_device &quickload(QUICKLOAD(config, "quickload", "gno", attotime::from_seconds(1)));
	quickload.set_load_callback(FUNC(neogeo_state::gno_q_cb));
	quickload.set_interface("gno_quik");
	SOFTWARE_LIST(config, "quik_list").set_original("gno_quik");
}

// Problems:
// - Protected sets either won't load, or get address error
// - Need to find out what "c-usage" and "s-usage" are for: (regions 10 and 11).
// - Gfx are broken; need to unscramble FIX and SPR
QUICKLOAD_LOAD_MEMBER(neogeo_state::gno_q_cb)
{
	if (image.length() < 0x60000)
	{
		image.seterror(IMAGE_ERROR_INVALIDIMAGE, "File too short");
		printf("File too short\n");
		image.message("File too short");
		return image_init_result::FAIL;
	}

	// main header
	// 8 bytes = "gnodmpv1"
	// 8 bytes = game name (we don't use)
	// 4 bytes = flags (we don't use)
	// 1 byte  = number of regions
	u8 header[21];
	image.fread( &header, 21);

	if ((header[0] == 'g') && (header[1] == 'n') && (header[2] == 'o') && (header[3] == 'd')
		&& (header[4] == 'm') && (header[5] == 'p') && (header[6] == 'v') && (header[7] == '1'))
	{
	}
	else
	{
		image.seterror(IMAGE_ERROR_INVALIDIMAGE, "GNO header missing");
		printf("GNO header missing\n");
		image.message("GNO header missing");
		return image_init_result::FAIL;
	}

	u32 region_size = 0, csize = 0, ssize = 0, ym2_region_size = 0, offset = 21;

	for (uint8_t regions = 0; regions < header[20]; regions++)
	{
		// Get a region header
		// 4 bytes = size
		// 1 byte  = region code
		// 1 byte  = type (we don't use, 00 assumed)
		// size bytes = data
		u8 region_code = 0, region_type = 0;
		image.fread( &region_size, 4);
		image.fread( &region_code, 1);
		image.fread( &region_type, 1);
		offset += 6;
		// choose region, check size, and write to it
		printf ("region code = %d with size %d at offset %08X\n",region_code,region_size,offset);
		offset += region_size;
		switch (region_code)
		{
			case 1:
				if (region_size > (audio_region_size - 0x10000))
				{
					image.seterror(IMAGE_ERROR_INVALIDIMAGE, "AUDIO region in GNO file is larger than supported");
					printf("AUDIO region (%08X) in GNO file is larger than supported\n",region_size);
					image.message("AUDIO region in GNO file is larger than supported");
					return image_init_result::FAIL;
				}
				else
				{
					image.fread(&audiocpu_region[0x10000], region_size);
					std::copy(&audiocpu_region[0x10000], &audiocpu_region[0x1ffff], &audiocpu_region[0]);
				}
				break;

			case 3:
				if (region_size > ym_region_size)
				{
					image.seterror(IMAGE_ERROR_INVALIDIMAGE, "ADPCMA region in GNO file is larger than supported");
					printf("ADPCMA size requested (%08X) is greater than available (%08X)\n",region_size,ym_region_size);
					image.message("ADPCMA region in GNO file is larger than supported");
					return image_init_result::FAIL;
				}
				else
				{
					image.fread(&ym_region[0], region_size);
					std::copy(&ym_region[0], &ym_region[region_size-1], &memregion("ymsnd.deltat")->base()[0]); // fix totc,rotd
				}
				break;

			case 4:
				ym2_region_size = memregion("ymsnd.deltat")->bytes();
				if ((region_size > ym2_region_size) || (ym_region_size > ym2_region_size))
				{
					image.seterror(IMAGE_ERROR_INVALIDIMAGE, "ADPCMB region in GNO file is larger than supported");
					printf("ADPCMB size requested (%08X) is greater than available (%08X)\n",region_size,ym2_region_size);
					image.message("ADPCMB region in GNO file is larger than supported");
					return image_init_result::FAIL;
				}
				else
				{
					image.fread(&memregion("ymsnd.deltat")->base()[0],region_size);
				}
				break;

			case 6:
				if (region_size > fix_region_size)
				{
					image.seterror(IMAGE_ERROR_INVALIDIMAGE, "FIX region in GNO file is larger than supported");
					printf("FIX size requested (%08X) is greater than available (%08X)\n",region_size,fix_region_size);
					image.message("FIX region in GNO file is larger than supported");
					return image_init_result::FAIL;
				}
				else
				{
					image.fread(&fix_region[0],region_size);
					ssize = region_size;
				}
				break;

			case 8:
				if (region_size > cpuregion_size)
				{
					image.seterror(IMAGE_ERROR_INVALIDIMAGE, "CPU region in GNO file is larger than supported");
					printf("CPU size requested (%08X) is greater than available (%08X)\n",region_size,cpuregion_size);
					image.message("CPU region in GNO file is larger than supported");
					return image_init_result::FAIL;
				}
				else
				{
					image.fread(&cpuregion[0],region_size);
				}
				break;

			case 9:
				if (region_size > spr_region_size)
				{
					image.seterror(IMAGE_ERROR_INVALIDIMAGE, "SPR region in GNO file is larger than supported");
					printf("SPR size requested (%08X) is greater than available (%08X)\n",region_size,spr_region_size);
					image.message("SPR region in GNO file is larger than supported");
					return image_init_result::FAIL;
				}
				else
				{
					image.fread(&spr_region[0],region_size);
					csize = region_size;
				}
				break;

			default:
				// Unsupported regions:
				// 0  - audio bios (not used)
				// 2  - audio encrypted (not used)
				// 5  - fix bios (not used)
				// 7  - 68k bios (not used)
				// 10 - C(SPR) usage
				// 11 - S(FIX) usage
				printf("...ignored\n");
				// jump past this region, point to next one
				image.fseek(region_size, SEEK_CUR);
		}
	}

	// Prepare the system
	printf("Ready to start\n");fflush(stdout);
	init_neogeo();
	m_sprgen->set_sprite_region(m_region_sprites->base(), csize); // fix wh2
	m_sprgen->set_fixed_regions(m_region_fixed->base(), ssize, m_region_fixedbios);
	m_sprgen->optimize_sprite_data(); // fix sprites

	// Fix the 512k text with horrible game-specific stuff
	if (ssize > 0x20000)
	{
		u16 game = cpuregion[0x109] * 256 + cpuregion[0x108];
		// identify kof2000, matrim, svc, kof2003
		if ((game == 0x257) || (game == 0x266) || (game == 0x269) || (game == 0x271))
			m_sprgen->m_fixed_layer_bank_type = 2;
		else
			m_sprgen->m_fixed_layer_bank_type = 1;
	}

	m_audiocpu->reset();
	machine_reset();

	return image_init_result::PASS;
}

/*********************************************** non-carts */

void neogeo_state::install_banked_bios()
{
	m_maincpu->space(AS_PROGRAM).install_read_bank(0xc00000, 0xc1ffff, 0x0e0000, "bankedbios");
	membank("bankedbios")->configure_entries(0, 2, memregion("mainbios")->base(), 0x20000);
	membank("bankedbios")->set_entry(1);
}

INPUT_CHANGED_MEMBER(neogeo_state::select_bios)
{
	membank("bankedbios")->set_entry(newval ? 0 : 1);
}

/* dummy entry for the dummy bios driver */
ROM_START( neogeo )
	NEOGEO_BIOS

	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "audiobios", 0 )
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(94416d67) SHA1(42f9d7ddd6c0931fd64226a60dc73602b2819dcf) )

	ROM_REGION( 0x50000, "audiocpu", 0 )
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(94416d67) SHA1(42f9d7ddd6c0931fd64226a60dc73602b2819dcf) )

	ROM_Y_ZOOM

	ROM_REGION( 0x20000, "fixed", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "fixedbios", 0 )
	ROM_LOAD( "sfix.sfix", 0x000000, 0x20000, CRC(c2ea0cfd) SHA1(fd4a618cdcdbf849374f0a50dd8efe9dbab706c3) )

	ROM_REGION( 0x100000, "sprites", ROMREGION_ERASEFF )
ROM_END

ROM_START( neosd )
	MULTIMVS_BIOS

	ROM_REGION( 0xa00000, "maincpu", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "audiobios", 0 )
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(94416d67) SHA1(42f9d7ddd6c0931fd64226a60dc73602b2819dcf) )

	ROM_REGION( 0x90000, "audiocpu", 0 )
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(94416d67) SHA1(42f9d7ddd6c0931fd64226a60dc73602b2819dcf) )

	ROM_Y_ZOOM

	ROM_REGION( 0x80000, "fixed", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "fixedbios", 0 )
	ROM_LOAD( "sfix.sfix", 0x000000, 0x20000, CRC(c2ea0cfd) SHA1(fd4a618cdcdbf849374f0a50dd8efe9dbab706c3) )

	ROM_REGION( 0x2000000, "ymsnd", ROMREGION_ERASEFF )

	ROM_REGION( 0x2000000, "ymsnd.deltat", ROMREGION_ERASEFF )

	ROM_REGION( 0x4000000, "sprites", ROMREGION_ERASEFF )
ROM_END

ROM_START( multimvs )
	MULTIMVS_BIOS

	ROM_REGION( 0xa00000, "maincpu", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "audiobios", 0 )
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(94416d67) SHA1(42f9d7ddd6c0931fd64226a60dc73602b2819dcf) )

	ROM_REGION( 0x90000, "audiocpu", 0 )
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(94416d67) SHA1(42f9d7ddd6c0931fd64226a60dc73602b2819dcf) )

	ROM_Y_ZOOM

	ROM_REGION( 0x80000, "fixed", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "fixedbios", 0 )
	ROM_LOAD( "sfix.sfix", 0x000000, 0x20000, CRC(c2ea0cfd) SHA1(fd4a618cdcdbf849374f0a50dd8efe9dbab706c3) )

	ROM_REGION( 0x2000000, "ymsnd", ROMREGION_ERASEFF )

	ROM_REGION( 0x2000000, "ymsnd.deltat", ROMREGION_ERASEFF )

	ROM_REGION( 0x4000000, "sprites", ROMREGION_ERASEFF )
ROM_END

ROM_START( neogno )
	MULTIMVS_BIOS

	ROM_REGION( 0x900000, "maincpu", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "audiobios", 0 )
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(94416d67) SHA1(42f9d7ddd6c0931fd64226a60dc73602b2819dcf) )

	ROM_REGION( 0x90000, "audiocpu", 0 )
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(94416d67) SHA1(42f9d7ddd6c0931fd64226a60dc73602b2819dcf) )

	ROM_Y_ZOOM

	ROM_REGION( 0x80000, "fixed", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "fixedbios", 0 )
	ROM_LOAD( "sfix.sfix", 0x000000, 0x20000, CRC(c2ea0cfd) SHA1(fd4a618cdcdbf849374f0a50dd8efe9dbab706c3) )

	ROM_REGION( 0x1000000, "ymsnd:adpcma", ROMREGION_ERASEFF )

	ROM_REGION( 0x1000000, "ymsnd.deltat", ROMREGION_ERASEFF )

	ROM_REGION( 0x4000000, "sprites", ROMREGION_ERASEFF )
ROM_END

/*    YEAR  NAME         PARENT    MACHINE      INPUT           CLASS         INIT    */
GAME( 1990, neogeo,      0,        mvs,         neogeo_6slot,   neogeo_state, init_neogeo,  ROT0, "SNK", "Neo-Geo", MACHINE_IS_BIOS_ROOT | MACHINE_SUPPORTS_SAVE )
GAME( 1990, neosd,       neogeo,   neosd,       neogeo,         neogeo_state, empty_init,   ROT0, "SNK", "Neo-Geo SD .neo Support", MACHINE_IS_BIOS_ROOT | MACHINE_SUPPORTS_SAVE )
GAME( 1990, multimvs,    neogeo,   multimvs,    neogeo,         neogeo_state, empty_init,   ROT0, "SNK", "Neo-Geo MultiMVS Support", MACHINE_IS_BIOS_ROOT | MACHINE_SUPPORTS_SAVE )
GAME( 1990, neogno,      neogeo,   neogno,      neogeo,         neogeo_state, empty_init,   ROT0, "SNK", "Neo-Geo .gno support", MACHINE_IS_BIOS_ROOT | MACHINE_SUPPORTS_SAVE )
