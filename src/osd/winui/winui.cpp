// license:BSD-3-Clause
// copyright-holders:Chris Kirmse, Mike Haaland, René Single, Mamesick

#include "winui.h"
#include "winui_faudit.h"
#include <fstream>

#ifdef MAME_AVI
#include "video.h"
#include "Avi.h"
#include "WAV.h"

static struct MAME_AVI_STATUS AviStatus;

static void     MamePlayGameAVI(void);
static void     MamePlayBackGameAVI(void);
static char		last_directory_avi[MAX_PATH];
int				_nAviNo = 0;

#endif /* MAME_AVI */

#define FAST_AUDIT
//#define LOGSAVE

// 修改的 代码来源 (缘来是你)
//=================================== 缘来是你 ========================================>>>
#define USE_SPLASH_SCREEN 1  // 关闭启动画面

// DPI
static UINT g_uCurrentDpi = 96;
static float g_fDpiScale = 1.0f;
static int g_guiPointSize = 0;
static int g_listPointSize = 0;
static int g_histPointSize = 0;
static int g_treePointSize = 0;
static bool g_fontPointsInitialized = false;

static UINT GetWindowDpiSafe(HWND hWnd)
{
    typedef UINT (WINAPI *GetDpiForWindowPtr)(HWND);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    GetDpiForWindowPtr fnGetDpiForWindow = (GetDpiForWindowPtr)GetProcAddress(hUser32, "GetDpiForWindow");
    
    if (fnGetDpiForWindow) {
        return fnGetDpiForWindow(hWnd);
    } else {
        HDC hdc = GetDC(NULL);
        UINT dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(NULL, hdc);
        return dpi;
    }
}

static void EnableNonClientDpiScalingSafe(HWND hWnd)
{
    typedef BOOL (WINAPI *EnableNonClientDpiScalingPtr)(HWND);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    EnableNonClientDpiScalingPtr fnEnableNonClientDpiScaling = 
        (EnableNonClientDpiScalingPtr)GetProcAddress(hUser32, "EnableNonClientDpiScaling");
    
    if (fnEnableNonClientDpiScaling) {
        fnEnableNonClientDpiScaling(hWnd);
    }
}
//======================================================================================>>>

static int MIN_WIDTH  = DBU_MIN_WIDTH;
static int MIN_HEIGHT = DBU_MIN_HEIGHT;

static HIMAGELIST   hHeaderImages = NULL;

typedef struct
{
	int resource;
	const char *icon_name;
} ICONDATA;

typedef struct
{
	const char *name;
	int index;
} srcdriver_data_type;

static const ICONDATA g_iconData[] =
{
	{ IDI_WIN_NOROMS,			"noroms" },
	{ IDI_WIN_ROMS,				"roms" },
	{ IDI_WIN_UNKNOWN,			"unknown" },
	{ IDI_WIN_CLONE,			"clone" },
	{ IDI_WIN_REDX,				"warning" },
	{ IDI_WIN_IMPERFECT,		"imperfect" },
	{ 0 }
};

static const struct TabViewCallbacks s_tabviewCallbacks =
{
	GetShowTabCtrl,				// pfnGetShowTabCtrl
	SetCurrentTab,				// pfnSetCurrentTab
	GetCurrentTab,				// pfnGetCurrentTab
	SetShowTab,					// pfnSetShowTab
	GetShowTab,					// pfnGetShowTab
	GetImageTabShortName,		// pfnGetTabShortName
	GetImageTabLongName,		// pfnGetTabLongName
	UpdateScreenShot			// pfnOnSelectionChanged
};

static const struct PickerCallbacks s_gameListCallbacks =
{
	SetSortColumn,					/* pfnSetSortColumn */
	GetSortColumn,					/* pfnGetSortColumn */
	SetSortReverse,					/* pfnSetSortReverse */
	GetSortReverse,					/* pfnGetSortReverse */
	SetViewMode,					/* pfnSetViewMode */
	GetViewMode,					/* pfnGetViewMode */
	SetColumnWidths,				/* pfnSetColumnWidths */
	GetColumnWidths,				/* pfnGetColumnWidths */
	SetColumnOrder,					/* pfnSetColumnOrder */
	GetColumnOrder,					/* pfnGetColumnOrder */
	SetColumnShown,					/* pfnSetColumnShown */
	GetColumnShown,					/* pfnGetColumnShown */
	GamePicker_Compare,				/* pfnCompare */
	MamePlayGame,					/* pfnDoubleClick */
	GamePicker_GetItemString,		/* pfnGetItemString */
	GamePicker_GetItemImage,		/* pfnGetItemImage */
	GamePicker_LeavingItem,			/* pfnLeavingItem */
	GamePicker_EnteringItem,		/* pfnEnteringItem */
	BeginListViewDrag,				/* pfnBeginListViewDrag */
	GamePicker_FindItemParent,		/* pfnFindItemParent */
	GamePicker_CheckNotWorkingItem,	/* pfnCheckNotWorkingItem */
	OnIdle,							/* pfnIdle */
	GamePicker_OnHeaderContextMenu,	/* pfnOnHeaderContextMenu */
	GamePicker_OnBodyContextMenu	/* pfnOnBodyContextMenu */
};

typedef struct _play_options play_options;
struct _play_options
{
	const char *record;			// OPTION_RECORD
	const char *playback;		// OPTION_PLAYBACK
	const char *state;			// OPTION_STATE
	const char *wavwrite;		// OPTION_WAVWRITE
	const char *mngwrite;		// OPTION_MNGWRITE
	const char *aviwrite;		// OPTION_AVIWRITE
#ifdef MAME_AVI
	const char *aviwrite2;		// OPTION_AVIWRITE
#endif /* MAME_AVI */	
};

//#ifdef USE_KLIST
#define TSVNAME "Arcade_List.lst"
#define LINEBUF_SIZE  1024
#define NUM_COLUMNS   3

typedef struct
{
	char *filename;	
	char *gamename;
	char *description;
	char *manufacturer;
} TSV;

static TSV  *tsv_index  = NULL;
static TSV  *tsv_data   = NULL;
static int  need_update = 0;
//#endif

/***************************************************************************
    function prototypes
 ***************************************************************************/

static void Win32UI_init(void);
static void Win32UI_exit(void);
static void	SaveWindowArea(void);
static void	SaveWindowStatus(void);
static void OnSize(HWND hwnd, UINT state, int width, int height);
static void SetView(int menu_id);
static void ResetListView(void);
static void UpdateGameList(void);
static void DestroyIcons(void);
static void ReloadIcons(void);
static void PollGUIJoystick(void);
static bool MameCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify);
static void UpdateStatusBar(void);
static bool TreeViewNotify(NMHDR *nm);
static int CLIB_DECL SrcDriverDataCompareFunc(const void *arg1, const void *arg2);
static void DisableSelection(void);
static void EnableSelection(int nGame);
static HICON GetSelectedPickItemIconSmall(void);
static void SetRandomPickItem(void);
static void	PickColor(COLORREF *cDefault);
static LPTREEFOLDER GetSelectedFolder(void);
static LRESULT CALLBACK PictureFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK PictureWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static HBITMAP CreateBitmapTransparent(HBITMAP hSource);
static void MamePlayRecordGame(void);
static void MamePlayBackGame(void);
static void MamePlayRecordWave(void);
static void MamePlayRecordMNG(void);
static void MamePlayRecordAVI(void);
static void	MameLoadState(void);
static void MamePlayGameWithOptions(int nGame, const play_options *playopts);
static bool GameCheck(void);
static void FolderCheck(void);
static void ToggleScreenShot(void);
static void AdjustMetrics(void);
/* Icon routines */
static void CreateIcons(void);
static int GetIconForDriver(int nItem);
static void AddDriverIcon(int nItem, int default_icon_index);
// Context Menu handlers
static void UpdateMenu(HMENU hMenu);
static void InitMainMenu(HMENU hMainMenu);
static void InitTreeContextMenu(HMENU hTreeMenu);
static void InitBodyContextMenu(HMENU hBodyContextMenu);
static void ToggleShowFolder(int folder);
static bool HandleTreeContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam);
static bool HandleScreenShotContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void InitListView(void);
/* Re/initialize the ListView header columns */
static void ResetColumnDisplay(void);
static void CopyToolTipText (LPTOOLTIPTEXT lpttt);
static void ProgressBarShow(void);
static void ProgressBarHide(void);
static void ResizeProgressBar(void);
static void InitProgressBar(void);
static void InitToolbar(void);
static void InitStatusBar(void);
static void InitTabView(void);
static void InitListTree(void);
static void	InitMenuIcons(void);
static void ResetFonts(void);
static void SetMainTitle(void);
static void UpdateHistory(void);
static void RemoveCurrentGameCustomFolder(void);
static void RemoveGameCustomFolder(int driver_index);
static void MouseMoveListViewDrag(POINTS pt);
static void ButtonUpListViewDrag(POINTS p);
static void CalculateBestScreenShotRect(HWND hWnd, RECT *pRect, bool restrict_height);
static void SwitchFullScreenMode(void);
static LRESULT CALLBACK MameWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// 修改的 代码来源 (缘来是你)
/******************************************************************************************/
#if USE_SPLASH_SCREEN
static intptr_t CALLBACK StartupProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
/******************************************************************************************/
static uintptr_t CALLBACK HookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static uintptr_t CALLBACK OFNHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static char* ConvertAmpersandString(const char *s);
void SetStatusBarTextW(int part_index, const wchar_t *fmt, ...);
static const char* GetCloneParentName(int nItem);
static const char* GetParentDriverGameName(int nItem);

enum
{
	FILETYPE_GAME_LIST = 1,
	FILETYPE_ROMS_LIST = 2,
};

static bool CommonListDialog(common_file_dialog_proc cfd, int filetype);
static void SaveGameListToFile(char *szFile, int filetype);

//#ifdef USE_KLIST
static void LoadGameListFromFile(int games);
static void SaveAllGameListToFile();
char *GetDescriptionByIndex(int nIndex, bool bUse);
char *GetDescriptionByName(const char *name, bool bUse);
char *GetGameNameByIndex(int nIndex, bool bUse);
char *GetGameName(const char *name, bool bUse);
char *GetGameManufactureByIndex(int nIndex, bool bUse);
//#endif

/***************************************************************************
    Internal structures
 ***************************************************************************/

/*
 * These next two structs represent how the icon information
 * is stored in an ICO file.
 */
typedef struct
{
	uint8_t bWidth;               /* Width of the image */
	uint8_t bHeight;              /* Height of the image (times 2) */
	uint8_t bColorCount;          /* Number of colors in image (0 if >=8bpp) */
	uint8_t bReserved;            /* Reserved */
	WORD wPlanes;              /* Color Planes */
	WORD wBitCount;            /* Bits per pixel */
	DWORD dwBytesInRes;        /* how many bytes in this resource? */
	DWORD dwImageOffset;       /* where in the file is this image */
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
	UINT Width, Height, Colors; 	/* Width, Height and bpp */
	LPBYTE lpBits;                	/* ptr to DIB bits */
	DWORD dwNumBytes;            	/* how many bytes? */
	LPBITMAPINFO lpbi;              /* ptr to header */
	LPBYTE lpXOR;                 	/* ptr to XOR image bits */
	LPBYTE lpAND;                 	/* ptr to AND image bits */
} ICONIMAGE, *LPICONIMAGE;

typedef struct
{
	int type;       		/* Either RA_ID or RA_HWND, to indicate which member of u is used; or RA_END to signify last entry */
	union                   /* Can identify a child window by control id or by handle */
	{
		int id;       		/* Window control id */
		HWND hwnd;     		/* Window handle */
	} u;
	int action;     		/* What to do when control is resized */
	void *subwindow; 		/* Points to a Resize structure for this subwindow; NULL if none */
} ResizeItem;

typedef struct
{
	RECT rect;       			/* Client rect of window; must be initialized before first resize */
	const ResizeItem* items;   	/* Array of subitems to be resized */
} Resize;

static void ResizeWindow(HWND hParent, Resize *r);

/* List view Icon defines */
#define LG_ICONMAP_WIDTH    GetSystemMetrics(SM_CXICON)
#define LG_ICONMAP_HEIGHT   GetSystemMetrics(SM_CYICON)
#define ICONMAP_WIDTH       GetSystemMetrics(SM_CXSMICON)
#define ICONMAP_HEIGHT      GetSystemMetrics(SM_CYSMICON)

/***************************************************************************
    Internal variables
 ***************************************************************************/
 
static wchar_t g_szPlayGameString[] = TEXT("Play %s\tAlt+O");
static char g_szGameCountString[] = "%d Games";
static HWND hMain = NULL;
static HWND	hSplash = NULL;
static HWND hWndList = NULL;
static HWND hTreeView = NULL;
static HWND hProgWnd = NULL;
static HWND hTabCtrl = NULL;
static HWND hSearchWnd = NULL;
static HWND	hProgress = NULL;
static HACCEL hAccel = NULL;
static HINSTANCE hInst = NULL;
static HFONT hFontGui = NULL;     /* Font for tab view and search window */
static HFONT hFontList = NULL;    /* Font for list view */
static HFONT hFontHist = NULL;    /* Font for history view */
static HFONT hFontTree = NULL;    /* Font for folders view */
/* menu icons bitmaps */
static HBITMAP hAboutMenu = NULL;
static HBITMAP hCustom = NULL;
static HBITMAP hDirectories = NULL;
static HBITMAP hExit = NULL;
static HBITMAP hFullscreen = NULL;
static HBITMAP hInterface = NULL;
static HBITMAP hHelp = NULL;
static HBITMAP hMameHome = NULL;
static HBITMAP hPlay = NULL;
static HBITMAP hPlayM1 = NULL;
static HBITMAP hOptions = NULL;
static HBITMAP hRefresh = NULL;
static HBITMAP hZip = NULL;
static HBITMAP hManual = NULL;
static HBITMAP hNotepad = NULL;
static HBITMAP hSaveList = NULL;
static HBITMAP hSaveRoms = NULL;
static HBITMAP hPlayback = NULL;
static HBITMAP hProperties = NULL;
static HBITMAP hAuditMenu = NULL;
static HBITMAP hVideo = NULL;
static HBITMAP hFonts = NULL;
static HBITMAP hFolders = NULL;
static HBITMAP hSort = NULL;
static HBITMAP hDriver = NULL;
static HBITMAP hFaq = NULL;
static HBITMAP hTabs = NULL;
static HBITMAP hTrouble = NULL;
static HBITMAP hCount = NULL;
static HBITMAP hRelease = NULL;
static HBITMAP hTime = NULL;
static HBITMAP hDescription	= NULL;
static HBITMAP hRom	= NULL;
static HBITMAP hSource = NULL;
static HBITMAP hManufacturer = NULL;
static HBITMAP hYear = NULL;
static HBITMAP hPlaywav	= NULL;
static HBITMAP hFont1 = NULL;
static HBITMAP hFont2 = NULL;
static HBITMAP hInfoback = NULL;
static HBITMAP hListback = NULL;
static HBITMAP hTreeback = NULL;
static HBITMAP hAscending = NULL;
static HBITMAP hFields = NULL;
static HBITMAP hRecavi = NULL;
static HBITMAP hRecinput = NULL;
static HBITMAP hRecwav = NULL;
static HBITMAP hPlaymng	= NULL;
static HBITMAP hRandom = NULL;
static HBITMAP hRecmng = NULL;
static HBITMAP hSavestate = NULL;
static HBITMAP hFilters	= NULL;
static HBITMAP hRemove = NULL;
static HBITMAP hRename = NULL;
static HBITMAP hReset = NULL;
static HBITMAP hklist = NULL;	//use_KLIST
static int optionfolder_count = 0;
/* global data--know where to send messages */
static bool in_emulation = false;
static bool game_launched = false;
/* idle work at startup */
static bool idle_work = false;
/* object pool in use */
static object_pool *mameui_pool;
static int game_index = 0;
static int game_total = 0;
static int oldpercent = 0;
static bool bDoGameCheck = false;
static bool bFolderCheck = false;
static bool bChangedHook = false;
static bool bHookFont = false;
/* Tree control variables */
static bool bEnableIndent = false;
static bool bShowTree = false;
static bool bShowToolBar = false;
static bool bShowStatusBar = false;
static bool bShowTabCtrl = false;
static bool bProgressShown = false;
static bool bListReady = false;
static bool bFullScreen = false;
/* use a joystick subsystem in the gui? */
static const struct OSDJoystick* g_pJoyGUI = NULL;
/* search */
static char g_SearchText[256];
static UINT lastColumnClick = 0;
static WNDPROC g_lpPictureFrameWndProc = NULL;
static WNDPROC g_lpPictureWndProc = NULL;
/* Tool and Status bar variables */
static HWND hStatusBar = NULL;
static HWND hToolBar = NULL;
/* Used to recalculate the main window layout */
static int bottomMargin = 0;
static int topMargin = 0;
static bool have_history = false;
static bool have_selection = false;
static HBITMAP hMissing_bitmap = NULL;
static HBRUSH hBrush = NULL;
static HBRUSH hBrushDlg = NULL;
static HDC hDC = NULL;
/* Icon variables */
static HIMAGELIST hLarge = NULL;
static HIMAGELIST hSmall = NULL;
static HICON hIcon = NULL;
static int *icon_index = NULL; 	/* for custom per-game icons */


static bool bklist = true; // USE_KLIST
static int game_count=0;

static const char* Preinfofilename = "preinfo.vp";
static const char* StartGameInfofilename = "startinfo.vp";

bool IsFileWritable(const WCHAR* pszFilePath);

static const TBBUTTON tbb[] =
{
	{0, ID_VIEW_FOLDERS,    	TBSTATE_ENABLED, BTNS_CHECK,      {0, 0}, 0, 0},
	{1, ID_VIEW_PICTURE_AREA,	TBSTATE_ENABLED, BTNS_CHECK,      {0, 0}, 0, 1},
	{0, 0,                  	TBSTATE_ENABLED, BTNS_SEP,        {0, 0}, 0, 0},
	{2, ID_VIEW_ICONS_LARGE,  	TBSTATE_ENABLED, BTNS_CHECKGROUP, {0, 0}, 0, 2},
	{3, ID_VIEW_ICONS_SMALL, 	TBSTATE_ENABLED, BTNS_CHECKGROUP, {0, 0}, 0, 3},
	{15, ID_VIEW_LIST_MENU,  	TBSTATE_ENABLED, BTNS_CHECKGROUP, {0, 0}, 0, 15},
	{5, ID_VIEW_DETAIL, 		TBSTATE_ENABLED, BTNS_CHECKGROUP, {0, 0}, 0, 5},
//	{6, ID_VIEW_GROUPED,  		TBSTATE_ENABLED, BTNS_CHECKGROUP, {0, 0}, 0, 6},
	{0, 0,                  	TBSTATE_ENABLED, BTNS_SEP,        {0, 0}, 0, 0},
	{4, ID_ENABLE_INDENT,  		TBSTATE_ENABLED, BTNS_CHECK,      {0, 0}, 0, 12},
	{0, 0,                  	TBSTATE_ENABLED, BTNS_SEP,        {0, 0}, 0, 0},
	{7, ID_UPDATE_GAMELIST,  	TBSTATE_ENABLED, BTNS_BUTTON,     {0, 0}, 0, 4},
	{0, 0,                    	TBSTATE_ENABLED, BTNS_SEP,        {0, 0}, 0, 0},
	{8, ID_OPTIONS_INTERFACE,	TBSTATE_ENABLED, BTNS_BUTTON,     {0, 0}, 0, 5},
	{9, ID_OPTIONS_DEFAULTS, 	TBSTATE_ENABLED, BTNS_BUTTON,     {0, 0}, 0, 6},
	{0, 0,                   	TBSTATE_ENABLED, BTNS_SEP,        {0, 0}, 0, 0},
	{10,ID_VIDEO_SNAP,			TBSTATE_ENABLED, BTNS_BUTTON,     {0, 0}, 0, 7},
	{11,ID_PLAY_M1,   			TBSTATE_ENABLED, BTNS_BUTTON,     {0, 0}, 0, 8},
	{0, 0,                    	TBSTATE_ENABLED, BTNS_SEP,        {0, 0}, 0, 0},
	{12,ID_HELP_ABOUT,          TBSTATE_ENABLED, BTNS_BUTTON,     {0, 0}, 0, 9},
	{13,ID_HELP_CONTENTS,       TBSTATE_ENABLED, BTNS_BUTTON,     {0, 0}, 0, 10},
	{14,ID_MAME_HOMEPAGE,       TBSTATE_ENABLED, BTNS_BUTTON,     {0, 0}, 0, 14},
	{0, 0,                    	TBSTATE_ENABLED, BTNS_SEP,        {0, 0}, 0, 0},
	{6, ID_KOREAN_GAMELIST,     TBSTATE_ENABLED, BTNS_CHECK,	  {0, 0}, 0, 13}, // USE_KLIST
	{0, 0,                    	TBSTATE_ENABLED, BTNS_SEP,        {0, 0}, 0, 0}
};

static const wchar_t szTbStrings[NUM_TOOLTIPS][30] =
{
	L"Toggle folders list",
	L"Toggle pictures area",
	L"Large icons",
	L"Small icons",
	L"List icons",
	L"Toggle grouped view",
	L"Refresh",
	L"Interface setttings",
	L"Default games options",
	L"Play ProgettoSnaps movie",
	L"M1FX",
	L"About",
	L"Help",
	L"MAME homepage",
	L"Details icons",
	L"Language game list"
};

static const int CommandToString[] =
{
	ID_VIEW_FOLDERS,
	ID_VIEW_PICTURE_AREA,
	ID_VIEW_ICONS_LARGE,
	ID_VIEW_ICONS_SMALL,
	ID_VIEW_LIST_MENU,
	ID_VIEW_DETAIL,
//	ID_VIEW_GROUPED,
	ID_UPDATE_GAMELIST,
	ID_OPTIONS_INTERFACE,
	ID_OPTIONS_DEFAULTS,
	ID_VIDEO_SNAP,
	ID_PLAY_M1,
	ID_HELP_ABOUT,
	ID_HELP_CONTENTS,
	ID_MAME_HOMEPAGE,
	ID_ENABLE_INDENT,
	ID_KOREAN_GAMELIST,
	-1
};

static const int s_nPickers[] =
{
	IDC_LIST
};

/* How to resize toolbar sub window */
static ResizeItem toolbar_resize_items[] =
{
	{ RA_ID,   { ID_TOOLBAR_EDIT }, RA_LEFT | RA_TOP,     NULL },
	{ RA_END,  { 0 },               0,                    NULL }
};

static Resize toolbar_resize = { {0, 0, 0, 0}, toolbar_resize_items };

/* How to resize main window */
static ResizeItem main_resize_items[] =
{
	{ RA_HWND, { 0 },            RA_LEFT  | RA_RIGHT  | RA_TOP,     &toolbar_resize },
	{ RA_HWND, { 0 },            RA_LEFT  | RA_RIGHT  | RA_BOTTOM,  NULL },
	{ RA_ID,   { IDC_DIVIDER },  RA_LEFT  | RA_RIGHT  | RA_TOP,     NULL },
	{ RA_ID,   { IDC_TREE },     RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SPLITTER }, RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_LIST },     RA_ALL,                            NULL },
	{ RA_ID,   { IDC_SPLITTER2 },RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSTAB },    RA_RIGHT | RA_TOP,                 NULL },
	{ RA_ID,   { IDC_SSPICTURE },RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_HISTORY },  RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSFRAME },  RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSBORDER }, RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_END,  { 0 },            0,                                 NULL }
};

static Resize main_resize = { {0, 0, 0, 0}, main_resize_items };

/* last directory for common file dialogs */
static wchar_t last_directory[MAX_PATH] = TEXT(".");
/* Last directory for Save Game or ROMs List dialogs */
static wchar_t list_directory[MAX_PATH] = TEXT(".");
static bool g_listview_dragging = false;
static HIMAGELIST himl_drag = NULL;
static int game_dragged = 0; 					/* which game started the drag */
static HTREEITEM prev_drag_drop_target = NULL; 	/* which tree view item we're currently highlighting */
static bool g_in_treeview_edit = false;
static srcdriver_data_type *sorted_srcdrivers = NULL;

/***************************************************************************
    Global variables
 ***************************************************************************/

/* Icon displayed in system tray */
static NOTIFYICONDATA MameIcon;

/* List view Column text */
extern const wchar_t* const column_names[COLUMN_MAX] =
{
	TEXT("Description"),
	TEXT("ROM name"),
	TEXT("Source file"),
	TEXT("Manufacturer"),
	TEXT("Year"),
	TEXT("Clone of"),
	TEXT("Played"),
	TEXT("Play time")
};


/***************************************************************************
    External functions
 ***************************************************************************/
class mameui_output_error : public osd_output
{
public:
	virtual void output_callback(osd_output_channel channel, const util::format_argument_pack<std::ostream> &args) override
	{
		std::ostringstream sbuffer;
		util::stream_format(sbuffer, args);
		string s = sbuffer.str();
		const char* buffer = s.c_str();
		if (channel == OSD_OUTPUT_CHANNEL_VERBOSE)
		{
#ifdef LOGSAVE		
			FILE *pFile;
			pFile = fopen("config/verbose.log", "a");
			fputs(buffer, pFile);
			fflush(pFile);
			fclose (pFile);
#endif			
			return;
		}

		int s_action = 0;

		if (channel == OSD_OUTPUT_CHANNEL_ERROR)
		{
			s_action = 0x80;
		}
		else
		if (channel == OSD_OUTPUT_CHANNEL_WARNING)
		{
			if (strstr(buffer, "WRONG"))
			{
				s_action = 0x81;
			}
		}

		if (s_action)
		{
			// if we are in fullscreen mode, go to windowed mode
			if ((video_config.windowed == 0) && !osd_common_t::s_window_list.empty())
				winwindow_toggle_full_screen();

			winui_message_box_utf8(!osd_common_t::s_window_list.empty() ?
				std::static_pointer_cast<win_window_info>(osd_common_t::s_window_list.front())->platform_window() :
					hMain, buffer, MAMEUINAME, (BIT(s_action, 0) ? MB_ICONINFORMATION : MB_ICONERROR) | MB_OK);
		}

//		else
//			chain_output(channel, msg, args);   // goes down the black hole
#ifdef LOGSAVE
		// LOG all messages
		FILE *pFile;
		pFile = fopen("config/winui.log", "a");
		fputs(buffer, pFile);
		fflush(pFile);
		fclose (pFile);
#endif		
	}
};

#ifdef MAME_AVI
void override_options(core_options &opts, void *param)
{
	const play_options *playopts = (const play_options *)param;

	if (playopts->aviwrite2)
	{
		opts.set_value("avi_avi_filename", playopts->aviwrite2, OPTION_PRIORITY_CMDLINE);

		opts.set_value("avi_def_fps", AviStatus.def_fps, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_fps", AviStatus.fps, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_frame_skip", AviStatus.frame_skip, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_frame_cmp", AviStatus.frame_cmp, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_frame_cmp_pre15", AviStatus.frame_cmp_pre15, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_frame_cmp_few", AviStatus.frame_cmp_few, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_width", AviStatus.width, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_height", AviStatus.height, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_depth", AviStatus.depth, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_orientation", AviStatus.orientation, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_rect_top", AviStatus.rect.m_Top, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_rect_left", AviStatus.rect.m_Left, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_rect_width", AviStatus.rect.m_Width, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_rect_height", AviStatus.rect.m_Height, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_interlace", AviStatus.interlace, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_interlace_odd_field", AviStatus.interlace_odd_number_field, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_avi_filesize", AviStatus.avi_filesize, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_avi_savefile_pause", AviStatus.avi_savefile_pause, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_avi_width", AviStatus.avi_width, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_avi_height", AviStatus.avi_height, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_avi_depth", AviStatus.avi_depth, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_avi_rect_top", AviStatus.avi_rect.m_Top, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_avi_rect_left", AviStatus.avi_rect.m_Left, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_avi_rect_width", AviStatus.avi_rect.m_Width, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_avi_rect_height", AviStatus.avi_rect.m_Height, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_avi_smooth_resize_x", AviStatus.avi_smooth_resize_x, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_avi_smooth_resize_y", AviStatus.avi_smooth_resize_y, OPTION_PRIORITY_CMDLINE);

		if (AviStatus.wav_filename && strlen((const char *)AviStatus.wav_filename))
			opts.set_value("avi_wav_filename", (const char *)AviStatus.wav_filename, OPTION_PRIORITY_CMDLINE);
			
		opts.set_value("avi_audio_type", AviStatus.audio_type, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_audio_channel", AviStatus.audio_channel, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_audio_samples_per_sec", AviStatus.audio_samples_per_sec, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_audio_bitrate", AviStatus.audio_bitrate, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_audio_record_type", AviStatus.avi_audio_record_type, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_avi_audio_channel", AviStatus.avi_audio_channel, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_avi_audio_samples_per_sec", AviStatus.avi_audio_samples_per_sec, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_avi_audio_bitrate", AviStatus.avi_audio_bitrate, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_audio_cmp", AviStatus.avi_audio_cmp, OPTION_PRIORITY_CMDLINE);

		opts.set_value("avi_hour", AviStatus.hour, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_minute", AviStatus.minute, OPTION_PRIORITY_CMDLINE);
		opts.set_value("avi_second", AviStatus.second, OPTION_PRIORITY_CMDLINE);
	}
	else
	{
		opts.set_value("avi_avi_filename", nullptr, OPTION_PRIORITY_CMDLINE);
	}

#ifdef KAILLERA
	if (playopts->playbacksub)
		options_set_wstring(opts, "pbsub", playopts->playbacksub, OPTION_PRIORITY_CMDLINE);
	if (playopts->recordsub)
		options_set_wstring(opts, "recsub", playopts->recordsub, OPTION_PRIORITY_CMDLINE);
	if (playopts->record != NULL && playopts->autorecname != NULL)
		options_set_wstring(opts, "at_rec_name", playopts->autorecname, OPTION_PRIORITY_CMDLINE);

	// kaillera force options
	if (kPlay)
	{
		options_set_bool  (opts, OPTION_AUTOFRAMESKIP, TRUE, OPTION_PRIORITY_CMDLINE);
		options_set_bool  (opts, OPTION_THROTTLE, TRUE, OPTION_PRIORITY_CMDLINE);
		options_set_bool  (opts, WINOPTION_WAITVSYNC, 0, OPTION_PRIORITY_CMDLINE);
		options_set_bool  (opts, WINOPTION_SYNCREFRESH, 0, OPTION_PRIORITY_CMDLINE);
		options_set_bool  (opts, OPTION_SOUND, TRUE, OPTION_PRIORITY_CMDLINE);
		options_set_int   (opts, OPTION_SAMPLERATE, 48000, OPTION_PRIORITY_CMDLINE);
		options_set_bool  (opts, OPTION_SAMPLES, TRUE, OPTION_PRIORITY_CMDLINE);
#if 0
		if (!mame_stricmp(GetDriverFilename(nGameIndex), "neogeo.c") && !mame_stricmp(pOpts->bios, "uni-bios.22"))
			options_set_string(opts, OPTION_BIOS, pOpts->bios, OPTION_PRIORITY_CMDLINE);
		else
#endif
		options_set_string(opts, OPTION_BIOS, "0", OPTION_PRIORITY_CMDLINE);
		options_set_bool  (opts, OPTION_CHEAT, 0, OPTION_PRIORITY_CMDLINE);
		options_set_bool  (opts, WINOPTION_MULTITHREADING, 0, OPTION_PRIORITY_CMDLINE);
		options_set_float (opts, OPTION_SPEED, 1.0, OPTION_PRIORITY_CMDLINE);
		options_set_bool  (opts, OPTION_REFRESHSPEED, 0, OPTION_PRIORITY_CMDLINE);
	}
#endif /* KAILLERA */
}
#endif

static void RunMAME(int nGameIndex, const play_options *playopts)
{
	time_t start = 0;
	time_t end = 0;
	windows_options mame_opts;
	std::ostringstream option_errors;
	const char* name = driver_list::driver(nGameIndex).name;

	// redirect messages to our handler
	mameui_output_error winerror;
	printf("********** STARTING %s **********\n", name);
	osd_output::push(&winerror);
	osd_printf_verbose("********** STARTING %s **********\n", name);
	osd_printf_info("********** STARTING %s **********\n", name);
	osd_output::pop(&winerror);

	// prepare MAMEUIFX to run the game
	ShowWindow(hMain, SW_HIDE);

	for (int i = 0; i < WINUI_ARRAY_LENGTH(s_nPickers); i++)
		Picker_ClearIdle(GetDlgItem(hMain, s_nPickers[i]));

	// Time the game run.
	windows_osd_interface osd(mame_opts);
	osd_output::push(&winerror);
	osd.register_options();
	mame_machine_manager *manager = mame_machine_manager::instance(mame_opts, osd);
	// set the new game name
	mame_opts.set_value(OPTION_SYSTEMNAME, GetDriverGameName(nGameIndex), OPTION_PRIORITY_CMDLINE);
	// set all needed paths
	SetDirectories(mame_opts);
	// parse all INI files
	mame_options::parse_standard_inis(mame_opts, option_errors);
	// load interface language
	load_translation(mame_opts);
	// start LUA engine & http server
	manager->start_http_server();
	manager->start_luaengine();

	// set any specified play options
	if (playopts != NULL)
	{
		if (playopts->record != NULL)
			mame_opts.set_value(OPTION_RECORD, playopts->record, OPTION_PRIORITY_CMDLINE);

		if (playopts->playback != NULL)
			mame_opts.set_value(OPTION_PLAYBACK, playopts->playback, OPTION_PRIORITY_CMDLINE);

		if (playopts->state != NULL)
			mame_opts.set_value(OPTION_STATE, playopts->state, OPTION_PRIORITY_CMDLINE);

		if (playopts->wavwrite != NULL)
			mame_opts.set_value(OPTION_WAVWRITE, playopts->wavwrite, OPTION_PRIORITY_CMDLINE);

		if (playopts->mngwrite != NULL)
			mame_opts.set_value(OPTION_MNGWRITE, playopts->mngwrite, OPTION_PRIORITY_CMDLINE);

		if (playopts->aviwrite != NULL)
			mame_opts.set_value(OPTION_AVIWRITE, playopts->aviwrite, OPTION_PRIORITY_CMDLINE);
	}

#if defined(KAILLERA) || defined(MAME_AVI)
		override_options(mame_opts, (void *)playopts);
#endif
// ekmame info stop
// preview video start
	if(IsFileWritable(win_wstring_from_utf8(Preinfofilename)))
	{
		FILE *f = NULL;

		f= fopen(StartGameInfofilename, "w");
		fprintf(f,"%s\n","GAMESTART");
		fclose(f);	
	}
// preview video end

	// start played time
	time(&start);
	// run the game
	manager->execute();
	// end played time
	time(&end);
// ekmame info stop
// preview video start
	if(IsFileWritable(win_wstring_from_utf8(Preinfofilename)))
	{
		FILE *f = NULL;

		f = fopen(StartGameInfofilename, "w");
		fprintf(f,"%s\n","GAMEEND");
		fclose(f);
	}
// preview video end

	// free the structure
	global_free(manager);
	osd_output::pop(&winerror);
	// Calc the duration
	double elapsedtime = end - start;
	// Increment our playtime
	IncrementPlayTime(nGameIndex, elapsedtime);

	// the emulation is complete; continue
	for (int i = 0; i < WINUI_ARRAY_LENGTH(s_nPickers); i++)
		Picker_ResetIdle(GetDlgItem(hMain, s_nPickers[i]));
}

int MameUIMain(HINSTANCE hInstance, LPWSTR lpCmdLine)
{
	// delete old log file, ignore any error
	unlink("config/winui.log");
	unlink("config/verbose.log");

	//printf("ARCADE starting\n");fflush(stdout);

	if (__argc != 1)
	{
		extern int main_(int argc, char *argv[]);
		exit(main_(__argc, __argv));
	}

	WNDCLASS wndclass;
	MSG msg;
	hInst = hInstance;

	// set up window class
	memset(&wndclass, 0, sizeof(WNDCLASS));
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = MameWindowProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = DLGWINDOWEXTRA;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAMEUI_ICON));
	wndclass.hCursor = NULL;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);// GetSysColorBrush(COLOR_3DFACE); 
	wndclass.lpszMenuName = MAKEINTRESOURCE(IDR_UI_MENU);
	wndclass.lpszClassName = TEXT("MainClass");

	RegisterClass(&wndclass);
	hMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, NULL);
	InitToolbar();
	InitStatusBar();
	InitProgressBar();
	InitTabView();
	InitMenuIcons();
	SetMainTitle();

	memset (&MameIcon, 0, sizeof(NOTIFYICONDATA));
	MameIcon.cbSize = sizeof(NOTIFYICONDATA);
	MameIcon.hWnd = hMain;
	MameIcon.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SYSTRAY));
	MameIcon.uID = 0;
	MameIcon.uCallbackMessage = WM_USER;
	MameIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
	MameIcon.dwInfoFlags = NIIF_USER;
	MameIcon.uVersion = NOTIFYICON_VERSION;
	wcscpy(MameIcon.szInfoTitle, TEXT("ARCADE"));
	wcscpy(MameIcon.szInfo, TEXT("Still running...."));
	wcscpy(MameIcon.szTip, TEXT("ARCADE"));

