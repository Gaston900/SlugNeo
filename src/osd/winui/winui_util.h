// license:BSD-3-Clause
// copyright-holders:Chris Kirmse, Mike Haaland, René Single, Mamesick

#ifndef MUI_UTIL_H
#define MUI_UTIL_H

void ErrorMessageBox(const char *fmt, ...);
void dprintf(const char *fmt, ...);
void ShellExecuteCommon(HWND hWnd, const char *cName);
char * MyStrStrI(const char *pFirst, const char *pSrch);
char * ConvertToWindowsNewlines(const char *source);
const char * GetVersionString(void);
const char * GetDriverGameTitle(int nIndex);
const char * GetDriverGameName(int nIndex);
const char * GetDriverGameManufacturer(int nIndex);
const char * GetDriverGameYear(int nIndex);
const char * GetDriverFileName(int nIndex);
int GetGameNameIndex(const char *name);
int DriverNumScreens(int driver_index);
bool DriverIsClone(int driver_index);
bool DriverIsBroken(int driver_index);
bool DriverIsHarddisk(int driver_index);
bool DriverHasOptionalBIOS(int driver_index);
bool DriverIsVector(int driver_index);
bool DriverIsBios(int driver_index);
bool DriverUsesRoms(int driver_index);
bool DriverUsesSamples(int driver_index);
bool DriverUsesLightGun(int driver_index);
bool DriverUsesTrackball(int driver_index);
bool DriverSupportsSaveState(int driver_index);
bool DriverIsVertical(int driver_index);
bool DriverIsMechanical(int driver_index);
bool DriverIsImperfect(int driver_index);
wchar_t *win_wstring_from_utf8(const char *utf8string);
char *win_utf8_from_wstring(const wchar_t *wstring);
void winui_output_debug_string_utf8(const char *string);
int winui_message_box_utf8(HWND hWnd, const char *text, const char *caption, UINT type);
bool winui_set_window_text_utf8(HWND hWnd, const char *text);
int winui_get_window_text_utf8(HWND hWnd, char *buffer, size_t buffer_size);
HICON winui_extract_icon_utf8(HINSTANCE inst, const char* exefilename, UINT iconindex);
HANDLE winui_find_first_file_utf8(const char* filename, WIN32_FIND_DATA *findfiledata);
bool winui_move_file_utf8(const char* existingfilename, const char* newfilename);
void CenterWindow(HWND hWnd);
bool IsWindowsSevenOrHigher(void);
//const char * GetDriverCloneName(int nIndex);
int GetDriverIsClone(int nIndex);

#endif
