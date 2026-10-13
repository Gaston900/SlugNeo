// license:BSD-3-Clause
// copyright-holders:Chris Kirmse, Mike Haaland, Ren챕 Single, Mamesick

#include "winui.h"

/***************************************************************************
    Internal structures
 ***************************************************************************/
struct DriversInfo
{
	int screenCount;
	bool isClone;
	bool isBroken;
	bool isHarddisk;
	bool hasOptionalBIOS;
	bool isVector;
	bool usesRoms;
	bool usesSamples;
	bool usesTrackball;
	bool usesLightGun;
	bool supportsSaveState;
	bool isVertical;
	bool isImperfect;
	bool isMechanical;
	bool isBIOS;
};

static std::vector<DriversInfo>	drivers_info;

enum
{
	DRIVER_CACHE_SCREEN     = 0x000F,
	DRIVER_CACHE_ROMS       = 0x0010,
	DRIVER_CACHE_CLONE      = 0x0020,
	DRIVER_CACHE_BIOS       = 0x0040,
	DRIVER_CACHE_HARDDISK   = 0x0080,
	DRIVER_CACHE_SAMPLES    = 0x0100,
	DRIVER_CACHE_VECTOR     = 0x0200,
	DRIVER_CACHE_LIGHTGUN   = 0x0400,
	DRIVER_CACHE_TRACKBALL  = 0x0800,
};

void ErrorMessageBox(const char *fmt, ...)
{
	char buf[1024];
	va_list ptr;

	va_start(ptr, fmt);
	vsnprintf(buf, WINUI_ARRAY_LENGTH(buf), fmt, ptr);
	winui_message_box_utf8(GetMainWindow(), buf, MAMEUINAME, MB_ICONERROR | MB_OK);
	va_end(ptr);
}

/* for debugging */
void dprintf(const char *fmt, ...)
{
	char buf[1024];
	va_list ptr;
	va_start(ptr, fmt);
	vsnprintf(buf, WINUI_ARRAY_LENGTH(buf), fmt, ptr);
	winui_output_debug_string_utf8(buf);
	va_end(ptr);
}

void ShellExecuteCommon(HWND hWnd, const char *cName)
{
	const char *msg = NULL;
	wchar_t *tName = win_wstring_from_utf8(cName);

	if(!tName)
		return;

	HINSTANCE hErr = ShellExecute(hWnd, NULL, tName, NULL, NULL, SW_SHOWNORMAL);

	if ((uintptr_t)hErr > 32)
	{
		free(tName);
		return;
	}

	switch((uintptr_t)hErr)
	{
	case 0:
		msg = "The Operating System is out of memory or resources.";
		break;

	case ERROR_FILE_NOT_FOUND:
		msg = "The specified file was not found.";
		break;

	case SE_ERR_NOASSOC :
		msg = "There is no application associated with the given filename extension.";
		break;

	case SE_ERR_OOM :
		msg = "There was not enough memory to complete the operation.";
		break;

	case SE_ERR_PNF :
		msg = "The specified path was not found.";
		break;

	case SE_ERR_SHARE :
		msg = "A sharing violation occurred.";
		break;

	default:
		msg = "Unknown error.";
	}

	ErrorMessageBox("%s\r\nPath: '%s'", msg, cName);
	free(tName);
}

char * MyStrStrI(const char* pFirst, const char* pSrch)
{
	char *cp = (char*)pFirst;

	while (*cp)
	{
		char *s1 = cp;
		char *s2 = (char*)pSrch;

		while (*s1 && *s2 && !core_strnicmp(s1, s2, 1))
			s1++, s2++;

		if (!*s2)
			return cp;

		cp++;
	}

	return NULL;
}

char * ConvertToWindowsNewlines(const char *source)
{
	static char buf[2048 * 2048];

	memset(&buf, 0, sizeof(buf));
	char *dest = buf;

	while (*source != 0)
	{
		if (*source == '\n')
		{
			*dest++ = '\r';
			*dest++ = '\n';
		}
		else
			*dest++ = *source;

		source++;
	}

	*dest = 0;
	return buf;
}

const char * GetVersionString(void)
{
	return MAMEUIFX_VERSION;
}