// 修改的 代码来源 (缘来是你)
//===============禁用启动画面 =============>>>
#if USE_SPLASH_SCREEN
	hSplash = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_STARTUP), hMain, StartupProc);
	SetActiveWindow(hSplash);
	SetForegroundWindow(hSplash);
	Win32UI_init();
	DestroyWindow(hSplash);
	#else
    Win32UI_init();
#endif
//==========================================>>>

	while(GetMessage(&msg, NULL, 0, 0))
	{
		if (IsWindow(hMain))
		{
			if (!TranslateAccelerator(hMain, hAccel, &msg))
			{
				if (!IsDialogMessage(hMain, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
	}

	return msg.wParam;
}

HWND GetMainWindow(void)
{
	return hMain;
}

HWND GetTreeView(void)
{
	return hTreeView;
}

HWND GetProgressBar(void)
{
	return hProgress;
}

object_pool *GetMameUIMemoryPool(void)
{
	return mameui_pool;
}

void GetRealColumnOrder(int order[])
{
	int tmpOrder[COLUMN_MAX];
	int nColumnMax = Picker_GetNumColumns(hWndList);

	/* Get the Column Order and save it */
	(void)ListView_GetColumnOrderArray(hWndList, nColumnMax, tmpOrder);

	for (int i = 0; i < nColumnMax; i++)
	{
		order[i] = Picker_GetRealColumnFromViewColumn(hWndList, tmpOrder[i]);
	}
}

/*
 * PURPOSE: Format raw data read from an ICO file to an HICON
 * PARAMS:  PBYTE ptrBuffer  - Raw data from an ICO file
 *          UINT nBufferSize - Size of buffer ptrBuffer
 * RETURNS: HICON - handle to the icon, NULL for failure
 * History: July '95 - Created
 *          March '00- Seriously butchered from MSDN for mine own
 *          purposes, sayeth H0ek.
 */
static HICON FormatICOInMemoryToHICON(PBYTE ptrBuffer, UINT nBufferSize)
{
	ICONIMAGE IconImage;
	UINT nBufferIndex = 0;
	HICON hIcon = NULL;

	/* Is there a WORD? */
	if (nBufferSize < sizeof(WORD))
		return NULL;

	/* Was it 'reserved' ?   (ie 0) */
	if ((WORD)(ptrBuffer[nBufferIndex]) != 0)
		return NULL;

	nBufferIndex += sizeof(WORD);

	/* Is there a WORD? */
	if (nBufferSize - nBufferIndex < sizeof(WORD))
		return NULL;

	/* Was it type 1? */
	if ((WORD)(ptrBuffer[nBufferIndex]) != 1)
		return NULL;

	nBufferIndex += sizeof(WORD);

	/* Is there a WORD? */
	if (nBufferSize - nBufferIndex < sizeof(WORD))
		return NULL;

	/* Then that's the number of images in the ICO file */
	int nNumImages = (WORD)(ptrBuffer[nBufferIndex]);

	/* Is there at least one icon in the file? */
	if (nNumImages < 1)
		return NULL;

	nBufferIndex += sizeof(WORD);

	/* Is there enough space for the icon directory entries? */
	if ((nBufferIndex + nNumImages * sizeof(ICONDIRENTRY)) > nBufferSize)
		return NULL;

	/* Assign icon directory entries from buffer */
	LPICONDIRENTRY lpIDE = (LPICONDIRENTRY)(&ptrBuffer[nBufferIndex]);
	nBufferIndex += nNumImages * sizeof (ICONDIRENTRY);

	IconImage.dwNumBytes = lpIDE->dwBytesInRes;

	/* Seek to beginning of this image */
	if (lpIDE->dwImageOffset > nBufferSize)
		return NULL;

	nBufferIndex = lpIDE->dwImageOffset;

	/* Read it in */
	if ((nBufferIndex + lpIDE->dwBytesInRes) > nBufferSize)
		return NULL;

	IconImage.lpBits = &ptrBuffer[nBufferIndex];

	/* We would break on NT if we try with a 16bpp image */
	if (((LPBITMAPINFO)IconImage.lpBits)->bmiHeader.biBitCount != 16)
		hIcon = CreateIconFromResourceEx(IconImage.lpBits, IconImage.dwNumBytes, true, 0x00030000, 0, 0, LR_DEFAULTSIZE);

	return hIcon;
}

HICON LoadIconFromFile(const char *iconname)
{
	HICON hIcon = NULL;
	WIN32_FIND_DATA FindFileData;
	PBYTE bufferPtr;
	util::archive_file::ptr zip;

	std::string tmpStr = std::string(GetIconsDir()).append(PATH_SEPARATOR).append(iconname).append(".ico");
	HANDLE hFind = winui_find_first_file_utf8(tmpStr.c_str(), &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE || (hIcon = winui_extract_icon_utf8(hInst, tmpStr.c_str(), 0)) == 0)
	{
		tmpStr = std::string(GetIconsDir()).append(PATH_SEPARATOR).append("icons.zip");
		std::string tmpIcoName = std::string(iconname).append(".ico");

		if (util::archive_file::open_zip(tmpStr, zip) == util::archive_file::error::NONE)
		{
			if (zip->search(tmpIcoName, false) >= 0)
			{
				bufferPtr = (PBYTE)malloc(zip->current_uncompressed_length());

				if (bufferPtr)
				{
					if (zip->decompress(bufferPtr, zip->current_uncompressed_length()) == util::archive_file::error::NONE)
						hIcon = FormatICOInMemoryToHICON(bufferPtr, zip->current_uncompressed_length());

					free(bufferPtr);
				}
			}

			zip.reset();
		}
		else
		{
			tmpStr = std::string(GetIconsDir()).append(PATH_SEPARATOR).append("icons.7z");
			tmpIcoName = std::string(iconname).append(".ico");

			if (util::archive_file::open_7z(tmpStr, zip) == util::archive_file::error::NONE)
			{
				if (zip->search(tmpIcoName, false) >= 0)
				{
					bufferPtr = (PBYTE)malloc(zip->current_uncompressed_length());

					if (bufferPtr)
					{
						if (zip->decompress(bufferPtr, zip->current_uncompressed_length()) == util::archive_file::error::NONE)
							hIcon = FormatICOInMemoryToHICON(bufferPtr, zip->current_uncompressed_length());

						free(bufferPtr);
					}
				}

				zip.reset();
			}
		}
	}

	return hIcon;
}

/* Return the number of folders with options */
void SetNumOptionFolders(int count)
{
	optionfolder_count = count;
}

/* search */
const char * GetSearchText(void)
{
	return g_SearchText;
}

/* Sets the treeview and listviews sizes in accordance with their visibility and the splitters */
static void ResizeTreeAndListViews(bool bResizeHidden)
{
	RECT rect;
	int nLastWidth = SPLITTER_WIDTH;
	int nLastWidth2 = 0;
	int nLeftWindowWidth = 0;

	/* Size the List Control in the Picker */
	GetClientRect(hMain, &rect);

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;

	if (bShowToolBar)
		rect.top += topMargin;

	/* Tree control */
	ShowWindow(GetDlgItem(hMain, IDC_TREE), bShowTree ? SW_SHOW : SW_HIDE);

	for (int i = 0; g_splitterInfo[i].nSplitterWindow; i++)
	{
		bool bVisible = (GetWindowLong(GetDlgItem(hMain, g_splitterInfo[i].nLeftWindow), GWL_STYLE) & WS_VISIBLE) ? true : false;

		if (bResizeHidden || bVisible)
		{
			nLeftWindowWidth = nSplitterOffset[i] - SPLITTER_WIDTH / 2 - nLastWidth;

			/* special case for the rightmost pane when the screenshot is gone */
			if (!GetShowScreenShot() && !g_splitterInfo[i + 1].nSplitterWindow)
				nLeftWindowWidth = rect.right - nLastWidth - 4;

			/* woah?  are we overlapping ourselves? */
			if (nLeftWindowWidth < MIN_VIEW_WIDTH)
			{
				nLastWidth = nLastWidth2;
				nLeftWindowWidth = nSplitterOffset[i] - MIN_VIEW_WIDTH - (SPLITTER_WIDTH * 3 / 2) - nLastWidth;
				i--;
			}

			MoveWindow(GetDlgItem(hMain, g_splitterInfo[i].nLeftWindow), nLastWidth, rect.top + 3, nLeftWindowWidth, (rect.bottom - rect.top) - 8, true);
			MoveWindow(GetDlgItem(hMain, g_splitterInfo[i].nSplitterWindow), nSplitterOffset[i], rect.top + 3, SPLITTER_WIDTH, (rect.bottom - rect.top) - 8, true);
		}

		if (bVisible)
		{
			nLastWidth2 = nLastWidth;
			nLastWidth += nLeftWindowWidth + SPLITTER_WIDTH;
		}
	}
}

/* Adjust the list view and screenshot button based on GetShowScreenShot() */
void UpdateScreenShot(void)
{
	RECT rect;
	RECT fRect;
	POINT p = {0, 0};

	/* first time through can't do this stuff */
	if (hWndList == NULL)
		return;

	/* Size the List Control in the Picker */
	GetClientRect(hMain, &rect);

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;

	if (bShowToolBar)
		rect.top += topMargin;

	if (GetShowScreenShot())
	{
		CheckMenuItem(GetMenu(hMain),ID_VIEW_PICTURE_AREA, MF_CHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_PICTURE_AREA, MF_CHECKED);
	}
	else
	{
		CheckMenuItem(GetMenu(hMain),ID_VIEW_PICTURE_AREA, MF_UNCHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_PICTURE_AREA, MF_UNCHECKED);
	}

	ResizeTreeAndListViews(false);
	FreeScreenShot();

	if (have_selection)
		LoadScreenShot(Picker_GetSelectedItem(hWndList), TabView_GetCurrentTab(hTabCtrl));

	// figure out if we have a history or not, to place our other windows properly
	UpdateHistory();

	// setup the picture area
	if (GetShowScreenShot())
	{
		ClientToScreen(hMain, &p);
		GetWindowRect(GetDlgItem(hMain, IDC_SSFRAME), &fRect);
		OffsetRect(&fRect, -p.x, -p.y);

		// show history on this tab IF
		// - we have history for the game
		// - we're on the first tab
		// - we DON'T have a separate history tab
		bool showing_history = (have_history && (TabView_GetCurrentTab(hTabCtrl) == GetHistoryTab() || GetHistoryTab() == TAB_ALL ) &&
			GetShowTab(TAB_HISTORY) == false);
		CalculateBestScreenShotRect(GetDlgItem(hMain, IDC_SSFRAME), &rect, showing_history);
		DWORD dwStyle = GetWindowLong(GetDlgItem(hMain, IDC_SSPICTURE), GWL_STYLE);
		DWORD dwStyleEx = GetWindowLong(GetDlgItem(hMain, IDC_SSPICTURE), GWL_EXSTYLE);
		AdjustWindowRectEx(&rect, dwStyle, false, dwStyleEx);
		MoveWindow(GetDlgItem(hMain, IDC_SSPICTURE), fRect.left + rect.left, fRect.top + rect.top, rect.right - rect.left, rect.bottom - rect.top, true);
		ShowWindow(GetDlgItem(hMain, IDC_SSPICTURE), (TabView_GetCurrentTab(hTabCtrl) != TAB_HISTORY) ? SW_SHOW : SW_HIDE);
		ShowWindow(GetDlgItem(hMain, IDC_SSFRAME), SW_SHOW);
		ShowWindow(GetDlgItem(hMain, IDC_SSTAB), bShowTabCtrl ? SW_SHOW : SW_HIDE);
		ShowWindow(GetDlgItem(hMain, IDC_SSBORDER), bShowTabCtrl ? SW_HIDE : SW_SHOW);
		InvalidateRect(GetDlgItem(hMain, IDC_SSBORDER), NULL, false);
		InvalidateRect(GetDlgItem(hMain, IDC_SSTAB), NULL, false);
		InvalidateRect(GetDlgItem(hMain, IDC_SSFRAME), NULL, false);
		InvalidateRect(GetDlgItem(hMain, IDC_SSPICTURE), NULL, false);
	}
	else
	{
		ShowWindow(GetDlgItem(hMain, IDC_SSPICTURE), SW_HIDE);
		ShowWindow(GetDlgItem(hMain, IDC_SSFRAME), SW_HIDE);
		ShowWindow(GetDlgItem(hMain, IDC_SSBORDER), SW_HIDE);
		ShowWindow(GetDlgItem(hMain, IDC_SSTAB), SW_HIDE);
	}
}

void ResizePickerControls(HWND hWnd)
{
	RECT frameRect;
	RECT rect, sRect;
	static bool firstTime = true;
	bool doSSControls = true;
	int nSplitterCount = GetSplitterCount();

	/* Size the List Control in the Picker */
	GetClientRect(hWnd, &rect);

	/* Calc the display sizes based on g_splitterInfo */
	if (firstTime)
	{
		RECT rWindow;

		for (int i = 0; i < nSplitterCount; i++)
			nSplitterOffset[i] = rect.right * g_splitterInfo[i].dPosition;

		GetWindowRect(hStatusBar, &rWindow);
		bottomMargin = rWindow.bottom - rWindow.top;
		GetWindowRect(hToolBar, &rWindow);
		topMargin = rWindow.bottom - rWindow.top;
		firstTime = false;
	}
	else
		doSSControls = GetShowScreenShot();

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;

	if (bShowToolBar)
		rect.top += topMargin;

	MoveWindow(GetDlgItem(hWnd, IDC_DIVIDER), rect.left - 1, rect.top - 3, rect.right + 2, 1, true);
	ResizeTreeAndListViews(true);
	int nListWidth = nSplitterOffset[nSplitterCount - 1];
	int nScreenShotWidth = (rect.right - nListWidth);

	/* Screen shot Page tab control */
	if (bShowTabCtrl)
	{
		MoveWindow(GetDlgItem(hWnd, IDC_SSTAB), nListWidth + 2, rect.top + 3, nScreenShotWidth - 4, (rect.bottom - rect.top) - 7, doSSControls);
		rect.top += 21;
	}
	else
		MoveWindow(GetDlgItem(hWnd, IDC_SSBORDER), nListWidth + 2, rect.top + 3, nScreenShotWidth - 6, (rect.bottom - rect.top) - 8, doSSControls);

	/* resize the Screen shot frame */
	MoveWindow(GetDlgItem(hWnd, IDC_SSFRAME), nListWidth + 3, rect.top + 4, nScreenShotWidth - 8, (rect.bottom - rect.top) - 10, doSSControls);
	/* The screen shot controls */
	GetClientRect(GetDlgItem(hWnd, IDC_SSFRAME), &frameRect);
	/* Text control - game history */
	sRect.left = nListWidth + 12;
	sRect.right = sRect.left + (nScreenShotWidth - 26);

	if (GetShowTab(TAB_HISTORY))
	{
		// We're using the new mode, with the history filling the entire tab (almost)
		sRect.top = rect.top + 16;
		sRect.bottom = (rect.bottom - rect.top) - 30;
	}
	else
	{
		// We're using the original mode, with the history beneath the SS picture
		sRect.top = rect.top + 264;
		sRect.bottom = (rect.bottom - rect.top) - 278;
	}

	MoveWindow(GetDlgItem(hWnd, IDC_HISTORY), sRect.left, sRect.top, sRect.right - sRect.left, sRect.bottom, doSSControls);

	/* the other screen shot controls will be properly placed in UpdateScreenshot() */
}

int GetMinimumScreenShotWindowWidth(void)
{
	BITMAP bmp;

	GetObject(hMissing_bitmap, sizeof(BITMAP), &bmp);
	return bmp.bmWidth + 6; 	// 6 is for a little breathing room
}

const char *funcGetParentName(const char *name)
{
	int index = GetGameNameIndex(name);// get current game index
	int parentindex = GetParentIndex(&driver_list::driver(index));
	//int parentindex = GetParentIndex_2(index);		   // get Parent current game
	if( parentindex >= 0)
	{
		const char *parentname =  GetDriverGameName(parentindex);
		return parentname;
	}

	return NULL;
}

int GetParentIndex(const game_driver *driver)
{
	return GetGameNameIndex(driver->parent);
}

//int GetParentIndex_2(int index)
//{
//	return GetParentIndex(&driver_list::driver(index));
//}

int GetParentRomSetIndex(const game_driver *driver)
{
	int nParentIndex = GetGameNameIndex(driver->parent);

	if(nParentIndex >= 0)
	{
		if ((driver_list::driver(nParentIndex).flags & MACHINE_IS_BIOS_ROOT) == 0)
			return nParentIndex;
	}

	return -1;
}

int GetSrcDriverIndex(const char *name)
{
	srcdriver_data_type *srcdriver_index_info;
	srcdriver_data_type key;
	key.name = name;

	srcdriver_index_info = (srcdriver_data_type *)bsearch(&key, sorted_srcdrivers, driver_list::total(), sizeof(srcdriver_data_type), SrcDriverDataCompareFunc);

	if (srcdriver_index_info == NULL)
		return -1;

	return srcdriver_index_info->index;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static int CLIB_DECL SrcDriverDataCompareFunc(const void *arg1, const void *arg2)
{
	return strcmp(((srcdriver_data_type *)arg1)->name, ((srcdriver_data_type *)arg2)->name);
}

static void SetMainTitle(void)
{
	char buffer[256];

	snprintf(buffer, WINUI_ARRAY_LENGTH(buffer), "%s %s", MAMEUINAME, GetVersionString());
	winui_set_window_text_utf8(hMain, buffer);
}

static void memory_error(const char *message)
{
	ErrorMessageBox(message);
	exit(-1);
}

static void Win32UI_init(void)
{
	RECT rect;
	extern const FOLDERDATA g_folderData[];
	extern const FILTER_ITEM g_filterList[];
	LONG_PTR l;
	struct TabViewOptions opts;

	printf("Win32UI_init: About to init options\n");fflush(stdout);

	/* Init DirectInput */
	DirectInputInitialize();
	OptionsInit();

	printf("Win32UI_init: Options loaded\n");fflush(stdout);

	if (RequiredDriverCache())
		winui_set_window_text_utf8(GetDlgItem(hSplash, IDC_PROGBAR), "Building folders structure...");
	else
		winui_set_window_text_utf8(GetDlgItem(hSplash, IDC_PROGBAR), "Loading folders structure...");
	SendMessage(hProgress, PBM_SETPOS, 10, 0);

#ifdef MAME_AVI
	strcpy_s(last_directory_avi, GetAVIDir());
#endif /* MAME_AVI */

	game_count =  driver_list::total();

	srand((unsigned)time(NULL));
	// create the memory pool
	mameui_pool = pool_alloc_lib(memory_error);
	// custom per-game icons
	icon_index = (int*)pool_malloc_lib(mameui_pool, sizeof(int) * driver_list::total());
	memset(icon_index, 0, sizeof(int) * driver_list::total());
	// sorted list of source drivers by name
	sorted_srcdrivers = (srcdriver_data_type *) pool_malloc_lib(mameui_pool, sizeof(srcdriver_data_type) * driver_list::total());
	memset(sorted_srcdrivers, 0, sizeof(srcdriver_data_type) * driver_list::total());

	for (int i = 0; i < driver_list::total(); i++)
	{
		const char *driver_name = core_strdup(GetDriverFileName(i));
		sorted_srcdrivers[i].name = driver_name;
		sorted_srcdrivers[i].index = i;
		driver_name = NULL;
	}

	qsort(sorted_srcdrivers, driver_list::total(), sizeof(srcdriver_data_type), SrcDriverDataCompareFunc);
	/* initialize tab control */
	memset(&opts, 0, sizeof(opts));
	opts.pCallbacks = &s_tabviewCallbacks;
	opts.nTabCount = MAX_TAB_TYPES;
	SetupTabView(hTabCtrl, &opts);
	/* subclass picture frame area */
	l = GetWindowLongPtr(GetDlgItem(hMain, IDC_SSFRAME), GWLP_WNDPROC);
	g_lpPictureFrameWndProc = (WNDPROC)l;
	SetWindowLongPtr(GetDlgItem(hMain, IDC_SSFRAME), GWLP_WNDPROC, (LONG_PTR)PictureFrameWndProc);
	/* subclass picture area */
	l = GetWindowLongPtr(GetDlgItem(hMain, IDC_SSPICTURE), GWLP_WNDPROC);
	g_lpPictureWndProc = (WNDPROC)l;
	SetWindowLongPtr(GetDlgItem(hMain, IDC_SSPICTURE), GWLP_WNDPROC, (LONG_PTR)PictureWndProc);
	/* Load the pic for the default screenshot. */
	hMissing_bitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SNAPSHOT));
	main_resize_items[0].u.hwnd = hToolBar;
	main_resize_items[1].u.hwnd = hStatusBar;
	GetClientRect(hMain, &rect);
	InitListTree();
	InitSplitters();
	int nSplitterCount = GetSplitterCount();

	for (int i = 0; i < nSplitterCount; i++)
	{
		HWND hWnd = GetDlgItem(hMain, g_splitterInfo[i].nSplitterWindow);
		HWND hWndLeft = GetDlgItem(hMain, g_splitterInfo[i].nLeftWindow);
		HWND hWndRight = GetDlgItem(hMain, g_splitterInfo[i].nRightWindow);
		AddSplitter(hWnd, hWndLeft, hWndRight, g_splitterInfo[i].pfnAdjust);
	}

	/* Initial adjustment of controls on the Picker window */
	ResizePickerControls(hMain);
	TabView_UpdateSelection(hTabCtrl);
	bShowTree = GetShowFolderList();
	bShowToolBar = GetShowToolBar();
	bShowStatusBar = GetShowStatusBar();
	bShowTabCtrl = GetShowTabCtrl();
	bEnableIndent = GetEnableIndent();
	CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
	ToolBar_CheckButton(hToolBar, ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
	ShowWindow(hToolBar, (bShowToolBar) ? SW_SHOW : SW_HIDE);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
	ShowWindow(hStatusBar, (bShowStatusBar) ? SW_SHOW : SW_HIDE);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_PAGETAB, (bShowTabCtrl) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetMenu(hMain), ID_ENABLE_INDENT, (bEnableIndent) ? MF_CHECKED : MF_UNCHECKED);
	ToolBar_CheckButton(hToolBar, ID_ENABLE_INDENT, (bEnableIndent) ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(GetMenu(hMain), ID_KOREAN_GAMELIST, (bklist) ? MF_CHECKED : MF_UNCHECKED); //USE_KLIST
	ToolBar_CheckButton(hToolBar, ID_KOREAN_GAMELIST, (bklist) ? MF_CHECKED : MF_UNCHECKED); //USE_KLIST	
	
	InitTree(g_folderData, g_filterList);
	SendMessage(hProgress, PBM_SETPOS, 112, 0);
	winui_set_window_text_utf8(GetDlgItem(hSplash, IDC_PROGBAR), "Building list structure...");
	/* Initialize listview columns */
	InitListView();
	SendMessage(hProgress, PBM_SETPOS, 120, 0);
	winui_set_window_text_utf8(GetDlgItem(hSplash, IDC_PROGBAR), "Initializing window...");
	ResetFonts();
	AdjustMetrics();
	UpdateScreenShot();
	InitMainMenu(GetMenu(hMain));
	hAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(IDA_TAB_KEYS));

	if (GetJoyGUI() == true)
	{
		g_pJoyGUI = &DIJoystick;

		if (g_pJoyGUI->init() != 0)
			g_pJoyGUI = NULL;
		else
			SetTimer(hMain, JOYGUI_TIMER, JOYGUI_MS, NULL);
	}
	else
		g_pJoyGUI = NULL;

	game_index = 0;
	game_total = driver_list::total();
	oldpercent = -1;
	bDoGameCheck = false;
	bFolderCheck = false;
	idle_work = true;
	bFullScreen = false;

//#ifdef USE_KLIST
	LoadGameListFromFile(game_total);
//#endif

	switch (GetViewMode())
	{
		case VIEW_ICONS_LARGE :
			SetView(ID_VIEW_ICONS_LARGE);
			break;

		case VIEW_ICONS_SMALL :
			SetView(ID_VIEW_ICONS_SMALL);
			break;
		
		case VIEW_INLIST :
			SetView(ID_VIEW_LIST_MENU);
			break;
		
		case VIEW_REPORT :
			SetView(ID_VIEW_DETAIL);
			break;
		
		case VIEW_GROUPED :
		default :
			SetView(ID_VIEW_GROUPED);
			break;
	}

	UpdateListView();
	ShowWindow(hSplash, SW_HIDE);
	CenterWindow(hMain);
	ShowWindow(hMain, GetWindowState());
	SetActiveWindow(hMain);
	SetForegroundWindow(hMain);
	SetFocus(hWndList);

	if (GetCycleScreenshot() > 0)
		SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot() * 1000, NULL);  //scale to Seconds
}

static void Win32UI_exit(void)
{
//#ifdef USE_KLIST
	SaveAllGameListToFile();
//#endif
	SaveWindowStatus();
	ShowWindow(hMain, SW_HIDE);

	if (GetMinimizeTrayIcon())
		Shell_NotifyIcon(NIM_DELETE, &MameIcon);

	if (g_pJoyGUI != NULL)
		g_pJoyGUI->exit();



	DeleteObject(hBrush);
	DeleteObject(hBrushDlg);
	DeleteBitmap(hAboutMenu);
	DeleteBitmap(hCustom);
	DeleteBitmap(hDirectories);
	DeleteBitmap(hExit);
	DeleteBitmap(hFullscreen);
	DeleteBitmap(hInterface);
	DeleteBitmap(hHelp);
	DeleteBitmap(hMameHome);
	DeleteBitmap(hPlay);
	DeleteBitmap(hPlayM1);
	DeleteBitmap(hOptions);
	DeleteBitmap(hRefresh);
	DeleteBitmap(hZip);
	DeleteBitmap(hManual);
	DeleteBitmap(hNotepad);
	DeleteBitmap(hSaveList);
	DeleteBitmap(hSaveRoms);
	DeleteBitmap(hPlayback);
	DeleteBitmap(hProperties);
	DeleteBitmap(hAuditMenu);
	DeleteBitmap(hVideo);
	DeleteBitmap(hFonts);
	DeleteBitmap(hFolders);
	DeleteBitmap(hSort);
	DeleteBitmap(hDriver);
	DeleteBitmap(hFaq);
	DeleteBitmap(hTabs);
	DeleteBitmap(hTrouble);
	DeleteBitmap(hCount);
	DeleteBitmap(hRelease);
	DeleteBitmap(hTime);
	DeleteBitmap(hDescription);
	DeleteBitmap(hRom);
	DeleteBitmap(hSource);
	DeleteBitmap(hManufacturer);
	DeleteBitmap(hYear);
	DeleteBitmap(hPlaywav);
	DeleteBitmap(hFont1);
	DeleteBitmap(hFont2);
	DeleteBitmap(hInfoback);
	DeleteBitmap(hListback);
	DeleteBitmap(hTreeback);
	DeleteBitmap(hAscending);
	DeleteBitmap(hFields);
	DeleteBitmap(hRecavi);
	DeleteBitmap(hRecinput);
	DeleteBitmap(hRecwav);
	DeleteBitmap(hPlaymng);
	DeleteBitmap(hRandom);
	DeleteBitmap(hRecmng);
	DeleteBitmap(hSavestate);
	DeleteBitmap(hFilters);
	DeleteBitmap(hRemove);
	DeleteBitmap(hRename);
	DeleteBitmap(hReset);
	DeleteBitmap(hMissing_bitmap);
	DeleteBitmap(hklist);// USE_KLIST
	DeleteFont(hFontGui);
	DeleteFont(hFontList);
	DeleteFont(hFontHist);
	DeleteFont(hFontTree);
	
	DestroyIcons();
	DestroyAcceleratorTable(hAccel);
	DirectInputClose();
	SetSavedFolderID(GetCurrentFolderID());
	SaveInterface();
	SaveGameList();
	SaveInternalUI();
	SavePlugins();
	SaveGameDefaults();
	FreeFolders();
	FreeScreenShot();
	pool_free_lib(mameui_pool);
	mameui_pool = NULL;
	DestroyWindow(hMain);
}

static LRESULT CALLBACK MameWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_CTLCOLORSTATIC:
			hDC = (HDC)wParam;
			SetBkMode(hDC, TRANSPARENT);

			if ((HWND)lParam == GetDlgItem(hMain, IDC_HISTORY))
				SetTextColor(hDC, GetHistoryFontColor());

			return (LRESULT) hBrushDlg;

// 修改的 代码来源 (缘来是你)
//=============== 缘来是你 ==== DPI ===============>>>
		case WM_CREATE:
		{
			EnableNonClientDpiScalingSafe(hWnd);
			g_uCurrentDpi = GetWindowDpiSafe(hWnd);
			g_fDpiScale = (float)g_uCurrentDpi / 96.0f;
			return FALSE; 
		}
//================================================>>>

		case WM_INITDIALOG:
			/* Initialize info for resizing subitems */
			GetClientRect(hWnd, &main_resize.rect);
			return true;

		case WM_SETFOCUS:
			UpdateWindow(hMain);
			SetFocus(hWndList);
			break;

		case WM_SETTINGCHANGE:
			AdjustMetrics();
			return false;

		case WM_SIZE:
			OnSize(hWnd, wParam, LOWORD(lParam), HIWORD(wParam));
			return true;

// 修改的 代码来源 (缘来是你)
//=============== 缘来是你 ==== DPI ===============>>>
		case WM_DPICHANGED:
		{
			UINT newDpi = HIWORD(wParam);
			if (newDpi == g_uCurrentDpi)
				break;
			g_uCurrentDpi = newDpi;
			g_fDpiScale = (float)newDpi / 96.0f;
			
			ResetFonts();
			
			RECT* rcNew = (RECT*)lParam;
			SetWindowPos(hWnd, NULL,
				rcNew->left, rcNew->top,
				rcNew->right - rcNew->left,
				rcNew->bottom - rcNew->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
			
			InvalidateRect(hWnd, NULL, TRUE);
			return 0;
		}
//================================================>>>

		case MM_PLAY_GAME:
			MamePlayGame();
			return true;

		case WM_INITMENUPOPUP:
			UpdateMenu(GetMenu(hWnd));
			break;

		case WM_CONTEXTMENU:
			if (HandleTreeContextMenu(hWnd, wParam, lParam) || HandleScreenShotContextMenu(hWnd, wParam, lParam))
				return false;
			break;

		case WM_COMMAND:
			return MameCommand(hMain,(int)(LOWORD(wParam)),(HWND)(lParam),(UINT)HIWORD(wParam));

		case WM_GETMINMAXINFO:
		{
			MINMAXINFO *mminfo;
			/* Don't let the window get too small; it can break resizing */
			mminfo = (MINMAXINFO *) lParam;
			mminfo->ptMinTrackSize.x = MIN_WIDTH;
			mminfo->ptMinTrackSize.y = MIN_HEIGHT;
			return false;
		}

		case WM_TIMER:
			switch (wParam)
			{
				case JOYGUI_TIMER:
					PollGUIJoystick();
					break;

				case SCREENSHOT_TIMER:
					TabView_CalculateNextTab(hTabCtrl);
					UpdateScreenShot();
					TabView_UpdateSelection(hTabCtrl);
					break;
			}

			return true;

		case WM_CLOSE:
			if (GetExitDialog())
			{
/*
				if (winui_message_box_utf8(hMain, "정말 종료하시겠습니까?", MAMEUINAME, MB_ICONQUESTION | MB_YESNO) == IDNO)
				{
					SetFocus(hWndList);
					return true;
				}
*/ 
			}

			
			Win32UI_exit();
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			return false;

		case WM_LBUTTONDOWN:
			OnLButtonDown(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
			break;

		case WM_MOUSEMOVE:
			if (g_listview_dragging)
				MouseMoveListViewDrag(MAKEPOINTS(lParam));
			else
				/* for splitters */
				OnMouseMove(hWnd, (UINT)wParam, MAKEPOINTS(lParam));

			break;

		case WM_LBUTTONUP:
			if (g_listview_dragging)
				ButtonUpListViewDrag(MAKEPOINTS(lParam));
			else
				/* for splitters */
				OnLButtonUp(hWnd, (UINT)wParam, MAKEPOINTS(lParam));

			break;

		case WM_NOTIFY:
			/* Where is this message intended to go */
			{
				LPNMHDR lpNmHdr = (LPNMHDR)lParam;
				wchar_t szClass[128];

				/* Fetch tooltip text */
				if (lpNmHdr->code == TTN_NEEDTEXT)
				{
					LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)lParam;
					CopyToolTipText(lpttt);
				}

				if (lpNmHdr->hwndFrom == hTreeView)
					return TreeViewNotify(lpNmHdr);

				GetClassName(lpNmHdr->hwndFrom, szClass, WINUI_ARRAY_LENGTH(szClass));

				if (!_tcscmp(szClass, WC_LISTVIEW))
					return Picker_HandleNotify(lpNmHdr);

				if (!_tcscmp(szClass, WC_TABCONTROL))
					return TabView_HandleNotify(lpNmHdr);

				break;
			}

		case WM_USER:
			if (lParam == WM_LBUTTONDBLCLK)
			{
				Shell_NotifyIcon(NIM_DELETE, &MameIcon);
				ShowWindow(hMain, SW_RESTORE);
				SetActiveWindow(hMain);
				SetForegroundWindow(hMain);
				SetFocus(hWndList);
			}

			break;

		case WM_SYSCOMMAND:
			if (wParam == SC_MINIMIZE)
			{
				if (!IsMaximized(hMain))
					SaveWindowArea();

				if (GetMinimizeTrayIcon())
				{
					ShowWindow(hMain, SW_MINIMIZE);
					ShowWindow(hMain, SW_HIDE);
					Shell_NotifyIcon(NIM_ADD, &MameIcon);
					Shell_NotifyIcon(NIM_SETVERSION, &MameIcon);
				}
			}
			else if (wParam == SC_MAXIMIZE)
				SaveWindowArea();

			break;

	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static void SaveWindowArea(void)
{
	/* save main window size */
	RECT rect;
	AREA area;

	GetWindowRect(hMain, &rect);
	area.x = rect.left;
	area.y = rect.top;
	area.width = rect.right  - rect.left;
	area.height = rect.bottom - rect.top;
	SetWindowArea(&area);
}

static void SaveWindowStatus(void)
{
	WINDOWPLACEMENT wndpl;

	wndpl.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hMain, &wndpl);
	UINT state = wndpl.showCmd;

	if (state == SW_MINIMIZE || state == SW_SHOWMINIMIZED)
		state = SW_RESTORE;
	else if(state == SW_MAXIMIZE)
	{
		state = SW_MAXIMIZE;
		ShowWindow(hMain, SW_RESTORE);
	}
	else
	{
		state = SW_SHOWNORMAL;
		SaveWindowArea();
	}

	SetWindowState(state);

	for (int i = 0; i < GetSplitterCount(); i++)
		SetSplitterPos(i, nSplitterOffset[i]);

	for (int i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
		Picker_SaveColumnWidths(GetDlgItem(hMain, s_nPickers[i]));

	int nItem = Picker_GetSelectedItem(hWndList);
	if (nItem < 0)
		return;
	SetDefaultGame(GetDriverGameName(nItem));
}

static void FolderCheck(void)
{
	int counter = ListView_GetItemCount(hWndList);

	for (int i = 0; i < counter; i++)
	{
		LVITEM lvi;

		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_PARAM;
		(void)ListView_GetItem(hWndList, &lvi);
		SetRomAuditResults(lvi.lParam, UNKNOWN);
	}

	game_index = 0;
	game_total = counter;
	oldpercent = -1;
	bDoGameCheck = false;
	bFolderCheck = true;
	idle_work = true;
	ReloadIcons();
	Picker_ResetIdle(hWndList);
}

static bool GameCheck(void)
{
	LVFINDINFO lvfi;
	int percentage = ((game_index + 1) * 100) / game_total;
	bool changed = false;
	
	AuditRefresh();

	if (game_index == 0)
	{		
		ProgressBarShow();
	}
	if (bFolderCheck == true)
	{
		LVITEM lvi;

		lvi.iItem = game_index;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_PARAM;
		(void)ListView_GetItem(hWndList, &lvi);
		MameUIVerifyRomSet(lvi.lParam, true);
		changed = true;
		lvfi.flags  = LVFI_PARAM;
		lvfi.lParam = lvi.lParam;

		//ErrorMessageBox("bFolderCheck");
	}
	else
	{
#ifdef 	FAST_AUDIT 
		if (GetEnableFastAudit())
		{
			Mame32FastVerifyRomSet(game_index);
		}
		else
		{
			MameUIVerifyRomSet(game_index, true);	
		}
#else	
		MameUIVerifyRomSet(game_index, true);
#endif
		changed = true;
		lvfi.flags  = LVFI_PARAM;
		lvfi.lParam = game_index;
	}

	int i = ListView_FindItem(hWndList, -1, &lvfi);

	if (changed && i != -1)
		(void)ListView_RedrawItems(hWndList, i, i);

	if (percentage != oldpercent)
	{
		SetStatusBarTextW(0, TEXT("Game search %d%% completed"), percentage);
		oldpercent = percentage;
	}

	SendMessage(hProgWnd, PBM_SETPOS, game_index, 0);
	game_index++;

	if (game_index >= game_total)
	{
		bDoGameCheck = false;
		bFolderCheck = false;
		ProgressBarHide();
		ResetWhichGamesInFolders();
		freeRam_fastAudit();					
		return false;		
	}

	return changed;
}

bool OnIdle(HWND hWnd)
{
	static bool bFirstTime = true;
	const char *pDescription;
	const char *pName; 

#ifdef USE_KLIST	// 버그. 게임 실행후 맨 앞으로 가는 문제점이 있음.
	  int		i;
	  LV_FINDINFO lvfi;
#else
	  int driver_index;
#endif  


	if (bFirstTime)
		bFirstTime = false;

	if ((bDoGameCheck) || (bFolderCheck))
	{
		GameCheck();
		return idle_work;
	}

	// in case it's not found, get it back

#ifdef USE_KLIST
	lvfi.flags = LVFI_STRING;
	lvfi.psz   = (LPCWSTR)GetDefaultGame();
	i = ListView_FindItem(hWndList, -1, &lvfi);

	Picker_SetSelectedPick(hWndList,(i != -1) ? i : 0);
	i = Picker_GetSelectedItem(hWndList);
#else	
	driver_index = Picker_GetSelectedItem(hWndList);
#endif

#ifdef USE_KLIST
  	pDescription = GetDescriptionByIndex(i, GetUsekoreanList());
#else	
	pDescription = GetDriverGameTitle(driver_index);
#endif
	SetStatusBarText(0, pDescription);

#ifdef USE_KLIST
	pName = GetGameNameByIndex(i, GetUsekoreanList());
#else
	pName = GetDriverGameName(driver_index);
#endif
	
	SetStatusBarText(1, pName);
	idle_work = false;
	UpdateStatusBar();
	bFirstTime = true;

	if (game_launched)
	{
		game_launched = false;
		return idle_work;
	}

	if (!idle_work)
		UpdateListView();

	return idle_work;
}

static void OnSize(HWND hWnd, UINT nState, int nWidth, int nHeight)
{
	static bool firstTime = true;

	if (nState != SIZE_MAXIMIZED && nState != SIZE_RESTORED)
		return;

	ResizeWindow(hWnd, &main_resize);
	ResizeProgressBar();

	if (firstTime == false)
		OnSizeSplitter(hMain);

	/* Update the splitters structures as appropriate */
	RecalcSplitters();

	if (firstTime == false)
		ResizePickerControls(hMain);

	firstTime = false;
	UpdateScreenShot();
}

static void ResizeWindow(HWND hParent, Resize *r)
{
	int cmkindex = 0;
	HWND hControl = NULL;
	RECT parent_rect, rect;
	POINT p = {0, 0};

	if (hParent == NULL)
		return;

	/* Calculate change in width and height of parent window */
	GetClientRect(hParent, &parent_rect);
	int dy = parent_rect.bottom - r->rect.bottom;
	int dx = parent_rect.right - r->rect.right;
	ClientToScreen(hParent, &p);

	while (r->items[cmkindex].type != RA_END)
	{
		const ResizeItem *ri = &r->items[cmkindex];

		if (ri->type == RA_ID)
			hControl = GetDlgItem(hParent, ri->u.id);
		else
			hControl = ri->u.hwnd;

		if (hControl == NULL)
		{
			cmkindex++;
			continue;
		}

		/* Get control's rectangle relative to parent */
		GetWindowRect(hControl, &rect);
		OffsetRect(&rect, -p.x, -p.y);
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;

		if (!(ri->action & RA_LEFT))
			rect.left += dx;

		if (!(ri->action & RA_TOP))
			rect.top += dy;

		if (ri->action & RA_RIGHT)
			rect.right += dx;

		if (ri->action & RA_BOTTOM)
			rect.bottom += dy;
		//Sanity Check the child rect
		if (parent_rect.top > rect.top)
			rect.top = parent_rect.top;

		if (parent_rect.left > rect.left)
			rect.left = parent_rect.left;

		if (parent_rect.bottom < rect.bottom) 
		{
			rect.bottom = parent_rect.bottom;
			//ensure we have at least a minimal height
			rect.top = rect.bottom - height;

			if (rect.top < parent_rect.top) 
				rect.top = parent_rect.top;
		}

		if (parent_rect.right < rect.right) 
		{
			rect.right = parent_rect.right;
			//ensure we have at least a minimal width
			rect.left = rect.right - width;

			if (rect.left < parent_rect.left) 
				rect.left = parent_rect.left;
		}

		MoveWindow(hControl, rect.left, rect.top, (rect.right - rect.left), (rect.bottom - rect.top), true);

		/* Take care of subcontrols, if appropriate */
		if (ri->subwindow != NULL)
			ResizeWindow(hControl, (Resize*)ri->subwindow);

		cmkindex++;
	}

	/* Record parent window's new location */
	memcpy(&r->rect, &parent_rect, sizeof(RECT));
}

static void ProgressBarShow()
{
	RECT rect;
	int widths[2] = {160, -1};

	SendMessage(hStatusBar, SB_SETPARTS, 2, (LPARAM)widths);
	SendMessage(hProgWnd, PBM_SETRANGE, 0, MAKELPARAM(0, game_total));
	SendMessage(hProgWnd, PBM_SETPOS, 0, 0);
	StatusBar_GetItemRect(hStatusBar, 1, &rect);
	MoveWindow(hProgWnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);
	ShowWindow(hProgWnd, SW_SHOW);
	bProgressShown = true;
}

static void ProgressBarHide()
{
	RECT rect;
	int widths[6];
	int numParts = 6;

	if (hProgWnd == NULL)
		return;

	ShowWindow(hProgWnd, SW_HIDE);
	widths[5] = 96;
	widths[4] = 80;
	widths[3] = 160;
	widths[2] = 120;
	widths[1] = 88;
	widths[0] = -1;
	SendMessage(hStatusBar, SB_SETPARTS, 1, (LPARAM)widths);
	StatusBar_GetItemRect(hStatusBar, 0, &rect);
	widths[0] = (rect.right - rect.left) - (widths[1] + widths[2] + widths[3] + widths[4] + widths[5]);
	widths[1] += widths[0];
	widths[2] += widths[1];
	widths[3] += widths[2];
	widths[4] += widths[3];
	widths[5] += widths[4];
	SendMessage(hStatusBar, SB_SETPARTS, numParts, (LPARAM)widths);
	UpdateStatusBar();
	bProgressShown = false;
}

static void ResizeProgressBar()
{
	if (bProgressShown)
	{
		RECT rect;
		int  widths[2] = {160, -1};

		SendMessage(hStatusBar, SB_SETPARTS, 2, (LPARAM)widths);
		StatusBar_GetItemRect(hStatusBar, 1, &rect);
		MoveWindow(hProgWnd, rect.left, rect.top, rect.right  - rect.left, rect.bottom - rect.top, true);
	}
	else
		ProgressBarHide();
}

static void InitProgressBar(void)
{
	RECT rect;

	StatusBar_GetItemRect(hStatusBar, 0, &rect);
	rect.left += 160;
	hProgWnd = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_CLIPSIBLINGS, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hStatusBar, NULL, hInst, NULL);
	SetWindowTheme(hProgWnd, L" ", L" ");
	SendMessage(hProgWnd, PBM_SETBARCOLOR, 0, RGB(34, 177, 76));
}

static void InitMenuIcons(void)
{
	HBITMAP hTemp = NULL;

	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ABOUTMENU));
	hAboutMenu = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CUSTOM));
	hCustom = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_DIRECTORIES));
	hDirectories = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_EXIT));
	hExit = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FULLSCREEN));
	hFullscreen = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_INTERFACE));
	hInterface = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_HELP));
	hHelp = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_MAMEHOME));
	hMameHome = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_PLAY));
	hPlay = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_PLAYM1));
	hPlayM1 = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_OPTIONS));
	hOptions = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_REFRESH));
	hRefresh = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ZIP));
	hZip = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_MANUAL));
	hManual = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_NOTEPAD));
	hNotepad = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_SAVELIST));
	hSaveList = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_SAVEROMS));
	hSaveRoms = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_PLAYBACK));
	hPlayback = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_PROPERTIES));
	hProperties = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AUDIT));
	hAuditMenu = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_VIDEO));
	hVideo = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FONTS));
	hFonts = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FOLDERS));
	hFolders = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_SORT));
	hSort = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_DRIVER));
	hDriver = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FAQ));
	hFaq = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TABS));
	hTabs = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TROUBLE));
	hTrouble = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_COUNT));
	hCount = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RELEASE));
	hRelease = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TIME));
	hTime = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_DESCRIPTION));
	hDescription = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ROM));
	hRom = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_SOURCE));
	hSource = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_MANUFACTURER));
	hManufacturer = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_YEAR));
	hYear = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_PLAYWAV));
	hPlaywav = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FONT1));
	hFont1 = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FONT2));
	hFont2 = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_INFOBACK));
	hInfoback = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LISTBACK));
	hListback = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TREEBACK));
	hTreeback = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ASCENDING));
	hAscending = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FIELDS));
	hFields = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RECAVI));
	hRecavi = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RECINPUT));
	hRecinput = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RECWAV));
	hRecwav = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_PLAYMNG));
	hPlaymng = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RANDOM));
	hRandom = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RECMNG));
	hRecmng = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_SAVESTATE));
	hSavestate = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FILTERS));
	hFilters = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_REMOVE));
	hRemove = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RENAME));
	hRename = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RESET));
	hReset = CreateBitmapTransparent(hTemp);
	hTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_KLIST)); // USE_KLIST
	hklist = CreateBitmapTransparent(hTemp);
}

