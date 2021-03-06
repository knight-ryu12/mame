// license:GPL-2.0+
// copyright-holders: Felipe Sanches
/***************************************************************************

  HENRY Prot I/II - brazilian document timestamp printers
  http://www.dataponto.com.br/protocoladores-prot1.html

  Driver by Felipe Sanches
  Technical info at https://www.garoa.net.br/wiki/HENRY

  Changelog:

   2014 JUN 13 [Felipe Sanches]:
   * new derivative "CARD I PCB rev.08A"
   * new derivative "CARD II PCB rev.6"
   * fixed LCD rendering (now both lines are displayed properly)
   * inverted logic of the inputs so that now we can navigate the menu

   2014 JAN 03 [Felipe Sanches]:
   * Initial driver skeleton
   * Address lines bitswaping
   * LCD works. We can see boot messages.
   * Inputs are not working correctly

TO-DO list:

======= hprotr8a ===========
Loops during boot displaying an error message related to low power supply voltage.
We need to emulate the ADM695AN chip (Microprocessor Supervisory Circuits) in order to properly boot the device.

======= hprot2r6 ===========
There are unhandled memory writes at 0xE000 and 0xA000

LCD commands are sent, but nothing shows up on screen.
The commands sent are supposed to display the message:

"*Pouca  Energia*" (LCD cmds range: 80-8F)  (cmds logged but not displayed on screen)
"*  no Sistema  *" (LCD cmds range: C0-CF)  (cmds logged but not displayed on screen)

which means something like "too little energy for the system to operate".
We need to emulate the ADM695AN chip (Microprocessor Supervisory Circuits) in order to properly boot the device.

Infinite loop is reached at address 0x7699
======= hprot1 ===========

  There seems to be an eeprom or a realtime clock placed at U2 (DIP8):
  pin1 -> 8031 pin 14 (T0: Timer 0 external input)
  pin2 -> crystal at X2 (labeled 32.768)
  pin3 -> ?
  pin4 -> GND
  pin5 -> ?
  pin6 -> 8031 pin 5 (Port 1 bit 4)
  pin7 -> 8031 pin 4 (Port 1 bit 3)
  pin8 -> VCC

****************************************************************************/

#include "emu.h"
#include "cpu/mcs51/mcs51.h"
#include "sound/spkrdev.h"
#include "video/hd44780.h"
#include "emupal.h"
#include "rendlay.h"
#include "screen.h"
#include "speaker.h"

class hprot1_state : public driver_device
{
public:
	hprot1_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_lcdc(*this, "hd44780")
	{ }

	void hprotr8a(machine_config &config);
	void hprot2r6(machine_config &config);
	void hprot1(machine_config &config);

	void init_hprot1();

private:
	DECLARE_WRITE8_MEMBER(henry_p1_w);
	DECLARE_WRITE8_MEMBER(henry_p3_w);
	DECLARE_PALETTE_INIT(hprot1);
	HD44780_PIXEL_UPDATE(hprot1_pixel_update);
	void i80c31_io(address_map &map);
	void i80c31_prg(address_map &map);

	virtual void machine_start() override;
	virtual void machine_reset() override;
	required_device<cpu_device> m_maincpu;
	required_device<hd44780_device> m_lcdc;
};

#define LOG_IO_PORTS 0

void hprot1_state::i80c31_prg(address_map &map)
{
	map(0x0000, 0xffff).rom();
}

void hprot1_state::init_hprot1()
{
	uint8_t *ROM = memregion("maincpu")->base();
	uint8_t bitswapped_ROM[0x10000];

	for (int i = 0x0000; i < 0x10000; i++)
		bitswapped_ROM[i] = ROM[i];

	for (int i = 0x0000; i < 0x10000; i++)
		ROM[bitswap<16>(i, 15, 14, 13, 12, 11, 10, 9, 8, 3, 2, 1, 0, 4, 5, 6, 7)] = bitswapped_ROM[i];
}

//A4 = display RS
//A5 = display R/W
//(A11 == 0) && (A10 == 0) => display CS
//(A14 == 1) && (A15 == 1) => enable signal for the mux that selects peripherals
//11?? 00?? ??00 ???? write data
//11?? 00?? ??01 ???? write command
//11?? 00?? ??10 ???? read data
//11?? 00?? ??11 ???? read command
//mirror=0x33cf

//write: 0xc400 => U12 (74373 - possibly for the dot matrix printhead ?)
//write: 0xc800 => U11 (74373 - possibly for the dot matrix printhead ?)
//read:  0xc020 => display
//write: 0xc000 => display
//write: 0xc010 => display