const char * GetDriverGameTitle(int nIndex)
{
	return driver_list::driver(nIndex).type.fullname();
}

const char * GetDriverGameName(int nIndex)
{
	return driver_list::driver(nIndex).name;
}

int GetDriverIsClone(int nIndex)
{
	return driver_list::clone(nIndex);
}

//const char * GetDriverCloneName(int nIndex)
//{
//	return driver_list::clone(nIndex).name;
//}

const char * GetDriverGameManufacturer(int nIndex)
{
	return driver_list::driver(nIndex).manufacturer;
}

const char * GetDriverGameYear(int nIndex)
{
	return driver_list::driver(nIndex).year;
}

const char * GetDriverFileName(int nIndex)
{
	static char tmp[40];

	std::string driver = core_filename_extract_base(driver_list::driver(nIndex).type.source(), false);
	strcpy(tmp, driver.c_str());
	return tmp;
}

int GetGameNameIndex(const char *name)
{
	return driver_list::find(name);
}

static int NumberOfScreens(const machine_config &config)
{
	screen_device_iterator scriter(config.root_device());
	return scriter.count();
}

static bool isDriverVector(const machine_config &config)
{
	const screen_device *screen = screen_device_iterator(config.root_device()).first();

	if (screen != nullptr) 
	{
		if (SCREEN_TYPE_VECTOR == screen->screen_type())
			return true;
	}

	return false;
}

static void SetDriversInfo(void)
{
	uint32_t cache;
	int total = driver_list::total();
	struct DriversInfo *gameinfo = NULL;

	for (int ndriver = 0; ndriver < total; ndriver++)
	{
		gameinfo = &drivers_info[ndriver];
		cache = gameinfo->screenCount & DRIVER_CACHE_SCREEN;

		if (gameinfo->isClone)
			cache += DRIVER_CACHE_CLONE;

		if (gameinfo->isHarddisk)
			cache += DRIVER_CACHE_HARDDISK;

		if (gameinfo->hasOptionalBIOS)
			cache += DRIVER_CACHE_BIOS;

		if (gameinfo->isVector)
			cache += DRIVER_CACHE_VECTOR;

		if (gameinfo->usesRoms)
			cache += DRIVER_CACHE_ROMS;

		if (gameinfo->usesSamples)
			cache += DRIVER_CACHE_SAMPLES;

		if (gameinfo->usesTrackball)
			cache += DRIVER_CACHE_TRACKBALL;

		if (gameinfo->usesLightGun)
			cache += DRIVER_CACHE_LIGHTGUN;

		SetDriverCache(ndriver, cache);
	}
}