static void CopyToolTipText(LPTOOLTIPTEXT lpttt)
{
	int iButton = lpttt->hdr.idFrom;
	static wchar_t t_s[80];
	bool bConverted = false;

	/* Map command ID to string index */
	for (int i = 0; CommandToString[i] != -1; i++)
	{
		if (CommandToString[i] == iButton)
		{
			iButton = i;
			bConverted = true;
			break;
		}
	}

	if (bConverted)
	{
		/* Check for valid parameter */
		if (iButton > NUM_TOOLTIPS)
			_tcscpy(t_s, TEXT("Invalid button index"));
		else
			_tcscpy(t_s, szTbStrings[iButton]);
	}
	else
	{
		int game = Picker_GetSelectedItem(hWndList);
		if (game < 0)
			_tcscpy(t_s, TEXT("No Selection"));
		else
			_tcscpy(t_s, win_wstring_from_utf8(GetDriverGameTitle(game)));
	}

	lpttt->lpszText = t_s;
}

static void InitToolbar(void)
{
	RECT rect;

	hToolBar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS, 0, 0, NUM_TOOLBUTTONS * 32, 32, hMain, NULL, hInst, NULL);
	HBITMAP hBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_TOOLBAR), IMAGE_BITMAP, 0, 0, LR_SHARED);
	HIMAGELIST hToolList = ImageList_Create(32, 32, ILC_COLORDDB | ILC_MASK, NUM_TOOLBUTTONS, 0);
	ImageList_AddMasked(hToolList, hBitmap, RGB(0, 0, 0));
	DeleteObject(hBitmap);
	SendMessage(hToolBar, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DOUBLEBUFFER);
	SendMessage(hToolBar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessage(hToolBar, TB_SETIMAGELIST, 0, (LPARAM)hToolList);
	SendMessage(hToolBar, TB_ADDBUTTONS, NUM_TOOLBUTTONS, (LPARAM)&tbb);
	SendMessage(hToolBar, TB_AUTOSIZE, 0, 0);
	// get Edit Control position
	int idx = SendMessage(hToolBar, TB_BUTTONCOUNT, 0, 0) - 1;
	SendMessage(hToolBar, TB_GETITEMRECT, idx, (LPARAM)&rect);
	int iPosX = rect.right + 8;
	int iPosY = (rect.bottom - rect.top) / 4;
	int iHeight = rect.bottom - rect.top - 17;
	// create Search Edit Control
	hSearchWnd = CreateWindowEx(0, WC_EDIT, TEXT(SEARCH_PROMPT), ES_LEFT | WS_CHILD | WS_CLIPSIBLINGS | WS_BORDER | WS_VISIBLE, iPosX, iPosY, 200, iHeight, hToolBar, (HMENU)ID_TOOLBAR_EDIT, hInst, NULL );
}

static void InitTabView(void)
{
	hTabCtrl = CreateWindowEx(0, WC_TABCONTROL, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | WS_TABSTOP | TCS_HOTTRACK, 0, 0, 0, 0, hMain, (HMENU)IDC_SSTAB, hInst, NULL);
}

static void InitStatusBar(void)
{
	hStatusBar = CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBARS_SIZEGRIP | SBARS_TOOLTIPS, 0, 0, 0, 0, hMain, NULL, hInst, NULL);
}

static char *GameInfoStatusBar(int driver_index)
{
	static char status[64];

	memset(&status, 0, sizeof(status));

	if (DriverIsBroken(driver_index))
		return strcpy(status, "Not working");
	else if (DriverIsImperfect(driver_index))
		return strcpy(status, "Working with problems");
	else
		return strcpy(status, "Working");
}

static char *GameInfoScreen(int driver_index)
{
	machine_config config(driver_list::driver(driver_index), MameUIGlobal());
	static char scrtxt[256];

	memset(&scrtxt, 0, sizeof(scrtxt));

	if (DriverIsVector(driver_index))
	{
		if (DriverIsVertical(driver_index))
			strcpy(scrtxt, "Vector (V)");
		else
			strcpy(scrtxt, "Vector (H)");
	}
	else
	{
		const screen_device *screen = screen_device_iterator(config.root_device()).first();

		if (screen == nullptr)
			strcpy(scrtxt, "Screenless");
		else
		{
			const rectangle &visarea = screen->visible_area();
			char tmpbuf[256];

			if (DriverIsVertical(driver_index))
				snprintf(tmpbuf, WINUI_ARRAY_LENGTH(tmpbuf), "%d x %d (V) %f Hz", visarea.width(), visarea.height(), ATTOSECONDS_TO_HZ(screen->refresh_attoseconds()));
			else
				snprintf(tmpbuf, WINUI_ARRAY_LENGTH(tmpbuf), "%d x %d (H) %f Hz", visarea.width(), visarea.height(), ATTOSECONDS_TO_HZ(screen->refresh_attoseconds()));

			strcat(scrtxt, tmpbuf);
		}
	}

	return scrtxt;
}

static void UpdateStatusBar(void)
{
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	int games_shown = 0;
	int i = -1;
	HICON hIconFX = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAMEUI_ICON), IMAGE_ICON, 16, 16, LR_SHARED);

	if (!lpFolder)
		return;

	while (1)
	{
		i = FindGame(lpFolder, i + 1);

		if (i == -1)
			break;

		if (!GameFiltered(i, lpFolder->m_dwFlags))
			games_shown++;
	}

	/* Show number of games in the current 'View' in the status bar */
	SetStatusBarTextF(4, g_szGameCountString, games_shown);
	i = Picker_GetSelectedItem(hWndList);

	if ((games_shown == 0) || (i < 0))
		DisableSelection();
	else
	{
		const char *pText = GetDriverGameTitle(i);
		char *pStatus = GameInfoStatusBar(i);
		char *pScreen = GameInfoScreen(i);
		const char *pName = GetDriverGameName(i);
		SetStatusBarText(0, pText);
		SetStatusBarText(1, pName);
		SendMessage(hStatusBar, SB_SETICON, 1, (LPARAM)GetSelectedPickItemIconSmall());
		SetStatusBarText(2, pStatus);
		SetStatusBarText(3, pScreen);
		SetStatusBarText(5, MAMEUINAME);
		SendMessage(hStatusBar, SB_SETICON, 5, (LPARAM)hIconFX);
	}
}

// 修改的 代码来源 (缘来是你)
//============================== 缘来是你 ===========================>>>
//DPI 修改
static void ResetFonts(void)
{
	LOGFONT font;
	
	if (!g_fontPointsInitialized)
	{
		int refDpi = 96; // 参考 DPI
		GetGuiFont(&font);
		g_guiPointSize = MulDiv(-font.lfHeight, 72, refDpi);
		GetListFont(&font);
		g_listPointSize = MulDiv(-font.lfHeight, 72, refDpi);
		GetHistoryFont(&font);
		g_histPointSize = MulDiv(-font.lfHeight, 72, refDpi);
		GetTreeFont(&font);
		g_treePointSize = MulDiv(-font.lfHeight, 72, refDpi);
		g_fontPointsInitialized = true;
	}
	
	int guiHeight = -MulDiv(g_guiPointSize, g_uCurrentDpi, 72);
	int listHeight = -MulDiv(g_listPointSize, g_uCurrentDpi, 72);
	int histHeight = -MulDiv(g_histPointSize, g_uCurrentDpi, 72);
	int treeHeight = -MulDiv(g_treePointSize, g_uCurrentDpi, 72);
	
	if (hFontGui) DeleteObject(hFontGui);
	if (hFontList) DeleteObject(hFontList);
	if (hFontHist) DeleteObject(hFontHist);
	if (hFontTree) DeleteObject(hFontTree);
	
	GetGuiFont(&font);
	font.lfHeight = guiHeight;
	hFontGui = CreateFontIndirect(&font);
	
	GetListFont(&font);
	font.lfHeight = listHeight;
	hFontList = CreateFontIndirect(&font);
	
	GetHistoryFont(&font);
	font.lfHeight = histHeight;
	hFontHist = CreateFontIndirect(&font);
	
	GetTreeFont(&font);
	font.lfHeight = treeHeight;
	hFontTree = CreateFontIndirect(&font);
	
	if (hFontGui)
	{
		SetWindowFont(hSearchWnd, hFontGui, TRUE);
		SetWindowFont(hTabCtrl, hFontGui, TRUE);
		SetWindowFont(hStatusBar, hFontGui, TRUE);
	}
	if (hFontList)
		SetWindowFont(hWndList, hFontList, TRUE);
	if (hFontHist)
		SetWindowFont(GetDlgItem(hMain, IDC_HISTORY), hFontHist, TRUE);
	if (hFontTree)
		SetWindowFont(hTreeView, hFontTree, TRUE);
	if (hWndList) {
		ListView_SetTextColor(hWndList, GetListFontColor());
		ListView_SetBkColor(hWndList, GetListBgColor());
	}
}
//=============================================================>>>

static void InitListTree(void)
{
	hTreeView = GetDlgItem(hMain, IDC_TREE);
	hWndList = GetDlgItem(hMain, IDC_LIST);
	SetWindowTheme(hWndList, L"Explorer", NULL);
	SetWindowTheme(hTreeView, L"Explorer", NULL);

	if (IsWindowsSevenOrHigher())
	{
		(void)ListView_SetExtendedListViewStyle(hWndList, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_LABELTIP | LVS_EX_ONECLICKACTIVATE | LVS_EX_DOUBLEBUFFER);
		SendMessage(hTreeView, TVM_SETEXTENDEDSTYLE, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
	}
	else
		(void)ListView_SetExtendedListViewStyle(hWndList, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_LABELTIP | LVS_EX_UNDERLINEHOT | LVS_EX_ONECLICKACTIVATE | LVS_EX_DOUBLEBUFFER);
}

static void UpdateHistory(void)
{
	have_history = false;

	hBrushDlg = CreateSolidBrush(GetHistoryBgColor());

	if (GetSelectedPick() >= 0)
	{
		char *histText = GetGameHistory(Picker_GetSelectedItem(hWndList));
		have_history = (histText && histText[0]) ? true : false;
		winui_set_window_text_utf8(GetDlgItem(hMain, IDC_HISTORY), histText);
	}

	if (have_history && GetShowScreenShot() && ((TabView_GetCurrentTab(hTabCtrl) == TAB_HISTORY) ||
		(TabView_GetCurrentTab(hTabCtrl) == GetHistoryTab() && GetShowTab(TAB_HISTORY) == false) ||
		(TAB_ALL == GetHistoryTab() && GetShowTab(TAB_HISTORY) == false) ))
		ShowWindow(GetDlgItem(hMain, IDC_HISTORY), SW_SHOW);
	else
		ShowWindow(GetDlgItem(hMain, IDC_HISTORY), SW_HIDE);
}

static void DisableSelection(void)
{
	MENUITEMINFO mmi;
	HMENU hMenu = GetMenu(hMain);
	bool prev_have_selection = have_selection;

	mmi.cbSize = sizeof(mmi);
	mmi.fMask = MIIM_TYPE;
	mmi.fType = MFT_STRING;
	mmi.dwTypeData = (wchar_t *)TEXT("Play\tAlt+O");
	mmi.cch = _tcslen(mmi.dwTypeData);

	SetMenuItemInfo(hMenu, ID_FILE_PLAY, false, &mmi);
	EnableMenuItem(hMenu, ID_FILE_PLAY, MF_GRAYED);
	EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD, MF_GRAYED);
	EnableMenuItem(hMenu, ID_GAME_PROPERTIES, MF_GRAYED);
	SetStatusBarText(0, "No selection");
	SetStatusBarText(1, "");
	SendMessage(hStatusBar, SB_SETICON, 1, (LPARAM)NULL);
	SetStatusBarText(2, "");
	SetStatusBarText(3, "");
	SetStatusBarText(4, "");

#ifdef MAME_AVI
    EnableMenuItem(hMenu, ID_FILE_PLAY_BACK_AVI,   MF_GRAYED);
	EnableMenuItem(hMenu, ID_FILE_PLAY_WITH_AVI,   MF_GRAYED);
#endif /* MAME_AVI */

	have_selection = false;

	if (prev_have_selection != have_selection)
		UpdateScreenShot();
}

bool IsFileWritable(const WCHAR* pszFilePath)  
{  
	HANDLE hFile = INVALID_HANDLE_VALUE;  

	hFile = CreateFile(pszFilePath, GENERIC_READ|GENERIC_WRITE,0, NULL, OPEN_EXISTING, 0, NULL);  
	if (hFile == INVALID_HANDLE_VALUE)  
	{  
		return FALSE;  
	}  

	CloseHandle(hFile);  

	return TRUE;  
}  

static void EnableSelection(int nGame)
{
	wchar_t buf[200];
	MENUITEMINFO mmi;
	HMENU hMenu = GetMenu(hMain);
	wchar_t *t_description = win_wstring_from_utf8(ConvertAmpersandString(GetDriverGameTitle(nGame)));
	const char *pText;
	const char *pName;

	if( !t_description )
		return;

	_sntprintf(buf, WINUI_ARRAY_LENGTH(buf), g_szPlayGameString, t_description);

	mmi.cbSize = sizeof(mmi);
	mmi.fMask = MIIM_TYPE;
	mmi.fType = MFT_STRING;
	mmi.dwTypeData = buf;
	mmi.cch = _tcslen(mmi.dwTypeData);

	SetMenuItemInfo(hMenu, ID_FILE_PLAY, false, &mmi);

//#ifdef USE_KLIST
	pText = GetDescriptionByIndex(nGame, GetUsekoreanList());
//#else	
//	pText = GetDriverGameTitle(nGame);
//#endif
	SetStatusBarText(0, pText);

//#ifdef USE_KLIST
	pName = GetGameNameByIndex(nGame,GetUsekoreanList());
//#else
//	pName = GetDriverGameName(nGame);
//#endif

	SetStatusBarText(1, pName);
	SendMessage(hStatusBar, SB_SETICON, 1, (LPARAM)GetSelectedPickItemIconSmall());
	char *pStatus = GameInfoStatusBar(nGame);
	SetStatusBarText(2, pStatus);
	char *pScreen = GameInfoScreen(nGame);
	SetStatusBarText(3, pScreen);
	EnableMenuItem(hMenu, ID_FILE_PLAY, 		MF_ENABLED);
	EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD,	MF_ENABLED);
	EnableMenuItem(hMenu, ID_GAME_PROPERTIES, 	MF_ENABLED);

	if (bProgressShown && bListReady == true)
//#ifdef USE_KLIST
		SetDefaultGame(GetGameNameByIndex(nGame, GetUsekoreanList()));
//#else
//		SetDefaultGame(GetDriverGameName(nGame));
//#endif
	have_selection = true;
	UpdateScreenShot();

// preview video start
	if(IsFileWritable(win_wstring_from_utf8(Preinfofilename)))
	{
		char path[MAX_PATH];

		FILE *f = fopen(Preinfofilename, "w");

		if(f != NULL)
		{
			snprintf(path, WINUI_ARRAY_LENGTH(path), "%s\\%s.mp4", GetVideoDir(), GetDriverGameName(nGame));
			fprintf(f,"%s\n",path );
			snprintf(path, WINUI_ARRAY_LENGTH(path), "%s\\%s.mp4", GetVideoDir(), GetParentDriverGameName(nGame));
			fprintf(f,"%s\n",path );
		}
		
		fclose(f);
	}
// preview video end


	free(t_description);
}

static const char* GetParentDriverGameName(int nItem)
{
	if (DriverIsClone(nItem))
	{
		int nParentIndex = GetParentIndex(&driver_list::driver(nItem));

		if( nParentIndex >= 0)
			return (char*)GetDriverGameName(nParentIndex);
	}

	return "";
}

static const char* GetCloneParentName(int nItem)
{
	if (DriverIsClone(nItem))
	{
		int nParentIndex = GetParentIndex(&driver_list::driver(nItem));

		if( nParentIndex >= 0)
//#ifdef USE_KLIST			
			return (char*)GetDescriptionByIndex(nParentIndex,GetUsekoreanList());
//#else
//			return GetDriverGameTitle(nParentIndex);
//#endif
	}

	return "";
}

static bool TreeViewNotify(LPNMHDR nm)
{
	switch (nm->code)
	{
		case TVN_SELCHANGED :
		{
			HTREEITEM hti = TreeView_GetSelection(hTreeView);
			TVITEM tvi;

			tvi.mask = TVIF_PARAM | TVIF_HANDLE;
			tvi.hItem = hti;

			if (TreeView_GetItem(hTreeView, &tvi))
			{
				SetCurrentFolder((LPTREEFOLDER)tvi.lParam);

				if (bListReady)
				{
					UpdateListView();
					UpdateScreenShot();
					SetFocus(hTreeView);
				}
			}

			return true;
		}

		case TVN_BEGINLABELEDIT :
		{
			TV_DISPINFO *ptvdi = (TV_DISPINFO *)nm;
			LPTREEFOLDER folder = (LPTREEFOLDER)ptvdi->item.lParam;

			if (folder->m_dwFlags & F_CUSTOM)
			{
				// user can edit custom folder names
				g_in_treeview_edit = true;
				return false;
			}

			// user can't edit built in folder names
			return true;
		}

		case TVN_ENDLABELEDIT :
		{
			TV_DISPINFO *ptvdi = (TV_DISPINFO *)nm;
			LPTREEFOLDER folder = (LPTREEFOLDER)ptvdi->item.lParam;

			g_in_treeview_edit = false;

			if (ptvdi->item.pszText == NULL || _tcslen(ptvdi->item.pszText) == 0)
				return false;

			char *utf8_szText = win_utf8_from_wstring(ptvdi->item.pszText);

			if (!utf8_szText)
				return false;

			bool result = TryRenameCustomFolder(folder, utf8_szText);
			free(utf8_szText);
			return result;
		}
	}

	return false;
}

void GamePicker_OnHeaderContextMenu(POINT pt, int nColumn)
{
	// Right button was clicked on header
	MENUINFO mi;
	HMENU hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_HEADER));
	HMENU hMenu = GetSubMenu(hMenuLoad, 0);

	memset(&mi, 0, sizeof(MENUINFO));
	mi.cbSize = sizeof(MENUINFO);
	mi.fMask = MIM_BACKGROUND | MIM_STYLE;
	mi.dwStyle = MNS_CHECKORBMP;
	mi.hbrBack = GetSysColorBrush(COLOR_WINDOW);

	SetMenuInfo(hMenu, &mi);
	SetMenuItemBitmaps(hMenu, ID_SORT_ASCENDING, MF_BYCOMMAND, hAscending, hAscending);
	SetMenuItemBitmaps(hMenu, ID_SORT_DESCENDING, MF_BYCOMMAND, hSort, hSort);
	SetMenuItemBitmaps(hMenu, ID_CUSTOMIZE_FIELDS, MF_BYCOMMAND, hFields, hFields);
	lastColumnClick = nColumn;
	TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hMain, NULL);
	DestroyMenu(hMenuLoad);
}

static char* ConvertAmpersandString(const char *s)
{
	/* takes a string and changes any ampersands to double ampersands,
       for setting text of window controls that don't allow us to disable
       the ampersand underlining.
      */
	/* returns a static buffer--use before calling again */

	static char buf[200];

	char *ptr = buf;

	while (*s)
	{
		if (*s == '&')
			*ptr++ = *s;

		*ptr++ = *s++;
	}

	*ptr = 0;

	return buf;
}