//P1.4 => WhatchDog Input (after timeout resets CPU)

void hprot1_state::i80c31_io(address_map &map)
{
	map(0x0000, 0x7fff).ram();
/*TODO: verify the mirror mask value for the HD44780 device */
	map(0xc000, 0xc000).mirror(0x13cf).w(m_lcdc, FUNC(hd44780_device::control_write));
	map(0xc010, 0xc010).mirror(0x13cf).w(m_lcdc, FUNC(hd44780_device::data_write));
	map(0xc020, 0xc020).mirror(0x13cf).r(m_lcdc, FUNC(hd44780_device::control_read));
	map(0xc030, 0xc030).mirror(0x13cf).r(m_lcdc, FUNC(hd44780_device::data_read));
/*TODO: attach the watchdog/brownout reset device:
    map(0xe000,0xe0??).mirror(?).r("adm965an", FUNC(adm965an_device::data_read)); */
}

static INPUT_PORTS_START( hprot1 )
/* FIXME: I am still unsure whether all these inputs are Active High or Active Low: */
	PORT_START("inputs")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Upper Black Button") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Lower Black Button") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Blue Button") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("Paper Detector") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("XMIN Endstop") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( hprot_jumpers )
/*
There is a set of 3 jumpers that switch the communications circuitry between a
RS232 chip (U8: MAX232AN) and a differential bus transceiver (U7: SN65176BP)

It seems that all three jumpers must select the same configuration:
eighter differential bus or RS232.
And I don't see yet how this could affect the emulation of the device, so, for now, I'll
simply leave this note here but not actually include this details in the driver code.

    PORT_START("jumpers")
    PORT_DIPNAME( 0x01, 0x01, "TX")
    PORT_DIPSETTING(    0x01, "differential bus")
    PORT_DIPSETTING(    0x00, "RS232")
    PORT_DIPNAME( 0x02, 0x02, "RX")
    PORT_DIPSETTING(    0x02, "differential bus")
    PORT_DIPSETTING(    0x00, "RS232")
    PORT_DIPNAME( 0x04, 0x04, "CPU-TX")
    PORT_DIPSETTING(    0x04, "differential bus")
    PORT_DIPSETTING(    0x00, "RS232")
*/
INPUT_PORTS_END

/* TODO: meanings for the jumpers may be different among machines,
 so we may have to have individual declarations for each board. */
static INPUT_PORTS_START( hprot2r6 )
	PORT_INCLUDE(hprot1)
	PORT_INCLUDE(hprot_jumpers)
INPUT_PORTS_END

static INPUT_PORTS_START( hprotr8a )
	PORT_INCLUDE(hprot1)
	PORT_INCLUDE(hprot_jumpers)
INPUT_PORTS_END

void hprot1_state::machine_start()
{
}

void hprot1_state::machine_reset()
{
}

WRITE8_MEMBER(hprot1_state::henry_p1_w)
{
	if (LOG_IO_PORTS && data != 0xFF && data != 0xEF)
		logerror("Write to P1: %02X\n", data);
}

WRITE8_MEMBER(hprot1_state::henry_p3_w)
{
	if (LOG_IO_PORTS)
		logerror("Write to P3: %02X\n", data);
}

PALETTE_INIT_MEMBER(hprot1_state, hprot1)
{
	palette.set_pen_color(0, rgb_t(138, 146, 148));
	palette.set_pen_color(1, rgb_t(92, 83, 88));
}