static void InitDriversInfo(void)
{
	int total = driver_list::total();
	const game_driver *gamedrv = NULL;
	struct DriversInfo *gameinfo = NULL;

	for (int ndriver = 0; ndriver < total; ndriver++)
	{
		uint32_t cache = GetDriverCacheLower(ndriver);
		gamedrv = &driver_list::driver(ndriver);
		gameinfo = &drivers_info[ndriver];
		machine_config config(*gamedrv, MameUIGlobal());
		bool const have_parent(strcmp(gamedrv->parent, "0"));
		auto const parent_idx(have_parent ? driver_list::find(gamedrv->parent) : -1);
		gameinfo->isClone = ( !have_parent || (0 > parent_idx) || BIT(GetDriverCacheLower(parent_idx),9)) ? false : true;
		//gameinfo->isClone = (GetParentRomSetIndex(gamedrv) != -1);
		gameinfo->isBroken = (cache & 0x4040) ? true : false;  // (MACHINE_NOT_WORKING | MACHINE_MECHANICAL)
		gameinfo->isImperfect = (cache & 0x3fa000) ? true : false;  // MACHINE_INCOMPLETE | NO_SOUND_HW | (IMPERFECT|UNEMULATED) | (PALETTE|GRAPHICS|SOUND)
		gameinfo->supportsSaveState = BIT(cache, 7) ? false : true;  //MACHINE_SUPPORTS_SAVE
		gameinfo->isVertical = BIT(cache, 2);  //ORIENTATION_SWAP_XY
		gameinfo->isMechanical = BIT(cache, 14);  //MACHINE_MECHANICAL
		gameinfo->isBIOS = BIT(cache, 9);  //MACHINE_IS_BIOS_ROOT
		gameinfo->screenCount = NumberOfScreens(config);
		gameinfo->isVector = isDriverVector(config);
		gameinfo->isHarddisk = false;
		gameinfo->usesRoms = false;
		gameinfo->hasOptionalBIOS = false;
		gameinfo->usesTrackball = false;
		gameinfo->usesLightGun = false;

		for (device_t &device : device_iterator(config.root_device()))
		{
			for (const rom_entry *region = rom_first_region(device); region; region = rom_next_region(region))
			{
				for (const rom_entry *rom = rom_first_file(region); rom; rom = rom_next_file(rom))
				{
					if (ROMREGION_ISDISKDATA(region))
						gameinfo->isHarddisk = true;

					gameinfo->usesRoms = true;
				}
			}
		}

		if (gamedrv->rom)
		{
			auto rom_entries = rom_build_entries(gamedrv->rom);

			for (const rom_entry *rom = rom_entries.data(); !ROMENTRY_ISEND(rom); rom++)
			{
				if (ROMENTRY_ISSYSTEM_BIOS(rom))
					gameinfo->hasOptionalBIOS = true;
			}
		}

		samples_device_iterator sampiter(config.root_device());
		gameinfo->usesSamples = sampiter.first() ? true : false;

		if (gamedrv->ipt)
		{
			ioport_list portlist;
			std::string errors;

			for (device_t &cfg : device_iterator(config.root_device()))
				if (cfg.input_ports())
					portlist.append(cfg, errors);

			for (auto &port : portlist)
			{
				for (ioport_field &field : port.second->fields())
				{
					UINT type = field.type();

					if (type == IPT_END)
						break;

					if (type == IPT_DIAL || type == IPT_PADDLE || type == IPT_TRACKBALL_X || type == IPT_TRACKBALL_Y)
						gameinfo->usesTrackball = true;

					if (type == IPT_LIGHTGUN_X || type == IPT_LIGHTGUN_Y || type == IPT_AD_STICK_X || type == IPT_AD_STICK_Y)
						gameinfo->usesLightGun = true;
				}
			}
		}
	}

	SetDriversInfo();
}

static void InitDriversCache(void)
{
	if (RequiredDriverCache())
	{
		InitDriversInfo();
		return;
	}

	uint32_t cache_lower, cache_upper;
	int total = driver_list::total();
	struct DriversInfo *gameinfo = NULL;

	for (int ndriver = 0; ndriver < total; ndriver++)
	{
		gameinfo = &drivers_info[ndriver];
		cache_lower = GetDriverCacheLower(ndriver);
		cache_upper = GetDriverCacheUpper(ndriver);

		gameinfo->isBroken          =  (cache_lower & 0x4040) ? true : false; //MACHINE_NOT_WORKING | MACHINE_MECHANICAL
		gameinfo->supportsSaveState =  BIT(cache_lower, 7) ? false : true;  //MACHINE_SUPPORTS_SAVE
		gameinfo->isVertical        =  BIT(cache_lower, 2) ? true : false;  //ORIENTATION_XY
		gameinfo->screenCount       =   cache_upper & DRIVER_CACHE_SCREEN;
		gameinfo->isClone           = ((cache_upper & DRIVER_CACHE_CLONE) != 0);
		gameinfo->isHarddisk        = ((cache_upper & DRIVER_CACHE_HARDDISK) != 0);
		gameinfo->hasOptionalBIOS   = ((cache_upper & DRIVER_CACHE_BIOS) != 0);
		gameinfo->isVector          = ((cache_upper & DRIVER_CACHE_VECTOR) != 0);
		gameinfo->usesRoms          = ((cache_upper & DRIVER_CACHE_ROMS) != 0);
		gameinfo->usesSamples       = ((cache_upper & DRIVER_CACHE_SAMPLES) != 0);
		gameinfo->usesTrackball     = ((cache_upper & DRIVER_CACHE_TRACKBALL) != 0);
		gameinfo->usesLightGun      = ((cache_upper & DRIVER_CACHE_LIGHTGUN) != 0);
		gameinfo->isImperfect       =  (cache_lower & 0x3fa000) ? true : false;
		gameinfo->isMechanical      =  BIT(cache_lower, 14);
		gameinfo->isBIOS            =  BIT(cache_lower, 9);
	}
}