static void PollGUIJoystick()
{
	if (in_emulation)
	{
		return;
	}
	if (g_pJoyGUI == NULL)
	{
		return;
	}

	// FILE *pFile;
	// pFile = fopen("PollGUIJoystick.txt", "a");
	// fprintf(pFile, "Joy UP Code %d :",JOYCODE(GetUIJoyUp(0), GetUIJoyUp(1), GetUIJoyUp(2), GetUIJoyUp(3)));
	// fprintf(pFile, "result %d \n",g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyUp(0), GetUIJoyUp(1), GetUIJoyUp(2), GetUIJoyUp(3))));

	// fprintf(pFile, "Joy down Code %d :",JOYCODE(GetUIJoyDown(0), GetUIJoyDown(1), GetUIJoyDown(2), GetUIJoyDown(3)));
	// fprintf(pFile, "result %d \n",g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyDown(0), GetUIJoyDown(1), GetUIJoyDown(2), GetUIJoyDown(3))));

	// fprintf(pFile, "Joy left Code %d :",JOYCODE(GetUIJoyLeft(0), GetUIJoyLeft(1), GetUIJoyLeft(2), GetUIJoyLeft(3)));
	// fprintf(pFile, "result %d \n",g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyLeft(0), GetUIJoyLeft(1), GetUIJoyLeft(2), GetUIJoyLeft(3))));

	// fprintf(pFile, "Joy rightCode  %d :",JOYCODE(GetUIJoyRight(0), GetUIJoyRight(1), GetUIJoyRight(2), GetUIJoyRight(3)));
	// fprintf(pFile, "result %d \n",g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyRight(0), GetUIJoyRight(1), GetUIJoyRight(2), GetUIJoyRight(3))));
	// fclose(pFile);

	g_pJoyGUI->poll_joysticks();

	// User pressed UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyUp(0), GetUIJoyUp(1), GetUIJoyUp(2), GetUIJoyUp(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_UP, 0);

	// User pressed DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyDown(0), GetUIJoyDown(1), GetUIJoyDown(2), GetUIJoyDown(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_DOWN, 0);

	// User pressed LEFT
	// if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyLeft(0), GetUIJoyLeft(1), GetUIJoyLeft(2), GetUIJoyLeft(3))))
	// 	SendMessage(hMain, WM_COMMAND, ID_UI_LEFT, 0);

	// // User pressed RIGHT
	// if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyRight(0), GetUIJoyRight(1), GetUIJoyRight(2), GetUIJoyRight(3))))
	// 	SendMessage(hMain, WM_COMMAND, ID_UI_RIGHT, 0);

	// User pressed START GAME
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyStart(0), GetUIJoyStart(1), GetUIJoyStart(2), GetUIJoyStart(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_START, 0);

	// User pressed PAGE UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyPageUp(0), GetUIJoyPageUp(1), GetUIJoyPageUp(2), GetUIJoyPageUp(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_PGUP, 0);

	// User pressed PAGE DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyPageDown(0), GetUIJoyPageDown(1), GetUIJoyPageDown(2), GetUIJoyPageDown(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_PGDOWN, 0);

	// User pressed HOME
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHome(0), GetUIJoyHome(1), GetUIJoyHome(2), GetUIJoyHome(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_HOME, 0);

	// User pressed END
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyEnd(0), GetUIJoyEnd(1), GetUIJoyEnd(2), GetUIJoyEnd(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_END, 0);

	// User pressed CHANGE SCREENSHOT
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoySSChange(0), GetUIJoySSChange(1), GetUIJoySSChange(2), GetUIJoySSChange(3))))
		SendMessage(hMain, WM_COMMAND, IDC_SSFRAME, 0);

	// User pressed SCROLL HISTORY UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHistoryUp(0), GetUIJoyHistoryUp(1), GetUIJoyHistoryUp(2), GetUIJoyHistoryUp(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_HISTORY_UP, 0);

	// User pressed SCROLL HISTORY DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHistoryDown(0), GetUIJoyHistoryDown(1), GetUIJoyHistoryDown(2), GetUIJoyHistoryDown(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_HISTORY_DOWN, 0);
}

static void SetView(int menu_id)
{
	BOOL force_reset = false;

	// first uncheck previous menu item, check new one
	CheckMenuRadioItem(GetMenu(hMain), ID_VIEW_ICONS_LARGE, ID_ENABLE_INDENT, menu_id, MF_CHECKED);
	ToolBar_CheckButton(hToolBar, menu_id, MF_CHECKED);

//	if (menu_id == ID_VIEW_DETAIL)
//		(void)ListView_SetImageList(hWndList, hLarge, LVSIL_SMALL);

	// Associate the image lists with the list view control.
//	if (menu_id == ID_VIEW_ICONS_LARGE)
//		(void)ListView_SetImageList(hWndList, hLarge, LVSIL_SMALL);
//	else
//		(void)ListView_SetImageList(hWndList, hSmall, LVSIL_SMALL);

	if (Picker_GetViewID(hWndList) == VIEW_GROUPED || menu_id == ID_VIEW_GROUPED)
	{
		// this changes the sort order, so redo everything
		force_reset = true;
	}

	for (int i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
		Picker_SetViewID(GetDlgItem(hMain, s_nPickers[i]), menu_id - ID_VIEW_ICONS_LARGE);

	if(force_reset)
	{
		for (int i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
			Picker_Sort(GetDlgItem(hMain, s_nPickers[i]));
	}
}

static void ResetListView()
{
	int i = 0;
	LVITEM lvi;
	bool no_selection = false;
	LPTREEFOLDER lpFolder = GetCurrentFolder();

	if (!lpFolder)
		return;
 
	/* If the last folder was empty, no_selection is true */
	if (have_selection == false)
		no_selection = true;
 
	int current_game = Picker_GetSelectedItem(hWndList);
	if (current_game < 0)
		no_selection = true;
	SetWindowRedraw(hWndList, false);
	(void)ListView_DeleteAllItems(hWndList);
	// hint to have it allocate it all at once
	ListView_SetItemCount(hWndList, driver_list::total());

	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_INDENT;
	lvi.stateMask = 0;

	i = -1;

	do
	{
		/* Add the games that are in this folder */
		if ((i = FindGame(lpFolder, i + 1)) != -1)
		{
			if (GameFiltered(i, lpFolder->m_dwFlags))
				continue;

			lvi.iItem = i;
			lvi.iSubItem = 0;
			lvi.lParam = i;
			lvi.pszText = LPSTR_TEXTCALLBACK;
			lvi.iImage = I_IMAGECALLBACK;
			lvi.iIndent = 0;

			if (GetEnableIndent())
			{
				if (GetParentFound(i) && DriverIsClone(i))
					lvi.iIndent = 1;
				else
					lvi.iIndent = 0;
			}

			(void)ListView_InsertItem(hWndList, &lvi);
		}
	} while (i != -1);

	Picker_Sort(hWndList);

	if (bListReady)
	{
	/* If last folder was empty, select the first item in this folder */
		if (no_selection)
			Picker_SetSelectedPick(hWndList, 0);
		else
			Picker_SetSelectedItem(hWndList, current_game);
	}

	/*RS Instead of the Arrange Call that was here previously on all Views
         We now need to set the ViewMode for SmallIcon again,
         for an explanation why, see SetView*/
	if (GetViewMode() == VIEW_ICONS_SMALL)
		SetView(ID_VIEW_ICONS_SMALL);
	

	SetWindowRedraw(hWndList, true);
	UpdateStatusBar();
}

static void UpdateGameList(void)
{
	for (int i = 0; i < driver_list::total(); i++)
	{
		SetRomAuditResults(i, UNKNOWN);
	}

	game_index = 0;
	game_total = driver_list::total();
	oldpercent = -1;
	bDoGameCheck = true;
	bFolderCheck = false;
	idle_work = true;
	ReloadIcons();
	Picker_ResetIdle(hWndList);

//#ifdef USE_KLIST
	SetDefaultGame(GetDescriptionByIndex(Picker_GetSelectedItem(hWndList), GetUsekoreanList()));
	/*SetSelectedPick(0);*/ /* To avoid flickering. */	
//#endif		
}

static uintptr_t CALLBACK HookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			CenterWindow(hDlg);
			hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAMEUI_ICON));
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			hBrush = CreateSolidBrush(RGB(240, 240, 240));
			if (bHookFont)
				winui_set_window_text_utf8(hDlg, "Choose a font");
			else
				winui_set_window_text_utf8(hDlg, "Choose a color");
			break;

		case WM_CTLCOLORDLG:
			return (LRESULT) hBrush;

		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORBTN:
			hDC = (HDC)wParam;
			SetBkMode(hDC, TRANSPARENT);
			return (LRESULT) hBrush;

		case WM_COMMAND:
			switch (GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK:
					bChangedHook = true;
					break;

				case IDCANCEL:
					bChangedHook = false;
					break;
			}

			break;

		case WM_DESTROY:
			DestroyIcon(hIcon);
			DeleteObject(hBrush);
			return true;
	}

	return false;
}

static void PickFont(LOGFONT *font, COLORREF *color)
{
	CHOOSEFONT cf;
	bChangedHook = false;
	bHookFont = true;

	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner = hMain;
	cf.lpLogFont = font;
	cf.lpfnHook = &HookProc;
	cf.rgbColors = *color;
	cf.Flags = CF_BOTH | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS | CF_ENABLEHOOK;

	if (!ChooseFont(&cf))
		return;

	*color = cf.rgbColors;
}

static void PickListFont(void)
{
	LOGFONT FontList;

	GetListFont(&FontList); 
	COLORREF ColorList = GetListFontColor();
	PickFont(&FontList, &ColorList);

	if (bChangedHook)
	{
		SetListFont(&FontList);
		SetListFontColor(ColorList);

		if (hFontList != NULL)
			DeleteFont(hFontList);

		hFontList = CreateFontIndirect(&FontList);

		if (hFontList != NULL)
		{
			SetWindowFont(hWndList, hFontList, true);
			(void)ListView_SetTextColor(hWndList, ColorList);
			UpdateListView();
		}
	}
}

static void PickHistoryFont(void)
{
	LOGFONT FontHist;

	GetHistoryFont(&FontHist); 
	COLORREF ColorHist = GetHistoryFontColor();
	PickFont(&FontHist, &ColorHist);

	if (bChangedHook)
	{
		SetHistoryFont(&FontHist);
		SetHistoryFontColor(ColorHist);

		if (hFontHist != NULL)
			DeleteFont(hFontHist);

		hFontHist = CreateFontIndirect(&FontHist);

		if (hFontHist != NULL)
			SetWindowFont(GetDlgItem(hMain, IDC_HISTORY), hFontHist, true);
	}
}

static void PickFoldersFont(void)
{
	LOGFONT FontTree;

	GetTreeFont(&FontTree); 
	COLORREF ColorTree = GetTreeFontColor();
	PickFont(&FontTree, &ColorTree);

	if (bChangedHook)
	{
		SetTreeFont(&FontTree);
		SetTreeFontColor(ColorTree);

		if (hFontTree != NULL)
			DeleteFont(hFontTree);

		hFontTree = CreateFontIndirect(&FontTree);

		if (hFontTree != NULL)
		{
			SetWindowFont(hTreeView, hFontTree, true);
			(void)TreeView_SetTextColor(hTreeView, ColorTree);
		}
	}
}

static void PickColor(COLORREF *cDefault)
{
	CHOOSECOLOR cc;
	COLORREF choice_colors[16];
	bChangedHook = false;
	bHookFont = false;

	for (int i = 0; i < 16; i++)
		choice_colors[i] = GetCustomColor(i);

	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = hMain;
	cc.lpfnHook = &HookProc;
	cc.rgbResult = *cDefault;
	cc.lpCustColors = choice_colors;
	cc.Flags = CC_ANYCOLOR | CC_RGBINIT | CC_FULLOPEN | CC_ENABLEHOOK;

	if (!ChooseColor(&cc))
		return;

	for (int i = 0; i < 16; i++)
		SetCustomColor(i,choice_colors[i]);

	*cDefault = cc.rgbResult;
}

static void PickTreeBgColor(void)
{
	COLORREF cTreeColor = GetFolderBgColor();
	PickColor(&cTreeColor);

	if (bChangedHook)
	{
		SetFolderBgColor(cTreeColor);
		(void)TreeView_SetBkColor(hTreeView, GetFolderBgColor());
	}
}
 
static void PickHistoryBgColor(void)
{
	COLORREF cHistoryColor = GetHistoryBgColor();
	PickColor(&cHistoryColor);

	if (bChangedHook)
	{
		SetHistoryBgColor(cHistoryColor);
		UpdateScreenShot();
	}
}

static void PickListBgColor(void)
{
	COLORREF cListColor = GetListBgColor();
	PickColor(&cListColor);

	if (bChangedHook)
	{
		SetListBgColor(cListColor);
		(void)ListView_SetBkColor(hWndList, GetListBgColor());
		UpdateListView();
	}
}

static bool MameCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	LPTREEFOLDER folder;

	switch (id)
	{
		case ID_FILE_PLAY:
			MamePlayGame();
			SetFocus(hWndList);
			return true;

#ifdef MAME_AVI
		case ID_FILE_PLAY_BACK_AVI:
			MamePlayBackGameAVI();
			return true;

		case ID_FILE_PLAY_WITH_AVI:	
			MamePlayGameAVI();
			return true;
 #endif /* MAME_AVI */

		case ID_FILE_PLAY_RECORD:
			MamePlayRecordGame();
			SetFocus(hWndList);
			return true;

		case ID_FILE_PLAY_BACK:
			MamePlayBackGame();
			SetFocus(hWndList);
			return true;

		case ID_FILE_PLAY_RECORD_WAVE:
			MamePlayRecordWave();
			SetFocus(hWndList);
			return true;

		case ID_FILE_PLAY_RECORD_MNG:
			MamePlayRecordMNG();
			SetFocus(hWndList);
			return true;

		case ID_FILE_PLAY_RECORD_AVI:
			MamePlayRecordAVI();
			SetFocus(hWndList);
			return true;

		case ID_FILE_LOADSTATE :
			MameLoadState();
			SetFocus(hWndList);
			return true;

		case ID_FILE_AUDIT:
			AuditDialog();
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_AUDIT), hMain, AuditWindowProc);
			UpdateListView();
			return true;

		case ID_FILE_GAMELIST:
			if (CommonListDialog(GetOpenFileName, FILETYPE_GAME_LIST))
				return true;
			else
				break;

		case ID_FILE_ROMSLIST:
			if (CommonListDialog(GetOpenFileName, FILETYPE_ROMS_LIST))
				return true;
			else
				break;

		case ID_FILE_EXIT:
			PostMessage(hMain, WM_CLOSE, 0, 0);
			return true;

		case ID_VIEW_ICONS_LARGE:
			SetView(ID_VIEW_ICONS_LARGE);
			UpdateListView();
			return true;

		case ID_VIEW_ICONS_SMALL:
			SetView(ID_VIEW_ICONS_SMALL);
			UpdateListView();
			return true;

		case ID_VIEW_LIST_MENU:
			SetView(ID_VIEW_LIST_MENU);
			return true;
		
		case ID_VIEW_DETAIL:
			SetView(ID_VIEW_DETAIL);
			return true;
		
		case ID_VIEW_GROUPED:
			SetView(ID_VIEW_GROUPED);
			return true;

		/* Arrange Icons submenu */
		case ID_VIEW_BYGAME:
			SetSortReverse(false);
			SetSortColumn(COLUMN_GAMES);
			Picker_Sort(hWndList);
			break;

		case ID_VIEW_BYDIRECTORY:
			SetSortReverse(false);
			SetSortColumn(COLUMN_ROMNAME);
			Picker_Sort(hWndList);
			break;

		case ID_VIEW_BYMANUFACTURER:
			SetSortReverse(false);
			SetSortColumn(COLUMN_MANUFACTURER);
			Picker_Sort(hWndList);
			break;

		case ID_VIEW_BYYEAR:
			SetSortReverse(false);
			SetSortColumn(COLUMN_YEAR);
			Picker_Sort(hWndList);
			break;

		case ID_VIEW_BYSOURCE:
			SetSortReverse(false);
			SetSortColumn(COLUMN_SOURCEFILE);
			Picker_Sort(hWndList);
			break;

		case ID_VIEW_BYTIMESPLAYED:
			SetSortReverse(false);
			SetSortColumn(COLUMN_PLAYED);
			Picker_Sort(hWndList);
			break;

		case ID_ENABLE_INDENT:
			bEnableIndent = !bEnableIndent;
			SetEnableIndent(bEnableIndent);
			CheckMenuItem(GetMenu(hMain), ID_ENABLE_INDENT, (bEnableIndent) ? MF_CHECKED : MF_UNCHECKED);
			ToolBar_CheckButton(hToolBar, ID_ENABLE_INDENT, (bEnableIndent) ? MF_CHECKED : MF_UNCHECKED);
			UpdateListView();
			break;

		case ID_VIEW_FOLDERS:
			bShowTree = !bShowTree;
			SetShowFolderList(bShowTree);
			CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
			ToolBar_CheckButton(hToolBar, ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
			UpdateScreenShot();
			break;

	    case ID_KOREAN_GAMELIST: // USE_KLIST
			bklist = !bklist;
			SetUsekoreanList(bklist);
			CheckMenuItem(GetMenu(hMain), ID_KOREAN_GAMELIST, (bklist) ? MF_CHECKED : MF_UNCHECKED);
			ToolBar_CheckButton(hToolBar, ID_KOREAN_GAMELIST, (bklist) ? MF_CHECKED : MF_UNCHECKED);
			ResetListView();
			break;
			
		case ID_VIEW_TOOLBARS:
			bShowToolBar = !bShowToolBar;
			SetShowToolBar(bShowToolBar);
			CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
			ToolBar_CheckButton(hToolBar, ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
			ShowWindow(hToolBar, (bShowToolBar) ? SW_SHOW : SW_HIDE);
			ResizePickerControls(hMain);
			UpdateScreenShot();
			break;

		case ID_VIEW_STATUS:
			bShowStatusBar = !bShowStatusBar;
			SetShowStatusBar(bShowStatusBar);
			CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
			ShowWindow(hStatusBar, (bShowStatusBar) ? SW_SHOW : SW_HIDE);
			ResizePickerControls(hMain);
			UpdateScreenShot();
			break;

		case ID_VIEW_PAGETAB:
			bShowTabCtrl = !bShowTabCtrl;
			SetShowTabCtrl(bShowTabCtrl);
			ShowWindow(hTabCtrl, (bShowTabCtrl) ? SW_SHOW : SW_HIDE);
			ResizePickerControls(hMain);
			UpdateScreenShot();
			InvalidateRect(hMain, NULL, true);
			break;

		case ID_VIEW_FULLSCREEN:
			SwitchFullScreenMode();
			break;

		case ID_TOOLBAR_EDIT:
		{
			setlocale(LC_ALL,"Korean");

			char buf[256];
			winui_get_window_text_utf8(hWndCtl, buf, WINUI_ARRAY_LENGTH(buf));

			switch (codeNotify)
			{
				case TOOLBAR_EDIT_ACCELERATOR_PRESSED:
				{
					HWND hToolbarEdit = GetDlgItem(hToolBar, ID_TOOLBAR_EDIT);
					SetFocus(hToolbarEdit);
					break;
				}

				case EN_CHANGE:
					//put search routine here first, add a 200ms timer later.
					if ((!_stricmp(buf, SEARCH_PROMPT) && !_stricmp(g_SearchText, "")) ||
					(!_stricmp(g_SearchText, SEARCH_PROMPT) && !_stricmp(buf, "")))
						strcpy(g_SearchText, buf);
					else
					{
						strcpy(g_SearchText, buf);
						ResetListView();
					}

					break;

				case EN_SETFOCUS:
					if (!_stricmp(buf, SEARCH_PROMPT))
					winui_set_window_text_utf8(hWndCtl, "");

					break;

				case EN_KILLFOCUS:
					if (*buf == 0)
						winui_set_window_text_utf8(hWndCtl, SEARCH_PROMPT);

					break;
			}

			break;
		}

		case ID_GAME_INFO:
			(void)DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_GAME_INFO), hMain, GamePropertiesDialogProc, Picker_GetSelectedItem(hWndList));
			SetFocus(hWndList);
			break;

		case ID_GAME_AUDIT:
			(void)DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_GAME_AUDIT), hMain, GameAuditDialogProc, Picker_GetSelectedItem(hWndList));
			UpdateStatusBar();
			SetFocus(hWndList);
			break;

		/* ListView Context Menu */
		case ID_CONTEXT_ADD_CUSTOM:
			(void)DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CUSTOM_FILE), hMain, AddCustomFileDialogProc, Picker_GetSelectedItem(hWndList));
			SetFocus(hWndList);
			break;

		case ID_CONTEXT_REMOVE_CUSTOM:
			RemoveCurrentGameCustomFolder();
			SetFocus(hWndList);
			break;

		/* Tree Context Menu */
		case ID_CONTEXT_FILTERS:
			if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FILTERS), hMain, FilterDialogProc) == true)
				UpdateListView();
			return true;

		// ScreenShot Context Menu
		// select current tab
		case ID_VIEW_TAB_SCREENSHOT:
		case ID_VIEW_TAB_TITLE:
		case ID_VIEW_TAB_SCORES:
		case ID_VIEW_TAB_HOWTO:
		case ID_VIEW_TAB_SELECT:
		case ID_VIEW_TAB_VERSUS:
		case ID_VIEW_TAB_BOSSES:
		case ID_VIEW_TAB_ENDS:
		case ID_VIEW_TAB_GAMEOVER:
		case ID_VIEW_TAB_LOGO:
		case ID_VIEW_TAB_ARTWORK:
		case ID_VIEW_TAB_FLYER:
		case ID_VIEW_TAB_CABINET:
		case ID_VIEW_TAB_MARQUEE:
		case ID_VIEW_TAB_CONTROL_PANEL:
		case ID_VIEW_TAB_PCB:
		case ID_VIEW_TAB_HISTORY:
			if (id == ID_VIEW_TAB_HISTORY && GetShowTab(TAB_HISTORY) == false)
				break;

			TabView_SetCurrentTab(hTabCtrl, id - ID_VIEW_TAB_SCREENSHOT);
			UpdateScreenShot();
			TabView_UpdateSelection(hTabCtrl);
			break;

		// toggle tab's existence
		case ID_TOGGLE_TAB_SCREENSHOT:
		case ID_TOGGLE_TAB_TITLE:
		case ID_TOGGLE_TAB_SCORES:
		case ID_TOGGLE_TAB_HOWTO:
		case ID_TOGGLE_TAB_SELECT:
		case ID_TOGGLE_TAB_VERSUS:
		case ID_TOGGLE_TAB_BOSSES:
		case ID_TOGGLE_TAB_ENDS:
		case ID_TOGGLE_TAB_GAMEOVER:
		case ID_TOGGLE_TAB_LOGO:
		case ID_TOGGLE_TAB_ARTWORK:
		case ID_TOGGLE_TAB_FLYER:
		case ID_TOGGLE_TAB_CABINET:
		case ID_TOGGLE_TAB_MARQUEE:
		case ID_TOGGLE_TAB_CONTROL_PANEL:
		case ID_TOGGLE_TAB_PCB:
		case ID_TOGGLE_TAB_HISTORY:
		{
			int toggle_flag = id - ID_TOGGLE_TAB_SCREENSHOT;

			if (AllowedToSetShowTab(toggle_flag,!GetShowTab(toggle_flag)) == false)
				// attempt to hide the last tab
				// should show error dialog? hide picture area? or ignore?
				break;

			SetShowTab(toggle_flag,!GetShowTab(toggle_flag));
			TabView_Reset(hTabCtrl);

			if (TabView_GetCurrentTab(hTabCtrl) == toggle_flag && GetShowTab(toggle_flag) == false)
				// we're deleting the tab we're on, so go to the next one
				TabView_CalculateNextTab(hTabCtrl);

			// Resize the controls in case we toggled to another history
			// mode (and the history control needs resizing).
			ResizePickerControls(hMain);
			UpdateScreenShot();
			TabView_UpdateSelection(hTabCtrl);
			break;
		}

		/* Header Context Menu */
		case ID_SORT_ASCENDING:
			SetSortReverse(false);
			SetSortColumn(Picker_GetRealColumnFromViewColumn(hWndList, lastColumnClick));
			Picker_Sort(hWndList);
			break;

		case ID_SORT_DESCENDING:
			SetSortReverse(true);
			SetSortColumn(Picker_GetRealColumnFromViewColumn(hWndList, lastColumnClick));
			Picker_Sort(hWndList);
			break;

		case ID_CUSTOMIZE_FIELDS:
			if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_COLUMNS), hMain, ColumnDialogProc) == true)
				ResetColumnDisplay();
			SetFocus(hWndList);
			return true;

		case ID_GAME_PROPERTIES:
		{
			int game = Picker_GetSelectedItem(hWndList);
			if (game >= 0)
			{
				folder = GetFolderByName(FOLDER_SOURCE, GetDriverFileName(game));
				InitPropertyPage(hInst, hWnd, OPTIONS_GAME, folder->m_nFolderId, game);
				UpdateStatusBar();
			}
			SetFocus(hWndList);
			return true;
		}

		case ID_FOLDER_PROPERTIES:
		{
			OPTIONS_TYPE curOptType = OPTIONS_SOURCE;
			folder = GetSelectedFolder();

			if(folder->m_nFolderId == FOLDER_RASTER) 
				curOptType = OPTIONS_RASTER;
			else if(folder->m_nFolderId == FOLDER_VECTOR) 
				curOptType = OPTIONS_VECTOR;
			else if(folder->m_nFolderId == FOLDER_HORIZONTAL) 
				curOptType = OPTIONS_HORIZONTAL;
			else if(folder->m_nFolderId == FOLDER_VERTICAL) 
				curOptType = OPTIONS_VERTICAL;

			InitPropertyPage(hInst, hWnd, curOptType, folder->m_nFolderId, Picker_GetSelectedItem(hWndList));
			UpdateStatusBar();
			SetFocus(hWndList);
			return true;
		}

		case ID_FOLDER_SOURCEPROPERTIES:
		{
			int game = Picker_GetSelectedItem(hWndList);
			if (game >= 0)
			{
				folder = GetFolderByName(FOLDER_SOURCE, GetDriverFileName(game));
				InitPropertyPage(hInst, hWnd, OPTIONS_SOURCE, folder->m_nFolderId, game);
				UpdateStatusBar();
			}
			SetFocus(hWndList);
			return true;
		}

		case ID_FOLDER_AUDIT:
			FolderCheck();
			UpdateListView();
			UpdateStatusBar();
			break;

		case ID_VIEW_PICTURE_AREA :
			ToggleScreenShot();
			break;

		case ID_UPDATE_GAMELIST:
			UpdateGameList();
			UpdateListView();
			UpdateStatusBar();
			break;

		case ID_OPTIONS_FONT:
			PickListFont();
			SetFocus(hWndList);
			return true;

		case ID_OPTIONS_HISTORY_FONT:
			PickHistoryFont();
			SetFocus(hWndList);
			return true;

		case ID_OPTIONS_TREE_FONT:
			PickFoldersFont();
			SetFocus(hWndList);
			return true;

		case ID_OPTIONS_FOLDERS_COLOR:
			PickTreeBgColor();
			SetFocus(hWndList);
			return true;

		case ID_OPTIONS_HISTORY_COLOR:
			PickHistoryBgColor();
			SetFocus(hWndList);
			return true;

		case ID_OPTIONS_LIST_COLOR:
			PickListBgColor();
			SetFocus(hWndList);
			return true;

		case ID_OPTIONS_DEFAULTS:
			InitPropertyPage(hInst, hWnd, OPTIONS_GLOBAL, -1, GLOBAL_OPTIONS);
			UpdateStatusBar();
			SetFocus(hWndList);
			return true;

		case ID_OPTIONS_DIR:
		{
			int nResult = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIRECTORIES), hMain, DirectoriesDialogProc);
			bool bUpdateRoms = ((nResult & DIRDLG_ROM) == DIRDLG_ROM) ? true : false;

			/* update game list */
			if (bUpdateRoms == true)
				UpdateGameList();

			SetFocus(hWndList);
			return true;
		}

		case ID_OPTIONS_RESET_DEFAULTS:
			if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_RESET), hMain, ResetDialogProc) == true)
				PostMessage(hMain, WM_CLOSE, 0, 0);
			else 
				UpdateListView();
			return true;

		case ID_OPTIONS_INTERFACE:
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_INTERFACE_OPTIONS), hMain, InterfaceDialogProc);
			KillTimer(hMain, SCREENSHOT_TIMER);

			if( GetCycleScreenshot() > 0)
				SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL ); // Scale to seconds

			SetFocus(hWndList);
			return true;

		case ID_MAME_HOMEPAGE:
			ShellExecuteCommon(hMain, "https://www.mamedev.org");
			SetFocus(hWndList);
			return true;

		case ID_MAME_FAQ:
			ShellExecuteCommon(hMain, "https://mamedev.org/devwiki/index.php?title=Frequently_Asked_Questions"); 
			SetFocus(hWndList);
			return true;

		case ID_PLAY_VIDEO:
		{
			char videoplay[MAX_PATH];
			*videoplay = 0;
			if (CommonFileDialog(GetOpenFileName, videoplay, FILETYPE_AVI_FILES, false))
				ShellExecuteCommon(hMain, videoplay);
			SetFocus(hWndList);
			return true;
		}

		case ID_PLAY_AUDIO:
		{
			char audioplay[MAX_PATH];
			*audioplay = 0;
			if (CommonFileDialog(GetOpenFileName, audioplay, FILETYPE_WAVE_FILES, false))
				ShellExecuteCommon(hMain, audioplay);
			SetFocus(hWndList);
			return true;
		}

		case ID_PLAY_MNG:
		{
			char mngplay[MAX_PATH];
			*mngplay = 0;
			if (CommonFileDialog(GetOpenFileName, mngplay, FILETYPE_MNG_FILES, false))
				ShellExecuteCommon(hMain, mngplay);
			SetFocus(hWndList);
			return true;
		}

		case ID_VIEW_ZIP:
		{
			// This will iterate through the rom path and stop at the first find
			UINT found = false;
			char path[MAX_PATH], viewzip[MAX_PATH];
			TCHAR* t_s = NULL;
			strcpy(path, GetRomDirs());
			int nGame = Picker_GetSelectedItem(hWndList);
			if (nGame >= 0)
			{
				char* dir_one = strtok(path, ";");
				while (dir_one && !found)
				{
					snprintf(viewzip, WINUI_ARRAY_LENGTH(viewzip), "%s\\%s.zip", dir_one, GetDriverGameName(nGame));
					t_s = win_wstring_from_utf8(viewzip);
					if (t_s)
					{
						if (PathFileExists(t_s))
						{
							found = true;
							ShellExecuteCommon(hMain, viewzip);
						}
					}
					dir_one = strtok(NULL, ";");
				}
				if (t_s)
					free(t_s);

				if (!found)    //zip file not found
					ErrorMessageBox("Can't find %s.zip in the ROMS PATH: %s", GetDriverGameName(nGame), GetRomDirs());
			}

			SetFocus(hWndList);
			return true;
		}

		case ID_VIDEO_SNAP:
		{
			int nGame = Picker_GetSelectedItem(hWndList);
			if (nGame >= 0)
			{
				char path[MAX_PATH];
				snprintf(path, WINUI_ARRAY_LENGTH(path), "%s\\%s.mp4", GetVideoDir(), GetDriverGameName(nGame));
				ShellExecuteCommon(hMain, path);
			}
			SetFocus(hWndList);
			return true;
		}

		case ID_MANUAL:
		{
			int nGame = Picker_GetSelectedItem(hWndList);
			if (nGame >= 0)
			{
				char path[MAX_PATH];
				snprintf(path, WINUI_ARRAY_LENGTH(path), "%s\\%s.pdf", GetManualsDir(), GetDriverGameName(nGame));
				ShellExecuteCommon(hMain, path);
			}
			SetFocus(hWndList);
			return true;
		}

	case ID_NOTEPAD:
		{
			int nGame = Picker_GetSelectedItem(hWndList);
			if (nGame >= 0)
			{
				const char* filename = "history.wtx";
				string t2 = GetGameHistory(nGame);
				std::ofstream outfile (filename, std::ios::out | std::ios::trunc);
				size_t size = t2.size();
				char t1[size+1];
				strcpy(t1, t2.c_str());
				outfile.write(t1, size);
				outfile.close();
				string path = string(".\\") + filename;
				ShellExecuteCommon(hMain, path.c_str());
			}
			SetFocus(hWndList);
		}
		break;

		case ID_PLAY_M1:
		{
			// we assume m1fx.exe is in the same folder as we are
			int nGame = Picker_GetSelectedItem(hWndList);
			if (nGame >= 0)
			{
				char m1exe [MAX_PATH] = "m1fx.exe";
				TCHAR *t_s = win_wstring_from_utf8(m1exe);
				if (t_s)
				{
					if (PathFileExists(t_s))
					{
						char command[MAX_PATH];
						const char *game = GetDriverGameName(nGame);
						int audit_result = GetRomAuditResults(nGame);
						snprintf(command, WINUI_ARRAY_LENGTH(command), "m1fx.exe %s", game);

						if (IsAuditResultYes(audit_result))
						{
							wchar_t* t_command = win_wstring_from_utf8(command);
							STARTUPINFO siStartupInfo;
							PROCESS_INFORMATION piProcessInfo;
							memset(&siStartupInfo, 0, sizeof(STARTUPINFO));
							memset(&piProcessInfo, 0, sizeof(PROCESS_INFORMATION));
							siStartupInfo.cb = sizeof(STARTUPINFO);
							CreateProcess(NULL, t_command, NULL, NULL, false, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &siStartupInfo, &piProcessInfo);
							free(t_command);
						}
						else
							ErrorMessageBox("Game '%s' is missing ROMs!\r\nM1FX cannot be executed!", game);
					}
					else
						ErrorMessageBox("Can't find M1FX.EXE in the current folder!");
				}
			}
			SetFocus(hWndList);
			return true;
		}

		case ID_HELP_ABOUT:
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT), hMain, AboutDialogProc);
			SetFocus(hWndList);
			return true;

		case IDOK:
			/* cmk -- might need to check more codes here, not sure */
			if (codeNotify != EN_CHANGE && codeNotify != EN_UPDATE)
			{
				/* enter key */
				if (g_in_treeview_edit)
				{
					(void)TreeView_EndEditLabelNow(hTreeView, false);
					return true;
				}
				else if (have_selection)
					MamePlayGame();
			}

			break;

		case IDCANCEL: /* esc key */
			if (g_in_treeview_edit)
				(void)TreeView_EndEditLabelNow(hTreeView, true);

			break;

		case IDC_PLAY_GAME:
			if (have_selection)
				MamePlayGame();

			break;

		case ID_UI_START:
			SetFocus(hWndList);
			MamePlayGame();
			break;

		case ID_UI_UP:
			Picker_SetSelectedPick(hWndList, GetSelectedPick() - 1);
			break;

		case ID_UI_DOWN:
			Picker_SetSelectedPick(hWndList, GetSelectedPick() + 1);
			break;

		case ID_UI_PGUP:
			Picker_SetSelectedPick(hWndList, GetSelectedPick() - ListView_GetCountPerPage(hWndList));
			break;

		case ID_UI_PGDOWN:
			if ((GetSelectedPick() + ListView_GetCountPerPage(hWndList)) < ListView_GetItemCount(hWndList))
				Picker_SetSelectedPick(hWndList, GetSelectedPick() + ListView_GetCountPerPage(hWndList));
			else
				Picker_SetSelectedPick(hWndList, ListView_GetItemCount(hWndList) - 1);
			break;

		case ID_UI_HOME:
			Picker_SetSelectedPick(hWndList, 0);
			break;

		case ID_UI_END:
			Picker_SetSelectedPick(hWndList, ListView_GetItemCount(hWndList) - 1);
			break;

		case ID_UI_LEFT:
			/* hmmmmm..... */
			SendMessage(hWndList, WM_HSCROLL, SB_LINELEFT, 0);
			break;

		case ID_UI_RIGHT:
			/* hmmmmm..... */
			SendMessage(hWndList, WM_HSCROLL, SB_LINERIGHT, 0);
			break;

		case ID_UI_HISTORY_UP:
		{
			/* hmmmmm..... */
			HWND hHistory = GetDlgItem(hMain, IDC_HISTORY);
			SendMessage(hHistory, EM_SCROLL, SB_PAGEUP, 0);
			break;
		}

		case ID_UI_HISTORY_DOWN:
		{
			/* hmmmmm..... */
			HWND hHistory = GetDlgItem(hMain, IDC_HISTORY);
			SendMessage(hHistory, EM_SCROLL, SB_PAGEDOWN, 0);
			break;
		}

		case IDC_SSFRAME:
			TabView_CalculateNextTab(hTabCtrl);
			UpdateScreenShot();
			TabView_UpdateSelection(hTabCtrl);
			break;

		case ID_CONTEXT_SELECT_RANDOM:
			SetRandomPickItem();
			break;

		case ID_CONTEXT_RESET_PLAYCOUNT:
			ResetPlayCount(Picker_GetSelectedItem(hWndList));
			(void)ListView_RedrawItems(hWndList, GetSelectedPick(), GetSelectedPick());
			break;

		case ID_CONTEXT_RESET_PLAYTIME:
			ResetPlayTime(Picker_GetSelectedItem(hWndList));
			(void)ListView_RedrawItems(hWndList, GetSelectedPick(), GetSelectedPick());
			break;

		case ID_CONTEXT_RENAME_CUSTOM:
			(void)TreeView_EditLabel(hTreeView, TreeView_GetSelection(hTreeView));
			break;

		case ID_HELP_CONTENTS :
			ShellExecuteCommon(hMain, "https://docs.mamedev.org/");
			SetFocus(hWndList);
			return true;

		case ID_HELP_TROUBLE:
			ShellExecuteCommon(hMain, "https://www.1emulation.com/forums/forum/127-arcade/");
			SetFocus(hWndList);
			return true;

		case ID_HELP_WHATS_NEW :
			ShellExecuteCommon(hMain, ".\\config/docs\\What's new.txt");
			SetFocus(hWndList);
			return true;

		default:
			if (id >= ID_SHOW_FOLDER_START1 && id <= ID_SHOW_FOLDER_START28)
			{
				ToggleShowFolder((id - ID_SHOW_FOLDER_START1) + 1);
				break;
			}
			else if (id >= ID_CONTEXT_SHOW_FOLDER_START && id < ID_CONTEXT_SHOW_FOLDER_END)
			{
				ToggleShowFolder(id - ID_CONTEXT_SHOW_FOLDER_START);
				break;
			}
	}

	return false;
}