static const gfx_layout henry_prot_charlayout =
{
	5, 8,                   /* 5 x 8 characters */
	256,                    /* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes */
	{ 3, 4, 5, 6, 7},
	{ 0, 8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8                     /* 8 bytes */
};

static GFXDECODE_START( gfx_hprot1 )
	GFXDECODE_ENTRY( "hd44780:cgrom", 0x0000, henry_prot_charlayout, 0, 1 )
GFXDECODE_END

HD44780_PIXEL_UPDATE(hprot1_state::hprot1_pixel_update)
{
	if ( pos < 16 && line==0 )
	{
		bitmap.pix16(y, pos*6 + x) = state;
	}

	if ( pos >= 64 && pos < 80 && line==0 )
	{
		bitmap.pix16(y+9,(pos-64)*6 + x) = state;
	}
}

MACHINE_CONFIG_START(hprot1_state::hprot1)
	/* basic machine hardware */
	MCFG_DEVICE_ADD("maincpu", I80C31, XTAL(10'000'000))
	MCFG_DEVICE_PROGRAM_MAP(i80c31_prg)
	MCFG_DEVICE_IO_MAP(i80c31_io)
	MCFG_MCS51_PORT_P1_IN_CB(IOPORT("inputs"))
	MCFG_MCS51_PORT_P1_OUT_CB(WRITE8(*this, hprot1_state, henry_p1_w))
	MCFG_MCS51_PORT_P3_OUT_CB(WRITE8(*this, hprot1_state, henry_p3_w))

	/* video hardware */
	MCFG_SCREEN_ADD("screen", LCD)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_UPDATE_DEVICE("hd44780", hd44780_device, screen_update)
	MCFG_SCREEN_SIZE(6*16, 9*2)
	MCFG_SCREEN_VISIBLE_AREA(0, 6*16-1, 0, 9*2-1)
	MCFG_SCREEN_PALETTE("palette")

	config.set_default_layout(layout_lcd);
	MCFG_PALETTE_ADD("palette", 2)
	MCFG_PALETTE_INIT_OWNER(hprot1_state, hprot1)
	MCFG_DEVICE_ADD("gfxdecode", GFXDECODE, "palette", gfx_hprot1)

	MCFG_HD44780_ADD("hd44780")
	MCFG_HD44780_LCD_SIZE(2, 16)
	MCFG_HD44780_PIXEL_UPDATE_CB(hprot1_state,hprot1_pixel_update)

	/* TODO: figure out which RTC chip is in use. */

	/* TODO: emulate the ADM695AN chip (watchdog/brownout reset)*/
MACHINE_CONFIG_END

MACHINE_CONFIG_START(hprot1_state::hprotr8a)
	hprot1(config);
	MCFG_DEVICE_REPLACE("maincpu", I80C31, 11059200) // value of X1 cristal on the PCB
	MCFG_DEVICE_PROGRAM_MAP(i80c31_prg)
	MCFG_DEVICE_IO_MAP(i80c31_io)

	/* sound hardware */
	SPEAKER(config, "mono").front_center();
	MCFG_DEVICE_ADD("speaker", SPEAKER_SOUND)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* TODO: add an RS232 interface (emulate MAX232N chip)
	(the board has GND/VCC/RX/TX pins available in a connector) */

	/* TODO: add an I2C interface (the board has GND/VCC/SDA/SCL pins available in a connector) */
MACHINE_CONFIG_END

MACHINE_CONFIG_START(hprot1_state::hprot2r6)
	hprot1(config);
	MCFG_DEVICE_REPLACE("maincpu", I80C31, 11059200) // value of X1 cristal on the PCB
	MCFG_DEVICE_PROGRAM_MAP(i80c31_prg)
	MCFG_DEVICE_IO_MAP(i80c31_io)

	/* sound hardware */
	SPEAKER(config, "mono").front_center();
	MCFG_DEVICE_ADD("speaker", SPEAKER_SOUND)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* TODO: add an RS232 interface (emulate MAX232N chip) */
MACHINE_CONFIG_END

ROM_START( hprot1 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "henry_prot1_rev1_v19.bin",  0x00000, 0x10000, CRC(dd7787fd) SHA1(61a37dd406b3440d568bd6da75a9fdc8a0f0e1e3) )
ROM_END

ROM_START( hprotr8a )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "hprot_card1_rev08a.u2",  0x00000, 0x10000, CRC(e827480f) SHA1(bd53e6cce9a0832ca01f1a485ddaab43c0baa136) )
ROM_END

ROM_START( hprot2r6 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "hprot_card2_rev6.u2",  0x00000, 0x10000, CRC(791f2425) SHA1(70af8911a27921cac6d98a5cd07602a7f59c2848) )
ROM_END

/*    YEAR  NAME      PARENT  COMPAT  MACHINE   INPUT     CLASS         INIT         COMPANY  FULLNAME                       FLAGS */
COMP( 2002, hprot1,   0,      0,      hprot1,   hprot1,   hprot1_state, init_hprot1, "HENRY", "Henry Prot I v19 (REV.1)",    MACHINE_IMPERFECT_GRAPHICS | MACHINE_NO_SOUND)
/* fw version: "R19"        Release date: February 1st, 2002.   */

COMP( 2006, hprotr8a, hprot1, 0,      hprotr8a, hprotr8a, hprot1_state, init_hprot1, "HENRY", "Henry Prot CARD I (REV.08A)", MACHINE_NOT_WORKING)
/* fw version: "V6.5QI I"   Release date: September 18th, 2006. */

COMP( 2003, hprot2r6, hprot1, 0,      hprot2r6, hprot2r6, hprot1_state, init_hprot1, "HENRY", "Henry Prot CARD II (REV.6)",  MACHINE_NOT_WORKING)
/* fw version: "V5.8CF II"  Release date: June 23rd, 2003.      */
