// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    inpttype.ipp

    Array of core-defined input types and default mappings.

***************************************************************************/


/***************************************************************************
    BUILT-IN CORE MAPPINGS
***************************************************************************/

namespace {

#define CORE_INPUT_TYPES_P1 \
		CORE_INPUT_TYPES_BEGIN(p1) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, JOYSTICK_UP,         "@P1 Up @Up",               input_seq(KEYCODE_UP, input_seq::or_code, JOYCODE_Y_UP_SWITCH_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, JOYSTICK_DOWN,       "@P1 Down @Down",           input_seq(KEYCODE_DOWN, input_seq::or_code, JOYCODE_Y_DOWN_SWITCH_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, JOYSTICK_LEFT,       "@P1 Left @Left",           input_seq(KEYCODE_LEFT, input_seq::or_code, JOYCODE_X_LEFT_SWITCH_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, JOYSTICK_RIGHT,      "@P1 Right @Right",         input_seq(KEYCODE_RIGHT, input_seq::or_code, JOYCODE_X_RIGHT_SWITCH_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, BUTTON1,             "@P1 Button 1 @Button1",    input_seq(KEYCODE_A, input_seq::or_code, JOYCODE_BUTTON1_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, BUTTON2,             "@P1 Button 2 @Button2",    input_seq(KEYCODE_S, input_seq::or_code, JOYCODE_BUTTON2_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, BUTTON3,             "@P1 Button 3 @Button3",    input_seq(KEYCODE_D, input_seq::or_code, JOYCODE_BUTTON3_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, BUTTON4,             "@P1 Button 4 @Button4",    input_seq(KEYCODE_Z, input_seq::or_code, JOYCODE_BUTTON4_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, BUTTON5,             "@P1 Button 5 @Button5",    input_seq(JOYCODE_BUTTON5_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, BUTTON6,             "@P1 Button 6 @Button6",    input_seq(JOYCODE_BUTTON6_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, BUTTON7,             "@P1 Button 7 @Button7",    input_seq(JOYCODE_BUTTON7_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, BUTTON8,             "@P1 Button 8 @Button8",    input_seq(JOYCODE_BUTTON8_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, BUTTON9,             "@P1 Button 9 @Button9",    input_seq(JOYCODE_BUTTON9_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, BUTTON10,            "@P1 Button 10 @Button10",  input_seq(JOYCODE_BUTTON10_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, BUTTON11,            "@P1 Button 11 @Button11",  input_seq(JOYCODE_BUTTON11_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, BUTTON12,            "@P1 Button 12 @Button12",  input_seq(JOYCODE_BUTTON12_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, START,               "@P1 Start @Start",         input_seq(KEYCODE_1, input_seq::or_code, JOYCODE_START_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 1, PLAYER1, SELECT,              "@P1 Select @Select",       input_seq(KEYCODE_5, input_seq::or_code, JOYCODE_SELECT_INDEXED(0)) ) \
		CORE_INPUT_TYPES_END()

#define CORE_INPUT_TYPES_P2 \
		CORE_INPUT_TYPES_BEGIN(p2) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, JOYSTICK_UP,         "@P2 Up @Up",               input_seq(KEYCODE_UP, input_seq::or_code, JOYCODE_Y_UP_SWITCH_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, JOYSTICK_DOWN,       "@P2 Down @Down",           input_seq(KEYCODE_DOWN, input_seq::or_code, JOYCODE_Y_DOWN_SWITCH_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, JOYSTICK_LEFT,       "@P2 Left @Left",           input_seq(KEYCODE_LEFT, input_seq::or_code, JOYCODE_X_LEFT_SWITCH_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, JOYSTICK_RIGHT,      "@P2 Right @Right",         input_seq(KEYCODE_RIGHT, input_seq::or_code, JOYCODE_X_RIGHT_SWITCH_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, BUTTON1,             "@P2 Button 1 @Button1",    input_seq(KEYCODE_A, input_seq::or_code, JOYCODE_BUTTON1_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, BUTTON2,             "@P2 Button 2 @Button2",    input_seq(KEYCODE_S, input_seq::or_code, JOYCODE_BUTTON2_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, BUTTON3,             "@P2 Button 3 @Button3",    input_seq(KEYCODE_D, input_seq::or_code, JOYCODE_BUTTON3_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, BUTTON4,             "@P2 Button 4 @Button4",    input_seq(KEYCODE_Z, input_seq::or_code, JOYCODE_BUTTON4_INDEXED(1)) ) \
	    INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, BUTTON5,             "@P2 Button 5 @Button5",    input_seq(JOYCODE_BUTTON5_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, BUTTON6,             "@P2 Button 6 @Button6",    input_seq(JOYCODE_BUTTON6_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, BUTTON7,             "@P2 Button 7 @Button7",    input_seq(JOYCODE_BUTTON7_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, BUTTON8,             "@P2 Button 8 @Button8",    input_seq(JOYCODE_BUTTON8_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, BUTTON9,             "@P2 Button 9 @Button9",    input_seq(JOYCODE_BUTTON9_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, BUTTON10,            "@P2 Button 10 @Button10",  input_seq(JOYCODE_BUTTON10_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, BUTTON11,            "@P2 Button 11 @Button11",  input_seq(JOYCODE_BUTTON11_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, BUTTON12,            "@P2 Button 12 @Button12",  input_seq(JOYCODE_BUTTON12_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, START,               "@P2 Start  @Start",        input_seq(KEYCODE_2, input_seq::or_code, JOYCODE_START_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 2, PLAYER2, SELECT,              "@P2 Select @Select",       input_seq(KEYCODE_6, input_seq::or_code, JOYCODE_SELECT_INDEXED(1)) ) \
		CORE_INPUT_TYPES_END()

#define CORE_INPUT_TYPES_P3 \
		CORE_INPUT_TYPES_BEGIN(p3) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, JOYSTICK_UP,         "@P3 Up @Up",               input_seq(JOYCODE_Y_UP_SWITCH_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, JOYSTICK_DOWN,       "@P3 Down @Down",           input_seq(JOYCODE_Y_DOWN_SWITCH_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, JOYSTICK_LEFT,       "@P3 Left @Left",           input_seq(JOYCODE_X_LEFT_SWITCH_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, JOYSTICK_RIGHT,      "@P3 Right @Right",         input_seq(JOYCODE_X_RIGHT_SWITCH_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, BUTTON1,             "@P3 Button 1 @Button1",    input_seq(JOYCODE_BUTTON1_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, BUTTON2,             "@P3 Button 2 @Button2",    input_seq(JOYCODE_BUTTON2_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, BUTTON3,             "@P3 Button 3 @Button3",    input_seq(JOYCODE_BUTTON3_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, BUTTON4,             "@P3 Button 4 @Button4",    input_seq(JOYCODE_BUTTON4_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, BUTTON5,             "@P3 Button 5 @Button5",    input_seq(JOYCODE_BUTTON5_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, BUTTON6,             "@P3 Button 6 @Button6",    input_seq(JOYCODE_BUTTON6_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, BUTTON7,             "@P3 Button 7 @Button7",    input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, BUTTON8,             "@P3 Button 8 @Button8",    input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, BUTTON9,             "@P3 Button 9 @Button9",    input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, BUTTON10,            "@P3 Button 10 @Button10",  input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, BUTTON11,            "@P3 Button 11 @Button11",  input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, BUTTON12,            "@P3 Button 12 @Button12",  input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, START,               "@P3 Start @Start",         input_seq(JOYCODE_START_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 3, PLAYER3, SELECT,              "@P3 Select @Select",       input_seq(JOYCODE_SELECT_INDEXED(2)) ) \
		CORE_INPUT_TYPES_END()

#define CORE_INPUT_TYPES_P4 \
		CORE_INPUT_TYPES_BEGIN(p4) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, JOYSTICK_UP,         "@P4 Up @Up",               input_seq(JOYCODE_Y_UP_SWITCH_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, JOYSTICK_DOWN,       "@P4 Down @Down",           input_seq(JOYCODE_Y_DOWN_SWITCH_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, JOYSTICK_LEFT,       "@P4 Left @Left",           input_seq(JOYCODE_X_LEFT_SWITCH_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, JOYSTICK_RIGHT,      "@P4 Right @Right",         input_seq(JOYCODE_X_RIGHT_SWITCH_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, BUTTON1,             "@P4 Button 1 @Button1",    input_seq(JOYCODE_BUTTON1_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, BUTTON2,             "@P4 Button 2 @Button2",    input_seq(JOYCODE_BUTTON2_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, BUTTON3,             "@P4 Button 3 @Button3",    input_seq(JOYCODE_BUTTON3_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, BUTTON4,             "@P4 Button 4 @Button4",    input_seq(JOYCODE_BUTTON4_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, BUTTON5,             "@P4 Button 5 @Button5",    input_seq(JOYCODE_BUTTON5_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, BUTTON6,             "@P4 Button 6 @Button6",    input_seq(JOYCODE_BUTTON6_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, BUTTON7,             "@P4 Button 7 @Button7",    input_seq(JOYCODE_BUTTON7_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, BUTTON8,             "@P4 Button 8 @Button8",    input_seq(JOYCODE_BUTTON8_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, BUTTON9,             "@P4 Button 9 @Button9",    input_seq(JOYCODE_BUTTON9_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, BUTTON10,            "@P4 Button 10 @Button10",  input_seq(JOYCODE_BUTTON10_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, BUTTON11,            "@P4 Button 11 @Button11",  input_seq(JOYCODE_BUTTON11_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, BUTTON12,            "@P4 Button 12 @Button12",  input_seq(JOYCODE_BUTTON12_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, START,               "@P4 Start @Start",         input_seq(JOYCODE_START_INDEXED(3)) ) \
		INPUT_PORT_DIGITAL_TYPE( 4, PLAYER4, SELECT,              "@P4 Select @Select",       input_seq(JOYCODE_SELECT_INDEXED(3)) ) \
		CORE_INPUT_TYPES_END()

#define CORE_INPUT_TYPES_START \
		CORE_INPUT_TYPES_BEGIN(start) \
		INPUT_PORT_DIGITAL_TYPE( 0, OTHER,   START1,              "@P1 1 Player Start @Start",    input_seq(KEYCODE_1, input_seq::or_code, JOYCODE_START_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, OTHER,   START2,              "@P2 2 Players Start @Start",   input_seq(KEYCODE_2, input_seq::or_code, JOYCODE_START_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, OTHER,   START3,              "@P3 3 Players Start @Start",   input_seq(JOYCODE_START_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, OTHER,   START4,              "@P4 4 Players Start @Start",   input_seq(JOYCODE_START_INDEXED(3)) ) \
		CORE_INPUT_TYPES_END()

#define CORE_INPUT_TYPES_COIN \
		CORE_INPUT_TYPES_BEGIN(coin) \
		INPUT_PORT_DIGITAL_TYPE( 0, OTHER,   COIN1,               "@P1 Coin 1",             input_seq(KEYCODE_5, input_seq::or_code, JOYCODE_SELECT_INDEXED(0)) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, OTHER,   COIN2,               "@P2 Coin 2",             input_seq(KEYCODE_6, input_seq::or_code, JOYCODE_SELECT_INDEXED(1)) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, OTHER,   COIN3,               "@P3 Coin 3",             input_seq(JOYCODE_SELECT_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, OTHER,   COIN4,               "@P4 Coin 4",             input_seq(JOYCODE_SELECT_INDEXED(3)) ) \
		CORE_INPUT_TYPES_END()

#define CORE_INPUT_TYPES_SERVICE \
		CORE_INPUT_TYPES_BEGIN(service) \
		INPUT_PORT_DIGITAL_TYPE( 0, OTHER,   SERVICE1,            "Service",                input_seq() ) \
		CORE_INPUT_TYPES_END()

#define CORE_INPUT_TYPES_OTHER \
		CORE_INPUT_TYPES_BEGIN(other) \
		INPUT_PORT_DIGITAL_TYPE( 0, OTHER,   SERVICE,             "Service",                input_seq(KEYCODE_0) ) \
		CORE_INPUT_TYPES_END()

#define CORE_INPUT_TYPES_UI \
		CORE_INPUT_TYPES_BEGIN(ui) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_ON_SCREEN_DISPLAY,"On Screen Display",      input_seq(KEYCODE_TILDE, input_seq::not_code, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_DEBUG_BREAK,      "Break in Debugger",      input_seq(KEYCODE_TILDE, input_seq::not_code, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_CONFIGURE,        "Config Menu",            input_seq(KEYCODE_TAB) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_PAUSE,            "Pause",                  input_seq(KEYCODE_P, input_seq::not_code, KEYCODE_LSHIFT, input_seq::not_code, KEYCODE_RSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_PAUSE_SINGLE,     "Pause - Single Step",    input_seq(KEYCODE_P, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_REWIND_SINGLE,    "Rewind - Single Step",   input_seq(KEYCODE_TILDE, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_RESET_MACHINE,    "Reset Machine",          input_seq(KEYCODE_F3, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_SOFT_RESET,       "Soft Reset",             input_seq(KEYCODE_9) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_SHOW_GFX,         "Show Gfx",               input_seq(KEYCODE_F4) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_FRAMESKIP_DEC,    "Frameskip Dec",          input_seq(KEYCODE_F8) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_FRAMESKIP_INC,    "Frameskip Inc",          input_seq(KEYCODE_F9) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_THROTTLE,         "Throttle",               input_seq(KEYCODE_F10) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_FAST_FORWARD,     "Fast Forward",           input_seq(KEYCODE_BACKSPACE) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_SHOW_FPS,         "Show FPS",               input_seq(KEYCODE_F11, input_seq::not_code, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_SNAPSHOT,         "Save Snapshot",          input_seq(KEYCODE_F12, input_seq::not_code, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_TIMECODE,         "Write current timecode", input_seq(KEYCODE_F12, input_seq::not_code, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_RECORD_MNG,       "Record MNG",             input_seq(KEYCODE_F12, KEYCODE_LSHIFT, input_seq::not_code, KEYCODE_LCONTROL) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_RECORD_AVI,       "Record AVI",             input_seq(KEYCODE_F12, KEYCODE_LSHIFT, KEYCODE_LCONTROL) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_TOGGLE_CHEAT,     "Toggle Cheat",           input_seq(KEYCODE_F6, KEYCODE_LSHIFT) )\
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_TOGGLE_CHEAT_CONFIG,"Toggle Cheat Config",  input_seq(KEYCODE_F6, input_seq::not_code, KEYCODE_LSHIFT) )\
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_TOGGLE_AUTOFIRE,  "Toggle Autofire",        input_seq() )\
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_UP,               "UI Up",                  input_seq(KEYCODE_UP, input_seq::or_code, JOYCODE_Y_UP_SWITCH_INDEXED(1), input_seq::or_code, JOYCODE_Y_UP_SWITCH_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_DOWN,             "UI Down",                input_seq(KEYCODE_DOWN, input_seq::or_code, JOYCODE_Y_DOWN_SWITCH_INDEXED(1), input_seq::or_code, JOYCODE_Y_UP_SWITCH_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_LEFT,             "UI Left",                input_seq(KEYCODE_LEFT, input_seq::or_code, JOYCODE_X_LEFT_SWITCH_INDEXED(1), input_seq::or_code, JOYCODE_Y_UP_SWITCH_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_RIGHT,            "UI Right",               input_seq(KEYCODE_RIGHT, input_seq::or_code, JOYCODE_X_RIGHT_SWITCH_INDEXED(1), input_seq::or_code, JOYCODE_Y_UP_SWITCH_INDEXED(2)) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_HOME,             "UI Home",                input_seq(KEYCODE_HOME) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_END,              "UI End",                 input_seq(KEYCODE_END) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_PAGE_UP,          "UI Page Up",             input_seq(KEYCODE_PGUP) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_PAGE_DOWN,        "UI Page Down",           input_seq(KEYCODE_PGDN) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_FOCUS_NEXT,       "UI Focus Next",          input_seq(KEYCODE_TAB, input_seq::not_code, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_FOCUS_PREV,       "UI Focus Previous",      input_seq(KEYCODE_TAB, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_SELECT,           "UI Select",              input_seq(KEYCODE_ENTER, input_seq::or_code, JOYCODE_BUTTON1_INDEXED(1), input_seq::or_code, JOYCODE_BUTTON1_INDEXED(2), input_seq::or_code, KEYCODE_ENTER_PAD) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_CANCEL,           "UI Cancel",              input_seq(KEYCODE_ESC) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_DISPLAY_COMMENT,  "UI Display Comment",     input_seq(KEYCODE_SPACE) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_CLEAR,            "UI Clear",               input_seq(KEYCODE_DEL) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_ZOOM_IN,          "UI Zoom In",             input_seq(KEYCODE_EQUALS) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_ZOOM_OUT,         "UI Zoom Out",            input_seq(KEYCODE_MINUS) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_PREV_GROUP,       "UI Previous Group",      input_seq(KEYCODE_OPENBRACE) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_NEXT_GROUP,       "UI Next Group",          input_seq(KEYCODE_CLOSEBRACE) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_ROTATE,           "UI Rotate",              input_seq(KEYCODE_R) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_SHOW_PROFILER,    "Show Profiler",          input_seq(KEYCODE_F11, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_TOGGLE_UI,        "UI Toggle",              input_seq(KEYCODE_SCRLOCK, input_seq::not_code, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_PASTE,            "UI Paste Text",          input_seq(KEYCODE_SCRLOCK, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_TOGGLE_DEBUG,     "Toggle Debugger",        input_seq(KEYCODE_F5, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_SAVE_STATE,       "Save State",             input_seq(KEYCODE_F7, input_seq::not_code,  KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_LOAD_STATE,       "Load State",             input_seq(KEYCODE_F7, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_TAPE_START,       "UI (First) Tape Start",  input_seq(KEYCODE_F2, input_seq::not_code, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_TAPE_STOP,        "UI (First) Tape Stop",   input_seq(KEYCODE_F2, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_DATS,             "UI External DAT View",   input_seq(KEYCODE_LALT, KEYCODE_D) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_FAVORITES,        "UI Add/Remove favorites",input_seq(KEYCODE_LALT, KEYCODE_F) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_EXPORT,           "UI Export list",         input_seq(KEYCODE_LALT, KEYCODE_E) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_AUDIT_FAST,       "UI Audit Unavailable",   input_seq(KEYCODE_F1, input_seq::not_code, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_AUDIT_ALL,        "UI Audit All",           input_seq(KEYCODE_F1, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_SHOW_TIME,        "Show Current Time",      input_seq(KEYCODE_G, KEYCODE_LSHIFT) ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      UI_SHOW_COMMAND,  	  "Show Current Command",   input_seq(KEYCODE_F5, input_seq::not_code, KEYCODE_LSHIFT) )\
		CORE_INPUT_TYPES_END()
		// Show Current Time - MAMEFX

#define CORE_INPUT_TYPES_OSD \
		CORE_INPUT_TYPES_BEGIN(osd) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      OSD_1,               nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      OSD_2,               nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      OSD_3,               nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      OSD_4,               nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      OSD_5,               nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      OSD_6,               nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      OSD_7,               nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      OSD_8,               nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      OSD_9,               nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      OSD_10,              nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      OSD_11,              nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      OSD_12,              nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      OSD_13,              nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      OSD_14,              nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      OSD_15,              nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, UI,      OSD_16,              nullptr,                     input_seq() ) \
		CORE_INPUT_TYPES_END()

#define CORE_INPUT_TYPES_INVALID \
		CORE_INPUT_TYPES_BEGIN(invalid) \
		INPUT_PORT_DIGITAL_TYPE( 0, INVALID, UNKNOWN,             nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, INVALID, UNUSED,              nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, INVALID, SPECIAL,             nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, INVALID, OTHER,               nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, INVALID, ADJUSTER,            nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, INVALID, DIPSWITCH,           nullptr,                     input_seq() ) \
		INPUT_PORT_DIGITAL_TYPE( 0, INVALID, CONFIG,              nullptr,                     input_seq() ) \
		CORE_INPUT_TYPES_END()


ATTR_COLD void emplace_core_digital_type(std::vector<input_type_entry> &typelist, ioport_type type, ioport_group group, int player, const char *token, const char *name, input_seq seq)
{
	typelist.emplace_back(type, group, (player == 0) ? player : (player - 1), token, name, seq);
}

// instantiate the contruct functions
#define CORE_INPUT_TYPES_BEGIN(_name) \
		ATTR_COLD inline void emplace_core_types_##_name(std::vector<input_type_entry> &typelist) \
		{
#define INPUT_PORT_DIGITAL_TYPE(_player, _group, _type, _name, _seq) \
		emplace_core_digital_type(typelist, IPT_##_type, IPG_##_group, _player, (_player == 0) ? #_type : ("P" #_player "_" #_type), _name, _seq);
#define CORE_INPUT_TYPES_END() \
		}
CORE_INPUT_TYPES_P1
CORE_INPUT_TYPES_P2
CORE_INPUT_TYPES_P3
CORE_INPUT_TYPES_P4
CORE_INPUT_TYPES_START
CORE_INPUT_TYPES_COIN
CORE_INPUT_TYPES_SERVICE
CORE_INPUT_TYPES_OTHER
CORE_INPUT_TYPES_UI
CORE_INPUT_TYPES_OSD
CORE_INPUT_TYPES_INVALID
#undef CORE_INPUT_TYPES_BEGIN
#undef INPUT_PORT_DIGITAL_TYPE
#undef CORE_INPUT_TYPES_END


// make a count function so we don't have to reallocate the vector
#define CORE_INPUT_TYPES_BEGIN(_name)
#define INPUT_PORT_DIGITAL_TYPE(_player, _group, _type, _name, _seq) + 1
#define CORE_INPUT_TYPES_END()
constexpr size_t core_input_types_count()
{
	return 0
			CORE_INPUT_TYPES_P1
			CORE_INPUT_TYPES_P2
			CORE_INPUT_TYPES_P3
			CORE_INPUT_TYPES_P4
			CORE_INPUT_TYPES_START
			CORE_INPUT_TYPES_COIN
			CORE_INPUT_TYPES_SERVICE
            CORE_INPUT_TYPES_OTHER
			CORE_INPUT_TYPES_UI
			CORE_INPUT_TYPES_OSD
			CORE_INPUT_TYPES_INVALID
			;
}
#undef CORE_INPUT_TYPES_BEGIN
#undef INPUT_PORT_DIGITAL_TYPE
#undef CORE_INPUT_TYPES_END


ATTR_COLD inline void emplace_core_types(std::vector<input_type_entry> &typelist)
{
	typelist.reserve(core_input_types_count());

	emplace_core_types_p1(typelist);
	emplace_core_types_p2(typelist);
	emplace_core_types_p3(typelist);
	emplace_core_types_p4(typelist);
	emplace_core_types_start(typelist);
	emplace_core_types_coin(typelist);
	emplace_core_types_service(typelist);
	emplace_core_types_other(typelist);
	emplace_core_types_ui(typelist);
	emplace_core_types_osd(typelist);
	emplace_core_types_invalid(typelist);
}

} // anonymous namespace