static void ResetColumnDisplay(void)
{
	int driver_index = GetGameNameIndex(GetDefaultGame());

	Picker_ResetColumnDisplay(hWndList);
	UpdateListView();
	Picker_SetSelectedItem(hWndList, driver_index);
}

int GamePicker_GetItemImage(HWND hwndPicker, int nItem)
{
	return GetIconForDriver(nItem);
}

const wchar_t *GamePicker_GetItemString(HWND hwndPicker, int nItem, int nColumn, wchar_t *pszBuffer, UINT nBufferLength)
{
	const wchar_t *s = NULL;
	const char* utf8_s = NULL;
	char playtime_buf[256];
	char playcount_buf[256];

	switch(nColumn)
	{
		case COLUMN_GAMES:
			/* Driver description */
//#ifdef USE_KLIST
			utf8_s = GetDescriptionByIndex(nItem, GetUsekoreanList());
//#else		
//			utf8_s = GetDriverGameTitle(nItem);
//#endif
  	        break;

		case COLUMN_ROMNAME:
			/* Driver name (directory) */
			utf8_s = GetDriverGameName(nItem);
			break;

		case COLUMN_MANUFACTURER:
			/* Manufacturer */
			utf8_s = GetDriverGameManufacturer(nItem);
			break;

		case COLUMN_PLAYED:
			/* played count */
			snprintf(playcount_buf, WINUI_ARRAY_LENGTH(playcount_buf), "%d",  GetPlayCount(nItem));
			utf8_s = playcount_buf;
			break;

		case COLUMN_PLAYTIME:
			/* played time */
			GetTextPlayTime(nItem, playtime_buf);
			utf8_s = playtime_buf;
			break;

		case COLUMN_YEAR:
			/* Year */
			utf8_s = GetDriverGameYear(nItem);
			break;

		case COLUMN_SOURCEFILE:
			/* Source drivers */
			utf8_s = GetDriverFileName(nItem);
			break;

		case COLUMN_CLONE:
			utf8_s = GetCloneParentName(nItem);
			break;
	}

	if(utf8_s)
	{
		wchar_t* t_s = win_wstring_from_utf8(utf8_s);

		if(!t_s)
			return s;

		_sntprintf(pszBuffer, nBufferLength, TEXT("%s"), t_s);
		free(t_s);
		s = pszBuffer;
	}

	return s;
}

void GamePicker_LeavingItem(HWND hwndPicker, int nItem)
{
	// leaving item...
}

void GamePicker_EnteringItem(HWND hwndPicker, int nItem)
{
	EnableSelection(nItem);

	// decide if it is valid to load a savestate
	EnableMenuItem(GetMenu(hMain), ID_FILE_LOADSTATE, (driver_list::driver(nItem).flags & MACHINE_SUPPORTS_SAVE) ? MFS_ENABLED : MFS_GRAYED);
}

int GamePicker_FindItemParent(HWND hwndPicker, int nItem)
{
	return GetParentRomSetIndex(&driver_list::driver(nItem));
}

bool GamePicker_CheckNotWorkingItem(HWND hwndPicker, int nItem)
{
	return DriverIsBroken(nItem);
}

/* Initialize the Picker and List controls */
static void InitListView(void)
{
	struct PickerOptions opts;

	// subclass the list view
	memset(&opts, 0, sizeof(opts));
	opts.pCallbacks = &s_gameListCallbacks;
	opts.nColumnCount = COLUMN_MAX;
	opts.ppszColumnNames = column_names;
	SetupPicker(hWndList, &opts);
	(void)ListView_SetTextBkColor(hWndList, CLR_NONE);
	(void)ListView_SetBkColor(hWndList, CLR_NONE);
	CreateIcons();
	ResetWhichGamesInFolders();
	ResetColumnDisplay();
	// Allow selection to change the default saved game
	bListReady = true;
}

static void AddDriverIcon(int nItem, int default_icon_index)
{
	HICON hIcon = 0;
	int nParentIndex = -1;

	/* if already set to rom or clone icon, we've been here before */
	if (icon_index[nItem] == 1 || icon_index[nItem] == 3)
		return;

	hIcon = LoadIconFromFile((char *)GetDriverGameName(nItem));

	if (hIcon == NULL)
	{
		//logmsg("Game %s Icon Not Found\n",(char *)GetDriverGameName(nItem));
		nParentIndex = GetParentIndex(&driver_list::driver(nItem));

		if( nParentIndex >= 0)
		{
			hIcon = LoadIconFromFile((char *)GetDriverGameName(nParentIndex));
			nParentIndex = GetParentIndex(&driver_list::driver(nParentIndex));

			if (hIcon == NULL && nParentIndex >= 0)
				hIcon = LoadIconFromFile((char *)GetDriverGameName(nParentIndex));
		}
	}

	if (hIcon != NULL)
	{
		//logmsg("Game %s Icon Found\n",(char *)GetDriverGameName(nItem));
		
		int nIconPos = ImageList_AddIcon(hSmall, hIcon);
		ImageList_AddIcon(hLarge, hIcon);

		if (nIconPos != -1)
			icon_index[nItem] = nIconPos;

		DestroyIcon(hIcon);
	}

	if (icon_index[nItem] == 0)
		icon_index[nItem] = default_icon_index;
}

static void DestroyIcons(void)
{
	if (hSmall != NULL)
	{
		ImageList_Destroy(hSmall);
		hSmall = NULL;
	}

	if (icon_index != NULL)
	{
		for (int i = 0; i < driver_list::total(); i++)
			icon_index[i] = 0; 	// these are indices into hSmall
	}

	if (hLarge != NULL)
	{
		ImageList_Destroy(hLarge);
		hLarge = NULL;
	}
}

static void ReloadIcons(void)
{
	// clear out all the images
	ImageList_RemoveAll(hSmall);
	ImageList_RemoveAll(hLarge);

	if (icon_index != NULL)
	{
		for (int i = 0; i < driver_list::total(); i++)
			icon_index[i] = 0; 	// these are indices into hSmall
	}

	for (int i = 0; g_iconData[i].icon_name; i++)
	{
		HICON hIcon = LoadIconFromFile((char *) g_iconData[i].icon_name);

		if (hIcon == NULL)
			hIcon = LoadIcon(hInst, MAKEINTRESOURCE(g_iconData[i].resource));

		ImageList_AddIcon(hSmall, hIcon);
		ImageList_AddIcon(hLarge, hIcon);
		DestroyIcon(hIcon);
	}
}

static DWORD GetShellLargeIconSize(void)
{
	DWORD  dwSize = 32, dwLength = 512, dwType = REG_SZ;
	HKEY   hKey;
	LPTSTR tErrorMessage = NULL;

	/* Get the Key */
	LONG lRes = RegOpenKey(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop\\WindowMetrics"), &hKey);
	if( lRes != ERROR_SUCCESS )
	{
		//GetSystemErrorMessage(lRes, &tErrorMessage);
		MessageBox(GetMainWindow(), tErrorMessage, TEXT("Large shell icon size registry access"), MB_OK | MB_ICONERROR);
		LocalFree(tErrorMessage);
		return dwSize;
	}

	/* Save the last size */
	TCHAR  szBuffer[512];
	lRes = RegQueryValueEx(hKey, TEXT("Shell Icon Size"), NULL, &dwType, (LPBYTE)szBuffer, &dwLength);
	if( lRes != ERROR_SUCCESS )
	{
		//GetSystemErrorMessage(lRes, &tErrorMessage);
		MessageBox(GetMainWindow(), tErrorMessage, TEXT("Large shell icon size registry query"), MB_OK | MB_ICONERROR);
		LocalFree(tErrorMessage);
		RegCloseKey(hKey);
		return dwSize;
	}

	dwSize = _ttol(szBuffer);
	if (dwSize < 32)
		dwSize = 32;

	if (dwSize > 48)
		dwSize = 48;

	/* Clean up */
	RegCloseKey(hKey);
	return dwSize;
}


static DWORD GetShellSmallIconSize(void)
{
	DWORD dwSize = ICONMAP_WIDTH;

	if (dwSize < 48)
	{
		if (dwSize < 32)
			dwSize = 16;
		else
			dwSize = 32;
	}
	else
	{
		dwSize = 48;
	}
	return dwSize;
}

// create iconlist for Listview control
static void CreateIcons(void)
{
	DWORD dwSmallIconSize = GetShellSmallIconSize();
	DWORD dwLargeIconSize = GetShellLargeIconSize();
	HICON hIcon;
	DWORD dwStyle;
	int icon_count = 0;
	int grow = 5000;

	while(g_iconData[icon_count].icon_name)
		icon_count++;

	dwStyle = GetWindowLong(hWndList,GWL_STYLE);
	SetWindowLong(hWndList,GWL_STYLE,(dwStyle & ~LVS_TYPEMASK) | LVS_ICON);

	hSmall = ImageList_Create(dwSmallIconSize, dwSmallIconSize, ILC_COLORDDB | ILC_MASK, icon_count, icon_count + grow);

	if (hSmall == NULL) 
	{
		ErrorMessageBox("Cannot allocate small icon image list!");
		PostQuitMessage(0);
	}

	hLarge = ImageList_Create(dwLargeIconSize, dwLargeIconSize, ILC_COLORDDB | ILC_MASK, icon_count, icon_count + grow);

	if (hLarge == NULL) 
	{
		ErrorMessageBox("Cannot allocate large icon image list!");
		PostQuitMessage(0);
	}

	ReloadIcons();
	
	// Associate the image lists with the list view control.
	(void)ListView_SetImageList(hWndList, hSmall, LVSIL_SMALL);
	(void)ListView_SetImageList(hWndList, hLarge, LVSIL_NORMAL);

	// restore our view
	SetWindowLong(hWndList,GWL_STYLE,dwStyle);

	// Now set up header specific stuff
	hHeaderImages = ImageList_Create(16,16,ILC_COLORDDB | ILC_MASK,2,2);
	hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_HEADER_UP));
	ImageList_AddIcon(hHeaderImages,hIcon);
	hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_HEADER_DOWN));
	ImageList_AddIcon(hHeaderImages,hIcon);

	for (int i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
		Picker_SetHeaderImageList(GetDlgItem(hMain, s_nPickers[i]), hHeaderImages);	
}


int GamePicker_Compare(HWND hwndPicker, int index1, int index2, int sort_subitem)
{
	int value = 0;  /* Default to 0, for unknown case */

	switch (sort_subitem)
	{
		case COLUMN_GAMES:
//#ifdef USE_KLIST
  			value = core_stricmp(GetDescriptionByIndex(index1,GetUsekoreanList()), GetDescriptionByIndex(index2, GetUsekoreanList()));
//#else			
//			value = core_stricmp(GetDriverGameTitle(index1), GetDriverGameTitle(index2));
//#endif
			break;

		case COLUMN_ROMNAME:
			value = core_stricmp(GetDriverGameName(index1), GetDriverGameName(index2));
			break;

		case COLUMN_MANUFACTURER:
			value = core_stricmp(GetDriverGameManufacturer(index1), GetDriverGameManufacturer(index2));
			break;

		case COLUMN_PLAYED:
			value = GetPlayCount(index1) - GetPlayCount(index2);
			break;

		case COLUMN_PLAYTIME:
			value = GetPlayTime(index1) - GetPlayTime(index2);
			break;

		case COLUMN_YEAR:
			value = core_stricmp(GetDriverGameYear(index1), GetDriverGameYear(index2));
			break;

		case COLUMN_SOURCEFILE: // don't try to "improve" this, it will break
			char file1[32];
			char file2[32];
			strcpy(file1, GetDriverFileName(index1));
			strcpy(file2, GetDriverFileName(index2));
			value = core_stricmp(file1, file2);
			break;

		case COLUMN_CLONE:
		{
			const char *name1 = GetCloneParentName(index1);
			const char *name2 = GetCloneParentName(index2);

			if (*name1 == '\0')
				name1 = NULL;

			if (*name2 == '\0')
				name2 = NULL;

			if (NULL == name1 && NULL == name2)
				value = 0;
			else if (name2 == NULL)
				value = -1;
			else if (name1 == NULL)
				value = 1;
			else
				value = core_stricmp(name1, name2);

			break;
		}
	}

	// Handle same comparisons here
	if (value == 0 && COLUMN_GAMES != sort_subitem)
		value = GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);

	return value;
}

int GetSelectedPick()
{
	/* returns index of listview selected item */
	/* This will return -1 if not found */
	return ListView_GetNextItem(hWndList, -1, LVIS_SELECTED | LVIS_FOCUSED);
}

static HICON GetSelectedPickItemIconSmall()
{
	LVITEM lvi;

	lvi.iItem = GetSelectedPick();
	lvi.iSubItem = 0;
	lvi.mask = LVIF_IMAGE;
	(void)ListView_GetItem(hWndList, &lvi);

	return ImageList_GetIcon(hSmall, lvi.iImage, ILD_TRANSPARENT);
}

static void SetRandomPickItem()
{
	int nListCount = ListView_GetItemCount(hWndList);

	if (nListCount > 0)
		Picker_SetSelectedPick(hWndList, rand() % nListCount);
}

static uintptr_t CALLBACK OFNHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			CenterWindow(GetParent(hWnd));
			hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAMEUI_ICON));
			SendMessage(GetParent(hWnd), WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			break;
	}

	return false;
}

bool CommonFileDialog(common_file_dialog_proc cfd, char *filename, int filetype, bool saving)
{
	bool success = false;
	OPENFILENAME of;
	const char *path = NULL;
	wchar_t t_filename_buffer[MAX_PATH];
	wchar_t fCurDir[MAX_PATH];

	// convert the filename to UTF-8 and copy into buffer
	wchar_t *t_filename = win_wstring_from_utf8(filename);

	if (t_filename != NULL)
	{
		_sntprintf(t_filename_buffer, WINUI_ARRAY_LENGTH(t_filename_buffer), TEXT("%s"), t_filename);
		free(t_filename);
	}

	if (GetCurrentDirectory(MAX_PATH, fCurDir) > MAX_PATH)
		fCurDir[0] = 0;

	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = hMain;
	of.hInstance = NULL;
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter = 0;
	of.nFilterIndex = 1;
	of.lpstrFile = t_filename_buffer;
	of.nMaxFile = WINUI_ARRAY_LENGTH(t_filename_buffer);
	of.lpstrFileTitle = NULL;
	of.nMaxFileTitle = 0;
	of.Flags  = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLEHOOK;
	of.nFileOffset = 0;
	of.nFileExtension = 0;
	of.lCustData = 0;
	of.lpfnHook = &OFNHookProc;
	of.lpTemplateName = NULL;

	switch (filetype)
	{
		case FILETYPE_INPUT_FILES :
			path = GetInpDir();
			of.lpstrInitialDir = win_wstring_from_utf8(path);
			of.lpstrFilter = TEXT("inputs (*.inp,*.zip)\0*.inp;*.zip\0");
			of.lpstrDefExt = TEXT("inp");

			if (!saving)
				of.lpstrTitle  = TEXT("Select an INP playback file");
			else
				of.lpstrTitle  = TEXT("Enter a name for the INP playback file");

			break;

		case FILETYPE_SAVESTATE_FILES :
			path = GetStateDir();
			of.lpstrInitialDir = win_wstring_from_utf8(path);
			of.lpstrFilter = TEXT("savestates (*.sta)\0*.sta;\0");
			of.lpstrDefExt = TEXT("sta");
			of.lpstrTitle  = TEXT("Select a STA savestate file");
			break;

		case FILETYPE_WAVE_FILES :
			path = GetAudioDir();
			of.lpstrInitialDir = win_wstring_from_utf8(path);
			of.lpstrFilter = TEXT("sounds (*.wav)\0*.wav;\0");
			of.lpstrDefExt = TEXT("wav");

			if (!saving)
				of.lpstrTitle  = TEXT("Select a WAV audio file");
			else
				of.lpstrTitle  = TEXT("Enter a name for the WAV audio file");

			break;

		case FILETYPE_MNG_FILES :
			path = GetVideoDir();
			of.lpstrInitialDir = win_wstring_from_utf8(path);
			of.lpstrFilter = TEXT("videos (*.mng)\0*.mng;\0");
			of.lpstrDefExt = TEXT("mng");

			if (!saving)
				of.lpstrTitle  = TEXT("Select a MNG image file");
			else
				of.lpstrTitle  = TEXT("Enter a name for the MNG image file");

			break;

		case FILETYPE_AVI_FILES :
			path = GetVideoDir();
			of.lpstrInitialDir = win_wstring_from_utf8(path);
			of.lpstrFilter = TEXT("videos (*.avi)\0*.avi;\0");
			of.lpstrDefExt = TEXT("avi");

			if (!saving)
				of.lpstrTitle  = TEXT("Select an AVI video file");
			else
				of.lpstrTitle  = TEXT("Enter a name for the AVI video file");

			break;

		case FILETYPE_EFFECT_FILES :
			path = GetArtDir();
			of.lpstrInitialDir = win_wstring_from_utf8(path);
			of.lpstrFilter = TEXT("effects (*.png)\0*.png;\0");
			of.lpstrDefExt = TEXT("png");
			of.lpstrTitle  = TEXT("Select an overlay PNG effect file");
			break;

		case FILETYPE_SHADER_FILES :
			path = GetGLSLDir();
			of.lpstrInitialDir = win_wstring_from_utf8(path);
			of.lpstrFilter = TEXT("shaders (*.vsh)\0*.vsh;\0");
			of.lpstrDefExt = TEXT("vsh");
			of.lpstrTitle  = TEXT("Select a GLSL shader file");
			break;

		case FILETYPE_CHEAT_FILES :
			of.lpstrInitialDir = last_directory;
			of.lpstrFilter = TEXT("cheats (*.7z,*.zip)\0*.7z;*.zip;\0");
			of.lpstrDefExt = TEXT("7z");
			of.lpstrTitle  = TEXT("Select a cheats archive file");
			break;

		case FILETYPE_BGFX_FILES :
			char temp[MAX_PATH];
			snprintf(temp, WINUI_ARRAY_LENGTH(temp), "%s\\chains", GetBGFXDir());
			of.lpstrInitialDir = win_wstring_from_utf8(temp);
			of.lpstrFilter = TEXT("chains (*.json)\0*.json;\0");
			of.lpstrDefExt = TEXT("json");
			of.lpstrTitle  = TEXT("Select a BGFX chain file");
			break;

		case FILETYPE_LUASCRIPT_FILES :
			of.lpstrInitialDir = last_directory;
			of.lpstrFilter = TEXT("scripts (*.lua)\0*.lua;\0");
			of.lpstrDefExt = TEXT("lua");
			of.lpstrTitle  = TEXT("Select a LUA script file");
			break;
	}

	success = cfd(&of);

	if (success)
	{
		GetCurrentDirectory(MAX_PATH, last_directory);

		if (fCurDir[0] != 0)
			SetCurrentDirectory(fCurDir);
	}

	char *utf8_filename = win_utf8_from_wstring(t_filename_buffer);

	if (utf8_filename != NULL)
	{
		snprintf(filename, MAX_PATH, "%s", utf8_filename);
		free(utf8_filename);
	}

	return success;
}

void SetStatusBarText(int part_index, const char *message)
{
	wchar_t *t_message = win_wstring_from_utf8(message);

	if(!t_message)
		return;

	SendMessage(hStatusBar, SB_SETTEXT, part_index, (LPARAM)t_message);
	free(t_message);
}

void SetStatusBarTextU(int part_index, const wchar_t *message)
{
//	wchar_t *t_message = win_wstring_from_utf8(message);
//
//	if(!t_message)
//		return;

	SendMessage(hStatusBar, SB_SETTEXT, part_index, (LPARAM)message);
	//free(t_message);
}


void SetStatusBarTextF(int part_index, const char *fmt, ...)
{
	char buf[256];
	va_list va;

	va_start(va, fmt);
	vsnprintf(buf, WINUI_ARRAY_LENGTH(buf), fmt, va);
	va_end(va);
	SetStatusBarText(part_index, buf);
}

void SetStatusBarTextW(int part_index, const wchar_t *fmt, ...)
{
	wchar_t buf[256];
	va_list va;

	va_start(va, fmt);
	_vstprintf(buf, WINUI_ARRAY_LENGTH(buf), fmt, va);
	va_end(va);
	SetStatusBarTextU(part_index, buf);
}


static void MamePlayBackGame(void)
{
	int nGame = Picker_GetSelectedItem(hWndList);
	if (nGame < 0)
		return;
	char filename[MAX_PATH];
	play_options playopts;

	memset(&playopts, 0, sizeof(playopts));
	*filename = 0;

	if (CommonFileDialog(GetOpenFileName, filename, FILETYPE_INPUT_FILES, false))
	{
		osd_file::error filerr;
		wchar_t *t_filename = win_wstring_from_utf8(filename);
		wchar_t *tempname = PathFindFileName(t_filename);
		char *fname = win_utf8_from_wstring(tempname);
		std::string const name = fname;
		free(t_filename);
		free(fname);

		emu_file check(GetInpDir(), OPEN_FLAG_READ);
		filerr = check.open(name);

		if (filerr != osd_file::error::NONE)
		{
			ErrorMessageBox("Could not open '%s' as a valid input file.", name);
			return;
		}

		inp_header header;

		// read the header and verify that it is a modern version; if not, print an error
		if (!header.read(check))
		{
			ErrorMessageBox("Input file is corrupt or invalid (missing header).");
			return;
		}

		// find game and play it
		std::string const sysname = header.get_sysname();

		for (int i = 0; i < driver_list::total(); i++)
		{
			if (sysname == GetDriverGameName(i))
			{
				nGame = i;
				break;
			}
		}

		playopts.playback = name.c_str();
		MamePlayGameWithOptions(nGame, &playopts);
	}
}

static void MameLoadState(void)
{
	int nGame = Picker_GetSelectedItem(hWndList);
	if (nGame < 0)
		return;
	char filename[MAX_PATH];
	play_options playopts;

	memset(&playopts, 0, sizeof(playopts));
	*filename = 0;

	if (CommonFileDialog(GetOpenFileName, filename, FILETYPE_SAVESTATE_FILES, false))
	{
		char name[MAX_PATH];
		wchar_t *t_filename = win_wstring_from_utf8(filename);
		wchar_t *tempname = PathFindFileName(t_filename);
		PathRemoveExtension(tempname);
		char *fname = win_utf8_from_wstring(tempname);
		strcpy(name, fname);
		free(t_filename);
		free(fname);
		playopts.state = name;
		MamePlayGameWithOptions(nGame, &playopts);
	}
}

static void MamePlayRecordGame(void)
{
	int nGame = Picker_GetSelectedItem(hWndList);
	if (nGame < 0)
		return;
	char filename[MAX_PATH];
	play_options playopts;

	memset(&playopts, 0, sizeof(playopts));
	*filename = 0;
	strcpy(filename, GetDriverGameName(nGame));

	if (CommonFileDialog(GetSaveFileName, filename, FILETYPE_INPUT_FILES, true))
	{
		char name[MAX_PATH];
		wchar_t *t_filename = win_wstring_from_utf8(filename);
		wchar_t *tempname = PathFindFileName(t_filename);
		char *fname = win_utf8_from_wstring(tempname);
		strcpy(name, fname);
		free(t_filename);
		free(fname);
		playopts.record = name;
		MamePlayGameWithOptions(nGame, &playopts);
	}
}

void MamePlayGame(void)
{
	int nGame = Picker_GetSelectedItem(hWndList);
	if (nGame < 0)
		return;
	play_options playopts;

	memset(&playopts, 0, sizeof(playopts));
	MamePlayGameWithOptions(nGame, &playopts);
}

static void MamePlayRecordWave(void)
{
	int nGame = Picker_GetSelectedItem(hWndList);
	if (nGame < 0)
		return;
	char filename[MAX_PATH];
	play_options playopts;

	memset(&playopts, 0, sizeof(playopts));
	*filename = 0;
	strcpy(filename, GetDriverGameName(nGame));

	if (CommonFileDialog(GetSaveFileName, filename, FILETYPE_WAVE_FILES, true))
	{
		playopts.wavwrite = filename;
		MamePlayGameWithOptions(nGame, &playopts);
	}
}

static void MamePlayRecordMNG(void)
{
	int nGame = Picker_GetSelectedItem(hWndList);
	if (nGame < 0)
		return;
	char filename[MAX_PATH];
	play_options playopts;

	memset(&playopts, 0, sizeof(playopts));
	*filename = 0;
	strcpy(filename, GetDriverGameName(nGame));

	if (CommonFileDialog(GetSaveFileName, filename, FILETYPE_MNG_FILES, true))
	{
		char name[MAX_PATH];
		wchar_t *t_filename = win_wstring_from_utf8(filename);
		wchar_t *tempname = PathFindFileName(t_filename);
		char *fname = win_utf8_from_wstring(tempname);
		strcpy(name, fname);
		free(t_filename);
		free(fname);
		playopts.mngwrite = name;
		MamePlayGameWithOptions(nGame, &playopts);
	}
}

static void MamePlayRecordAVI(void)
{
	int nGame = Picker_GetSelectedItem(hWndList);
	if (nGame < 0)
		return;
	char filename[MAX_PATH];
	play_options playopts;

	memset(&playopts, 0, sizeof(playopts));
	*filename = 0;
	strcpy(filename, GetDriverGameName(nGame));

	if (CommonFileDialog(GetSaveFileName, filename, FILETYPE_AVI_FILES, true))
	{
		char name[MAX_PATH];
		wchar_t *t_filename = win_wstring_from_utf8(filename);
		wchar_t *tempname = PathFindFileName(t_filename);
		char *fname = win_utf8_from_wstring(tempname);
		strcpy(name, fname);
		free(t_filename);
		free(fname);
		playopts.aviwrite = name;
		MamePlayGameWithOptions(nGame, &playopts);
	}
}

static void MamePlayGameWithOptions(int nGame, const play_options *playopts)
{
	if (g_pJoyGUI != NULL)
		KillTimer(hMain, JOYGUI_TIMER);

	if (GetCycleScreenshot() > 0)
		KillTimer(hMain, SCREENSHOT_TIMER);

	in_emulation = true;

	//ErrorMessageBox("select game index %d",nGame);

	RunMAME(nGame, playopts);
	IncrementPlayCount(nGame);

	//ErrorMessageBox("select game index %d",GetSelectedPick());

	(void)ListView_RedrawItems(hWndList, GetSelectedPick(), GetSelectedPick());
	in_emulation = false;
	game_launched = true;

	// re-sort if sorting on # of times played
	if (GetSortColumn() == COLUMN_PLAYED)
		Picker_Sort(hWndList);

	UpdateStatusBar();
	UpdateWindow(hMain);
	ShowWindow(hMain, SW_SHOW);
	SetActiveWindow(hMain);
	SetForegroundWindow(hMain);
	SetFocus(hWndList);

	if (g_pJoyGUI != NULL)
		SetTimer(hMain, JOYGUI_TIMER, JOYGUI_MS, NULL);

	if (GetCycleScreenshot() > 0)
		SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL); //scale to seconds
}

/* Toggle ScreenShot ON/OFF */
static void ToggleScreenShot(void)
{
	bool showScreenShot = GetShowScreenShot();

	SetShowScreenShot((showScreenShot) ? false : true);
	UpdateScreenShot();

	/* Redraw list view */
	if (showScreenShot)
		InvalidateRect(hWndList, NULL, false);
}

static void AdjustMetrics(void)
{
	TEXTMETRIC tm;
	AREA area;

	/* WM_SETTINGCHANGE also */
	int xtraX = GetSystemMetrics(SM_CXFIXEDFRAME); 	/* Dialog frame width */
	int xtraY = GetSystemMetrics(SM_CYFIXEDFRAME); 	/* Dialog frame height */
	xtraY += GetSystemMetrics(SM_CYMENUSIZE);		/* Menu height */
	xtraY += GetSystemMetrics(SM_CYCAPTION);		/* Caption Height */
	int maxX = GetSystemMetrics(SM_CXSCREEN); 		/* Screen Width */
	int maxY = GetSystemMetrics(SM_CYSCREEN); 		/* Screen Height */
	HDC hDC = GetDC(hMain);
	GetTextMetrics (hDC, &tm);
	/* Convert MIN Width/Height from Dialog Box Units to pixels. */
	MIN_WIDTH  = (int)((tm.tmAveCharWidth / 4.0) * DBU_MIN_WIDTH)  + xtraX;
	MIN_HEIGHT = (int)((tm.tmHeight / 8.0) * DBU_MIN_HEIGHT) + xtraY;
	ReleaseDC(hMain, hDC);

	HWND hWnd = GetWindow(hMain, GW_CHILD);

	while(hWnd)
	{
		wchar_t szClass[128];

		if (GetClassName(hWnd, szClass, WINUI_ARRAY_LENGTH(szClass)))
		{
			if (!_tcscmp(szClass, WC_LISTVIEW))
			{
				(void)ListView_SetBkColor(hWndList, GetListBgColor());
				(void)ListView_SetTextColor(hWndList, GetListFontColor());
			}
			else if (!_tcscmp(szClass, WC_TREEVIEW))
			{
				(void)TreeView_SetBkColor(hTreeView, GetFolderBgColor());
				(void)TreeView_SetTextColor(hTreeView, GetTreeFontColor());
			}
		}

		hWnd = GetWindow(hWnd, GW_HWNDNEXT);
	}

	GetWindowArea(&area);
	int offX = area.x + area.width;
	int offY = area.y + area.height;

	if (offX > maxX)
	{
		offX = maxX;
		area.x = (offX - area.width > 0) ? (offX - area.width) : 0;
	}

	if (offY > maxY)
	{
		offY = maxY;
		area.y = (offY - area.height > 0) ? (offY - area.height) : 0;
	}

	SetWindowArea(&area);
	SetWindowPos(hMain, HWND_TOP, area.x, area.y, area.width, area.height, 0);
}

int FindIconIndex(int nIconResource)
{
	for (int i = 0; g_iconData[i].icon_name; i++)
	{
		if (g_iconData[i].resource == nIconResource)
			return i;
	}

	return -1;
}

int FindIconIndexByName(const char *icon_name)
{
	for (int i = 0; g_iconData[i].icon_name; i++)
	{
		if (!strcmp(g_iconData[i].icon_name, icon_name))
			return i;
	}

	return -1;
}

static bool UseBrokenIcon(int type)
{
	if (type == 4 && !GetUseBrokenIcon())
		return false;

	return true;
}

static int GetIconForDriver(int nItem)
{
	int iconRoms = 1;

	if (DriverUsesRoms(nItem))
	{
		int audit_result = GetRomAuditResults(nItem);
		if (audit_result == -1)
			return 2;
		else
		if (IsAuditResultYes(audit_result))
			iconRoms = 1;
		else
			iconRoms = 0;
	}

	// iconRoms is now either 0 (no roms), 1 (roms), or 2 (unknown)

	/* these are indices into icon_names, which maps into our image list
     * also must match IDI_WIN_NOROMS + iconRoms
     */

	if (iconRoms == 1)
	{
		// Show Red-X if the ROMs are present and flagged as NOT WORKING
		if (DriverIsBroken(nItem))
			iconRoms = FindIconIndex(IDI_WIN_REDX);  // iconRoms now = 4
		else
		// Show imperfect if the ROMs are present and flagged as imperfect
		if (DriverIsImperfect(nItem))
			iconRoms = FindIconIndex(IDI_WIN_IMPERFECT); // iconRoms now = 5
		else
		// show clone icon if we have roms and game is working
		if (DriverIsClone(nItem))
			iconRoms = FindIconIndex(IDI_WIN_CLONE); // iconRoms now = 3
	}

	// if we have the roms, then look for a custom per-game icon to override
	// not 2, because this indicates F5 must be done; not 0, because this indicates roms are missing; only use 4 if user chooses it
	if (iconRoms == 1 || iconRoms == 3 || iconRoms == 5 || (!UseBrokenIcon(iconRoms)))
	{
		if (icon_index[nItem] == 0)
			AddDriverIcon(nItem,iconRoms);
		iconRoms = icon_index[nItem];
	}

	return iconRoms;
}