static struct DriversInfo* GetDriversInfo(int driver_index)
{
	static bool bFirst = true;

	if (bFirst)
	{
		bFirst = false;
		drivers_info.clear();
		drivers_info.reserve(driver_list::total());
		InitDriversCache();
	}

	return &drivers_info[driver_index];
}

bool DriverIsClone(int driver_index)
{
	return GetDriversInfo(driver_index)->isClone;
}

bool DriverIsBroken(int driver_index)
{
	return GetDriversInfo(driver_index)->isBroken;
}

bool DriverIsHarddisk(int driver_index)
{
	return GetDriversInfo(driver_index)->isHarddisk;
}

bool DriverIsBios(int driver_index)
{
	return GetDriversInfo(driver_index)->isBIOS;
}

bool DriverIsMechanical(int driver_index)
{
	return GetDriversInfo(driver_index)->isMechanical;
}

bool DriverHasOptionalBIOS(int driver_index)
{
	return GetDriversInfo(driver_index)->hasOptionalBIOS;
}

int DriverNumScreens(int driver_index)
{
	return GetDriversInfo(driver_index)->screenCount;
}

bool DriverIsVector(int driver_index)
{
	return GetDriversInfo(driver_index)->isVector;
}

bool DriverUsesRoms(int driver_index)
{
	return GetDriversInfo(driver_index)->usesRoms;
}

bool DriverUsesSamples(int driver_index)
{
	return GetDriversInfo(driver_index)->usesSamples;
}

bool DriverUsesTrackball(int driver_index)
{
	return GetDriversInfo(driver_index)->usesTrackball;
}

bool DriverUsesLightGun(int driver_index)
{
	return GetDriversInfo(driver_index)->usesLightGun;
}

bool DriverSupportsSaveState(int driver_index)
{
	return GetDriversInfo(driver_index)->supportsSaveState;
}

bool DriverIsVertical(int driver_index)
{
	return GetDriversInfo(driver_index)->isVertical;
}

bool DriverIsImperfect(int driver_index)
{
	return GetDriversInfo(driver_index)->isImperfect;
}

//============================================================
//  win_wstring_from_utf8
//============================================================

wchar_t *win_wstring_from_utf8(const char *utf8string)
{
	// convert MAME string (UTF-8) to UTF-16
	int char_count = MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, nullptr, 0);
	wchar_t *result = (wchar_t *)malloc(char_count * sizeof(*result));

	if (result != nullptr)
		MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, result, char_count);

	return result;
}


//============================================================
//  win_utf8_from_wstring
//============================================================

char *win_utf8_from_wstring(const wchar_t *wstring)
{
	// convert UTF-16 to MAME string (UTF-8)
	int char_count = WideCharToMultiByte(CP_UTF8, 0, wstring, -1, nullptr, 0, nullptr, nullptr);
	char *result = (char *)malloc(char_count * sizeof(*result));

	if (result != nullptr)
		WideCharToMultiByte(CP_UTF8, 0, wstring, -1, result, char_count, nullptr, nullptr);

	return result;
}

//============================================================
//  winui_output_debug_string_utf8
//============================================================

void winui_output_debug_string_utf8(const char *string)
{
	wchar_t *t_string = win_wstring_from_utf8(string);

	if (t_string != NULL)
	{
		OutputDebugString(t_string);
		free(t_string);
	}
}

//============================================================
//  winui_message_box_utf8
//============================================================

int winui_message_box_utf8(HWND hWnd, const char *text, const char *caption, UINT type)
{
	int result = IDCANCEL;
	wchar_t *t_text = win_wstring_from_utf8(text);
	wchar_t *t_caption = win_wstring_from_utf8(caption);

	if (!t_text)
		return result;

	if (!t_caption)
	{
		free(t_text);
		return result;
	}

	result = MessageBox(hWnd, t_text, t_caption, type);
	free(t_text);
	free(t_caption);
	return result;
}

//============================================================
//  winui_set_window_text_utf8
//============================================================