static bool HandleTreeContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	TVHITTESTINFO hti;
	POINT pt;

	if ((HWND)wParam != GetDlgItem(hWnd, IDC_TREE))
		return false;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);

	if (pt.x < 0 && pt.y < 0)
		GetCursorPos(&pt);

	/* select the item that was right clicked or shift-F10'ed */
	hti.pt = pt;
	ScreenToClient(hTreeView,&hti.pt);
	(void)TreeView_HitTest(hTreeView,&hti);

	if ((hti.flags & TVHT_ONITEM) != 0)
		(void)TreeView_SelectItem(hTreeView,hti.hItem);

	HMENU hTreeMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_TREE));
	InitTreeContextMenu(hTreeMenu);
	HMENU hMenu = GetSubMenu(hTreeMenu, 0);
	UpdateMenu(hMenu);
	TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
	DestroyMenu(hTreeMenu);
	return true;
}

void GamePicker_OnBodyContextMenu(POINT pt)
{
	if (Picker_GetSelectedItem(hWndList) < 0)
		return;
	HMENU hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_MENU));
	HMENU hMenu = GetSubMenu(hMenuLoad, 0);

	InitBodyContextMenu(hMenu);
	UpdateMenu(hMenu);
	TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hMain, NULL);
	DestroyMenu(hMenuLoad);
}

static bool HandleScreenShotContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	POINT pt;
	MENUINFO mi;

	if ((HWND)wParam != GetDlgItem(hWnd, IDC_SSPICTURE) && (HWND)wParam != GetDlgItem(hWnd, IDC_SSFRAME))
		return false;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);

	if (pt.x < 0 && pt.y < 0)
		GetCursorPos(&pt);

	HMENU hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_SCREENSHOT));
	HMENU hMenu = GetSubMenu(hMenuLoad, 0);
	HMENU hSubMenu = GetSubMenu(hMenu, 2);

	memset(&mi, 0, sizeof(MENUINFO));
	mi.cbSize = sizeof(MENUINFO);
	mi.fMask = MIM_BACKGROUND | MIM_STYLE;
	mi.dwStyle = MNS_CHECKORBMP;
	mi.hbrBack = GetSysColorBrush(COLOR_WINDOW);

	SetMenuInfo(hMenu, &mi);
	SetMenuInfo(hSubMenu, &mi);
	SetMenuItemBitmaps(hMenu, 2, MF_BYPOSITION, hTabs, hTabs);
	UpdateMenu(hMenu);
	TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
	DestroyMenu(hMenuLoad);
	return true;
}

static void UpdateMenu(HMENU hMenu)
{
	MENUITEMINFO mItem;
	LPTREEFOLDER lpFolder = GetCurrentFolder();

	if (have_selection)
	{
		wchar_t buf[200];
		int nGame = Picker_GetSelectedItem(hWndList);

		wchar_t *t_description;
//#ifdef USE_KLIST
		t_description= win_wstring_from_utf8(ConvertAmpersandString(GetDescriptionByIndex(nGame, GetUsekoreanList())));
//#else
//		t_description= win_wstring_from_utf8(ConvertAmpersandString(GetDriverGameTitle(nGame)));
//#endif

 #ifdef MAME_AVI
        EnableMenuItem(hMenu, ID_FILE_PLAY_BACK_AVI,    MF_ENABLED);
        EnableMenuItem(hMenu, ID_FILE_PLAY_WITH_AVI, 	MF_ENABLED);
 #endif /* MAME_AVI */

		if( !t_description )
			return;

		_sntprintf(buf, WINUI_ARRAY_LENGTH(buf), g_szPlayGameString, t_description);
		memset(&mItem, 0, sizeof(MENUITEMINFO));
		mItem.cbSize = sizeof(MENUITEMINFO);
		mItem.fMask = MIIM_TYPE;
		mItem.fType = MFT_STRING;
		mItem.dwTypeData = buf;
		mItem.cch = _tcslen(mItem.dwTypeData);

		SetMenuItemInfo(hMenu, ID_FILE_PLAY, false, &mItem);
		EnableMenuItem(hMenu, ID_CONTEXT_SELECT_RANDOM, MF_ENABLED);
		free(t_description);
	}
	else
	{
		EnableMenuItem(hMenu, ID_FILE_PLAY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD, MF_GRAYED);
		EnableMenuItem(hMenu, ID_GAME_PROPERTIES, MF_GRAYED);
		EnableMenuItem(hMenu, ID_CONTEXT_SELECT_RANDOM, MF_GRAYED);
 #ifdef MAME_AVI
        EnableMenuItem(hMenu, ID_FILE_PLAY_WITH_AVI,    MF_GRAYED);	
 #endif /* MAME_AVI */
	}

	if (lpFolder->m_dwFlags & F_CUSTOM)
	{
		EnableMenuItem(hMenu, ID_CONTEXT_REMOVE_CUSTOM, MF_ENABLED);
		EnableMenuItem(hMenu, ID_CONTEXT_RENAME_CUSTOM, MF_ENABLED);
	}
	else
	{
		EnableMenuItem(hMenu, ID_CONTEXT_REMOVE_CUSTOM, MF_GRAYED);
		EnableMenuItem(hMenu, ID_CONTEXT_RENAME_CUSTOM, MF_GRAYED);
	}

	if (lpFolder->m_dwFlags & F_INIEDIT)
		EnableMenuItem(hMenu,ID_FOLDER_PROPERTIES, MF_ENABLED);
	else
		EnableMenuItem(hMenu,ID_FOLDER_PROPERTIES, MF_GRAYED);

	CheckMenuRadioItem(hMenu, ID_VIEW_TAB_SCREENSHOT, ID_VIEW_TAB_HISTORY, ID_VIEW_TAB_SCREENSHOT + TabView_GetCurrentTab(hTabCtrl), MF_BYCOMMAND);

	// set whether we're showing the tab control or not
	if (bShowTabCtrl)
		CheckMenuItem(hMenu, ID_VIEW_PAGETAB, MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_VIEW_PAGETAB, MF_BYCOMMAND | MF_UNCHECKED);


	for (int i = 0; i < MAX_TAB_TYPES; i++)
	{
		// disable menu items for tabs we're not currently showing
		if (GetShowTab(i))
			EnableMenuItem(hMenu, ID_VIEW_TAB_SCREENSHOT + i, MF_BYCOMMAND | MF_ENABLED);
		else
			EnableMenuItem(hMenu, ID_VIEW_TAB_SCREENSHOT + i, MF_BYCOMMAND | MF_GRAYED);

		// check toggle menu items
		if (GetShowTab(i))
			CheckMenuItem(hMenu, ID_TOGGLE_TAB_SCREENSHOT + i, MF_BYCOMMAND | MF_CHECKED);
		else
			CheckMenuItem(hMenu, ID_TOGGLE_TAB_SCREENSHOT + i, MF_BYCOMMAND | MF_UNCHECKED);
	}

	for (int i = 0; i < MAX_FOLDERS; i++)
	{
		if (GetShowFolder(i))
		{
			CheckMenuItem(hMenu, ID_CONTEXT_SHOW_FOLDER_START + i, MF_BYCOMMAND | MF_CHECKED);
			CheckMenuItem(hMenu, (ID_SHOW_FOLDER_START1 + i) - 1, MF_BYCOMMAND | MF_CHECKED);
		}
		else
		{
			CheckMenuItem(hMenu, ID_CONTEXT_SHOW_FOLDER_START + i, MF_BYCOMMAND | MF_UNCHECKED);
			CheckMenuItem(hMenu, (ID_SHOW_FOLDER_START1 + i) - 1, MF_BYCOMMAND | MF_UNCHECKED);
		}
	}
}

void InitMainMenu(HMENU hMainMenu)
{
	MENUINFO mi;
	MENUITEMINFO mif;
	extern const FOLDERDATA g_folderData[];
	HMENU hFile = GetSubMenu(hMainMenu, 0);
	HMENU hView = GetSubMenu(hMainMenu, 1);
	HMENU hOption = GetSubMenu(hMainMenu, 2);
	HMENU hTools = GetSubMenu(hMainMenu, 3);
	HMENU hAbout = GetSubMenu(hMainMenu, 4);
	HMENU hSubSort = GetSubMenu(hView, 11);
	HMENU hSubFold = GetSubMenu(hView, 13);
	HMENU hSubView = GetSubMenu(hView, 15);
	HMENU hSubFonts = GetSubMenu(hOption, 4);

	memset(&mi, 0, sizeof(MENUINFO));
	mi.cbSize = sizeof(MENUINFO);
	mi.fMask = MIM_BACKGROUND | MIM_STYLE;
	mi.dwStyle = MNS_CHECKORBMP;
	mi.hbrBack = GetSysColorBrush(COLOR_WINDOW);

	SetMenuInfo(hFile, &mi);
	SetMenuInfo(hView, &mi);
	SetMenuInfo(hOption, &mi);
	SetMenuInfo(hTools, &mi);
	SetMenuInfo(hAbout, &mi);
	SetMenuInfo(hSubSort, &mi);
	SetMenuInfo(hSubFold, &mi);
	SetMenuInfo(hSubView, &mi);
	SetMenuInfo(hSubFonts, &mi);

	memset(&mif, 0, sizeof(MENUITEMINFO));
	mif.cbSize = sizeof(MENUITEMINFO);

	for (int i = 0; g_folderData[i].m_lpTitle != NULL; i++)
	{
		wchar_t* t_title = win_wstring_from_utf8(g_folderData[i].m_lpTitle);

		if(!t_title)
			return;

		mif.fMask = MIIM_TYPE | MIIM_ID;
		mif.fType = MFT_STRING;
		mif.dwTypeData = t_title;
		mif.cch = _tcslen(mif.dwTypeData);
		mif.wID = ID_SHOW_FOLDER_START1 + i;

		SetMenuItemInfo(hMainMenu, ID_SHOW_FOLDER_START1 + i, false, &mif);
		free(t_title);
	}

	SetMenuItemBitmaps(hMainMenu, ID_HELP_ABOUT, MF_BYCOMMAND, hAboutMenu, hAboutMenu);
	SetMenuItemBitmaps(hMainMenu, ID_OPTIONS_DIR, MF_BYCOMMAND, hDirectories, hDirectories);
	SetMenuItemBitmaps(hMainMenu, ID_FILE_EXIT, MF_BYCOMMAND, hExit, hExit);
	SetMenuItemBitmaps(hMainMenu, ID_VIEW_FULLSCREEN, MF_BYCOMMAND, hFullscreen, hFullscreen);
	SetMenuItemBitmaps(hMainMenu, ID_OPTIONS_INTERFACE, MF_BYCOMMAND, hInterface, hInterface);
	SetMenuItemBitmaps(hMainMenu, ID_HELP_CONTENTS, MF_BYCOMMAND, hHelp, hHelp);
	SetMenuItemBitmaps(hMainMenu, ID_MAME_HOMEPAGE, MF_BYCOMMAND, hMameHome, hMameHome);
	SetMenuItemBitmaps(hMainMenu, ID_FILE_PLAY, MF_BYCOMMAND, hPlay, hPlay);
	SetMenuItemBitmaps(hMainMenu, ID_VIDEO_SNAP, MF_BYCOMMAND, hVideo, hVideo);
	SetMenuItemBitmaps(hMainMenu, ID_PLAY_M1, MF_BYCOMMAND, hPlayM1, hPlayM1);
	SetMenuItemBitmaps(hMainMenu, ID_OPTIONS_DEFAULTS, MF_BYCOMMAND, hOptions, hOptions);
	SetMenuItemBitmaps(hMainMenu, ID_UPDATE_GAMELIST, MF_BYCOMMAND, hRefresh, hRefresh);
	SetMenuItemBitmaps(hMainMenu, ID_FILE_GAMELIST, MF_BYCOMMAND, hSaveList, hSaveList);
	SetMenuItemBitmaps(hMainMenu, ID_FILE_ROMSLIST, MF_BYCOMMAND, hSaveRoms, hSaveRoms);
	SetMenuItemBitmaps(hMainMenu, ID_FILE_PLAY_BACK, MF_BYCOMMAND, hPlayback, hPlayback);
	SetMenuItemBitmaps(hMainMenu, ID_FILE_AUDIT, MF_BYCOMMAND, hAuditMenu, hAuditMenu);
	SetMenuItemBitmaps(hMainMenu, ID_PLAY_VIDEO, MF_BYCOMMAND, hVideo, hVideo);
	SetMenuItemBitmaps(hOption, 4, MF_BYPOSITION, hFonts, hFonts);
	SetMenuItemBitmaps(hView, 13, MF_BYPOSITION, hFolders, hFolders);
	SetMenuItemBitmaps(hView, 11, MF_BYPOSITION, hSort, hSort);
	SetMenuItemBitmaps(hMainMenu, ID_MAME_FAQ, MF_BYCOMMAND, hFaq, hFaq);
	SetMenuItemBitmaps(hView, 15, MF_BYPOSITION, hTabs, hTabs);
	SetMenuItemBitmaps(hMainMenu, ID_HELP_TROUBLE, MF_BYCOMMAND, hTrouble, hTrouble);
	SetMenuItemBitmaps(hMainMenu, ID_HELP_WHATS_NEW, MF_BYCOMMAND, hRelease, hRelease);
	SetMenuItemBitmaps(hSubSort, ID_VIEW_BYGAME, MF_BYCOMMAND, hDescription, hDescription);
	SetMenuItemBitmaps(hSubSort, ID_VIEW_BYDIRECTORY, MF_BYCOMMAND, hRom, hRom);
	SetMenuItemBitmaps(hSubSort, ID_VIEW_BYSOURCE, MF_BYCOMMAND, hSource, hSource);
	SetMenuItemBitmaps(hSubSort, ID_VIEW_BYMANUFACTURER, MF_BYCOMMAND, hManufacturer, hManufacturer);
	SetMenuItemBitmaps(hSubSort, ID_VIEW_BYYEAR, MF_BYCOMMAND, hYear, hYear);
	SetMenuItemBitmaps(hSubSort, ID_VIEW_BYTIMESPLAYED, MF_BYCOMMAND, hCount, hCount);
	SetMenuItemBitmaps(hMainMenu, ID_PLAY_AUDIO, MF_BYCOMMAND, hPlaywav, hPlaywav);
	SetMenuItemBitmaps(hSubFonts, ID_OPTIONS_TREE_FONT, MF_BYCOMMAND, hFont1, hFont1);
	SetMenuItemBitmaps(hSubFonts, ID_OPTIONS_HISTORY_FONT, MF_BYCOMMAND, hFont2, hFont2);
	SetMenuItemBitmaps(hSubFonts, ID_OPTIONS_FONT, MF_BYCOMMAND, hFont1, hFont1);
	SetMenuItemBitmaps(hSubFonts, ID_OPTIONS_HISTORY_COLOR, MF_BYCOMMAND, hInfoback, hInfoback);
	SetMenuItemBitmaps(hSubFonts, ID_OPTIONS_LIST_COLOR, MF_BYCOMMAND, hListback, hListback);
	SetMenuItemBitmaps(hSubFonts, ID_OPTIONS_FOLDERS_COLOR, MF_BYCOMMAND, hTreeback, hTreeback);
	SetMenuItemBitmaps(hMainMenu, ID_CUSTOMIZE_FIELDS, MF_BYCOMMAND, hFields, hFields);
	SetMenuItemBitmaps(hMainMenu, ID_FILE_PLAY_RECORD_AVI, MF_BYCOMMAND, hRecavi, hRecavi);
	SetMenuItemBitmaps(hMainMenu, ID_FILE_PLAY_RECORD, MF_BYCOMMAND, hRecinput, hRecinput);
	SetMenuItemBitmaps(hMainMenu, ID_FILE_PLAY_RECORD_WAVE, MF_BYCOMMAND, hRecwav, hRecwav);
	SetMenuItemBitmaps(hMainMenu, ID_PLAY_MNG, MF_BYCOMMAND, hPlaymng, hPlaymng);
	SetMenuItemBitmaps(hMainMenu, ID_CONTEXT_SELECT_RANDOM, MF_BYCOMMAND, hRandom, hRandom);
	SetMenuItemBitmaps(hMainMenu, ID_FILE_PLAY_RECORD_MNG, MF_BYCOMMAND, hRecmng, hRecmng);
	SetMenuItemBitmaps(hMainMenu, ID_FILE_LOADSTATE, MF_BYCOMMAND, hSavestate, hSavestate);
	SetMenuItemBitmaps(hMainMenu, ID_CONTEXT_FILTERS, MF_BYCOMMAND, hFilters, hFilters);
	SetMenuItemBitmaps(hMainMenu, ID_OPTIONS_RESET_DEFAULTS, MF_BYCOMMAND, hReset, hReset);
	SetMenuItemBitmaps(hMainMenu, ID_KOREAN_GAMELIST, MF_BYCOMMAND, hklist, hklist); // USE_KLIST
	
	
}

void InitTreeContextMenu(HMENU hTreeMenu)
{
	MENUINFO mi;
	MENUITEMINFO mii;
	extern const FOLDERDATA g_folderData[];
	HMENU hMenuTree = GetSubMenu(hTreeMenu, 0);

	memset(&mi, 0, sizeof(MENUINFO));
	mi.cbSize = sizeof(MENUINFO);
	mi.fMask = MIM_BACKGROUND | MIM_STYLE;
	mi.dwStyle = MNS_CHECKORBMP;
	mi.hbrBack = GetSysColorBrush(COLOR_WINDOW);

	SetMenuInfo(hMenuTree, &mi);

	memset(&mii, 0, sizeof(MENUITEMINFO));
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.wID = -1;
	mii.fMask = MIIM_SUBMENU | MIIM_ID;

	if (GetMenuItemInfo(hMenuTree, 3, true, &mii) == false)
		return;

	if (mii.hSubMenu == NULL)
		return;

	SetMenuItemBitmaps(hMenuTree, ID_FOLDER_PROPERTIES, MF_BYCOMMAND, hProperties, hProperties);
	SetMenuItemBitmaps(hMenuTree, ID_FOLDER_AUDIT, MF_BYCOMMAND, hAuditMenu, hAuditMenu);
	SetMenuItemBitmaps(hMenuTree, 3, MF_BYPOSITION, hFolders, hFolders);
	SetMenuItemBitmaps(hMenuTree, ID_CONTEXT_FILTERS, MF_BYCOMMAND, hFilters, hFilters);
	SetMenuItemBitmaps(hMenuTree, ID_CONTEXT_RENAME_CUSTOM, MF_BYCOMMAND, hRename, hRename);
	hMenuTree = mii.hSubMenu;
	SetMenuInfo(hMenuTree, &mi);

	for (int i = 0; g_folderData[i].m_lpTitle != NULL; i++)
	{
		wchar_t* t_title = win_wstring_from_utf8(g_folderData[i].m_lpTitle);

		if(!t_title)
			return;

		mii.fMask = MIIM_TYPE | MIIM_ID;
		mii.fType = MFT_STRING;
		mii.dwTypeData = t_title;
		mii.cch = _tcslen(mii.dwTypeData);
		mii.wID = ID_CONTEXT_SHOW_FOLDER_START + g_folderData[i].m_nFolderId;

		// menu in resources has one empty item (needed for the submenu to setup properly)
		// so overwrite this one, append after
		if (i == 0)
			SetMenuItemInfo(hMenuTree, ID_CONTEXT_SHOW_FOLDER_START, false, &mii);
		else
			InsertMenuItem(hMenuTree, i, false, &mii);

		free(t_title);
	}
}

void InitBodyContextMenu(HMENU hBodyContextMenu)
{
	wchar_t tmp[64];
	MENUINFO mi;
	MENUITEMINFO mii;

	memset(&mi, 0, sizeof(MENUINFO));
	mi.cbSize = sizeof(MENUINFO);
	mi.fMask = MIM_BACKGROUND | MIM_STYLE;
	mi.dwStyle = MNS_CHECKORBMP;
	mi.hbrBack = GetSysColorBrush(COLOR_WINDOW);

	SetMenuInfo(hBodyContextMenu, &mi);

	memset(&mii, 0, sizeof(MENUITEMINFO));
	mii.cbSize = sizeof(MENUITEMINFO);

	if (GetMenuItemInfo(hBodyContextMenu, ID_FOLDER_SOURCEPROPERTIES, false, &mii) == false)
		return;

	LPTREEFOLDER lpFolder = GetFolderByName(FOLDER_SOURCE, GetDriverFileName(Picker_GetSelectedItem(hWndList)));
	_sntprintf(tmp, WINUI_ARRAY_LENGTH(tmp), TEXT("Properties for %s\tAlt+D"), lpFolder->m_lptTitle);

	mii.fMask = MIIM_TYPE | MIIM_ID;
	mii.fType = MFT_STRING;
	mii.dwTypeData = tmp;
	mii.cch = _tcslen(mii.dwTypeData);
	mii.wID = ID_FOLDER_SOURCEPROPERTIES;

	// menu in resources has one default item
	// so overwrite this one
	SetMenuItemInfo(hBodyContextMenu, ID_FOLDER_SOURCEPROPERTIES, false, &mii);
	SetMenuItemBitmaps(hBodyContextMenu, ID_CONTEXT_ADD_CUSTOM, MF_BYCOMMAND, hCustom, hCustom);
	SetMenuItemBitmaps(hBodyContextMenu, ID_FILE_PLAY, MF_BYCOMMAND, hPlay, hPlay);
	SetMenuItemBitmaps(hBodyContextMenu, ID_VIDEO_SNAP, MF_BYCOMMAND, hVideo, hVideo);
	SetMenuItemBitmaps(hBodyContextMenu, ID_PLAY_M1, MF_BYCOMMAND, hPlayM1, hPlayM1);
	SetMenuItemBitmaps(hBodyContextMenu, ID_VIEW_ZIP, MF_BYCOMMAND, hZip, hZip);
	SetMenuItemBitmaps(hBodyContextMenu, ID_MANUAL, MF_BYCOMMAND, hManual, hManual);
	SetMenuItemBitmaps(hBodyContextMenu, ID_NOTEPAD, MF_BYCOMMAND, hNotepad, hNotepad);
	SetMenuItemBitmaps(hBodyContextMenu, ID_GAME_PROPERTIES, MF_BYCOMMAND, hProperties, hProperties);
	SetMenuItemBitmaps(hBodyContextMenu, ID_GAME_INFO, MF_BYCOMMAND, hRelease, hRelease);
	SetMenuItemBitmaps(hBodyContextMenu, ID_GAME_AUDIT, MF_BYCOMMAND, hAuditMenu, hAuditMenu);
	SetMenuItemBitmaps(hBodyContextMenu, ID_FOLDER_SOURCEPROPERTIES, MF_BYCOMMAND, hDriver, hDriver);
	SetMenuItemBitmaps(hBodyContextMenu, ID_CONTEXT_RESET_PLAYCOUNT, MF_BYCOMMAND, hCount, hCount);
	SetMenuItemBitmaps(hBodyContextMenu, ID_CONTEXT_RESET_PLAYTIME, MF_BYCOMMAND, hTime, hTime);
	SetMenuItemBitmaps(hBodyContextMenu, ID_FILE_PLAY_RECORD, MF_BYCOMMAND, hRecinput, hRecinput);
	SetMenuItemBitmaps(hBodyContextMenu, ID_CONTEXT_REMOVE_CUSTOM, MF_BYCOMMAND, hRemove, hRemove);
	SetMenuItemBitmaps(hBodyContextMenu, ID_FILE_LOADSTATE, MF_BYCOMMAND, hSavestate, hSavestate);
}

void ToggleShowFolder(int folder)
{
	int current_id = GetCurrentFolderID();

	SetWindowRedraw(hWndList, false);
	SetWindowRedraw(hTreeView, false);
	SetShowFolder(folder, !GetShowFolder(folder));
	ResetTreeViewFolders();
	SelectTreeViewFolder(current_id);
	SetWindowRedraw(hTreeView, true);
	SetWindowRedraw(hWndList, true);
}

static LRESULT CALLBACK PictureFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_NCHITTEST :
		{
			POINT pt;
			RECT  rect;
			HWND hHistory = GetDlgItem(hMain, IDC_HISTORY);

			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			GetWindowRect(hHistory, &rect);
			// check if they clicked on the picture area (leave 6 pixel no man's land
			// by the history window to reduce mistaken clicks)
			// no more no man's land, the Cursor changes when Edit control is left, should be enough feedback
			if (have_history && ( ( (TabView_GetCurrentTab(hTabCtrl) == TAB_HISTORY) ||
				(TabView_GetCurrentTab(hTabCtrl) == GetHistoryTab() && GetShowTab(TAB_HISTORY) == false) ||
				(TAB_ALL == GetHistoryTab() && GetShowTab(TAB_HISTORY) == false) ) && PtInRect( &rect, pt ) ) )
				return HTTRANSPARENT;
			else
				return HTCLIENT;

			break;
		}

		case WM_CONTEXTMENU:
			if ( HandleScreenShotContextMenu(hWnd, wParam, lParam))
				return false;

			break;
	}

	return CallWindowProc(g_lpPictureFrameWndProc, hWnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK PictureWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC	hdc, hdc_temp;
			RECT rect;
			HBITMAP old_bitmap;
			int width = 0;
			int height = 0;
			RECT rect2;
			int nBordersize = GetScreenshotBorderSize();
			HBRUSH hBrush = CreateSolidBrush(GetScreenshotBorderColor());

			hdc = BeginPaint(hWnd, &ps);
			hdc_temp = CreateCompatibleDC(hdc);

			if (ScreenShotLoaded())
			{
				width = GetScreenShotWidth();
				height = GetScreenShotHeight();
				old_bitmap = (HBITMAP)SelectObject(hdc_temp,GetScreenShotHandle());
			}
			else
			{
				BITMAP bmp;

				GetObject(hMissing_bitmap, sizeof(BITMAP), &bmp);
				width = bmp.bmWidth;
				height = bmp.bmHeight;
				old_bitmap = (HBITMAP)SelectObject(hdc_temp,hMissing_bitmap);
			}

			GetClientRect(hWnd,&rect);
			rect2 = rect;
			//Configurable Borders around images
			rect.bottom -= nBordersize;

			if (rect.bottom < 0)
				rect.bottom = rect2.bottom;

			rect.right -= nBordersize;

			if (rect.right < 0)
				rect.right = rect2.right;

			rect.top += nBordersize;

			if (rect.top > rect.bottom)
				rect.top = rect2.top;

			rect.left += nBordersize;

			if (rect.left > rect.right)
				rect.left = rect2.left;

			HRGN region1 = CreateRectRgnIndirect(&rect);
			HRGN region2 = CreateRectRgnIndirect(&rect2);
			CombineRgn(region2, region2, region1, RGN_DIFF);
			HBRUSH holdBrush = (HBRUSH)SelectObject(hdc, hBrush);
			FillRgn(hdc,region2, hBrush);
			SelectObject(hdc, holdBrush);
			DeleteBrush(hBrush);
			SetStretchBltMode(hdc, STRETCH_HALFTONE);
			StretchBlt(hdc,nBordersize,nBordersize,rect.right-rect.left,rect.bottom-rect.top, hdc_temp, 0, 0, width, height, SRCCOPY);
			SelectObject(hdc_temp,old_bitmap);
			DeleteDC(hdc_temp);
			DeleteObject(region1);
			DeleteObject(region2);
			EndPaint(hWnd,&ps);
			return true;
		}
	}

	return CallWindowProc(g_lpPictureWndProc, hWnd, uMsg, wParam, lParam);
}

static void RemoveCurrentGameCustomFolder(void)
{
	RemoveGameCustomFolder(Picker_GetSelectedItem(hWndList));
}

static void RemoveGameCustomFolder(int driver_index)
{
	TREEFOLDER **folders;
	int num_folders = 0;

	GetFolders(&folders, &num_folders);

	for (int i = 0; i < num_folders; i++)
	{
		if (folders[i]->m_dwFlags & F_CUSTOM && folders[i]->m_nFolderId == GetCurrentFolderID())
		{
			RemoveFromCustomFolder(folders[i], driver_index);

			if (driver_index == Picker_GetSelectedItem(hWndList))
			{
				/* if we just removed the current game,
				move the current selection so that when we rebuild the listview it leaves the cursor on next or previous one */
				int current_pick_index = GetSelectedPick();
				Picker_SetSelectedPick(hWndList, GetSelectedPick() + 1);

				if (current_pick_index == GetSelectedPick()) /* we must have deleted the last item */
					Picker_SetSelectedPick(hWndList, GetSelectedPick() - 1);
			}

			UpdateListView();
			return;
		}
	}

	ErrorMessageBox("Error searching for custom folder");

}

void BeginListViewDrag(NM_LISTVIEW *pnmv)
{
	LVITEM lvi;
	POINT pt;

	lvi.iItem = pnmv->iItem;
	lvi.mask = LVIF_PARAM;

	(void)ListView_GetItem(hWndList, &lvi);
	game_dragged = lvi.lParam;
	pt.x = 0;
	pt.y = 0;
	/* Tell the list view control to create an image to use for dragging. */
	himl_drag = ListView_CreateDragImage(hWndList, pnmv->iItem, &pt);
	/* Start the drag operation. */
	ImageList_BeginDrag(himl_drag, 0, 0, 0);
	pt = pnmv->ptAction;
	ClientToScreen(hWndList,&pt);
	ImageList_DragEnter(GetDesktopWindow(), pt.x, pt.y);
	/* Hide the mouse cursor, and direct mouse input to the parent window. */
	SetCapture(hMain);
	prev_drag_drop_target = NULL;
	g_listview_dragging = true;
}

static void MouseMoveListViewDrag(POINTS p)
{
	TV_HITTESTINFO tvht;
	POINT pt;

	pt.x = p.x;
	pt.y = p.y;
	ClientToScreen(hMain,&pt);
	ImageList_DragMove(pt.x,pt.y);
	MapWindowPoints(GetDesktopWindow(), hTreeView, &pt, 1);
	tvht.pt = pt;
	HTREEITEM htiTarget = TreeView_HitTest(hTreeView,&tvht);

	if (htiTarget != prev_drag_drop_target)
	{
		ImageList_DragShowNolock(false);

		if (htiTarget != NULL)
			(void)TreeView_SelectDropTarget(hTreeView,htiTarget);
		else
			(void)TreeView_SelectDropTarget(hTreeView,NULL);

		ImageList_DragShowNolock(true);
		prev_drag_drop_target = htiTarget;
	}
}

static void ButtonUpListViewDrag(POINTS p)
{
	POINT pt;
	TV_HITTESTINFO tvht;
	TVITEM tvi;

	ReleaseCapture();
	ImageList_DragLeave(hWndList);
	ImageList_EndDrag();
	ImageList_Destroy(himl_drag);
	(void)TreeView_SelectDropTarget(hTreeView,NULL);
	g_listview_dragging = false;
	/* see where the game was dragged */
	pt.x = p.x;
	pt.y = p.y;
	MapWindowPoints(hMain, hTreeView, &pt, 1);
	tvht.pt = pt;
	HTREEITEM htiTarget = TreeView_HitTest(hTreeView,&tvht);

	if (htiTarget == NULL)
	{
		LVHITTESTINFO lvhtti;
		RECT rcList;

		/* the user dragged a game onto something other than the treeview */
		/* try to remove if we're in a custom folder */
		/* see if it was dragged within the list view; if so, ignore */
		MapWindowPoints(hTreeView, hWndList, &pt, 1);
		lvhtti.pt = pt;
		GetWindowRect(hWndList, &rcList);
		ClientToScreen(hWndList, &pt);

		if( PtInRect(&rcList, pt) != 0 )
			return;

		LPTREEFOLDER folder = GetCurrentFolder();

		if (folder->m_dwFlags & F_CUSTOM)
		{
			/* dragged out of a custom folder, so let's remove it */
			RemoveCurrentGameCustomFolder();
		}

		return;
	}

	tvi.lParam = 0;
	tvi.mask = TVIF_PARAM | TVIF_HANDLE;
	tvi.hItem = htiTarget;

	if (TreeView_GetItem(hTreeView, &tvi))
	{
		LPTREEFOLDER folder = (LPTREEFOLDER)tvi.lParam;
		AddToCustomFolder(folder, game_dragged);
	}

}

static LPTREEFOLDER GetSelectedFolder(void)
{
	HTREEITEM htree = TreeView_GetSelection(hTreeView);
	TVITEM tvi;

	if(htree != NULL)
	{
		tvi.hItem = htree;
		tvi.mask = TVIF_PARAM;
		(void)TreeView_GetItem(hTreeView,&tvi);
		return (LPTREEFOLDER)tvi.lParam;
	}

	return NULL;
}

/* Updates all currently displayed Items in the List with the latest Data*/
void UpdateListView(void)
{
	//ErrorMessageBox("update listview");

	ResetWhichGamesInFolders();
	ResetListView();
	if( (GetViewMode() == VIEW_GROUPED) || (GetViewMode() == VIEW_REPORT ) )
		(void)ListView_RedrawItems(hWndList, ListView_GetTopIndex(hWndList), ListView_GetTopIndex(hWndList) + ListView_GetCountPerPage(hWndList));
	SetFocus(hWndList);
}

static void CalculateBestScreenShotRect(HWND hWnd, RECT *pRect, bool restrict_height)
{
	RECT rect;
	/* for scaling */
	int destW = 0; 
	int destH = 0;
	int x = 0;
	int y = 0;
	double scale = 0;
	bool bReduce = false;

	GetClientRect(hWnd, &rect);

	// Scale the bitmap to the frame specified by the passed in hwnd
	if (ScreenShotLoaded())
	{
		x = GetScreenShotWidth();
		y = GetScreenShotHeight();
	}
	else
	{
		BITMAP bmp;

		GetObject(hMissing_bitmap,sizeof(BITMAP),&bmp);
		x = bmp.bmWidth;
		y = bmp.bmHeight;
	}

	int rWidth  = (rect.right  - rect.left);
	int rHeight = (rect.bottom - rect.top);

	/* Limit the screen shot to max height of 264 */
	if (restrict_height == true && rHeight > 264)
	{
		rect.bottom = rect.top + 264;
		rHeight = 264;
	}

	/* If the bitmap does NOT fit in the screenshot area */
	if (x > rWidth - 10 || y > rHeight - 10)
	{
		rect.right -= 10;
		rect.bottom -= 10;
		rWidth -= 10;
		rHeight -= 10;
		bReduce = true;

		/* Try to scale it properly */
		/*  assumes square pixels, doesn't consider aspect ratio */
		if (x > y)
			scale = (double)rWidth / x;
		else
			scale = (double)rHeight / y;

		destW = (int)(x * scale);
		destH = (int)(y * scale);

		/* If it's still too big, scale again */
		if (destW > rWidth || destH > rHeight)
		{
			if (destW > rWidth)
				scale = (double)rWidth / destW;
			else
				scale = (double)rHeight / destH;

			destW = (int)(destW * scale);
			destH = (int)(destH * scale);
		}
	}
	else
	{
		if (GetStretchScreenShotLarger())
		{
			rect.right -= 10;
			rect.bottom -= 10;
			rWidth -= 10;
			rHeight -= 10;
			bReduce = true;

			// Try to scale it properly
			// assumes square pixels, doesn't consider aspect ratio
			if (x < y)
				scale = (double)rWidth / x;
			else
				scale = (double)rHeight / y;

			destW = (int)(x * scale);
			destH = (int)(y * scale);

			// If it's too big, scale again
			if (destW > rWidth || destH > rHeight)
			{
				if (destW > rWidth)
					scale = (double)rWidth / destW;
				else
					scale = (double)rHeight / destH;

				destW = (int)(destW * scale);
				destH = (int)(destH * scale);
			}
		}
		else
		{
			// Use the bitmaps size if it fits
			destW = x;
			destH = y;
		}

	}

	int destX = ((rWidth  - destW) / 2);
	int destY = ((rHeight - destH) / 2);

	if (bReduce)
	{
		destX += 5;
		destY += 5;
	}

	int nBorder = GetScreenshotBorderSize();

	if( destX > nBorder+1)
		pRect->left = destX - nBorder;
	else
		pRect->left = 2;

	if( destY > nBorder+1)
		pRect->top = destY - nBorder;
	else
		pRect->top = 2;

	if( rWidth >= destX + destW + nBorder)
		pRect->right = destX + destW + nBorder;
	else
		pRect->right = rWidth - pRect->left;

	if( rHeight >= destY + destH + nBorder)
		pRect->bottom = destY + destH + nBorder;
	else
		pRect->bottom = rHeight - pRect->top;
}

/*
  Switches to either fullscreen or normal mode, based on the
  current mode.

  POSSIBLE BUGS:
  Removing the menu might cause problems later if some
  function tries to poll info stored in the menu. Don't
  know if you've done that, but this was the only way I
  knew to remove the menu dynamically.
*/

static void SwitchFullScreenMode(void)
{
	// LONG lStyle = GetWindowLong(hMain, GWL_STYLE);
	// lStyle &= ~(WS_CAPTION | WS_THICKFRAME );
	// SetWindowLong(hMain, GWL_STYLE, lStyle);

	// LONG lExStyle = GetWindowLong(hMain, GWL_EXSTYLE);
	// lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
	// SetWindowLong(hMain, GWL_EXSTYLE, lExStyle);
	
	// SetWindowPos(hMain, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

	if (bFullScreen)
	{
		// Hide the window
		ShowWindow(hMain, SW_HIDE);
		// Restore the menu
		SetMenu(hMain, LoadMenu(hInst, MAKEINTRESOURCE(IDR_UI_MENU)));
		InitMainMenu(GetMenu(hMain));
		// Refresh the checkmarks
		CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, GetShowFolderList() ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, GetShowToolBar() ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, GetShowStatusBar() ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_PAGETAB, GetShowTabCtrl() ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(GetMenu(hMain), ID_ENABLE_INDENT, GetEnableIndent() ? MF_CHECKED : MF_UNCHECKED);

		// Add frame to dialog again
		SetWindowLong(hMain, GWL_STYLE, GetWindowLong(hMain, GWL_STYLE) | WS_OVERLAPPEDWINDOW);

		// Restore the window
		if (GetWindowState() == SW_MAXIMIZE)
			ShowWindow(hMain, SW_MAXIMIZE);
		else
			ShowWindow(hMain, SW_SHOWNORMAL);

		bFullScreen = !bFullScreen;
	}
	else
	{
		// Hide the window
		ShowWindow(hMain, SW_HIDE);
		// Remove menu
		//SetMenu(hMain, NULL);
		// Frameless dialog
		SetWindowLong(hMain, GWL_STYLE, GetWindowLong(hMain, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);

		// Keep track if we're already maximized before fullscreen
		if (IsMaximized(hMain))
			SetWindowState(SW_MAXIMIZE);
		else
			SetWindowState(SW_SHOWNORMAL);

		// Maximize the window
		ShowWindow(hMain, SW_MAXIMIZE);
		bFullScreen = !bFullScreen;
	}
}

// 修改的 代码来源 (缘来是你)
// 缘来是你==================================== DPI ===================================>>>
#if USE_SPLASH_SCREEN
static intptr_t CALLBACK StartupProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			int imgWidth = (int)(461 * g_fDpiScale);
			int imgHeight = (int)(136 * g_fDpiScale);
			
			HBITMAP hBmp = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SPLASH), IMAGE_BITMAP, imgWidth, imgHeight, LR_CREATEDIBSECTION);
			SendMessage(GetDlgItem(hDlg, IDC_SPLASH), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
			
			hBrush = GetSysColorBrush(COLOR_3DFACE);
			
			int progX = 0;
			int progY = imgHeight; 
			int progW = imgWidth;
			int progH = (int)(18 * g_fDpiScale);
			
			hProgress = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE, progX, progY, progW, progH, hDlg, NULL, hInst, NULL);
			SetWindowTheme(hProgress, L" ", L" ");
			SendMessage(hProgress, PBM_SETBKCOLOR, 0, GetSysColor(COLOR_3DFACE));
			SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 120));
			SendMessage(hProgress, PBM_SETPOS, 0, 0);
			return true;
		}

		case WM_DESTROY:
		{
			HBITMAP hBmp = (HBITMAP)SendMessage(GetDlgItem(hDlg, IDC_SPLASH), STM_GETIMAGE, IMAGE_BITMAP, 0);
			if (hBmp) DeleteObject(hBmp);
			return true;
		}

		case WM_CTLCOLORDLG:
			return (LRESULT) hBrush;

		case WM_CTLCOLORSTATIC:
			hDC = (HDC)wParam;
			SetBkMode(hDC, TRANSPARENT);
			SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
			return (LRESULT) hBrush;
	}

	return false;
}
#endif
//======================================================================================>>>

static bool CommonListDialog(common_file_dialog_proc cfd, int filetype)
{
	bool success = false;
	OPENFILENAME of;
	wchar_t szFile[MAX_PATH];
	wchar_t szCurDir[MAX_PATH];

	szFile[0] = 0;

	// Save current directory (avoids mame file creation further failure)
	if (GetCurrentDirectory(MAX_PATH, szCurDir) > MAX_PATH)
	{
		// Path too large
		szCurDir[0] = 0;
	}

	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = hMain;
	of.hInstance = NULL;

	if (filetype == FILETYPE_GAME_LIST)
		of.lpstrTitle  = TEXT("Enter a name for the game list file");
	else
		of.lpstrTitle  = TEXT("Enter a name for the ROMs list file");

	of.lpstrFilter = TEXT("Standard text file (*.txt)\0*.txt\0");
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter = 0;
	of.nFilterIndex = 1;
	of.lpstrFile = szFile;
	of.nMaxFile = sizeof(szFile);
	of.lpstrFileTitle = NULL;
	of.nMaxFileTitle = 0;
	of.lpstrInitialDir = list_directory;
	of.nFileOffset = 0;
	of.nFileExtension = 0;
	of.lpstrDefExt = TEXT("txt");
	of.lCustData = 0;
	of.lpfnHook = &OFNHookProc;
	of.lpTemplateName = NULL;
	of.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLEHOOK;

	while(!success)
	{
		if (GetSaveFileName(&of))
		{
			if (GetFileAttributes(szFile) != -1)
			{
				if (winui_message_box_utf8(hMain, "File already exists, overwrite ?", MAMEUINAME, MB_ICONQUESTION | MB_YESNO) != IDYES )
					continue;
				else
					success = true;

				SetFileAttributes(szFile, FILE_ATTRIBUTE_NORMAL);
			}

			SaveGameListToFile(win_utf8_from_wstring(szFile), filetype);
			// Save current directory (avoids mame file creation further failure)
			GetCurrentDirectory(MAX_PATH, list_directory);
			// Restore current file path
			if (szCurDir[0] != 0)
				SetCurrentDirectory(szCurDir);

			success = true;
		}
		else
			break;
	}

	if (success)
		return true;
	else
		return false;
}

static void SaveGameListToFile(char *szFile, int filetype)
{
	int nListCount = ListView_GetItemCount(hWndList);
	const char *CrLf = "\n\n";
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	LVITEM lvi;

	FILE *f = fopen(szFile, "w");

	if (f == NULL)
	{
		ErrorMessageBox("Error : unable to access file");
		return;
	}

	// Title
	fprintf(f, "%s %s.%s", MAMEUINAME, GetVersionString(), CrLf);

	if (filetype == FILETYPE_GAME_LIST)
		fprintf(f, "This is the current list of games.%s", CrLf);
	else
		fprintf(f, "This is the current list of ROMs.%s", CrLf);

	// Current folder
	fprintf(f, "Current folder : <");

	if (lpFolder->m_nParent != -1)
	{
		// Shows only 2 levels (last and previous)
		LPTREEFOLDER lpF = GetFolder(lpFolder->m_nParent);

		if (lpF->m_nParent == -1)
				fprintf(f, "\\");
 
		fprintf(f, "%s", lpF->m_lpTitle);
		fprintf(f, "\\");
	}
	else
		fprintf(f, "\\");
 
	fprintf(f, "%s>%s.%s", lpFolder->m_lpTitle, (lpFolder->m_dwFlags & F_CUSTOM) ? " (custom folder)" : "", CrLf);

	// Sorting
	if (GetSortColumn() > 0)

		fprintf(f, "Sorted by <%s> descending order", win_utf8_from_wstring(column_names[GetSortColumn()]));
	else
		fprintf(f, "Sorted by <%s> ascending order", win_utf8_from_wstring(column_names[-GetSortColumn()]));

	fprintf(f, ", %d game(s) found.%s", nListCount, CrLf);

	// Games
	for (int nIndex = 0; nIndex < nListCount; nIndex++)
	{
		lvi.iItem = nIndex;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_PARAM;

		if (ListView_GetItem(hWndList, &lvi))
		{
			int nGameIndex  = lvi.lParam;

			if (filetype == FILETYPE_GAME_LIST)
				fprintf(f, "%s", GetDriverGameTitle(nGameIndex));
			else
				fprintf(f, "%s", GetDriverGameName(nGameIndex));

			fprintf(f, "\n");
		}
	}

	fclose(f);
	winui_message_box_utf8(hMain, "File saved successfully.", MAMEUINAME, MB_ICONINFORMATION | MB_OK);
}

//#ifdef USE_KLIST
static void TSV_GetPath(char *path)
{
	char drive[256];
	char dir[256];
	char szFilname[MAX_PATH];
	
	GetModuleFileNameA(NULL, szFilname, MAX_PATH);
	_splitpath(szFilname, drive, dir, NULL, NULL);

	strcpy(szFilname, drive);
	strcat(szFilname, dir);
	strcat(szFilname, TSVNAME);
	
	strcpy(path,szFilname);	
}

static void LoadGameListFromFile(int games)
{

	char tsvname[MAX_PATH];
	int  i, j;


	tsv_index = (TSV *)calloc(games + 1, sizeof(TSV));
	if (tsv_index == NULL)
	{
		return;
	}
	
	tsv_data = (TSV *)calloc(games + 1, sizeof(TSV));
	if (tsv_data == NULL)
	{
		free(tsv_index);
		tsv_index = NULL;
		return;
	}


	TSV_GetPath(tsvname);	

	FILE *f = fopen(tsvname, "r");

	//winui_message_box_utf8(hMain, "mame32k list file open complete", MAMEUINAME, MB_ICONINFORMATION | MB_OK);

	if (f != NULL)
	{	
		char *line, *p[NUM_COLUMNS], buf[LINEBUF_SIZE];
		const char *token = "\t\r\n";

		i = 0;
		while (fgets(buf, LINEBUF_SIZE, f))
		{
			line = buf;
			for (j = 0; j < NUM_COLUMNS; j++)
			{
				p[j] = strtok(line,  token);
				if (p[j] == NULL) break;
				line = NULL;
			}

			tsv_data[i].gamename      = strdup(p[0]);
			tsv_data[i].description   = strdup(p[1]);
			tsv_data[i].manufacturer  = strdup(p[2]);

			if (++i > games)
			{
				tsv_data = (TSV *)realloc(tsv_data, (i + 12) * sizeof(TSV));
				//games += 12;
			}
		}					
	}
    else
    {
		need_update = 1;
    }

	//winui_message_box_utf8(hMain, "mame32k list load complete", MAMEUINAME, MB_ICONINFORMATION | MB_OK);

	for (int i = 0; i< games ; i++)
	{
		int nIndex = -1;

		for (j = 0; tsv_data[j].gamename; j++)
		{
			if (!strcmp(  (char *)GetDriverGameName(i), tsv_data[j].gamename)  )
			{
				nIndex = j;
				break;
			}
		}
		if (nIndex == -1)
		{				
			tsv_index[i].gamename     = (char *)GetDriverGameName(i);
			tsv_index[i].description  = (char *)GetDriverGameTitle(i);
			tsv_index[i].manufacturer = (char *)GetDriverGameManufacturer(i);
			need_update = 1;
		}
		else
		{
			tsv_index[i].gamename	  = tsv_data[nIndex].gamename;
			tsv_index[i].description  = tsv_data[nIndex].description;
			tsv_index[i].manufacturer = tsv_data[nIndex].manufacturer;
		}			
	}

	//winui_message_box_utf8(hMain, "Game list load complete", MAMEUINAME, MB_ICONINFORMATION | MB_OK);

	
}

static void SaveAllGameListToFile()
{
	if (need_update)
	{
		char tsvname[_MAX_PATH];
		int  i;
		const char *format = "%s\t%s\t%s\n";

		TSV_GetPath(tsvname);
		
		FILE *f = fopen(tsvname, "w");

		if (f != NULL)
		{
			for (i = 0; tsv_index[i].gamename != NULL; i++)
			{
				fprintf(f, format,					
						tsv_index[i].gamename,
						tsv_index[i].description,
						tsv_index[i].manufacturer);
			}
			fclose(f);
		}

		free(tsv_index);
		for (i = 0; tsv_data[i].gamename; i++)
		{			
			free(tsv_data[i].gamename);
			free(tsv_data[i].description);
			free(tsv_data[i].manufacturer);
		}
		free(tsv_data);
	
	}
	
	//winui_message_box_utf8(hMain, "game list save complete", MAMEUINAME, MB_ICONINFORMATION | MB_OK);
}

int GetGameIndex(const char *name)
{
	int i;

	for (i = 0; tsv_index[i].gamename != NULL; i++)
		if (!strcmp(name, tsv_index[i].gamename)) 
			return i;

	return 0;
}

char *GetDescriptionByIndex(int nIndex, bool bUse)
{

	if (tsv_index && bUse)
	{
		return (tsv_index[nIndex].description);
	}
	else
	{
		return (char *)GetDriverGameTitle(nIndex);
	}
}

char *GetDescriptionByName(const char *name, bool bUse)
{
	return GetDescriptionByIndex(GetGameIndex(name), bUse);
}

char *GetGameNameByIndex(int nIndex, bool bUse)
{

	if (tsv_index && bUse)
	{
		return (tsv_index[nIndex].gamename);
	}
	else
	{
		return (char *)GetDriverGameName(nIndex);
	}

}

char *GetGameName(const char *name, bool bUse)
{
	return GetGameNameByIndex(GetGameIndex(name), bUse);
}

char *GetGameManufactureByIndex(int nIndex, bool bUse)
{

	if (tsv_index && bUse)
	{
		return (tsv_index[nIndex].manufacturer);
	}
	else
	{
		return (char *)GetDriverGameManufacturer(nIndex);
	}

}


//#endif

static HBITMAP CreateBitmapTransparent(HBITMAP hSource)
{
	BITMAP bm;

	HDC hSrc = CreateCompatibleDC(NULL);
	HDC hDst = CreateCompatibleDC(NULL);
	GetObject(hSource, sizeof(bm), &bm);
	SelectObject(hSrc, hSource);
	HBITMAP hNew = CreateBitmap(bm.bmWidth, bm.bmHeight, bm.bmPlanes, bm.bmBitsPixel, NULL);
	SelectObject(hDst, hNew);
	BitBlt(hDst, 0, 0, bm.bmWidth, bm.bmHeight, hSrc, 0, 0, SRCCOPY);
	COLORREF clrTP = RGB(239, 239, 239);
	COLORREF clrBK = GetSysColor(COLOR_MENU);

	for (int nRow = 0; nRow < bm.bmHeight; nRow++)
	{
		for (int nCol = 0; nCol < bm.bmWidth; nCol++)
		{
			if (GetPixel(hSrc, nCol, nRow) == clrTP)
				SetPixel(hDst, nCol, nRow, clrBK);
		}
	}

	DeleteDC(hDst);
	DeleteDC(hSrc);
	return hNew;
}

int GetNumGames(void)
{
	return game_count;
}

static int logmsg_init = 0;
void logmsg(const char*lpszFmt,...)
{
	CHAR szBuf[2048];
	va_list vargs;
	FILE *fp;

	va_start(vargs,lpszFmt);
	vsprintf(szBuf, lpszFmt, vargs);
	va_end(vargs);

	if( !logmsg_init ) {
		remove( "msglog.txt" );
		logmsg_init = 1;
	}

	fp = fopen( "msglog.txt", "at" );
	if( fp == NULL ) {
		fp = fopen( "msglog.txt", "wt" );
		if( fp == NULL ) return;
	}

	fprintf( fp, "%s", szBuf );

	fclose(fp);
}


#ifdef MAME_AVI

#include <math.h>

void AviDialogProcRefresh(HWND hDlg)
{
	// '프레임 압축'에 체크된 경우
	if (Button_GetCheck(GetDlgItem(hDlg, IDC_FRAME_CMP)) && AviStatus.depth == 16	)
	{
		Button_Enable(GetDlgItem(hDlg, IDC_FRAME_CMP_PRE), TRUE);
		Button_Enable(GetDlgItem(hDlg, IDC_FRAME_CMP_FEW), TRUE);
	}
	else
	{
		Button_Enable(GetDlgItem(hDlg, IDC_FRAME_CMP_PRE), FALSE);
		Button_Enable(GetDlgItem(hDlg, IDC_FRAME_CMP_FEW), FALSE);
	}

	// '16bit -> 24bit변환'에 체크된 경우
	if (Button_GetCheck(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO24)))
	{
		Button_Enable(GetDlgItem(hDlg, IDC_INTERLACE), TRUE);
	} else
	{
		Button_Enable(GetDlgItem(hDlg, IDC_INTERLACE), FALSE);
	}

	// '16bit->24bit변환' 과 '인터레이스드 방식'에 체크가 된 경우
	if (Button_GetCheck(GetDlgItem(hDlg, IDC_INTERLACE)) && Button_GetCheck(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO24))	)
	{	
		Button_Enable(GetDlgItem(hDlg, IDC_INTERLACE_ODD),	 TRUE);
		Button_Enable(GetDlgItem(hDlg, IDC_SMOOTH_RESIZE_X), TRUE);
		Button_Enable(GetDlgItem(hDlg, IDC_SMOOTH_RESIZE_Y), TRUE);

		Button_Enable(GetDlgItem(hDlg, IDC_AVISIZE_WIDTH),		TRUE);
		Button_Enable(GetDlgItem(hDlg, IDC_AVISIZE_HEIGHT),		TRUE);
		Button_Enable(GetDlgItem(hDlg, IDC_AVI_LEFT),			TRUE);
		Button_Enable(GetDlgItem(hDlg, IDC_AVI_WIDTH),			TRUE);
		Button_Enable(GetDlgItem(hDlg, IDC_AVI_TOP),			TRUE);
		Button_Enable(GetDlgItem(hDlg, IDC_AVI_HEIGHT),			TRUE);
	
		Button_Enable(GetDlgItem(hDlg, IDC_AVI_POS_CENTER),			TRUE);
		Static_SetText(GetDlgItem(hDlg, IDC_TEXT_FPS_DIV2),        TEXT("/2"));
		Static_SetText(GetDlgItem(hDlg, IDC_TEXT_AVI_TOP_MUL2),    TEXT("x2"));
		Static_SetText(GetDlgItem(hDlg, IDC_TEXT_AVI_HEIGHT_MUL2), TEXT("x2"));
	}
	else
	{
		Button_Enable(GetDlgItem(hDlg, IDC_INTERLACE_ODD), FALSE);
		Button_Enable(GetDlgItem(hDlg, IDC_SMOOTH_RESIZE_X), FALSE);
		Button_Enable(GetDlgItem(hDlg, IDC_SMOOTH_RESIZE_Y), FALSE);

		Button_Enable(GetDlgItem(hDlg, IDC_AVISIZE_WIDTH),		FALSE);
		Button_Enable(GetDlgItem(hDlg, IDC_AVISIZE_HEIGHT),		FALSE);
		Button_Enable(GetDlgItem(hDlg, IDC_AVI_LEFT),			FALSE);
		Button_Enable(GetDlgItem(hDlg, IDC_AVI_WIDTH),			FALSE);
		Button_Enable(GetDlgItem(hDlg, IDC_AVI_TOP),			FALSE);
		Button_Enable(GetDlgItem(hDlg, IDC_AVI_HEIGHT),			FALSE);

		Button_Enable(GetDlgItem(hDlg, IDC_AVI_POS_CENTER),			FALSE);
		Static_SetText(GetDlgItem(hDlg, IDC_TEXT_FPS_DIV2),        TEXT(""));
		Static_SetText(GetDlgItem(hDlg, IDC_TEXT_AVI_TOP_MUL2),    TEXT(""));
		Static_SetText(GetDlgItem(hDlg, IDC_TEXT_AVI_HEIGHT_MUL2), TEXT(""));
	}
}