bool winui_set_window_text_utf8(HWND hWnd, const char *text)
{
	bool result = false;
	wchar_t *t_text = win_wstring_from_utf8(text);

	if (!t_text)
		return result;

	result = SetWindowText(hWnd, t_text);
	free(t_text);
	return result;
}

//============================================================
//  winui_get_window_text_utf8
//============================================================

int winui_get_window_text_utf8(HWND hWnd, char *buffer, size_t buffer_size)
{
	int result = 0;
	wchar_t t_buffer[256];

	t_buffer[0] = '\0';
	// invoke the core Win32 API
	GetWindowText(hWnd, t_buffer, ARRAY_LENGTH(t_buffer));
	char *utf8_buffer = win_utf8_from_wstring(t_buffer);

	if (!utf8_buffer)
		return result;

	result = snprintf(buffer, buffer_size, "%s", utf8_buffer);
	free(utf8_buffer);
	return result;
}

//============================================================
//  winui_extract_icon_utf8
//============================================================

HICON winui_extract_icon_utf8(HINSTANCE inst, const char* exefilename, UINT iconindex)
{
	wchar_t *t_exefilename = win_wstring_from_utf8(exefilename);

	if (!t_exefilename)
		return NULL;

	HICON icon = ExtractIcon(inst, t_exefilename, iconindex);
	free(t_exefilename);
	return icon;
}

//============================================================
//  winui_find_first_file_utf8
//============================================================

HANDLE winui_find_first_file_utf8(const char* filename, WIN32_FIND_DATA *findfiledata)
{
	wchar_t *t_filename = win_wstring_from_utf8(filename);

	if (!t_filename)
		return NULL;

	HANDLE result = FindFirstFile(t_filename, findfiledata);
	free(t_filename);
	return result;
}

//============================================================
//  winui_move_file_utf8
//============================================================

bool winui_move_file_utf8(const char* existingfilename, const char* newfilename)
{
	bool result = false;

	wchar_t *t_existingfilename = win_wstring_from_utf8(existingfilename);

	if (!t_existingfilename)
		return result;

	wchar_t *t_newfilename = win_wstring_from_utf8(newfilename);

	if (!t_newfilename) 
	{
		free(t_existingfilename);
		return result;
	}

	result = MoveFile(t_existingfilename, t_newfilename);
	free(t_newfilename);
	free(t_existingfilename);
	return result;
}

void CenterWindow(HWND hWnd)
{
	RECT rcCenter, rcWnd;

	// 부모윈도 획득(없으면 NULL이 리턴됨)
	HWND hWndParent = GetParent(hWnd);

	// Center에 위치시킬려는 윈도의 크기
	GetWindowRect(hWnd, &rcWnd);
	int iWndWidth  = rcWnd.right - rcWnd.left;
	int iWndHeight = rcWnd.bottom - rcWnd.top;
	
	// Center의 기준이 될 타겟좌표 계산 - 부모윈도가 있는 경우 부모의 위치가 기준
	if (hWndParent != NULL)
	{
		GetWindowRect(hWndParent, &rcCenter);
	}
	else
	{
		rcCenter.left = 0;
		rcCenter.top = 0;
		rcCenter.right = GetSystemMetrics(SM_CXFULLSCREEN);
		rcCenter.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
	}

	int iScrWidth  = rcCenter.right - rcCenter.left;
	int iScrHeight = rcCenter.bottom - rcCenter.top;

	// 새로운 위치를 계산	
	int xLeft = rcCenter.left;
	int yTop = rcCenter.top;

	if (iScrWidth > iWndWidth)
		xLeft += ((iScrWidth - iWndWidth) / 2);

	if (iScrHeight > iWndHeight)
		yTop += ((iScrHeight - iWndHeight) / 2);

	// map screen coordinates to child coordinates
	SetWindowPos(hWnd, HWND_TOP, xLeft, yTop, -1, -1, SWP_NOSIZE);
}

bool IsWindowsSevenOrHigher(void) 
{
	OSVERSIONINFO osvi;

	memset(&osvi, 0, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	GetVersionEx(&osvi);

	if ((osvi.dwMajorVersion >= 6) && (osvi.dwMinorVersion >= 1))
		return true;

	return false;
}