INT_PTR CALLBACK AviDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{

    switch (Msg)
    {
    case WM_INITDIALOG:
		{// 다이얼로그 초기화
			WCHAR buf[64];
			HWND hCtrl = GetDlgItem(hDlg, IDC_AVIFRAMESKIP);
			int j;

			AviStatus = (*(GetAviStatus()));
			if (hCtrl)
			{
				int i;
				WCHAR fmt[64];
				const WCHAR *szFmtStr = TEXT("Skip %d of 12 frames");
				(void)ComboBox_AddString(hCtrl, TEXT("Draw every frame  @"));

				for (i = 1; i < 12; i++)
				{
					wsprintf( fmt, szFmtStr, i );
					if (i==6 || i==8 || i==9)
						wsprintf(buf, TEXT("%s @"), fmt);
					else
						wsprintf(buf, TEXT("%s"), fmt);

					(void)ComboBox_AddString(hCtrl, buf);
				}
				(void)ComboBox_SetCurSel(hCtrl, AviStatus.frame_skip);
			}

			hCtrl = GetDlgItem(hDlg, IDC_FPS);
			if (hCtrl)
			{
				static const WCHAR *avi_fps_values[]={
					TEXT("60"),
					TEXT("59.94"),
					TEXT("53.333333"),
					TEXT("48"),
					TEXT("40"),
					TEXT("30"),
					TEXT("29.97"),
					TEXT("24"),
					TEXT("20"),
					TEXT("15"),
					TEXT("12"),
					TEXT("10")
				};
				swprintf(buf, TEXT("%5.6f"), AviStatus.fps);
				(void)ComboBox_AddString(hCtrl, buf);

				for (j = 0; j < 12; j++)
					(void)ComboBox_AddString(hCtrl, avi_fps_values[j]);
				(void)ComboBox_SetCurSel(hCtrl, 0);

				(void)ComboBox_LimitText(hCtrl, 10);
				Edit_SetText(hCtrl, buf);

			}

			hCtrl = GetDlgItem(hDlg, IDC_AVISIZE_WIDTH);
			if (hCtrl)
			{
				static const WCHAR *avi_width_values[]={
					TEXT("720"),
					TEXT("640"),
					TEXT("512"),
					TEXT("480"),
					TEXT("384"),
					TEXT("352"),
					TEXT("320"),
					TEXT("304"),
					TEXT("256"),
					TEXT("240")
				};
				wsprintf(buf, TEXT("%u"), AviStatus.avi_width);
				(void)ComboBox_AddString(hCtrl, buf);
				for (j = 0; j < 10; j++)
					(void)ComboBox_AddString(hCtrl, avi_width_values[j]);
				(void)ComboBox_SetCurSel(hCtrl, 0);

				(void)ComboBox_LimitText(hCtrl, 5);
				wsprintf(buf, TEXT("%u"), AviStatus.avi_width);
				Edit_SetText(hCtrl, buf);
			}

			hCtrl = GetDlgItem(hDlg, IDC_AVISIZE_HEIGHT);
			if (hCtrl)
			{
				static const WCHAR *avi_height_values[]={
					TEXT("480"),
					TEXT("384"),
					TEXT("320"),
					TEXT("240"),
					TEXT("232"),
					TEXT("224")
				};
				wsprintf(buf, TEXT("%u"), AviStatus.avi_height);
				(void)ComboBox_AddString(hCtrl, buf);
				for (j = 0; j < 6; j++)
					(void)ComboBox_AddString(hCtrl, avi_height_values[j]);
				(void)ComboBox_SetCurSel(hCtrl, 0);

				(void)ComboBox_LimitText(hCtrl, 5);
				wsprintf(buf, TEXT("%u"), AviStatus.avi_height);
				Edit_SetText(hCtrl, buf);
			}

			wsprintf(buf, TEXT("%lu"), AviStatus.avi_rect.m_Left);
			Edit_SetText(GetDlgItem(hDlg, IDC_AVI_LEFT),   buf);
			wsprintf(buf, TEXT("%lu"), AviStatus.avi_rect.m_Top);
			Edit_SetText(GetDlgItem(hDlg, IDC_AVI_TOP),    buf);
			wsprintf(buf, TEXT("%lu"), AviStatus.avi_rect.m_Width);
			Edit_SetText(GetDlgItem(hDlg, IDC_AVI_WIDTH),  buf);
			wsprintf(buf, TEXT("%lu"), AviStatus.avi_rect.m_Height);
			Edit_SetText(GetDlgItem(hDlg, IDC_AVI_HEIGHT), buf);
			
			hCtrl = GetDlgItem(hDlg, IDC_AVI_FILESIZE);
			if (hCtrl)
			{
				static const WCHAR *avi_filesize_values[]={
					TEXT("2000"),
					TEXT("1500"),
					TEXT("1000"),
					TEXT("500"),
					TEXT("100")
				};
				wsprintf(buf, TEXT("%u"), AviStatus.avi_filesize);
				(void)ComboBox_AddString(hCtrl, buf);
				for (j = 0; j < 5; j++)
					(void)ComboBox_AddString(hCtrl, avi_filesize_values[j]);
				(void)ComboBox_SetCurSel(hCtrl, 0);

				(void)ComboBox_LimitText(hCtrl, 7);
				wsprintf(buf, TEXT("%u"), AviStatus.avi_filesize);
				Edit_SetText(hCtrl, buf);
			}
			hCtrl = GetDlgItem(hDlg, IDC_AVI_FILESIZE_CHECK_FRAME);
			if (hCtrl)
			{
				static const WCHAR *avi_filesize_ck_values[]={
					TEXT("60"),
					TEXT("30"),
					TEXT("20"),
					TEXT("15"),
					TEXT("12"),
					TEXT("10"),
					TEXT("6"),
					TEXT("5"),
					TEXT("4"),
					TEXT("3"),
					TEXT("2"),
					TEXT("1")
				};
				for (j = 0; j < 12; j++)
					(void)ComboBox_AddString(hCtrl, avi_filesize_ck_values[j]);
				(void)ComboBox_SetCurSel(hCtrl, 0);

				(void)ComboBox_LimitText(hCtrl, 11);
				wsprintf(buf, TEXT("%u"), AviStatus.avi_filesizecheck_frame);
				Edit_SetText(hCtrl, buf);
			}

			hCtrl = GetDlgItem(hDlg, IDC_AUDIO_RECORD_TYPE);
			if (hCtrl)
			{
				(void)ComboBox_AddString(hCtrl, TEXT("Do not record sound"));
				(void)ComboBox_AddString(hCtrl, TEXT("Record as WAV file"));
				(void)ComboBox_AddString(hCtrl, TEXT("Record to AVI"));

				(void)ComboBox_SetCurSel(hCtrl, AviStatus.avi_audio_record_type);
			}

			Edit_LimitText(	GetDlgItem(hDlg, IDC_HOUR),   3);
			Edit_LimitText(	GetDlgItem(hDlg, IDC_MINUTE), 3);
			Edit_LimitText(	GetDlgItem(hDlg, IDC_SECOND), 3);

			{
				wsprintf(buf, TEXT("%d x %d x %d bit"), AviStatus.width, AviStatus.height, AviStatus.depth);
				Static_SetText(GetDlgItem(hDlg, IDC_BITMAP_SIZE),       buf);

				if (AviStatus.audio_type == 0)
				{

					Static_SetText(GetDlgItem(hDlg, IDC_AUDIO_SRC_FORMAT),TEXT("No sound"));

					Static_SetText(GetDlgItem(hDlg, IDC_AUDIO_DEST_FORMAT),TEXT(""));

					Button_Enable(GetDlgItem(hDlg, IDC_AUDIO_RECORD_TYPE), FALSE);

				}else
				{
                    const WCHAR *lpszStereo = TEXT("Stereo");
					const WCHAR *lpszMono   = TEXT("Mono");				
					wsprintf(buf, TEXT("%uHz %ubit %s"), AviStatus.audio_samples_per_sec, AviStatus.audio_bitrate, (AviStatus.audio_channel == 2) ? lpszStereo:lpszMono);
					Static_SetText(GetDlgItem(hDlg, IDC_AUDIO_SRC_FORMAT),        buf);
					wsprintf(buf, TEXT("%uHz %ubit %s"), AviStatus.avi_audio_samples_per_sec, AviStatus.avi_audio_bitrate, (AviStatus.avi_audio_channel == 2) ? lpszStereo:lpszMono);
					Static_SetText(GetDlgItem(hDlg, IDC_AUDIO_DEST_FORMAT),       buf);
					Button_Enable(GetDlgItem(hDlg, IDC_AUDIO_RECORD_TYPE), TRUE);
				}


				wsprintf(buf, TEXT("%d"), AviStatus.hour);
				Edit_SetText(	GetDlgItem(hDlg, IDC_HOUR), buf);
				wsprintf(buf, TEXT("%d"), AviStatus.minute);
				Edit_SetText(	GetDlgItem(hDlg, IDC_MINUTE), buf);
				wsprintf(buf, TEXT("%d"), AviStatus.second);
				Edit_SetText(	GetDlgItem(hDlg, IDC_SECOND), buf);
			}
			

			Button_SetCheck(GetDlgItem(hDlg, IDC_FRAME_CMP),      AviStatus.frame_cmp);
			Button_SetCheck(GetDlgItem(hDlg, IDC_FRAME_CMP_PRE),  AviStatus.frame_cmp_pre15);
			Button_SetCheck(GetDlgItem(hDlg, IDC_FRAME_CMP_FEW),  AviStatus.frame_cmp_few);
			Button_SetCheck(GetDlgItem(hDlg, IDC_WAVE_RECORD),    AviStatus.wave_record);
			Button_SetCheck(GetDlgItem(hDlg, IDC_INTERLACE),      AviStatus.interlace);
			Button_SetCheck(GetDlgItem(hDlg, IDC_INTERLACE_ODD),	AviStatus.interlace_odd_number_field);

			Button_SetCheck(GetDlgItem(hDlg, IDC_CHECK_FORCEFLIPY),     AviStatus.force_flip_y);

			Button_SetCheck(GetDlgItem(hDlg, IDC_AVI_SAVEFILE_PAUSE),	AviStatus.avi_savefile_pause);
			Button_SetCheck(GetDlgItem(hDlg, IDC_SMOOTH_RESIZE_X),		AviStatus.avi_smooth_resize_x);
			Button_SetCheck(GetDlgItem(hDlg, IDC_SMOOTH_RESIZE_Y),		AviStatus.avi_smooth_resize_y);
			
			Button_SetCheck(GetDlgItem(hDlg, IDC_AUDIO_16BIT),    (AviStatus.avi_audio_bitrate>8) ? TRUE:FALSE);
			Button_SetCheck(GetDlgItem(hDlg, IDC_AUDIO_STEREO),   (AviStatus.avi_audio_channel==2) ? TRUE:FALSE);

	
			if (AviStatus.depth == 16)
			{
				if (AviStatus.avi_depth == 8)	Button_SetCheck(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO8),TRUE);
				else							Button_SetCheck(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO8),FALSE);
				if (AviStatus.avi_depth == 24)	Button_SetCheck(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO24),TRUE);
				else							Button_SetCheck(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO24),FALSE);
			
				Button_Enable(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO8), TRUE);
				Button_Enable(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO24), TRUE);

				Button_Enable(GetDlgItem(hDlg, IDC_INTERLACE), TRUE);
				Button_Enable(GetDlgItem(hDlg, IDC_SET_TV_DISPLAY_SETTING), TRUE);
				

			} else
			{
				Button_SetCheck(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO8),FALSE);
				Button_SetCheck(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO24),FALSE);
				Button_Enable(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO8), FALSE);
				Button_Enable(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO24), FALSE);
				Button_Enable(GetDlgItem(hDlg, IDC_INTERLACE), FALSE);
				Button_Enable(GetDlgItem(hDlg, IDC_SET_TV_DISPLAY_SETTING), FALSE);
			}
			

			if (AviStatus.depth == 16)
			{
				Button_Enable(GetDlgItem(hDlg, IDC_FRAME_CMP), TRUE);
				Button_Enable(GetDlgItem(hDlg, IDC_FRAME_CMP_PRE), TRUE);
				Button_Enable(GetDlgItem(hDlg, IDC_FRAME_CMP_FEW), TRUE);
			}else 
			{
				Button_Enable(GetDlgItem(hDlg, IDC_FRAME_CMP), FALSE);
				Button_Enable(GetDlgItem(hDlg, IDC_FRAME_CMP_PRE), FALSE);
				Button_Enable(GetDlgItem(hDlg, IDC_FRAME_CMP_FEW), FALSE);
			}

			Edit_LimitText(GetDlgItem(hDlg, IDC_LEFT), 4);
			Edit_SetText(GetDlgItem(hDlg, IDC_LEFT),	TEXT("0"));
			SendDlgItemMessage(hDlg, IDC_LEFT_SPIN, UDM_SETRANGE, 0,
										(LPARAM)MAKELONG(AviStatus.width-1, 0));
		    SendDlgItemMessage(hDlg, IDC_LEFT_SPIN, UDM_SETPOS, 0,
										(LPARAM)MAKELONG(AviStatus.rect.m_Left, 0));

			Edit_LimitText(GetDlgItem(hDlg, IDC_TOP), 4);
			Edit_SetText(GetDlgItem(hDlg, IDC_TOP),	TEXT("0"));
			SendDlgItemMessage(hDlg, IDC_TOP_SPIN, UDM_SETRANGE, 0,
										(LPARAM)MAKELONG(AviStatus.height-1, 0));
		    SendDlgItemMessage(hDlg, IDC_TOP_SPIN, UDM_SETPOS, 0,
										(LPARAM)MAKELONG(AviStatus.rect.m_Top, 0));

			Edit_LimitText(GetDlgItem(hDlg, IDC_WIDTH), 4);
			SendDlgItemMessage(hDlg, IDC_WIDTH_SPIN, UDM_SETRANGE, 0,
										(LPARAM)MAKELONG(AviStatus.width, 1));
		    SendDlgItemMessage(hDlg, IDC_WIDTH_SPIN, UDM_SETPOS, 0,
										(LPARAM)MAKELONG(AviStatus.rect.m_Width, 0));

			Edit_LimitText(GetDlgItem(hDlg, IDC_HEIGHT), 4);
			SendDlgItemMessage(hDlg, IDC_HEIGHT_SPIN, UDM_SETRANGE, 0,
										(LPARAM)MAKELONG(AviStatus.height, 1));
		    SendDlgItemMessage(hDlg, IDC_HEIGHT_SPIN, UDM_SETPOS, 0,
										(LPARAM)MAKELONG(AviStatus.rect.m_Height, 0));


		}

		AviDialogProcRefresh(hDlg);
        return TRUE;

    case WM_HELP:
        break;

    case WM_CONTEXTMENU:
        break;

    case WM_COMMAND :
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
		case IDC_FRAME_CMP:
			AviDialogProcRefresh(hDlg);
			break;
		case IDC_FRAME_CMP_PRE:
			if (Button_GetCheck(GetDlgItem(hDlg, IDC_FRAME_CMP_PRE)))
				Button_SetCheck(GetDlgItem(hDlg, IDC_FRAME_CMP_FEW),      FALSE);
			break;
		case IDC_FRAME_CMP_FEW:
			if (Button_GetCheck(GetDlgItem(hDlg, IDC_FRAME_CMP_FEW)))
				Button_SetCheck(GetDlgItem(hDlg, IDC_FRAME_CMP_PRE),      FALSE);
			break;
		case IDC_INTERLACE:
			if (Button_GetCheck(GetDlgItem(hDlg, IDC_INTERLACE)) == TRUE)
			{	
				char buf[32];
				sprintf(buf, "%lu", AviStatus.rect.m_Height*2);
				Edit_SetText(GetDlgItem(hDlg, IDC_AVISIZE_HEIGHT),        (LPTSTR)buf);
			}
			else
			{
				char buf[32];
				sprintf(buf, "%lu", AviStatus.rect.m_Height);
				Edit_SetText(GetDlgItem(hDlg, IDC_AVISIZE_HEIGHT),        (LPTSTR)buf);
			}
			AviDialogProcRefresh(hDlg);
			break;
		case IDC_COLOR_CNV_16TO8:
			if (Button_GetCheck(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO8)))
				Button_SetCheck(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO24),      FALSE);
			AviDialogProcRefresh(hDlg);
			break;
		case IDC_COLOR_CNV_16TO24:
			if (Button_GetCheck(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO24)))
			{
				Button_SetCheck(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO8),      FALSE);
			}
			AviDialogProcRefresh(hDlg);
			break;
		case IDC_SET_TV_DISPLAY_SETTING:
			{
				char buf[100];
				unsigned int x,y,width,height;
				unsigned int width_src,height_src;

				Button_SetCheck(GetDlgItem(hDlg, IDC_INTERLACE),			TRUE);
				Button_SetCheck(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO24),		TRUE);
				Button_SetCheck(GetDlgItem(hDlg, IDC_FRAME_CMP),			FALSE);
				Button_SetCheck(GetDlgItem(hDlg, IDC_SMOOTH_RESIZE_Y),		TRUE);

				Edit_SetText(GetDlgItem(hDlg, IDC_FPS),					TEXT("59.94"));
				Edit_SetText(GetDlgItem(hDlg, IDC_AVISIZE_WIDTH),		TEXT("720"));
				Edit_SetText(GetDlgItem(hDlg, IDC_AVISIZE_HEIGHT),		TEXT("480"));


				Edit_GetText(GetDlgItem(hDlg, IDC_WIDTH), (LPTSTR)buf, 100);
				sscanf(buf,"%u", &width);	if (width == 0)		width = AviStatus.rect.m_Width;
				Edit_GetText(GetDlgItem(hDlg, IDC_HEIGHT), (LPTSTR)buf, 100);
				sscanf(buf,"%u", &height);	if (height == 0)	height = AviStatus.rect.m_Height;

				width_src = width; 
				height_src = height;

				width *= 2;
				if (height > 240) {height = 240; width = 480*0.75;}
				y = (240 - height)/2;
				x = (720 - width)/2;
				if (width > 670) {width = 670; x = 22;}

				sprintf(buf, "%u", x);
				Edit_SetText(GetDlgItem(hDlg, IDC_AVI_LEFT),        (LPTSTR)buf);
				sprintf(buf, "%u", y);
				Edit_SetText(GetDlgItem(hDlg, IDC_AVI_TOP),        (LPTSTR)buf);
				sprintf(buf, "%u", width);
				Edit_SetText(GetDlgItem(hDlg, IDC_AVI_WIDTH),        (LPTSTR)buf);	
				sprintf(buf, "%u", height);
				Edit_SetText(GetDlgItem(hDlg, IDC_AVI_HEIGHT),        (LPTSTR)buf);
				
				if (width % width_src)		AviStatus.avi_smooth_resize_x = TRUE;
				else						AviStatus.avi_smooth_resize_x = FALSE;
				if (height % height_src)	AviStatus.avi_smooth_resize_y = TRUE;
				else						AviStatus.avi_smooth_resize_y = FALSE;

				Button_SetCheck(GetDlgItem(hDlg, IDC_SMOOTH_RESIZE_X),		AviStatus.avi_smooth_resize_x);
				Button_SetCheck(GetDlgItem(hDlg, IDC_SMOOTH_RESIZE_Y),		AviStatus.avi_smooth_resize_y);
				
				(void)ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_AVIFRAMESKIP), 0);

			}
			AviDialogProcRefresh(hDlg);
			break;
		case IDC_AVI_POS_CENTER:
			{
				char buf[100];
				int x,y,width,height;
				Edit_GetText(GetDlgItem(hDlg, IDC_AVI_WIDTH), (LPTSTR)buf, 100);
				sscanf(buf,"%u", &width);
				Edit_GetText(GetDlgItem(hDlg, IDC_AVI_HEIGHT), (LPTSTR)buf, 100);
				sscanf(buf,"%u", &height);

				y = (240 - height)/2;
				x = (720 - width)/2;

				if (x<0) x=0;
				if (y<0) y=0;

				sprintf(buf, "%u", x);
				Edit_SetText(GetDlgItem(hDlg, IDC_AVI_LEFT),(LPTSTR)buf);
				sprintf(buf, "%u", y);
				Edit_SetText(GetDlgItem(hDlg, IDC_AVI_TOP),(LPTSTR)buf);
			}
			break;
			
        case IDOK :
			{
				char buf[100];
				char buf2[256];

				Edit_GetText(GetDlgItem(hDlg, IDC_FPS), (LPTSTR)buf2, 100);
				strcpy(buf, buf2);

				{
					int i,j;
					int di,df;
					int di2,df2,j2;

					AviStatus.fps = 0;
					j=-1;
					for(i=0; i<100; i++)
					{
						if (buf[i] == 0 ) break;
						if (buf[i] == '.')
						{
							buf[i] = 0;
							j=i+1;
							break;
						}
					}
					sscanf(buf,"%d", &di);
					df = 0;
					if (j!=-1) sscanf(&buf[j],"%d", &df);
					j=strlen(&buf[j]);
					AviStatus.fps = (double)df / pow(10,j);
					AviStatus.fps += (double)di;

									
					sprintf(buf, "%5.6f", AviStatus.def_fps);
					j2=-1;
					for(i=0; i<100; i++)
					{
						if (buf[i] == 0 ) break;
						if (buf[i] == '.')
						{
							buf[i] = 0;
							j2=i+1;
							break;
						}
					}
					sscanf(buf,"%d", &di2);
					df2 = 0;
					if (j2!=-1) sscanf(&buf[j2],"%d", &df2);
					j2=strlen(&buf[j2]);
					if ( AviStatus.fps == (double)df2 / pow(10,j2) + (double)di2 ) AviStatus.fps = AviStatus.def_fps;

				}

				if (AviStatus.fps <= 0) AviStatus.fps = AviStatus.def_fps;

				AviStatus.frame_skip = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_AVIFRAMESKIP));

				AviStatus.frame_cmp       = Button_GetCheck(GetDlgItem(hDlg, IDC_FRAME_CMP));
				AviStatus.frame_cmp_pre15 = Button_GetCheck(GetDlgItem(hDlg, IDC_FRAME_CMP_PRE));
				AviStatus.frame_cmp_few   = Button_GetCheck(GetDlgItem(hDlg, IDC_FRAME_CMP_FEW));
				AviStatus.wave_record     = Button_GetCheck(GetDlgItem(hDlg, IDC_WAVE_RECORD));
				AviStatus.interlace       = Button_GetCheck(GetDlgItem(hDlg, IDC_INTERLACE));
				AviStatus.interlace_odd_number_field = Button_GetCheck(GetDlgItem(hDlg, IDC_INTERLACE_ODD));

				AviStatus.force_flip_y    = Button_GetCheck(GetDlgItem(hDlg, IDC_CHECK_FORCEFLIPY));
				
				AviStatus.avi_savefile_pause  = Button_GetCheck(GetDlgItem(hDlg, IDC_AVI_SAVEFILE_PAUSE));
				AviStatus.avi_smooth_resize_x = Button_GetCheck(GetDlgItem(hDlg, IDC_SMOOTH_RESIZE_X));
				AviStatus.avi_smooth_resize_y = Button_GetCheck(GetDlgItem(hDlg, IDC_SMOOTH_RESIZE_Y));

				AviStatus.avi_audio_record_type = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_AUDIO_RECORD_TYPE));
				AviStatus.avi_audio_bitrate = (Button_GetCheck(GetDlgItem(hDlg, IDC_AUDIO_16BIT))==TRUE) ? 16:8;
				AviStatus.avi_audio_channel = (Button_GetCheck(GetDlgItem(hDlg, IDC_AUDIO_STEREO))==TRUE) ? 2:1;

				AviStatus.bmp_16to8_cnv = Button_GetCheck(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO8));
				AviStatus.avi_depth = AviStatus.depth;
				if (AviStatus.depth == 16)
				{
					if (Button_GetCheck(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO8)) == TRUE) AviStatus.avi_depth = 8;
					if (Button_GetCheck(GetDlgItem(hDlg, IDC_COLOR_CNV_16TO24)) == TRUE) AviStatus.avi_depth = 24;
				}


				Edit_GetText(GetDlgItem(hDlg, IDC_AVI_FILESIZE), (LPTSTR)buf, 100);
				if (buf[0] != 0)	sscanf(buf,"%d", &AviStatus.avi_filesize);
				Edit_GetText(GetDlgItem(hDlg, IDC_AVI_FILESIZE_CHECK_FRAME), (LPTSTR)buf, 100);
				if (buf[0] != 0)	sscanf(buf,"%d", &AviStatus.avi_filesizecheck_frame);
				Edit_GetText(GetDlgItem(hDlg, IDC_AVISIZE_WIDTH), (LPTSTR)buf, 100);
				if (buf[0] != 0)	sscanf(buf,"%d", &AviStatus.avi_width);
				Edit_GetText(GetDlgItem(hDlg, IDC_AVISIZE_HEIGHT), (LPTSTR)buf, 100);
				if (buf[0] != 0)	sscanf(buf,"%d", &AviStatus.avi_height);

				Edit_GetText(GetDlgItem(hDlg, IDC_AVI_LEFT), (LPTSTR)buf, 100);
				if (buf[0] != 0)	sscanf(buf,"%lu", &AviStatus.avi_rect.m_Left);
				Edit_GetText(GetDlgItem(hDlg, IDC_AVI_TOP), (LPTSTR)buf, 100);
				if (buf[0] != 0)	sscanf(buf,"%lu", &AviStatus.avi_rect.m_Top);
				Edit_GetText(GetDlgItem(hDlg, IDC_AVI_WIDTH), (LPTSTR)buf, 100);
				if (buf[0] != 0)	sscanf(buf,"%lu", &AviStatus.avi_rect.m_Width);
				Edit_GetText(GetDlgItem(hDlg, IDC_AVI_HEIGHT), (LPTSTR)buf, 100);
				if (buf[0] != 0)	sscanf(buf,"%lu", &AviStatus.avi_rect.m_Height);

				Edit_GetText(GetDlgItem(hDlg, IDC_LEFT), (LPTSTR)buf, 100);
				if (buf[0] != 0)	sscanf(buf,"%lu", &AviStatus.rect.m_Left);
				Edit_GetText(GetDlgItem(hDlg, IDC_TOP), (LPTSTR)buf, 100);
				if (buf[0] != 0)	sscanf(buf,"%lu", &AviStatus.rect.m_Top);
				Edit_GetText(GetDlgItem(hDlg, IDC_WIDTH), (LPTSTR)buf, 100);
				if (buf[0] != 0)	sscanf(buf,"%lu", &AviStatus.rect.m_Width);
				Edit_GetText(GetDlgItem(hDlg, IDC_HEIGHT), (LPTSTR)buf, 100);
				if (buf[0] != 0)	sscanf(buf,"%lu", &AviStatus.rect.m_Height);
				
				Edit_GetText(GetDlgItem(hDlg, IDC_HOUR), (LPTSTR)buf, 100);
				if (buf[0] != 0)	sscanf(buf,"%d", &AviStatus.hour);
				Edit_GetText(GetDlgItem(hDlg, IDC_MINUTE), (LPTSTR)buf, 100);
				if (buf[0] != 0)	sscanf(buf,"%d", &AviStatus.minute);
				Edit_GetText(GetDlgItem(hDlg, IDC_SECOND), (LPTSTR)buf, 100);
				if (buf[0] != 0)	sscanf(buf,"%d", &AviStatus.second);

				if (AviStatus.rect.m_Width	> AviStatus.width)	AviStatus.rect.m_Width	= AviStatus.width;
				if (AviStatus.rect.m_Height	> AviStatus.height)	AviStatus.rect.m_Height	= AviStatus.height;
				if (AviStatus.rect.m_Left+AviStatus.rect.m_Width	> AviStatus.width)	AviStatus.rect.m_Left	= AviStatus.width - AviStatus.rect.m_Width;
				if (AviStatus.rect.m_Top+AviStatus.rect.m_Height	> AviStatus.height) AviStatus.rect.m_Top	= AviStatus.height - AviStatus.rect.m_Height;

				SetAviStatus(&AviStatus);
			}
            /* Fall through */

			EndDialog(hDlg, 1);
			return TRUE;

        case IDCANCEL :
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }
    return 0;
}


static void set_mame_mixer_wfm(int drvindex, core_options &opt)
{
	extern int mame_mixer_wave_cnvnmb;
	extern struct WAV_WAVEFORMAT		mame_mixer_dstwfm, mame_mixer_srcwfm;
	extern struct WAV_SAMPLES_RESIZE	mame_mixer_wsr;	

	mame_mixer_srcwfm.samplespersec = opt.int_value(OPTION_SAMPLERATE);//options_get_int(o, OPTION_SAMPLERATE);
	mame_mixer_srcwfm.channel = 2;		// changed from 0.93 (Aaron's big sound system change) - DarkCoder
	mame_mixer_srcwfm.bitrate = 16;

	mame_mixer_dstwfm = mame_mixer_srcwfm;

	mame_mixer_wave_cnvnmb = wav_convert_select(&mame_mixer_dstwfm, &mame_mixer_srcwfm, 
												&mame_mixer_wsr, NULL ); //&mame_mixer_wsre );
}

static void SetupAviStatus(int nGame)
{
	extern int neogeo_check_lower_resolution( const char *name );
	struct MAME_AVI_STATUS *OldAviStatus;	//kt
	machine_config config(driver_list::driver(nGame), MameUIGlobal());

	//machine_config config(*drivers[nGame]);
	//const device_config *screen;
	//const screen_device_config *scrconfig;
	
	const screen_device *screen = screen_device_iterator(config.root_device()).first();

	//core_options *o = load_options(OPTIONS_GAME, nGame);
	windows_options opt;

	LoadOptions(opt, OPTIONS_GAME, nGame);

	//screen = config.first_screen();
	//scrconfig = downcast<const screen_device_config *>(config.first_screen());

	AviStatus.source_file = (char*)GetDriverFileName(nGame);//(char*)drivers[nGame]->source_file;
	AviStatus.index = nGame + 1;

	AviStatus.def_fps = ATTOSECONDS_TO_HZ(screen->refresh_attoseconds()); // fps
	AviStatus.fps     = AviStatus.def_fps;
	AviStatus.depth   = 16; //playing_game_options.depth;	// (auto/16bit/32bit)
	AviStatus.flags   = DriverIsBios(nGame);//drivers[nGame]->flags;
	AviStatus.orientation = AviStatus.flags;// & ORIENTATION_MASK;

	if (opt.int_value(OPTION_ROR))
	{
		if ((AviStatus.orientation & ROT180) == ORIENTATION_FLIP_X ||(AviStatus.orientation & ROT180) == ORIENTATION_FLIP_Y) 
		{
			AviStatus.orientation ^= ROT180;
		}
		AviStatus.orientation ^= ROT90;
	}
	else if(opt.int_value(OPTION_ROL))
	{
		if ((AviStatus.orientation & ROT180) == ORIENTATION_FLIP_X ||(AviStatus.orientation & ROT180) == ORIENTATION_FLIP_Y) 
		{
			AviStatus.orientation ^= ROT180;
		}
		AviStatus.orientation ^= ROT270;
	}

	if (opt.int_value(OPTION_FLIPX))
		AviStatus.orientation ^= ORIENTATION_FLIP_X;
	if (opt.int_value(OPTION_FLIPY))
		AviStatus.orientation ^= ORIENTATION_FLIP_Y;

	AviStatus.frame_skip		= 0;
	AviStatus.frame_cmp			= TRUE;
	AviStatus.frame_cmp_pre15	= FALSE;
	AviStatus.frame_cmp_few		= FALSE;
	AviStatus.wave_record		= FALSE;
	AviStatus.bmp_16to8_cnv		= FALSE;

	AviStatus.force_flip_y		= FALSE;

	AviStatus.avi_filesize				= 1800;
	AviStatus.avi_filesizecheck_frame	= 10;
	AviStatus.avi_savefile_pause		= FALSE;


	AviStatus.interlace	= FALSE;
	AviStatus.interlace_odd_number_field = FALSE;

	{
		extern struct WAV_WAVEFORMAT mame_mixer_dstwfm, mame_mixer_srcwfm;
		set_mame_mixer_wfm(nGame, opt);

		AviStatus.audio_channel         = mame_mixer_srcwfm.channel;
		AviStatus.audio_samples_per_sec	= mame_mixer_srcwfm.samplespersec;
		AviStatus.audio_bitrate			= mame_mixer_srcwfm.bitrate;
		
		AviStatus.avi_audio_channel			= mame_mixer_dstwfm.channel;
		AviStatus.avi_audio_samples_per_sec = mame_mixer_dstwfm.samplespersec;
		AviStatus.avi_audio_bitrate			= mame_mixer_dstwfm.bitrate;

		AviStatus.audio_type			= opt.int_value(OPTION_AVI_AUDIO_TYPE);
		AviStatus.avi_audio_record_type	= (AviStatus.audio_type!=0) ? 2:0;
	}

	AviStatus.hour   = 0;
	AviStatus.minute = 0;
	AviStatus.second = 0;

	if (AviStatus.orientation & ORIENTATION_SWAP_XY)
	{
		AviStatus.width  = screen->height();
		AviStatus.height = screen->width();
		AviStatus.rect.m_Left   = screen->visible_area().min_y;
		AviStatus.rect.m_Top    = screen->visible_area().min_x;
		AviStatus.rect.m_Width  = screen->visible_area().max_y - screen->visible_area().min_y + 1;
		AviStatus.rect.m_Height = screen->visible_area().max_x - screen->visible_area().min_x + 1;
	}
	else
	{
		AviStatus.width  = screen->width();
		AviStatus.height = screen->height();
		AviStatus.rect.m_Left   = screen->visible_area().min_x;
		AviStatus.rect.m_Top    = screen->visible_area().min_y;
		AviStatus.rect.m_Width  = screen->visible_area().max_x - screen->visible_area().min_x + 1;
		AviStatus.rect.m_Height = screen->visible_area().max_y - screen->visible_area().min_y + 1;
	}
	
#if 0
	//neogeo
	if (!strcmp(drivers[nGame]->source_file+17, "neogeo.c") && neogeo_check_lower_resolution(drivers[nGame]->name))
	{
		AviStatus.rect.m_Left   = 1*8;
		AviStatus.rect.m_Top    = scrconfig->defstate.visarea.min_y;
		AviStatus.rect.m_Width  = 39*8-1 - 1*8 + 1;
		AviStatus.rect.m_Height = scrconfig->defstate.visarea.max_y - scrconfig->defstate.visarea.min_y + 1;
	}
#endif

	AviStatus.avi_width			= AviStatus.rect.m_Width;
	AviStatus.avi_height		= AviStatus.rect.m_Height;
	AviStatus.avi_depth			= 16;
	AviStatus.avi_rect.m_Left	= 0;
	AviStatus.avi_rect.m_Top	= 0;
	AviStatus.avi_rect.m_Width	= AviStatus.rect.m_Width;
	AviStatus.avi_rect.m_Height	= AviStatus.rect.m_Height;
	AviStatus.avi_smooth_resize_x	= FALSE;
	AviStatus.avi_smooth_resize_y	= FALSE;

	if (AviStatus.avi_rect.m_Width < AviStatus.rect.m_Width ||
		(int)((double)(AviStatus.avi_rect.m_Width<<16) / (double)AviStatus.rect.m_Width) & 0xffff)	
		AviStatus.avi_smooth_resize_x	= TRUE;

	if (AviStatus.avi_rect.m_Height < AviStatus.rect.m_Height ||
		(int)((double)(AviStatus.avi_rect.m_Height<<16) / (double)AviStatus.rect.m_Height) & 0xffff)
		AviStatus.avi_smooth_resize_y	= TRUE;		


	OldAviStatus = GetAviStatus();

	if (OldAviStatus->source_file)
	if (!strcmp(AviStatus.source_file, OldAviStatus->source_file))
	{
		if (AviStatus.def_fps == OldAviStatus->def_fps)
			AviStatus.fps = OldAviStatus->fps;

		AviStatus.frame_skip = OldAviStatus->frame_skip;
		AviStatus.frame_cmp = OldAviStatus->frame_cmp;
		AviStatus.frame_cmp_pre15 = OldAviStatus->frame_cmp_pre15;
		AviStatus.frame_cmp_few = OldAviStatus->frame_cmp_few;
		AviStatus.wave_record = OldAviStatus->wave_record;
		AviStatus.bmp_16to8_cnv = OldAviStatus->bmp_16to8_cnv;

		AviStatus.avi_depth = OldAviStatus->avi_depth;
		AviStatus.interlace = OldAviStatus->interlace;
		AviStatus.interlace_odd_number_field = OldAviStatus->interlace_odd_number_field;
		AviStatus.avi_filesize = OldAviStatus->avi_filesize;
		AviStatus.avi_filesizecheck_frame = OldAviStatus->avi_filesizecheck_frame;
		AviStatus.avi_savefile_pause = OldAviStatus->avi_savefile_pause;


		if (AviStatus.audio_type == OldAviStatus->audio_type)
			AviStatus.avi_audio_record_type	= OldAviStatus->avi_audio_record_type;
		
		AviStatus.hour = OldAviStatus->hour;
		AviStatus.minute = OldAviStatus->minute;
		AviStatus.second = OldAviStatus->second;

		if ((AviStatus.flags & ORIENTATION_SWAP_XY) == (OldAviStatus->flags & ORIENTATION_SWAP_XY) &&
				AviStatus.orientation == OldAviStatus->orientation)
		{
			AviStatus.rect = OldAviStatus->rect;
			AviStatus.avi_width = OldAviStatus->avi_width;
			AviStatus.avi_height = OldAviStatus->avi_height;
			AviStatus.avi_rect = OldAviStatus->avi_rect;
			AviStatus.avi_smooth_resize_x = OldAviStatus->avi_smooth_resize_x;
			AviStatus.avi_smooth_resize_y = OldAviStatus->avi_smooth_resize_y;
		}
	}

	SetAviStatus(&AviStatus);
	//options_free(o);
}

void get_autofilename(int nGame, char *avidir, WCHAR *avifilename, WCHAR *ext)
{
	WCHAR sztmpfile[MAX_PATH];

	wsprintf( sztmpfile, TEXT("%s\\%s.%s"), avidir, GetDriverGameName(nGame), ext );
	if( _waccess(sztmpfile, 0) != -1 ) {
		do
		{
			wsprintf(sztmpfile, TEXT("%s\\%.4s%04d.%s"), avidir, GetDriverGameName(nGame), _nAviNo++, ext);
		} while (_waccess(sztmpfile, 0) != -1);
	}

	wcscpy( avifilename, sztmpfile );
}

static void MamePlayGameAVI(void)
{
	int nGame, hr;
	WCHAR filename_avi[MAX_PATH];
	WCHAR filename_wav[MAX_PATH];

	nGame = Picker_GetSelectedItem(hWndList);

	SetupAviStatus(nGame);
	
	hr = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_AVI_STATUS),
				   hMain, AviDialogProc);

	if (hr != 1) return;

	AviStatus = (*(GetAviStatus()));

	get_autofilename(nGame, last_directory_avi, filename_avi, (WCHAR *)TEXT("avi") );

	wcscpy(filename_wav, TEXT(""));
	if (AviStatus.avi_audio_record_type)
	{
		extern struct WAV_WAVEFORMAT mame_mixer_dstwfm;

		AviStatus.wav_filename = filename_wav;

		if (AviStatus.avi_audio_record_type == 1)
		{
			WCHAR drive[_MAX_DRIVE];
			WCHAR dir  [_MAX_DIR];
			WCHAR fname[_MAX_FNAME];
			WCHAR ext  [_MAX_EXT];

			_wsplitpath(filename_avi, drive, dir, fname, ext);
			_wmakepath(filename_wav, drive, dir, fname, TEXT("wav"));

			if (wav_start_log_wave(filename_wav, &mame_mixer_dstwfm) == 0)
			{
				wav_stop_log_wave();
			} 
			else
			{
				WCHAR buf[1024];

				wsprintf(buf, TEXT("Could not open '%s' as a valid wave file."), filename_wav);
				MessageBox(hMain, buf, TEXT("EKMAME"), MB_OK | MB_ICONERROR);

				//MameMessageBox(TEXT("Could not open '%s' as a valid wave file."), filename_wav);
				hr = 0;
				AviStatus.avi_audio_record_type = 0;
			}
		}
	}

	if ( hr == 1 )
	{
		int width, height, depth;
		tRect rect;
		int avi_depth;
		int fcmp;

		extern int			nAviFrameSkip;
		extern unsigned int	nAviFrameCount;
		extern unsigned int	nAviFrameCountStart;
		extern int			nAviAudioRecord;

		width  = AviStatus.width;
		height = AviStatus.height;
		depth  = AviStatus.depth;

		rect = AviStatus.rect;

		avi_depth = depth;
		if (AviStatus.bmp_16to8_cnv == TRUE) avi_depth = 8;

		fcmp=0;
		if (AviStatus.frame_cmp == TRUE)
		{
			fcmp=3;
			if (AviStatus.frame_cmp_pre15 == TRUE)	fcmp=1;
			if (AviStatus.frame_cmp_few == TRUE)	fcmp=2;
		}
		if (AviStatus.fps == AviStatus.def_fps)		fcmp=0;

		nAviFrameSkip = AviStatus.frame_skip;

		nAviFrameCount = 0;
		nAviFrameCountStart = (unsigned int)(((AviStatus.hour*60 + AviStatus.minute)*60 + AviStatus.second) * AviStatus.def_fps);

		nAviAudioRecord = AviStatus.avi_audio_record_type;

		//if (AviStartCapture(hMain, filename_avi, &AviStatus))
		{
			play_options playopts;
			WCHAR buf[1024];
			wsprintf(buf, TEXT("Use 'Record AVI' key to toggle start/stop AVI recording."));
			MessageBox(hMain, buf, TEXT("EKMAME"), MB_OK | MB_ICONEXCLAMATION );

			memset(&playopts, 0, sizeof(playopts));
			playopts.aviwrite2 = (char*)filename_avi;
			MamePlayGameWithOptions(nGame, &playopts);
			//AviEndCapture();
		}
		//else 
		{
			
			//if( _nAviNo ) _nAviNo--;
		}
	}
}

static void MamePlayBackGameAVI()
{
	int nGame;
	char filename[MAX_PATH];
	char filename_avi[MAX_PATH];
	char filename_wav[MAX_PATH];
	int	hr;
	play_options playopts;

	memset(&playopts, 0, sizeof(playopts));

	*filename = 0;

	nGame = Picker_GetSelectedItem(hWndList);
	if (nGame != -1)
		strcpy(filename, GetDriverGameName(nGame));


	if (CommonFileDialog(GetOpenFileName, (char *)filename, FILETYPE_INPUT_FILES,false))
	{
		osd_file::error filerr;
		// char drive[_MAX_DRIVE];
		// char dir[_MAX_DIR];
		// char bare_fname[_MAX_FNAME];
		// char ext[_MAX_EXT];

		// char path[MAX_PATH];
		// char fname[MAX_PATH];
#ifdef KAILLERA
		char fname2[MAX_PATH];
#endif /* KAILEERA */
		// char *stemp;

		// _splitpath(filename, drive, dir, bare_fname, ext);

		// sprintf(path,"%s%s",drive,dir);
		// sprintf(fname,"%s%s",bare_fname,ext);
		// if (path[strlen(path)-1] == '\\')
		// 	path[strlen(path)-1] = 0; // take off trailing back slash

		// stemp = win_wstring_from_utf8(fname);
		
		//emu_file pPlayBack = emu_file(*(MameUIGlobal()), SEARCHPATH_INPUTLOG, OPEN_FLAG_READ);

		wchar_t *t_filename = win_wstring_from_utf8(filename);
		wchar_t *tempname = PathFindFileName(t_filename);
		char *fname = win_utf8_from_wstring(tempname);
		std::string const name = fname;
		free(t_filename);


		emu_file check(GetInpDir(), OPEN_FLAG_READ);
		filerr = check.open(name);

		//fileerr = pPlayBack.open(stemp);
		
		// free(stemp);

		// check for game name embedded in .inp header
		if (filerr != osd_file::error::NONE)
		{
			ErrorMessageBox("'%s' 를 열수 없습니다.", name);
			return;
		}

		inp_header header;

		// read the header and verify that it is a modern version; if not, print an error
		if (!header.read(check))
		{
			ErrorMessageBox("손상된 파일 입니다.");
			return;
		}

		// find game and play it
		std::string const sysname = header.get_sysname();

		for (int i = 0; i < driver_list::total(); i++)
		{
			if (sysname == GetDriverGameName(i))
			{
				nGame = i;
				break;
			}
		}

		playopts.playback = name.c_str();
		MamePlayGameWithOptions(nGame, &playopts);

		SetupAviStatus(nGame);
		
		hr = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_AVI_STATUS),  hMain, AviDialogProc);

		if (hr == 1)
		{
			AviStatus = (*(GetAviStatus()));

			// if (1)
			// {
			// 	//char fname[_MAX_FNAME];

			// 	_wsplitpath(filename, NULL, NULL, bare_fname, NULL);
			// 	wcscpy(filename_avi, bare_fname);
			// } else
			// 	wcscpy(filename_avi, driversw[nGame]->name);

			char fname[_MAX_FNAME];

			_splitpath(filename, NULL, NULL, fname, NULL);
			strcpy(filename_avi, fname);

			if (!CommonFileDialog(GetSaveFileName, filename_avi, FILETYPE_AVI_FILES,true))
				hr = 0;
		}

		if (hr == 1) 
		{
			strcpy(filename_wav, "");
			if (AviStatus.avi_audio_record_type)
			{
				extern struct WAV_WAVEFORMAT mame_mixer_dstwfm;

				AviStatus.wav_filename = filename_wav;

				if (AviStatus.avi_audio_record_type == 1)
				{
					char drive[_MAX_DRIVE];
					char dir[_MAX_DIR];
					char fname[_MAX_FNAME];
					char ext[_MAX_EXT];

					_splitpath(filename_avi, drive, dir, fname, ext);
					_makepath(filename_wav, drive, dir, fname, "wav");

					if (wav_start_log_wave(filename_wav, &mame_mixer_dstwfm) == 0)
					{
						wav_stop_log_wave();
					} 
					else
					{
						ErrorMessageBox("Wav 파일 '%s' 를 열수 없습니다.", filename_wav);
						// MameMessageBox(_UIW(TEXT("Could not open '%s' as a valid wave file.")), filename_wav);
						hr = 0;
						AviStatus.avi_audio_record_type = 0;
					}
				}
			}
		}

		if ( hr == 1 )
		{
			int width, height, depth;
			tRect rect;
			int avi_depth;
			int fcmp;
			extern int	nAviFrameSkip;
			extern unsigned int				nAviFrameCount;
			extern unsigned int				nAviFrameCountStart;
			extern int						nAviAudioRecord;


			width  = AviStatus.width;
			height = AviStatus.height;
			depth  = AviStatus.depth;

			rect = AviStatus.rect;

			avi_depth = depth;
			if (AviStatus.bmp_16to8_cnv == TRUE) avi_depth = 8;

			fcmp=0;
			if (AviStatus.frame_cmp == TRUE)
			{
				fcmp=3;
				if (AviStatus.frame_cmp_pre15 == TRUE)	fcmp=1;
				if (AviStatus.frame_cmp_few == TRUE)	fcmp=2;
			}
			if (AviStatus.fps == AviStatus.def_fps)		fcmp=0;

			nAviFrameSkip = AviStatus.frame_skip;

			nAviFrameCount = 0;
			nAviFrameCountStart = (unsigned int)(((AviStatus.hour*60 + AviStatus.minute)*60 + AviStatus.second) * AviStatus.def_fps);

			nAviAudioRecord = AviStatus.avi_audio_record_type;

			//if (AviStartCapture(hMain, filename_avi, &AviStatus))
			{
				
				WCHAR buf[1024];
				wsprintf(buf, TEXT(" 'Record AVI' 키를 이용하여 AVI 녹화를 시작/중지"));
				MessageBox(hMain, buf, TEXT(MAMEUINAME), MB_OK | MB_ICONEXCLAMATION );

				playopts.playback = fname;
				playopts.aviwrite2 = filename_avi;
				MamePlayGameWithOptions(nGame, &playopts);
				//AviEndCapture();
			}
		}

#ifdef KAILLERA
		if (playopts.playbacksub != NULL)
			DeleteTrctempStateSaveFile(playopts.playbacksub);
#endif /* KAILLERA */
		free(fname);
	}

}
 #endif /* MAME_AVI */