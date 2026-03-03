// license:BSD-3-Clause
// copyright-holders:Chris Kirmse, Mike Haaland, René Single, Mamesick

#include "winui.h"

/***************************************************************************
    private variables
 ***************************************************************************/

/* this has an entry for every folder eventually in the UI, including subfolders */
static TREEFOLDER **treeFolders = 0;
static UINT numFolders  = 0;        		/* Number of folder in the folder array */
static UINT next_folder_id = MAX_FOLDERS;
static UINT folderArrayLength = 0;  		/* Size of the folder array */
static LPTREEFOLDER lpCurrentFolder = 0;    /* Currently selected folder */
static UINT nCurrentFolder = 0;     		/* Current folder ID */
static WNDPROC g_lpTreeWndProc = 0;   		/* for subclassing the TreeView */
static HIMAGELIST hTreeSmall = 0;         	/* TreeView Image list of icons */
/* this only has an entry for each TOP LEVEL extra folder + SubFolders*/
LPEXFOLDERDATA ExtraFolderData[MAX_EXTRA_FOLDERS * MAX_EXTRA_SUBFOLDERS];
static int numExtraFolders = 0;
static int numExtraIcons = 0;
static char *ExtraFolderIcons[MAX_EXTRA_FOLDERS];
// built in folders and filters
static LPCFOLDERDATA  g_lpFolderData;
static LPCFILTER_ITEM g_lpFilterList;

/***************************************************************************
    private function prototypes
 ***************************************************************************/

static bool	InitFolders(void);
static bool CreateTreeIcons(void);
static void	CreateAllChildFolders(void);
static bool AddFolder(LPTREEFOLDER lpFolder);
static LPTREEFOLDER NewFolder(const char *lpTitle, UINT nFolderId, int nParent, UINT nIconId, DWORD dwFlags);
static void DeleteFolder(LPTREEFOLDER lpFolder);
static LRESULT CALLBACK TreeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static int InitExtraFolders(void);
static void FreeExtraFolders(void);
static void SetExtraIcons(char *name, int *id);
static bool TryAddExtraFolderAndChildren(int parent_index);
static bool TrySaveExtraFolder(LPTREEFOLDER lpFolder);
//static void LoadExternalFolders(int parent_index, const char *fname, int id);
//static void SaveExternalFolders(int parent_index, const char *fname);
static bool FilterAvailable(int driver_index);

/***************************************************************************
    Functions to build builtin folder lists
 ***************************************************************************/

//static void CreateManufacturerFolders(int parent_index);
//static void CreateYearFolders(int parent_index);
static void CreateSourceFolders(int parent_index);
//static void CreateDeficiencyFolders(int parent_index);
static void CreateBIOSFolders(int parent_index);
//static void CreateCPUFolders(int parent_index);
//static void CreateSoundFolders(int parent_index);
//static void CreateScreenFolders(int parent_index);
//static void CreateFPSFolders(int parent_index);
//static void CreateResolutionFolders(int parent_index);
//static void CreateSaveStateFolders(int parent_index);
//static void CreateDumpingFolders(int parent_index);

/***************************************************************************
    public structures
 ***************************************************************************/
char * ConvertWCtoC(wchar_t* str)
{

    char* pStr ; 


    int strSize = WideCharToMultiByte(CP_ACP, 0,str,-1, NULL, 0,NULL, NULL);


    pStr = new char[strSize];


    WideCharToMultiByte(CP_ACP, 0, str, -1, pStr, strSize, 0,0);

    return pStr;
}

extern const FOLDERDATA g_folderData[] =
{
	// commented-out lines have parts not defined elsewhere
	{"All Games",           "allgames",            FOLDER_ALLGAMES,            IDI_FOLDER_ALLGAMES,            0,            0,            0, NULL,                       NULL,                    true },
	{"Working",             "working",             FOLDER_WORKING,             IDI_WORKING,                    F_WORKING,    F_NONWORKING, 0, NULL,                       DriverIsBroken,          false },
//	{"Available",           "available",           FOLDER_AVAILABLE,           IDI_FOLDER_AVAILABLE,           F_AVAILABLE,  0,            0, NULL,                       FilterAvailable,         true },
    {"Not Working",         "nonworking",          FOLDER_NONWORKING,          IDI_NONWORKING,                 F_NONWORKING, F_WORKING,    0, NULL,                       DriverIsBroken,          true },
	{"Console",             "console",             FOLDER_CONSOLE,             IDI_FOLDER_CONSOLE,             0,            0,            0, NULL,                       DriverIsConsole,        true },
	{"BIOS",                "bios",                FOLDER_BIOS,                IDI_FOLDER_BIOS,                0,            0,            1, CreateBIOSFolders,          DriverIsBios,            true },
	{"Collection",          "collection",	       FOLDER_COLLECTION,          IDI_FOLDER_COLLECTION,          0,			 0, 		   0, CreateCOLLECTIONFolders },
	{"Metal Slug 1",        "metalslug1",	       FOLDER_MSLUG,		       IDI_FOLDER_MSLUG,		       0,			 0, 		   0, CreateMSLUGFolders },
	{"Metal Slug 2",        "metalslug2", 	       FOLDER_MSLUG2,		       IDI_FOLDER_MSLUG2,		       0,			 0, 		   0, CreateMSLUG2Folders },
	{"Metal Slug 3",        "metalslug3", 	       FOLDER_MSLUG3,		       IDI_FOLDER_MSLUG3,		       0,			 0, 		   0, CreateMSLUG3Folders },
	{"Metal Slug 4",        "metalslug4",		   FOLDER_MSLUG4,		       IDI_FOLDER_MSLUG4,		       0,			 0, 		   0, CreateMSLUG4Folders },
	{"Metal Slug 5",        "metalslug5",		   FOLDER_MSLUG5,		       IDI_FOLDER_MSLUG5,		       0,			 0, 		   0, CreateMSLUG5Folders },
	{"Metal Slug X",        "metalslugx",		   FOLDER_MSLUGX,		       IDI_FOLDER_MSLUGX,		       0,			 0, 		   0, CreateMSLUGXFolders },
	{"Update 2026", 	    "update 2026",	       FOLDER_UPDATE,              IDI_FOLDER_UPDATE,              0,			 0, 		   0, CreateUPDATEFolders },
    {"Misterix", 	        "mvs",	               FOLDER_MVS,                 IDI_FOLDER_MVS,                 0,			 0, 		   0, NULL,                       DriverIsMvs,             true },
//	{"Mechanical",          "mechanical",          FOLDER_MECHANICAL,          IDI_MECHANICAL,                 0,            0,            0, NULL,                       DriverIsMechanical,      true },
//	{"Unavailable",   "unavailable",      FOLDER_UNAVAILABLE,  IDI_FOLDER_UNAVAILABLE,   0,             F_AVAILABLE,  0, NULL,                       FilterAvailable,         false },
//	{"Parents",       "originals",        FOLDER_ORIGINAL,     IDI_FOLDER_ORIGINALS,     F_ORIGINALS,   F_CLONES,     0, NULL,                       DriverIsClone,           false },
//	{"Clones",        "clones",           FOLDER_CLONES,       IDI_FOLDER_CLONES,        F_CLONES,      F_ORIGINALS,  0, NULL,                       DriverIsClone,           true },
	{"Source",        "source",           FOLDER_SOURCE,       IDI_FOLDER_SOURCE,        0,             0,            0, CreateSourceFolders },
//	{"Vertical",      "vertical",         FOLDER_VERTICAL,     IDI_FOLDER_VERTICAL,      F_VERTICAL,    F_HORIZONTAL, 0, NULL,                       DriverIsVertical,        true },
//	{"Horizontal",    "horizontal",       FOLDER_HORIZONTAL,   IDI_FOLDER_HORIZONTAL,    F_HORIZONTAL,  F_VERTICAL,   0, NULL,                       DriverIsVertical,        false },
//	{"Working",       "working",          FOLDER_WORKING,      IDI_WORKING,              F_WORKING,     F_NONWORKING, 0, NULL,                       DriverIsBroken,          false },
//	{"Imperfect",     "imperfect",        FOLDER_DEFICIENCY,   IDI_FOLDER_IMPERFECT,     0,             0,            0, CreateDeficiencyFolders },
//	{"Year",          "year",             FOLDER_YEAR,         IDI_FOLDER_YEAR,          0,             0,            0, CreateYearFolders },
//	{"Manufacturer",  "manufacturer",     FOLDER_MANUFACTURER, IDI_FOLDER_MANUFACTURER,  0,             0,            0, CreateManufacturerFolders },
//	{"CHD",           "harddisk",         FOLDER_HARDDISK,     IDI_HARDDISK,             0,             0,            0, NULL,                       DriverIsHarddisk,        true },
//	{"CPU",           "cpu",              FOLDER_CPU,          IDI_FOLDER_CPU,           0,             0,            1, CreateCPUFolders },
//	{"Lightgun",      "lightgun",         FOLDER_LIGHTGUN,     IDI_FOLDER_LIGHTGUN,      0,             0,            0, NULL,                       DriverUsesLightGun,      true },
//	{"Trackball",     "trackball",        FOLDER_TRACKBALL,    IDI_FOLDER_TRACKBALL,     0,             0,            0, NULL,                       DriverUsesTrackball,     true },
//	{"Raster",        "raster",           FOLDER_RASTER,       IDI_FOLDER_RASTER,        F_RASTER,      F_VECTOR,     0, NULL,                       DriverIsVector,          false },
//	{"Vector",        "vector",           FOLDER_VECTOR,       IDI_FOLDER_VECTOR,        F_VECTOR,      F_RASTER,     0, NULL,                       DriverIsVector,          true },
//	{"Savestate",     "savestate",        FOLDER_SAVESTATE,    IDI_FOLDER_SAVESTATE,     0,             0,            0, CreateSaveStateFolders },
//	{"Dumping Status","dumping",          FOLDER_DUMPING,      IDI_FOLDER_DUMP,          0,             0,            1, CreateDumpingFolders },
//  {"Non Mechanical",  "nonmechanical",     FOLDER_NONMECHANICAL,IDI_FOLDER,               0,             0,            0, NULL,                       DriverIsMechanical,      FALSE },
//	{"Refresh",       "refresh",          FOLDER_FPS,          IDI_FOLDER_FPS,           0,             0,            1, CreateFPSFolders },
//	{"Resolution",    "resolution",       FOLDER_RESOLUTION,   IDI_FOLDER_RESOL,         0,             0,            1, CreateResolutionFolders },
//	{"Sound",         "sound",            FOLDER_SOUND,        IDI_FOLDER_SOUND,         0,             0,            1, CreateSoundFolders },
//	{"Samples",       "samples",          FOLDER_SAMPLES,      IDI_FOLDER_SAMPLES,       0,             0,            0, NULL,                       DriverUsesSamples,       true },
//	{"Screens",       "screens",          FOLDER_SCREENS,      IDI_FOLDER_MONITOR,       0,             0,            0, CreateScreenFolders },
//  {"Stereo",        "stereo",       FOLDER_STEREO,       IDI_FOLDER,               0,             0,            0, NULL,                       DriverIsStereo,          TRUE },
	{ NULL }
};

/* list of filter/control Id pairs */
extern const FILTER_ITEM g_filterList[] =
{
	{ F_CLONES,       IDC_FILTER_CLONES,      DriverIsClone, 		true },
	{ F_NONWORKING,   IDC_FILTER_NONWORKING,  DriverIsBroken, 		true },
	{ F_UNAVAILABLE,  IDC_FILTER_UNAVAILABLE, FilterAvailable, 		false },
	{ F_RASTER,       IDC_FILTER_RASTER,      DriverIsVector, 		false },
	{ F_VECTOR,       IDC_FILTER_VECTOR,      DriverIsVector, 		true },
	{ F_ORIGINALS,    IDC_FILTER_ORIGINALS,   DriverIsClone, 		false },
	{ F_WORKING,      IDC_FILTER_WORKING,     DriverIsBroken, 		false },
	{ F_AVAILABLE,    IDC_FILTER_AVAILABLE,   FilterAvailable, 		true },
	{ F_HORIZONTAL,   IDC_FILTER_HORIZONTAL,  DriverIsVertical, 	false },
	{ F_VERTICAL,     IDC_FILTER_VERTICAL,    DriverIsVertical, 	true },
	{ 0 }
};

static const TREEICON treeIconNames[] =
{
	{ IDI_FOLDER_OPEN,         	"foldopen" },
	{ IDI_FOLDER,              	"folder" },
	{ IDI_FOLDER_AVAILABLE,    	"foldavail" },
	{ IDI_FOLDER_UNAVAILABLE,  	"foldunav" },
	{ IDI_MANUFACTURER,        	"manufact" },
	{ IDI_YEAR,                	"year" },
	{ IDI_CHIP,                	"chip" },
	{ IDI_BIOS,                	"bios" },
	{ IDI_SOURCE,              	"source" },
	{ IDI_MONITOR,             	"monitor" },
	{ IDI_FOLDER_ALLGAMES,     	"allgames" },
	{ IDI_FOLDER_CLONES,       	"foldclone" },
	{ IDI_FOLDER_ORIGINALS,    	"foldorig" },
	{ IDI_FOLDER_SOURCE,		"foldsrc" },	
//#ifdef USE_GAMEFOLDERS
	{ IDI_FOLDER_MSLUG,	        "fold_mslug" },
	{ IDI_FOLDER_MSLUG2,        "fold_mslug2" },
	{ IDI_FOLDER_MSLUG3,        "fold_mslug3" },
    { IDI_FOLDER_MSLUG4,	    "fold_mslug4" },
    { IDI_FOLDER_MSLUG5,	    "fold_mslug5" },
    { IDI_FOLDER_MSLUGX,	    "fold_mslugx" },
	{ IDI_FOLDER_COLLECTION,	"fold_collection" },
	{ IDI_FOLDER_UPDATE,	    "fold_update" },
	{ IDI_FOLDER_CONSOLE,    	"fold_console" },
    { IDI_FOLDER_MVS,	        "fold_mvs" },

//#endif	
	{ IDI_FOLDER_MANUFACTURER,	"foldmanu" },
	{ IDI_FOLDER_YEAR,			"foldyear" },
	{ IDI_FOLDER_HORIZONTAL,	"horz" },
	{ IDI_FOLDER_VERTICAL,		"vert" },
	{ IDI_WORKING,				"working" },
	{ IDI_NONWORKING,			"nonwork" },
	{ IDI_HARDDISK, 			"harddisk" },
	{ IDI_MECHANICAL,			"mechanical" },
	{ IDI_FOLDER_IMPERFECT, 	"foldimp" },
	{ IDI_FOLDER_CPU,			"foldcsb" },
	{ IDI_FOLDER_SOUND, 		"foldcsb" },
	{ IDI_FOLDER_BIOS,			"foldcsb" },
	{ IDI_FOLDER_RASTER,		"foldrast" },
	{ IDI_FOLDER_VECTOR,		"foldvect" },
	{ IDI_FOLDER_MONITOR,		"multimon" },
	{ IDI_FOLDER_FPS,			"foldfps" },
	{ IDI_FOLDER_RESOL, 		"foldres" },
	{ IDI_FOLDER_LIGHTGUN,		"foldgun" },
	{ IDI_FOLDER_TRACKBALL, 	"foldball" },
	{ IDI_FOLDER_SAVESTATE, 	"savestate" },
	{ IDI_FOLDER_DUMP,			"foldcsb" },
	{ IDI_FOLDER_SAMPLES,		"foldsamp" }

};

/***************************************************************************
    public functions
 ***************************************************************************/

/* De-allocate all folder memory */
void FreeFolders(void)
{
	if (treeFolders != NULL)
	{
		if (numExtraFolders)
		{
			FreeExtraFolders();
			numFolders -= numExtraFolders;
		}

		for (int i = numFolders - 1; i >= 0; i--)
		{
			DeleteFolder(treeFolders[i]);
			treeFolders[i] = NULL;
			numFolders--;
		}

		free(treeFolders);
		treeFolders = NULL;
	}

	numFolders = 0;
}

/* Reset folder filters */
void ResetFilters(void)
{
	if (treeFolders != 0)
		for (int i = 0; i < (int)numFolders; i++)
			treeFolders[i]->m_dwFlags &= ~F_MASK;
}

void InitTree(LPCFOLDERDATA lpFolderData, LPCFILTER_ITEM lpFilterList)
{
	g_lpFolderData = lpFolderData;
	g_lpFilterList = lpFilterList;

	InitFolders();
	/* this will subclass the treeview (where WM_DRAWITEM gets sent for the header control) */
	LONG_PTR l = GetWindowLongPtr(GetTreeView(), GWLP_WNDPROC);
	g_lpTreeWndProc = (WNDPROC)l;
	SetWindowLongPtr(GetTreeView(), GWLP_WNDPROC, (LONG_PTR)TreeWndProc);
}

void SetCurrentFolder(LPTREEFOLDER lpFolder)
{
	lpCurrentFolder = (lpFolder == 0) ? treeFolders[0] : lpFolder;
	nCurrentFolder = (lpCurrentFolder) ? lpCurrentFolder->m_nFolderId : 0;
}

LPTREEFOLDER GetCurrentFolder(void)
{
	return lpCurrentFolder;
}

UINT GetCurrentFolderID(void)
{
	return nCurrentFolder;
}

int GetNumFolders(void)
{
	return numFolders;
}

LPTREEFOLDER GetFolder(UINT nFolder)
{
	return (nFolder < numFolders) ? treeFolders[nFolder] : NULL;
}

LPTREEFOLDER GetFolderByID(UINT nID)
{
	for (int i = 0; i < numFolders; i++)
	{
		if (treeFolders[i]->m_nFolderId == nID)
			return treeFolders[i];
	}

	return (LPTREEFOLDER)0;
}

void AddGame(LPTREEFOLDER lpFolder, UINT nGame)
{
	if (lpFolder)
		SetBit(lpFolder->m_lpGameBits, nGame);
}

void RemoveGame(LPTREEFOLDER lpFolder, UINT nGame)
{
	ClearBit(lpFolder->m_lpGameBits, nGame);
}

int FindGame(LPTREEFOLDER lpFolder, int nGame)
{
	return FindBit(lpFolder->m_lpGameBits, nGame, true);
}

// Called to re-associate games with folders
void ResetWhichGamesInFolders(void)
{
	for (int i = 0; i < numFolders; i++)
	{
		LPTREEFOLDER lpFolder = treeFolders[i];
		// setup the games in our built-in folders
		for (int k = 0; g_lpFolderData[k].m_lpTitle; k++)
		{
			if (lpFolder->m_nFolderId == g_lpFolderData[k].m_nFolderId)
			{
				if (g_lpFolderData[k].m_pfnQuery || g_lpFolderData[k].m_bExpectedResult)
				{
					SetAllBits(lpFolder->m_lpGameBits, false);

					for (int jj = 0; jj < driver_list::total(); jj++)
					{
						// invoke the query function
						bool b = g_lpFolderData[k].m_pfnQuery ? g_lpFolderData[k].m_pfnQuery(jj) : true;

						// if we expect false, flip the result
						if (!g_lpFolderData[k].m_bExpectedResult)
							b = !b;

						// if we like what we hear, add the game
						if (b)
							AddGame(lpFolder, jj);
					}
				}
				break;
			}
		}
	}
}

/* Used to build the GameList */
bool GameFiltered(int nGame, DWORD dwMask)
{
	LPTREEFOLDER lpFolder = GetCurrentFolder();

	//Filter out the Bioses on all Folders, except for the Bios Folder
	if(lpFolder && lpFolder->m_nFolderId != FOLDER_BIOS)
	{
		if(DriverIsBios(nGame))
			return true;
	}

// (cache, 0 = cache, 1)
/* Add games "MACHINE_TYPE_CONSOLE" "MACHINE_TYPE_COMPUTER" "MACHINE_TYPE_OTHER"*/
	if(lpFolder && lpFolder->m_nFolderId != FOLDER_CONSOLE)
	{
		if(DriverIsConsole(nGame))
			return true;
	}

/* Add games "MACHINE_IS_INCOMPLETE" */
	if(lpFolder && lpFolder->m_nFolderId != FOLDER_MVS)
	{
		if(DriverIsMvs(nGame))

			return true;
	}

// (cache, 8) "MACHINE_NO_COCKTAIL"
// (cache, 10) "MACHINE_REQUIRES_ARTWORK"
// (cache, 11) "MACHINE_CLICKABLE_ARTWORK"
// (cache, 12) "MACHINE_UNOFFICIAL"
// (cache, 21) "MACHINE_WRONG_COLORS"
// (cache, 22) "MACHINE_UNEMULATED_PROTECTION"
// (cache, 23) "MACHINE_IMPERFECT_CONTROLS"

	if(driver_list::driver(nGame).name[0] == '_')
		return true;

	// Filter games--return true if the game should be HIDDEN in this view
	if(GetFilterInherit())
	{
		if(lpFolder)
		{
			LPTREEFOLDER lpParent = GetFolder(lpFolder->m_nParent);

			if(lpParent)
			{
				/* Check the Parent Filters and inherit them on child,
                * The inherited filters don't display on the custom Filter Dialog for the Child folder
                * No need to promote all games to parent folder, works as is */
				dwMask |= lpParent->m_dwFlags;
			}
		}
	}

	if (strlen(GetSearchText()) && _stricmp(GetSearchText(), SEARCH_PROMPT))
	{
		
		if (MyStrStrI(GetDescriptionByIndex(nGame, GetUsekoreanList()), GetSearchText()) == NULL &&
			MyStrStrI(GetGameNameByIndex(nGame,GetUsekoreanList()), GetSearchText()) == NULL )
			return true;
		
//		if (MyStrStrI(GetDriverGameTitle(nGame), GetSearchText()) == NULL &&
//			MyStrStrI(GetDriverGameName(nGame), GetSearchText()) == NULL)
//			return true;
	}
	
	/*Filter Text is already global*/
//	if (MyStrStrI(GetDriverGameTitle(nGame), GetFilterText()) == NULL &&
//		MyStrStrI(GetDriverGameName(nGame), GetFilterText()) == NULL &&
//		MyStrStrI(GetDriverFileName(nGame), GetFilterText()) == NULL &&
//		MyStrStrI(GetDriverGameManufacturer(nGame), GetFilterText()) == NULL)
//		return true;


	if (MyStrStrI(GetDescriptionByIndex(nGame, GetUsekoreanList()), GetFilterText()) == NULL &&
		MyStrStrI(GetGameNameByIndex(nGame,GetUsekoreanList()), GetFilterText()) == NULL &&
		MyStrStrI(GetDriverFileName(nGame), GetFilterText()) == NULL &&
		MyStrStrI(GetGameManufactureByIndex(nGame,GetUsekoreanList()), GetFilterText()) == NULL)
		return true;

	// Are there filters set on this folder?
	if ((dwMask & F_MASK) == 0)
		return false;

	// Filter out clones?
	if (dwMask & F_CLONES && DriverIsClone(nGame))
		return true;

	for (int i = 0; g_lpFilterList[i].m_dwFilterType; i++)
		if (dwMask & g_lpFilterList[i].m_dwFilterType)
			if (g_lpFilterList[i].m_pfnQuery(nGame) == g_lpFilterList[i].m_bExpectedResult)
				return true;

	return false;
}

/* Get the parent of game in this view */
bool GetParentFound(int nGame)
{
	LPTREEFOLDER lpFolder = GetCurrentFolder();

	if(lpFolder)
	{
		int nParentIndex = GetParentIndex(&driver_list::driver(nGame));

		/* return false if no parent is there in this view */
		if( nParentIndex == -1)
			return false;

		/* return false if the folder should be HIDDEN in this view */
		if (TestBit(lpFolder->m_lpGameBits, nParentIndex) == 0)
			return false;

		/* return false if the game should be HIDDEN in this view */
		if (GameFiltered(nParentIndex, lpFolder->m_dwFlags))
			return false;

		return true;
	}

	return false;
}

LPCFILTER_ITEM GetFilterList(void)
{
	return g_lpFilterList;
}

/***************************************************************************
    private functions
 ***************************************************************************/

static void CreateSourceFolders(int parent_index)
{
	int i = 0; 
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER lpTemp;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits, false);
	
	for (int jj = 0; jj < driver_list::total(); jj++)
	{
		const char *source = GetDriverFileName(jj);

		// look for an existant source treefolder for this game
		// (likely to be the previous one, so start at the end)
		for (i = numFolders - 1; i >= start_folder; i--)
		{
			if (strcmp(treeFolders[i]->m_lpTitle, source) == 0)
			{
				AddGame(treeFolders[i], jj);
				break;
			}
		}

		if (i == start_folder - 1)
		{
			// nope, it's a source file we haven't seen before, make it.
			lpTemp = NewFolder(source, next_folder_id++, parent_index, IDI_SOURCE, GetFolderFlags(numFolders));
			AddFolder(lpTemp);
			AddGame(lpTemp, jj);
		}
	}

	SetNumOptionFolders(-1);
}


/*
static void CreateManufacturerFolders(int parent_index)
{
	int i = 0; 
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER lpTemp;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits, false);

	for (int jj = 0; jj < driver_list::total(); jj++)
	{
		const char *made = GetDriverGameManufacturer(jj);

		// look for an existant manufacturer treefolder for this game
		// (likely to be the previous one, so start at the end)
		for (i = numFolders - 1; i >= start_folder; i--)
		{
			if (strcmp(treeFolders[i]->m_lpTitle, made) == 0)
			{
				AddGame(treeFolders[i], jj);
				break;
			}
		}

		if (i == start_folder - 1)
		{
			// nope, it's a manufacturer we haven't seen before, make it.
			lpTemp = NewFolder(made, next_folder_id++, parent_index, IDI_MANUFACTURER, GetFolderFlags(numFolders));
			AddFolder(lpTemp);
			AddGame(lpTemp, jj);
		}
	}
}
*/

/*
static void CreateDeficiencyFolders(int parent_index)
{
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER lpWrongCol, lpImpCol, lpImpGraph, lpMissSnd, lpImpSnd, lpIncomplete, lpNoSndHw;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits, false);
	// create our subfolders
	lpWrongCol = NewFolder("Wrong Colors", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpImpCol = NewFolder("Imperfect Colors", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpImpGraph = NewFolder("Imperfect Graphics", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMissSnd = NewFolder("Missing Sound", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpImpSnd = NewFolder("Imperfect Sound", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpIncomplete = NewFolder("Incomplete Prototype", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpNoSndHw = NewFolder("No Sound Hardware", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	AddFolder(lpWrongCol);
	AddFolder(lpImpCol);
	AddFolder(lpImpGraph);
	AddFolder(lpMissSnd);
	AddFolder(lpImpSnd);
	AddFolder(lpIncomplete);
	AddFolder(lpNoSndHw);

	for (int jj = 0; jj < driver_list::total(); jj++)
	{
		uint32_t cache = GetDriverCacheLower(jj);
		if (BIT(cache, 21))
			AddGame(lpWrongCol, jj);

		if (BIT(cache, 20))
			AddGame(lpImpCol, jj);

		if (BIT(cache, 18))
			AddGame(lpImpGraph, jj);

		if (BIT(cache, 17))
			AddGame(lpMissSnd, jj);

		if (BIT(cache, 16))
			AddGame(lpImpSnd, jj);

		if (BIT(cache, 15))
			AddGame(lpIncomplete, jj);

		if (BIT(cache, 13))
			AddGame(lpNoSndHw, jj);
	}
}
*/

/*
static void CreateYearFolders(int parent_index)
{
	int i = 0; 
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER lpTemp;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits, false);

	for (int jj = 0; jj < driver_list::total(); jj++)
	{
		const char *year = GetDriverGameYear(jj);

		// look for an extant year treefolder for this game
		// (likely to be the previous one, so start at the end)
		for (i = numFolders - 1; i >= start_folder; i--)
		{
			if (strcmp(treeFolders[i]->m_lpTitle, year) == 0)
			{
				AddGame(treeFolders[i], jj);
				break;
			}
		}

		if (i == start_folder - 1)
		{
			// nope, it's a year we haven't seen before, make it.
			lpTemp = NewFolder(year, next_folder_id++, parent_index, IDI_YEAR, GetFolderFlags(numFolders));
			AddFolder(lpTemp);
			AddGame(lpTemp, jj);
		}
	}
}
*/

static void CreateBIOSFolders(int parent_index)
{
	int i = 0; 
	int start_folder = numFolders;
	const game_driver *drv;
	int nParentIndex = -1;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER lpTemp;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits, false);

	for (int jj = 0; jj < driver_list::total(); jj++)
	{
		if (DriverIsClone(jj))
		{
			nParentIndex = GetParentIndex(&driver_list::driver(jj));

			if (nParentIndex < 0) 
				return;

			drv = &driver_list::driver(nParentIndex);
		}
		else
			drv = &driver_list::driver(jj);

		nParentIndex = GetParentIndex(drv);

		if (nParentIndex < 0 || !GetDriverGameTitle(nParentIndex))
			continue;

		for (i = numFolders - 1; i >= start_folder; i--)
		{
			if (strcmp(treeFolders[i]->m_lpTitle, GetDriverGameTitle(nParentIndex)) == 0)
			{
				AddGame(treeFolders[i], jj);
				break;
			}
		}

		if (i == start_folder - 1)
		{
			lpTemp = NewFolder(GetDriverGameTitle(nParentIndex), next_folder_id++, parent_index, IDI_BIOS, GetFolderFlags(numFolders));
			AddFolder(lpTemp);
			AddGame(lpTemp, jj);
		}
	}
}


/*
static void CreateScreenFoldersIni(int parent_index)
{
	int i = 0; 
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER lpTemp;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits, false);

	for (int jj = 0; jj < driver_list::total(); jj++)
	{
		char screen[4];
		snprintf(screen, WINUI_ARRAY_LENGTH(screen), "%d", DriverNumScreens(jj));

		// look for an existant screen treefolder for this game
		// (likely to be the previous one, so start at the end)
		for (i = numFolders - 1; i >= start_folder; i--)
		{
			if (strcmp(treeFolders[i]->m_lpTitle, screen) == 0)
			{
				AddGame(treeFolders[i], jj);
				break;
			}
		}

		if (i == start_folder - 1)
		{
			// nope, it's a screen we haven't seen before, make it.
			lpTemp = NewFolder(screen, next_folder_id++, parent_index, IDI_MONITOR, GetFolderFlags(numFolders));
			AddFolder(lpTemp);
			AddGame(lpTemp, jj);
		}
	}
}
*/

/*
static void CreateCPUFoldersIni(int parent_index)
{
	int i = 0; 
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER lpTemp;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits, false);

	for (int jj = 0; jj < driver_list::total(); jj++)
	{
		machine_config config(driver_list::driver(jj), MameUIGlobal());

		// enumerate through all devices
		for (device_execute_interface &device : execute_interface_iterator(config.root_device()))
		{
			// get the name
			const char *cpu = device.device().name();

			// look for an existant CPU treefolder for this game
			// (likely to be the previous one, so start at the end)
			for (i = numFolders - 1; i >= start_folder; i--)
			{
				if (strcmp(treeFolders[i]->m_lpTitle, cpu) == 0)
				{
					AddGame(treeFolders[i], jj);
					break;
				}
			}

			if (i == start_folder - 1)
			{
				// nope, it's a CPU we haven't seen before, make it.
				lpTemp = NewFolder(cpu, next_folder_id++, parent_index, IDI_CHIP, GetFolderFlags(numFolders));
				AddFolder(lpTemp);
				AddGame(lpTemp, jj);
			}
		}
	}
}
*/

/*
static void CreateSoundFoldersIni(int parent_index)
{
	int i = 0; 
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER lpTemp;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits, false);

	for (int jj = 0; jj < driver_list::total(); jj++)
	{
		machine_config config(driver_list::driver(jj), MameUIGlobal());

		// enumerate through all devices
		for (device_sound_interface &device : sound_interface_iterator(config.root_device()))
		{
			// get the name
			const char *sound = device.device().name();

			// look for an existant sound chip treefolder for this game
			// (likely to be the previous one, so start at the end)
			for (i = numFolders - 1; i >= start_folder; i--)
			{
				if (strcmp(treeFolders[i]->m_lpTitle, sound) == 0)
				{
					AddGame(treeFolders[i], jj);
					break;
				}
			}

			if (i == start_folder - 1)
			{
				// nope, it's a sound chip we haven't seen before, make it.
				lpTemp = NewFolder(sound, next_folder_id++, parent_index, IDI_CHIP, GetFolderFlags(numFolders));
				AddFolder(lpTemp);
				AddGame(lpTemp, jj);
			}
		}
	}
}
*/

/*
static void CreateSaveStateFolders(int parent_index)
{
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER lpSupported, lpUnsupported;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,false);
	// create our two subfolders
	lpSupported = NewFolder("Supported", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpUnsupported = NewFolder("Unsupported", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	AddFolder(lpSupported);
	AddFolder(lpUnsupported);

	for (int jj = 0; jj < driver_list::total(); jj++)
	{
		if (DriverSupportsSaveState(jj))
			AddGame(lpSupported, jj);
		else
			AddGame(lpUnsupported, jj);
	}
}
*/

/*
static void CreateResolutionFoldersIni(int parent_index)
{
	int i = 0; 
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER lpTemp, lpVectorH, lpVectorV, lpScreenless;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits, false);
	// create our two subfolders
	lpVectorH = NewFolder("Vector (H)", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpVectorV = NewFolder("Vector (V)", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpScreenless = NewFolder("Screenless", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	AddFolder(lpVectorH);
	AddFolder(lpVectorV);
	AddFolder(lpScreenless);

	for (int jj = 0; jj < driver_list::total(); jj++)
	{
		machine_config config(driver_list::driver(jj), MameUIGlobal());
		char res[32];

		if (DriverIsVector(jj))
		{
			if (DriverIsVertical(jj))
			{
				AddGame(lpVectorV, jj);
				continue;
			}
			else
			{
				AddGame(lpVectorH, jj);
				continue;
			}
		}

		const screen_device *screen = screen_device_iterator(config.root_device()).first();

		if (screen == nullptr)
		{
			AddGame(lpScreenless, jj);
			continue;
		}

		const rectangle &visarea = screen->visible_area();

		if (DriverIsVertical(jj))
			snprintf(res, WINUI_ARRAY_LENGTH(res), "%d x %d (V)", visarea.width(), visarea.height());
		else
			snprintf(res, WINUI_ARRAY_LENGTH(res), "%d x %d (H)", visarea.width(), visarea.height());

		// look for an existant resolution treefolder for this game
		// (likely to be the previous one, so start at the end)
		for (i = numFolders - 1; i >= start_folder; i--)
		{
			if (strcmp(treeFolders[i]->m_lpTitle, res) == 0)
			{
				AddGame(treeFolders[i], jj);
				break;
			}
		}

		if (i == start_folder - 1)
		{
			// nope, it's a resolution we haven't seen before, make it.
			lpTemp = NewFolder(res, next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
			AddFolder(lpTemp);
			AddGame(lpTemp, jj);
		}
	}
}
*/

/*
static void CreateFPSFoldersIni(int parent_index)
{
	int i = 0; 
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER lpTemp, lpVector, lpScreenless;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits, false);
	// create our two subfolders
	lpVector = NewFolder("Vector", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpScreenless = NewFolder("Screenless", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	AddFolder(lpVector);
	AddFolder(lpScreenless);

	for (int jj = 0; jj < driver_list::total(); jj++)
	{
		machine_config config(driver_list::driver(jj), MameUIGlobal());
		char fps[16];
		
		if (DriverIsVector(jj))
		{
			AddGame(lpVector, jj);
			continue;
		}

		const screen_device *screen = screen_device_iterator(config.root_device()).first();

		if (screen == nullptr)
		{
			AddGame(lpScreenless, jj);
			continue;
		}

		snprintf(fps, WINUI_ARRAY_LENGTH(fps), "%f Hz", ATTOSECONDS_TO_HZ(screen->refresh_attoseconds()));

		// look for an existant refresh treefolder for this game
		// (likely to be the previous one, so start at the end)
		for (i = numFolders - 1; i >= start_folder; i--)
		{
			if (strcmp(treeFolders[i]->m_lpTitle, fps) == 0)
			{
				AddGame(treeFolders[i], jj);
				break;
			}
		}

		if (i == start_folder - 1)
		{
			// nope, it's a refresh we haven't seen before, make it.
			lpTemp = NewFolder(fps, next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
			AddFolder(lpTemp);
			AddGame(lpTemp, jj);
		}
	}
}
*/

void CreateDumpingFoldersIni(int parent_index)
{
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	LPTREEFOLDER lpBadDump, lpNoDump, lpGoodDump;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,false);
	// create our two subfolders
	lpNoDump = NewFolder("No Dump", next_folder_id, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpBadDump = NewFolder("Bad Dump", next_folder_id, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpGoodDump = NewFolder("Good Dump", next_folder_id, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	AddFolder(lpNoDump);
	AddFolder(lpBadDump);
	AddFolder(lpGoodDump);

	for (int jj = 0; jj < driver_list::total(); jj++)
	{
		machine_config config(driver_list::driver(jj), MameUIGlobal());
		bool bBadDump = false;
		bool bNoDump = false;

		if (!DriverUsesRoms(jj))
			continue;

		for (device_t &device : device_iterator(config.root_device()))
		{
			for (const rom_entry *region = rom_first_region(device); region != nullptr; region = rom_next_region(region))
			{
				for (const rom_entry *rom = rom_first_file(region); rom != nullptr; rom = rom_next_file(rom))
				{
					if (ROMREGION_ISROMDATA(region) || ROMREGION_ISDISKDATA(region))
					{
						util::hash_collection hashes(ROM_GETHASHDATA(rom));
						
						if (hashes.flag(util::hash_collection::FLAG_NO_DUMP))
							bNoDump = true;
						
						if (hashes.flag(util::hash_collection::FLAG_BAD_DUMP))
							bBadDump = true;
					}
				}
			}
		}

		if (bNoDump)
		{
			AddGame(lpNoDump, jj);
			continue;
		}

		if (bBadDump)
		{
			AddGame(lpBadDump, jj);
			continue;
		}

		AddGame(lpGoodDump, jj);
	}
}

/*
static void CreateCPUFolders(int parent_index)
{
	const char *fname = "cpu.ini";

	if (RequiredDriverCache())
	{
		CreateCPUFoldersIni(parent_index);
		SaveExternalFolders(parent_index, fname);
	}
	else
		LoadExternalFolders(parent_index, fname, IDI_CHIP);

	SendMessage(GetProgressBar(), PBM_SETPOS, 20, 0);
}
*/

/*
static void CreateSoundFolders(int parent_index)
{
	const char *fname = "sound.ini";

	if (RequiredDriverCache())
	{
		CreateSoundFoldersIni(parent_index);
		SaveExternalFolders(parent_index, fname);
	}
	else
		LoadExternalFolders(parent_index, fname, IDI_CHIP);

	SendMessage(GetProgressBar(), PBM_SETPOS, 95, 0);
}
*/

/*
static void CreateScreenFolders(int parent_index)
{
	const char *fname = "screen.ini";

	if (RequiredDriverCache())
	{
		CreateScreenFoldersIni(parent_index);
		SaveExternalFolders(parent_index, fname);
	}
	else
		LoadExternalFolders(parent_index, fname, IDI_MONITOR);

	SendMessage(GetProgressBar(), PBM_SETPOS, 80, 0);
}
*/

/*
static void CreateResolutionFolders(int parent_index)
{
	const char *fname = "resolution.ini";

	if (RequiredDriverCache())
	{
		CreateResolutionFoldersIni(parent_index);
		SaveExternalFolders(parent_index, fname);
	}
	else
		LoadExternalFolders(parent_index, fname, IDI_FOLDER);

	SendMessage(GetProgressBar(), PBM_SETPOS, 65, 0);
}
*/

/*
static void CreateFPSFolders(int parent_index)
{
	const char *fname = "refresh.ini";

	if (RequiredDriverCache())
	{
		CreateFPSFoldersIni(parent_index);
		SaveExternalFolders(parent_index, fname);
	}
	else
		LoadExternalFolders(parent_index, fname, IDI_FOLDER);

	SendMessage(GetProgressBar(), PBM_SETPOS, 50, 0);
}
*/

/*
static void CreateDumpingFolders(int parent_index)
{
	const char *fname = "dumping.ini";

	if (RequiredDriverCache())
	{
		CreateDumpingFoldersIni(parent_index);
		SaveExternalFolders(parent_index, fname);
	}
	else
		LoadExternalFolders(parent_index, fname, IDI_FOLDER);

	SendMessage(GetProgressBar(), PBM_SETPOS, 35, 0);
}
*/

/*
static void LoadExternalFolders(int parent_index, const char *fname, int id)
{
	char readbuf[256];
	char filename[MAX_PATH];
	char *name = NULL;
	LPTREEFOLDER lpTemp = NULL;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	int current_id = lpFolder->m_nFolderId;
	snprintf(filename, WINUI_ARRAY_LENGTH(filename), "%s\\%s", GetGuiDir(), fname);
	FILE *f = fopen(filename, "r");
 
	if (f == NULL)
		return;

	while (fgets(readbuf, 256, f))
	{
		//do we have [...] ?
		if (readbuf[0] == '[')
		{
			char *p = strchr(readbuf, ']');
			
			if (p == NULL)
				continue;

			*p = '\0';
			name = &readbuf[1];

			// is it [FOLDER_SETTINGS]?
			if (strcmp(name, "FOLDER_SETTINGS") == 0)
			{
				current_id = -1;
				continue;
			}
			else
			{
				// is it [ROOT_FOLDER]?
				if (!strcmp(name, "ROOT_FOLDER"))
				{
					current_id = lpFolder->m_nFolderId;
					lpTemp = lpFolder;
				}
				else
				{
					current_id = next_folder_id++;
					lpTemp = NewFolder(name, current_id, parent_index, id, GetFolderFlags(numFolders));
					AddFolder(lpTemp);
				}
			}
		}
		else if (current_id != -1)
		{
			// string on a line by itself -- game name
			name = strtok(readbuf, " \t\r\n");

			if (name == NULL)
			{
				current_id = -1;
				continue;
			}

			AddGame(lpTemp, GetGameNameIndex(name));
		}
	}

	fclose(f);
}
*/

/*
static void SaveExternalFolders(int parent_index, const char *fname)
{
	int i = 0;
	char filename[MAX_PATH];
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	TREEFOLDER *folder_data;

	snprintf(filename, WINUI_ARRAY_LENGTH(filename), "%s\\%s", GetGuiDir(), fname);
	wchar_t *temp = win_wstring_from_utf8(GetGuiDir());
	CreateDirectory(temp, NULL);
	free(temp);
	FILE *f = fopen(filename, "w");

	if (f == NULL)
		return;

	fprintf(f, "[FOLDER_SETTINGS]\n");
	fprintf(f, "RootFolderIcon custom\n");
	fprintf(f, "SubFolderIcon custom\n");

	// need to loop over all our TREEFOLDERs--first the root one, then each child.
	// start with the root
	folder_data = lpFolder;
	fprintf(f, "\n[ROOT_FOLDER]\n");

	for (i = 0; i < driver_list::total(); i++)
	{
		if (TestBit(folder_data->m_lpGameBits, i))
			fprintf(f, "%s\n", GetDriverGameName(i));
	}

	// look through the custom folders for ones with our root as parent
	for (int jj = 0; jj < numFolders; jj++)
	{
		folder_data = treeFolders[jj];

		if (folder_data->m_nParent >= 0 && treeFolders[folder_data->m_nParent] == lpFolder)
		{
			fprintf(f, "\n[%s]\n", folder_data->m_lpTitle);

			for (i = 0; i < driver_list::total(); i++)
			{
				if (TestBit(folder_data->m_lpGameBits, i))
					fprintf(f, "%s\n", GetDriverGameName(i));
			}
		}
	}

	fclose(f);
}
*/

// creates child folders of all the top level folders, including custom ones
void CreateAllChildFolders(void)
{
	int num_top_level_folders = numFolders;

	for (int i = 0; i < num_top_level_folders; i++)
	{
		LPTREEFOLDER lpFolder = treeFolders[i];
		LPCFOLDERDATA lpFolderData = NULL;

		for (int j = 0; g_lpFolderData[j].m_lpTitle; j++)
		{
			if (g_lpFolderData[j].m_nFolderId == lpFolder->m_nFolderId)
			{
				lpFolderData = &g_lpFolderData[j];
				break;
			}
		}

		if (lpFolderData)
		{
			if (lpFolderData->m_pfnCreateFolders)
				lpFolderData->m_pfnCreateFolders(i);
		}
		else
		{
			if ((lpFolder->m_dwFlags & F_CUSTOM) == 0)
				continue;

			// load the extra folder files, which also adds children
			if (TryAddExtraFolderAndChildren(i) == false)
				lpFolder->m_nFolderId = FOLDER_NONE;
		}
	}
}

// adds these folders to the treeview
void ResetTreeViewFolders(void)
{
	HWND hTreeView = GetTreeView();
	TVITEM tvi;
	TVINSERTSTRUCT tvs;

	// currently "cached" parent
	HTREEITEM hti_parent = NULL;
	int index_parent = -1;

	(void)TreeView_DeleteAllItems(hTreeView);
	tvs.hInsertAfter = TVI_LAST; // main items inserted according to g_folderData[] array

	for (int i = 0; i < numFolders; i++)
	{
		LPTREEFOLDER lpFolder = treeFolders[i];

		if (lpFolder->m_nParent == -1)
		{
			if (lpFolder->m_nFolderId < MAX_FOLDERS)
			{
				// it's a built in folder, let's see if we should show it
				if (GetShowFolder(lpFolder->m_nFolderId) == false)
					continue;
			}

			tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvs.hParent = TVI_ROOT;
			tvi.pszText = lpFolder->m_lptTitle;
			tvi.lParam = (LPARAM)lpFolder;
			tvi.iImage = GetTreeViewIconIndex(lpFolder->m_nIconId);
			tvi.iSelectedImage = 0;
			tvs.item = tvi;

			// Add root branch
			hti_parent = TreeView_InsertItem(hTreeView, &tvs);
			continue;
		}

		// not a top level branch, so look for parent
		if (treeFolders[i]->m_nParent != index_parent)
		{
			hti_parent = TreeView_GetRoot(hTreeView);

			while (1)
			{
				if (hti_parent == NULL)
					// couldn't find parent folder, so it's a built-in but
					// not shown folder
					break;

				tvi.hItem = hti_parent;
				tvi.mask = TVIF_PARAM;

				(void)TreeView_GetItem(hTreeView, &tvi);

				if (((LPTREEFOLDER)tvi.lParam) == treeFolders[treeFolders[i]->m_nParent])
					break;

				hti_parent = TreeView_GetNextSibling(hTreeView, hti_parent);
			}

			// if parent is not shown, then don't show the child either obviously!
			if (hti_parent == NULL)
				continue;

			index_parent = treeFolders[i]->m_nParent;
		}

		tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvs.hParent = hti_parent;
		tvi.iImage = GetTreeViewIconIndex(treeFolders[i]->m_nIconId);
		tvi.iSelectedImage = 0;
		tvi.pszText = treeFolders[i]->m_lptTitle;
		tvi.lParam = (LPARAM)treeFolders[i];
		tvs.item = tvi;

		// Add it to this tree branch
		tvs.hInsertAfter = TVI_SORT; // sub items always sorted
		(void)TreeView_InsertItem(hTreeView, &tvs);
		tvs.hInsertAfter = TVI_LAST; // restore for next main item
	}
}

void SelectTreeViewFolder(int folder_id)
{
	HWND hTreeView = GetTreeView();
	HTREEITEM hti = TreeView_GetRoot(hTreeView);
	TVITEM tvi;

	memset(&tvi, 0, sizeof(TVITEM));

	while (hti != NULL)
	{
		tvi.hItem = hti;
		tvi.mask = TVIF_PARAM;

		(void)TreeView_GetItem(hTreeView, &tvi);

		if (((LPTREEFOLDER)tvi.lParam)->m_nFolderId == folder_id)
		{
			(void)TreeView_SelectItem(hTreeView,tvi.hItem);
			SetCurrentFolder((LPTREEFOLDER)tvi.lParam);
			return;
		}

		HTREEITEM hti_next = TreeView_GetChild(hTreeView, hti);

		if (hti_next == NULL)
		{
			hti_next = TreeView_GetNextSibling(hTreeView, hti);

			if (hti_next == NULL)
			{
				hti_next = TreeView_GetParent(hTreeView, hti);

				if (hti_next != NULL)
					hti_next = TreeView_GetNextSibling(hTreeView, hti_next);
			}
		}

		hti = hti_next;
	}

	// could not find folder to select
	// make sure we select something
	tvi.hItem = TreeView_GetRoot(hTreeView);
	tvi.mask = TVIF_PARAM;

	(void)TreeView_GetItem(hTreeView, &tvi);
	(void)TreeView_SelectItem(hTreeView, tvi.hItem);
	SetCurrentFolder((LPTREEFOLDER)tvi.lParam);

}

/*
 * Does this folder have an INI associated with it?
 * Currently only true for FOLDER_VECTOR and children
 * of FOLDER_SOURCE.
 */
static bool FolderHasIni(LPTREEFOLDER lpFolder) 
{
	if (FOLDER_RASTER == lpFolder->m_nFolderId || FOLDER_VECTOR == lpFolder->m_nFolderId ||
		FOLDER_VERTICAL == lpFolder->m_nFolderId || FOLDER_HORIZONTAL == lpFolder->m_nFolderId) 
			return true;

	if (lpFolder->m_nParent != -1 && FOLDER_SOURCE == treeFolders[lpFolder->m_nParent]->m_nFolderId) 
		return true;

	return false;
}

/* Add a folder to the list.  Does not allocate */
static bool AddFolder(LPTREEFOLDER lpFolder)
{
	TREEFOLDER **tmpTree = NULL;
	UINT oldFolderArrayLength = folderArrayLength;

	if (numFolders + 1 >= folderArrayLength)
	{
		folderArrayLength += 500;
		tmpTree = (TREEFOLDER **)malloc(sizeof(TREEFOLDER **) * folderArrayLength);
		memcpy(tmpTree, treeFolders, sizeof(TREEFOLDER **) * oldFolderArrayLength);

		if (treeFolders) 
			free(treeFolders);

		treeFolders = tmpTree;
	}

	/* Is there an folder.ini that can be edited? */
	if (FolderHasIni(lpFolder)) 
		lpFolder->m_dwFlags |= F_INIEDIT;

	treeFolders[numFolders] = lpFolder;
	numFolders++;
	return true;
}

/* Allocate and initialize a NEW TREEFOLDER */
static LPTREEFOLDER NewFolder(const char *lpTitle, UINT nFolderId, int nParent, UINT nIconId, DWORD dwFlags)
{
	LPTREEFOLDER lpFolder = (LPTREEFOLDER)malloc(sizeof(TREEFOLDER));
	memset(lpFolder, 0, sizeof(TREEFOLDER));
	lpFolder->m_lpTitle = (char *)malloc(strlen(lpTitle) + 1);
	strcpy((char *)lpFolder->m_lpTitle, lpTitle);
	lpFolder->m_lptTitle = win_wstring_from_utf8(lpFolder->m_lpTitle);
	lpFolder->m_lpGameBits = NewBits(driver_list::total());
	lpFolder->m_nFolderId = nFolderId;
	lpFolder->m_nParent = nParent;
	lpFolder->m_nIconId = nIconId;
	lpFolder->m_dwFlags = dwFlags;
	return lpFolder;
}

/* Deallocate the passed in LPTREEFOLDER */
static void DeleteFolder(LPTREEFOLDER lpFolder)
{
	if (lpFolder)
	{
		if (lpFolder->m_lpGameBits)
		{
			DeleteBits(lpFolder->m_lpGameBits);
			lpFolder->m_lpGameBits = 0;
		}

		free(lpFolder->m_lptTitle);
		lpFolder->m_lptTitle = 0;
		free(lpFolder->m_lpTitle);
		lpFolder->m_lpTitle = 0;
		free(lpFolder);
	}
}

/* Can be called to re-initialize the array of treeFolders */
static bool InitFolders(void)
{
	int i = 0;
	DWORD dwFolderFlags = 0;

	if (treeFolders)
	{
		for (i = numFolders - 1; i >= 0; i--)
		{
			DeleteFolder(treeFolders[i]);
			treeFolders[i] = 0;
			numFolders--;
		}
	}

	numFolders = 0;

	if (folderArrayLength == 0)
	{
		folderArrayLength = 200;
		treeFolders = (TREEFOLDER **)malloc(sizeof(TREEFOLDER **) * folderArrayLength);

		if (!treeFolders)
		{
			folderArrayLength = 0;
			return 0;
		}
		else
			memset(treeFolders, 0, sizeof(TREEFOLDER **) * folderArrayLength);
	}
	
	// built-in top level folders
	for (i = 0; g_lpFolderData[i].m_lpTitle; i++)
	{
		LPCFOLDERDATA fData = &g_lpFolderData[i];
		/* get the saved folder flags */
		dwFolderFlags = GetFolderFlags(numFolders);
		/* create the folder */
		AddFolder(NewFolder(fData->m_lpTitle, fData->m_nFolderId, -1, fData->m_nIconId, dwFolderFlags));
	}

	numExtraFolders = InitExtraFolders();

	for (i = 0; i < numExtraFolders; i++)
	{
		LPEXFOLDERDATA  fExData = ExtraFolderData[i];
		// OR in the saved folder flags
		dwFolderFlags = fExData->m_dwFlags | GetFolderFlags(numFolders);
		// create the folder
		AddFolder(NewFolder(fExData->m_szTitle, fExData->m_nFolderId, fExData->m_nParent, fExData->m_nIconId, dwFolderFlags));
	}

	CreateAllChildFolders();
	CreateTreeIcons();
	ResetWhichGamesInFolders();
	ResetTreeViewFolders();
	SelectTreeViewFolder(GetSavedFolderID());
	LoadFolderFlags();
	return true;
}

// create iconlist and Treeview control
static bool CreateTreeIcons(void)
{
	HICON hIcon = NULL;
	HINSTANCE hInst = GetModuleHandle(NULL);
	int numIcons = ICON_MAX + numExtraIcons;

	hTreeSmall = ImageList_Create (16, 16, ILC_COLORDDB | ILC_MASK, numIcons, numIcons);

	for (int i = 0; i < ICON_MAX; i++)
	{
		hIcon = LoadIconFromFile(treeIconNames[i].lpName);

		if (!hIcon)
			hIcon = LoadIcon(hInst, MAKEINTRESOURCE(treeIconNames[i].nResourceID));

		if (ImageList_AddIcon(hTreeSmall, hIcon) == -1)
		{
			ErrorMessageBox("Error creating icon on regular folder, %i %i", i, hIcon != NULL);
			return false;
		}
	}

	for (int i = 0; i < numExtraIcons; i++)
	{
		if ((hIcon = LoadIconFromFile(ExtraFolderIcons[i])) == 0)
			hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_FOLDER));

		if (ImageList_AddIcon(hTreeSmall, hIcon) == -1)
		{
			ErrorMessageBox("Error creating icon on extra folder, %i %i", i, hIcon != NULL);
			return false;
		}
	}

	// Be sure that all the small icons were added.
	if (ImageList_GetImageCount(hTreeSmall) < numIcons)
	{
		ErrorMessageBox("Error with icon list--too few images.  %i %i", ImageList_GetImageCount(hTreeSmall), numIcons);
		return false;
	}

	// Be sure that all the small icons were added.
	if (ImageList_GetImageCount (hTreeSmall) < ICON_MAX)
	{
		ErrorMessageBox("Error with icon list--too few images.  %i < %i", ImageList_GetImageCount(hTreeSmall), ICON_MAX);
		return false;
	}

	// Associate the image lists with the list view control.
	(void)TreeView_SetImageList(GetTreeView(), hTreeSmall, TVSIL_NORMAL);
	return true;
}

/* Header code - Directional Arrows */
static LRESULT CALLBACK TreeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_KEYDOWN :
			if (wParam == VK_F2)
			{
				if (lpCurrentFolder->m_dwFlags & F_CUSTOM)
				{
					(void)TreeView_EditLabel(hWnd, TreeView_GetSelection(hWnd));
					return true;
				}
			}

			break;
	}

	/* message not handled */
	return CallWindowProc(g_lpTreeWndProc, hWnd, uMsg, wParam, lParam);
}

/*
 * Filter code - should be moved to filter.c/filter.h
 * Added 01/09/99 - MSH <mhaaland@hypertech.com>
 */

/* find a FOLDERDATA by folderID */
LPCFOLDERDATA FindFilter(DWORD folderID)
{
	for (int i = 0; g_lpFolderData[i].m_lpTitle; i++)
		if (g_lpFolderData[i].m_nFolderId == folderID)
			return &g_lpFolderData[i];

	return (LPFOLDERDATA) 0;
}

LPTREEFOLDER GetFolderByName(int nParentId, const char *pszFolderName)
{
	//First Get the Parent TreeviewItem
	//Enumerate Children
	for (int i = 0; i < numFolders; i++)
	{
		if (!strcmp(treeFolders[i]->m_lpTitle, pszFolderName))
		{
			int nParent = treeFolders[i]->m_nParent;

			if ((nParent >= 0) && treeFolders[nParent]->m_nFolderId == nParentId)
				return treeFolders[i];
		}
	}

	return NULL;
}

static int InitExtraFolders(void)
{
	WIN32_FIND_DATA FindFileData;
	int count = 0;
	char buf[256];
	char path[MAX_PATH];
	const char *dir = GetFolderDir();

	memset(ExtraFolderData, 0, (MAX_EXTRA_FOLDERS * MAX_EXTRA_SUBFOLDERS)* sizeof(LPEXFOLDERDATA));

	// Why create the directory if it doesn't exist, just return 0 folders.
	if (osd::directory::open(dir) == nullptr)
		return 0;

	snprintf(path, WINUI_ARRAY_LENGTH(path), "%s\\*.*", dir);
	HANDLE hFind = winui_find_first_file_utf8(path, &FindFileData);

	for (int i = 0; i < MAX_EXTRA_FOLDERS; i++)
	{
		ExtraFolderIcons[i] = NULL;
	}

	numExtraIcons = 0;

	if (hFind != INVALID_HANDLE_VALUE)
	{
		while (FindNextFile (hFind, &FindFileData) != 0)
		{
			char *file = win_utf8_from_wstring(FindFileData.cFileName);
			char inifile[MAX_PATH];

			memset(&inifile, 0, sizeof(inifile));
			snprintf(inifile, WINUI_ARRAY_LENGTH(inifile), "%s\\%s", dir, file);
			FILE *readfile = fopen(inifile, "r");

			if (readfile != NULL)
			{
				int icon[2] = { 0, 0 };

				while (fgets(buf, 256, readfile))
				{
					if (buf[0] == '[')
					{
						char *p = strchr(buf, ']');

						if (p == NULL)
							continue;

						*p = '\0';
						char *name = &buf[1];

						if (!strcmp(name, "FOLDER_SETTINGS"))
						{
							while (fgets(buf, 256, readfile))
							{
								name = strtok(buf, " =\r\n");

								if (name == NULL)
									break;

								if (!strcmp(name, "RootFolderIcon"))
								{
									name = strtok(NULL, " =\r\n");

									if (name != NULL)
										SetExtraIcons(name, &icon[0]);
								}
								if (!strcmp(name, "SubFolderIcon"))
								{
									name = strtok(NULL, " =\r\n");

									if (name != NULL)
										SetExtraIcons(name, &icon[1]);
								}
							}

							break;
						}
					}
				}

				fclose(readfile);
				strcpy(buf, file);
				char *ext = strrchr(buf, '.');
				free(file);

				if (ext && *(ext + 1) && !core_stricmp(ext + 1, "ini"))
				{
					ExtraFolderData[count] =(EXFOLDERDATA*) malloc(sizeof(EXFOLDERDATA));

					if (ExtraFolderData[count])
					{
						*ext = '\0';
						memset(ExtraFolderData[count], 0, sizeof(EXFOLDERDATA));
						strncpy(ExtraFolderData[count]->m_szTitle, buf, 63);
						ExtraFolderData[count]->m_nFolderId = next_folder_id++;
						ExtraFolderData[count]->m_nParent = -1;
						ExtraFolderData[count]->m_dwFlags = F_CUSTOM;
						ExtraFolderData[count]->m_nIconId = icon[0] ? -icon[0] : IDI_FOLDER;
						ExtraFolderData[count]->m_nSubIconId = icon[1] ? -icon[1] : IDI_FOLDER;
						count++;
					}
				}
			}
		}
	}

	return count;
}

void FreeExtraFolders(void)
{
	for (int i = 0; i < numExtraFolders; i++)
	{
		if (ExtraFolderData[i])
		{
			free(ExtraFolderData[i]);
			ExtraFolderData[i] = NULL;
		}
	}

	for (int i = 0; i < numExtraIcons; i++)
	{
		free(ExtraFolderIcons[i]);
	}

	numExtraIcons = 0;
}


static void SetExtraIcons(char *name, int *id)
{
	char *p = strchr(name, '.');

	if (p != NULL)
		*p = '\0';

	ExtraFolderIcons[numExtraIcons] = (char*)malloc(strlen(name) + 1);

	if (ExtraFolderIcons[numExtraIcons])
	{
		*id = ICON_MAX + numExtraIcons;
		strcpy(ExtraFolderIcons[numExtraIcons], name);
		numExtraIcons++;
	}
}


// Called to add child folders of the top level extra folders already created
bool TryAddExtraFolderAndChildren(int parent_index)
{
	char fname[MAX_PATH];
	char readbuf[256];
	char *name = NULL;
	LPTREEFOLDER lpTemp = NULL;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	int current_id = lpFolder->m_nFolderId;
	int id = lpFolder->m_nFolderId - MAX_FOLDERS;
	snprintf(fname, WINUI_ARRAY_LENGTH(fname), "%s\\%s.ini", GetFolderDir(), ExtraFolderData[id]->m_szTitle);
	FILE *f = fopen(fname, "r");
 
	if (f == NULL)
		return false;

	while (fgets(readbuf, 256, f))
	{
		/* do we have [...] ? */
		if (readbuf[0] == '[')
		{
			char *p = strchr(readbuf, ']');

			if (p == NULL)
				continue;

			*p = '\0';
			name = &readbuf[1];

			/* is it [FOLDER_SETTINGS]? */
			if (strcmp(name, "FOLDER_SETTINGS") == 0)
			{
				current_id = -1;
				continue;
			}
			else
			{
				/* is it [ROOT_FOLDER]? */
				if (!strcmp(name, "ROOT_FOLDER"))
				{
					current_id = lpFolder->m_nFolderId;
					lpTemp = lpFolder;
				}
				else
				{
					/* must be [folder name] */
					current_id = next_folder_id++;
					/* create a new folder with this name,
					and the flags for this folder as read from the registry */
					lpTemp = NewFolder(name, current_id, parent_index, ExtraFolderData[id]->m_nSubIconId, GetFolderFlags(numFolders) | F_CUSTOM);
					ExtraFolderData[current_id] = (EXFOLDERDATA*)malloc(sizeof(EXFOLDERDATA));
					memset(ExtraFolderData[current_id], 0, sizeof(EXFOLDERDATA));
					ExtraFolderData[current_id]->m_nFolderId = current_id - MAX_EXTRA_FOLDERS;
					ExtraFolderData[current_id]->m_nIconId = ExtraFolderData[id]->m_nSubIconId;
					ExtraFolderData[current_id]->m_nParent = ExtraFolderData[id]->m_nFolderId;
					ExtraFolderData[current_id]->m_nSubIconId = -1;
					strcpy(ExtraFolderData[current_id]->m_szTitle, name);
					ExtraFolderData[current_id]->m_dwFlags = ExtraFolderData[id]->m_dwFlags;
					AddFolder(lpTemp);
				}
			}
		}
		else if (current_id != -1)
		{
			/* string on a line by itself -- game name */
			name = strtok(readbuf, " \t\r\n");

			if (name == NULL)
			{
				current_id = -1;
				continue;
			}

			/* IMPORTANT: This assumes that all driver names are lowercase! */
			for (int i = 0; name[i]; i++)
				name[i] = tolower(name[i]);

			if (lpTemp == NULL)
			{
				//ErrorMessageBox("Error parsing %s: missing [folder name] or [ROOT_FOLDER]", fname);
				current_id = lpFolder->m_nFolderId;
				lpTemp = lpFolder;
			}

			AddGame(lpTemp, GetGameNameIndex(name));
		}
	}

	fclose(f);
	return true;
}


void GetFolders(TREEFOLDER ***folders,int *num_folders)
{
	*folders = treeFolders;
	*num_folders = numFolders;
}

static bool TryRenameCustomFolderIni(LPTREEFOLDER lpFolder, const char *old_name, const char *new_name)
{
	char filename[MAX_PATH];
	char new_filename[MAX_PATH];

	if (lpFolder->m_nParent >= 0)
	{
		//it is a custom SubFolder
		LPTREEFOLDER lpParent = GetFolder(lpFolder->m_nParent);

		if(lpParent)
		{
			snprintf(filename, WINUI_ARRAY_LENGTH(filename), "%s\\%s\\%s.ini", GetIniDir(), lpParent->m_lpTitle, old_name);
			snprintf(new_filename, WINUI_ARRAY_LENGTH(new_filename), "%s\\%s\\%s.ini", GetIniDir(), lpParent->m_lpTitle, new_name);
			winui_move_file_utf8(filename, new_filename);
		}
	}
	else
	{
		//Rename the File, if it exists
		snprintf(filename, WINUI_ARRAY_LENGTH(filename), "%s\\%s.ini", GetIniDir(), old_name);
		snprintf(new_filename, WINUI_ARRAY_LENGTH(new_filename), "%s\\%s.ini", GetIniDir(), new_name);
		winui_move_file_utf8(filename, new_filename);
		//Rename the Directory, if it exists
		snprintf(filename, WINUI_ARRAY_LENGTH(filename), "%s\\%s", GetIniDir(), old_name);
		snprintf(new_filename, WINUI_ARRAY_LENGTH(new_filename), "%s\\%s", GetIniDir(), new_name);
		winui_move_file_utf8(filename, new_filename);
	}

	return true;
}

bool TryRenameCustomFolder(LPTREEFOLDER lpFolder, const char *new_name)
{
	char filename[MAX_PATH];
	char new_filename[MAX_PATH];

	if (lpFolder->m_nParent >= 0)
	{
		// a child extra folder was renamed, so do the rename and save the parent
		// save old title
		char *old_title = lpFolder->m_lpTitle;
		// set new title
		lpFolder->m_lpTitle = (char *)malloc(strlen(new_name) + 1);
		strcpy(lpFolder->m_lpTitle, new_name);

		// try to save
		if (TrySaveExtraFolder(lpFolder) == false)
		{
			// failed, so free newly allocated title and restore old
			free(lpFolder->m_lpTitle);
			lpFolder->m_lpTitle = old_title;
			return false;
		}
		
		TryRenameCustomFolderIni(lpFolder, old_title, new_name);
		// successful, so free old title
		free(old_title);
		return true;
	}

	// a parent extra folder was renamed, so rename the file
	snprintf(new_filename, WINUI_ARRAY_LENGTH(new_filename), "%s\\%s.ini", GetFolderDir(), new_name);
	snprintf(filename, WINUI_ARRAY_LENGTH(filename), "%s\\%s.ini", GetFolderDir(), lpFolder->m_lpTitle);
	bool retval = winui_move_file_utf8(filename, new_filename);

	if (retval)
	{
		TryRenameCustomFolderIni(lpFolder, lpFolder->m_lpTitle, new_name);
		free(lpFolder->m_lpTitle);
		lpFolder->m_lpTitle = (char *)malloc(strlen(new_name) + 1);
		strcpy(lpFolder->m_lpTitle, new_name);
	}
	else
		ErrorMessageBox("Error while renaming custom file %s to %s", filename, new_filename);

	return retval;
}

void AddToCustomFolder(LPTREEFOLDER lpFolder, int driver_index)
{
	if ((lpFolder->m_dwFlags & F_CUSTOM) == 0)
	{
		ErrorMessageBox("Unable to add game to non-custom folder");
		return;
	}

	if (TestBit(lpFolder->m_lpGameBits, driver_index) == 0)
	{
		AddGame(lpFolder, driver_index);

		if (TrySaveExtraFolder(lpFolder) == false)
			RemoveGame(lpFolder, driver_index); 	// undo on error
	}
}

void RemoveFromCustomFolder(LPTREEFOLDER lpFolder, int driver_index)
{
	if ((lpFolder->m_dwFlags & F_CUSTOM) == 0)
	{
		ErrorMessageBox("Unable to remove game from non-custom folder");
		return;
	}

	if (TestBit(lpFolder->m_lpGameBits, driver_index) != 0)
	{
		RemoveGame(lpFolder, driver_index);

		if (TrySaveExtraFolder(lpFolder) == false)
			AddGame(lpFolder, driver_index); // undo on error
	}
}

bool TrySaveExtraFolder(LPTREEFOLDER lpFolder)
{
	char fname[MAX_PATH];
	bool error = false;
	LPTREEFOLDER root_folder = NULL;
	LPEXFOLDERDATA extra_folder = NULL;

	for (int i = 0; i < numExtraFolders; i++)
	{
		if (ExtraFolderData[i]->m_nFolderId == lpFolder->m_nFolderId)
		{
			root_folder = lpFolder;
			extra_folder = ExtraFolderData[i];
			break;
		}

		if (lpFolder->m_nParent >= 0 && ExtraFolderData[i]->m_nFolderId == treeFolders[lpFolder->m_nParent]->m_nFolderId)
		{
			root_folder = treeFolders[lpFolder->m_nParent];
			extra_folder = ExtraFolderData[i];
			break;
		}
	}

	if (extra_folder == NULL || root_folder == NULL)
	{
		ErrorMessageBox("Error finding custom file name to save");
		return false;
	}

	snprintf(fname, WINUI_ARRAY_LENGTH(fname), "%s\\%s.ini", GetFolderDir(), extra_folder->m_szTitle);
	wchar_t *temp = win_wstring_from_utf8(GetFolderDir());
	CreateDirectory(temp, NULL);
	free(temp);  	
	FILE *f = fopen(fname, "w");

	if (f == NULL)
		error = true;
	else
	{
		TREEFOLDER *folder_data;

		fprintf(f, "[FOLDER_SETTINGS]\n");
		
		// negative values for icons means it's custom, so save 'em
		if (extra_folder->m_nIconId < 0)
			fprintf(f, "RootFolderIcon %s\n", ExtraFolderIcons[(-extra_folder->m_nIconId) - ICON_MAX]);

		if (extra_folder->m_nSubIconId < 0)
			fprintf(f, "SubFolderIcon %s\n", ExtraFolderIcons[(-extra_folder->m_nSubIconId) - ICON_MAX]);

		/* need to loop over all our TREEFOLDERs--first the root one, then each child.
		   start with the root */
		folder_data = root_folder;
		fprintf(f, "\n[ROOT_FOLDER]\n");

		for (int i = 0; i < driver_list::total(); i++)
		{
			if (TestBit(folder_data->m_lpGameBits, i))
				fprintf(f, "%s\n", GetDriverGameName(i));
		}

		/* look through the custom folders for ones with our root as parent */
		for (int j = 0; j < numFolders; j++)
		{
			folder_data = treeFolders[j];

			if (folder_data->m_nParent >= 0 && treeFolders[folder_data->m_nParent] == root_folder)
			{
				fprintf(f, "\n[%s]\n", folder_data->m_lpTitle);

				for (int i = 0; i < driver_list::total(); i++)
				{
					if (TestBit(folder_data->m_lpGameBits, i))
						fprintf(f, "%s\n", GetDriverGameName(i));
				}
			}
		}

		fclose(f);
	}

	if (error)
		ErrorMessageBox("Error while saving custom file %s", fname);

	return !error;
}

HIMAGELIST GetTreeViewIconList(void)
{
	return hTreeSmall;
}

int GetTreeViewIconIndex(int icon_id)
{
	if (icon_id < 0)
		return -icon_id;

	for (int i = 0; i < WINUI_ARRAY_LENGTH(treeIconNames); i++)
	{
		if (icon_id == treeIconNames[i].nResourceID)
			return i;
	}

	return -1;
}

static bool FilterAvailable(int driver_index)
{
	if (!DriverUsesRoms(driver_index))
	{
		if (GetDisplayNoRomsGames())
			return true;
		else
			return false;
	}

	return IsAuditResultYes(GetRomAuditResults(driver_index));
}


//#ifdef USE_GAMEFOLDERS
void CreateMSLUGFolders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();

	LPTREEFOLDER lpFolder = treeFolders[parent_index];
    LPTREEFOLDER lpMsLugCZ, lpMsLugDS, lpMsLugDC, lpMsLugF2, lpMsLugOR, lpMsLugHC, lpMsLugSD, lpMsLugRM, lpMsLugRE;

	// create our two subfolders
    lpMsLugCZ = NewFolder("Cuztom", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLugDS = NewFolder("Darksoft", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLugDC = NewFolder("Decrypted C", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLugF2 = NewFolder("Fightcade 2", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLugOR = NewFolder("Original", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLugHC = NewFolder("Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLugSD = NewFolder("NeoSD", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLugRM = NewFolder("Remix", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLugRE = NewFolder("Remix Extreme", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));

	AddFolder(lpMsLugCZ);
	AddFolder(lpMsLugDS);
	AddFolder(lpMsLugDC);
	AddFolder(lpMsLugF2);
	AddFolder(lpMsLugOR);
	AddFolder(lpMsLugHC);
	AddFolder(lpMsLugSD);		
	AddFolder(lpMsLugRM);
	AddFolder(lpMsLugRE);

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const char *s = GetDriverGameName(jj);

		if (s == NULL || s[0] == '\0')
			continue;

        // Cuztom
		if (!strcmp("mslugctma01", s))			    AddGame(lpMsLugCZ, jj);
		if (!strcmp("mslugctma02", s))			    AddGame(lpMsLugCZ, jj);
		if (!strcmp("mslugctma03", s))			    AddGame(lpMsLugCZ, jj);
		if (!strcmp("mslugctma04", s))			    AddGame(lpMsLugCZ, jj);
		if (!strcmp("mslugctma05", s))			    AddGame(lpMsLugCZ, jj);
		if (!strcmp("mslugctma06", s))			    AddGame(lpMsLugCZ, jj);
		if (!strcmp("mslugctma07", s))			    AddGame(lpMsLugCZ, jj);
		if (!strcmp("mslugctma08", s))			    AddGame(lpMsLugCZ, jj);
		if (!strcmp("mslugctma09", s))			    AddGame(lpMsLugCZ, jj);

        // Darksoft
		if (!strcmp("mslugdd", s))			        AddGame(lpMsLugDS, jj);

        // Decrypted C
		if (!strcmp("msboot", s))			        AddGame(lpMsLugDC, jj);
		if (!strcmp("msluge", s))	                AddGame(lpMsLugDC, jj);

        // Fightcade 2
		if (!strcmp("mslugdyf1", s))			    AddGame(lpMsLugF2, jj);

        // Original
		if (!strcmp("mslug", s))			        AddGame(lpMsLugOR, jj);

        // Hack
		if (!strcmp("mslughc01", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc02", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc03", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc04", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc05", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc06", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc07", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc08", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc09", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc10", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc11", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc12", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc13", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc14", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc15", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc16", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc17", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc18", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc19", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc20", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc21", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc22", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc23", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc24", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc25", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc26", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc27", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc28", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc29", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc30", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc31", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc32", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc33", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc34", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc35", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc36", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc37", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc38", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc39", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc40", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc41", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc42", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc43", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc44", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc45", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc46", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc47", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc48", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc49", s))		        AddGame(lpMsLugHC, jj);
		if (!strcmp("mslughc50", s))		        AddGame(lpMsLugHC, jj);

        // NeoSD
		if (!strcmp("mslugnsd", s))			        AddGame(lpMsLugSD, jj);

        // Remix
		if (!strcmp("mslugla01", s))			    AddGame(lpMsLugRM, jj);
		if (!strcmp("msluglb01", s))			    AddGame(lpMsLugRM, jj);

        // Remix Extreme
		if (!strcmp("mslugrma01", s))			    AddGame(lpMsLugRE, jj);
		if (!strcmp("mslugrmb01", s))			    AddGame(lpMsLugRE, jj);
	}
}

void CreateMSLUG2Folders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();

	LPTREEFOLDER lpFolder = treeFolders[parent_index];
    LPTREEFOLDER lpMsLug2AP, lpMsLug2CZ, lpMsLug2DS, lpMsLug2DSH, lpMsLug2DC, lpMsLug2F2, lpMsLug2OR, lpMslug2HC, lpMsLug2SD, lpMsLug2SDH, lpMsLug2RM, lpMsLug2RE;

	// create our two subfolders
    lpMsLug2AP = NewFolder("Apocalyptic Time", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug2CZ = NewFolder("Cuztom", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug2DS = NewFolder("Darksoft", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug2DSH = NewFolder("Darksoft ~ Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug2DC = NewFolder("Decrypted C", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug2F2 = NewFolder("Fightcade 2", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug2OR = NewFolder("Original", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMslug2HC = NewFolder("Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug2SD = NewFolder("NeoSD", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug2SDH = NewFolder("NeoSD ~ Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug2RM = NewFolder("Remix", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug2RE = NewFolder("Remix Extreme", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));

	AddFolder(lpMsLug2AP);
	AddFolder(lpMsLug2CZ);
	AddFolder(lpMsLug2DS);
	AddFolder(lpMsLug2DSH);
	AddFolder(lpMsLug2DC);
	AddFolder(lpMsLug2F2);
	AddFolder(lpMsLug2OR);
	AddFolder(lpMslug2HC);
	AddFolder(lpMsLug2SD);
    AddFolder(lpMsLug2SDH);	
	AddFolder(lpMsLug2RM);
	AddFolder(lpMsLug2RE);

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const char *s = GetDriverGameName(jj);

		if (s == NULL || s[0] == '\0')
			continue;

        // Apocalyptic Time
		if (!strcmp("mslug2at01", s))			    AddGame(lpMsLug2AP, jj);
	
        // Cuztom
		if (!strcmp("mslug2ctma01", s))			    AddGame(lpMsLug2CZ, jj);
		if (!strcmp("mslug2ctma02", s))			    AddGame(lpMsLug2CZ, jj);
		if (!strcmp("mslug2ctma03", s))			    AddGame(lpMsLug2CZ, jj);
		if (!strcmp("mslug2ctma04", s))			    AddGame(lpMsLug2CZ, jj);
		if (!strcmp("mslug2ctma05", s))			    AddGame(lpMsLug2CZ, jj);
		if (!strcmp("mslug2ctma06", s))			    AddGame(lpMsLug2CZ, jj);
		if (!strcmp("mslug2ctma07", s))			    AddGame(lpMsLug2CZ, jj);
		if (!strcmp("mslug2ctma08", s))			    AddGame(lpMsLug2CZ, jj);
		if (!strcmp("mslug2ctma09", s))		        AddGame(lpMsLug2CZ, jj);
		if (!strcmp("mslug2ctma10", s))			    AddGame(lpMsLug2CZ, jj);
		if (!strcmp("mslug2ctma11", s))			    AddGame(lpMsLug2CZ, jj);
		if (!strcmp("mslug2ctma12", s))			    AddGame(lpMsLug2CZ, jj);
		if (!strcmp("mslug2ctma13", s))			    AddGame(lpMsLug2CZ, jj);
		if (!strcmp("mslug2ctma14", s))			    AddGame(lpMsLug2CZ, jj);

        // Darksoft
        if (!strcmp("mslug2dd", s))			        AddGame(lpMsLug2DS, jj);
		if (!strcmp("mslug2tdd", s))			    AddGame(lpMsLug2DS, jj);
   
        // Darksoft ~ Hack
	    if (!strcmp("mslug2dddd", s))		     	AddGame(lpMsLug2DSH, jj);
		if (!strcmp("mslug2egdd", s))			    AddGame(lpMsLug2DSH, jj);
		if (!strcmp("mslug2frdd", s))			    AddGame(lpMsLug2DSH, jj);
		
		// Decrypted C
		if (!strcmp("mslug2t", s))		     	    AddGame(lpMsLug2DC, jj);

        // Fightcade 2
		if (!strcmp("mslug2fm", s))			        AddGame(lpMsLug2F2, jj);

        // Original
		if (!strcmp("mslug2", s))			        AddGame(lpMsLug2OR, jj);

        // Hack
		if (!strcmp("mslug2hc01", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc02", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc03", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc04", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc05", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc06", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc07", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc08", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc09", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc10", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc11", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc12", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc13", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc14", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc15", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc16", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc17", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc18", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc19", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc20", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc21", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc22", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc23", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc24", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc25", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc26", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc27", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc28", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc29", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc30", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc31", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc32", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc33", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc34", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc35", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc36", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc37", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc38", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc39", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc40", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc41", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc42", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc43", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc44", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc45", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc46", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc47", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc48", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc49", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc50", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc51", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc52", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc53", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc54", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc55", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc56", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc57", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc58", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc59", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc60", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc61", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc62", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc63", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc64", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc65", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc66", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc67", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc68", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc69", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc70", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc71", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc72", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc73", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc74", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc75", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc76", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc77", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc78", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc79", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc80", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc81", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc82", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc83", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc84", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc85", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc86", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc87", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc88", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc89", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc90", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc91", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc92", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc93", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc94", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc95", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc96", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc97", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc98", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc99", s))		        AddGame(lpMslug2HC, jj);
		if (!strcmp("mslug2hc100", s))		        AddGame(lpMslug2HC, jj);

        // NeoSD
		if (!strcmp("mslug2nsd", s))			    AddGame(lpMsLug2SD, jj);
		if (!strcmp("mslug2tnsd", s))			    AddGame(lpMsLug2SD, jj);

        // NeoSD ~ Hack
		if (!strcmp("mslug2ddnds", s))			    AddGame(lpMsLug2SDH, jj);
		if (!strcmp("mslug2egnds", s))		 	    AddGame(lpMsLug2SDH, jj);
		if (!strcmp("mslug2frnds", s))			    AddGame(lpMsLug2SDH, jj);

        // Remix
		if (!strcmp("mslug2la01", s))	            AddGame(lpMsLug2RM, jj);
		if (!strcmp("mslug2la02", s))		        AddGame(lpMsLug2RM, jj);
		if (!strcmp("mslug2la03", s))			    AddGame(lpMsLug2RM, jj);
		if (!strcmp("mslug2la04", s))	            AddGame(lpMsLug2RM, jj);
		if (!strcmp("mslug2la05", s))			    AddGame(lpMsLug2RM, jj);
		if (!strcmp("mslug2la06", s))			    AddGame(lpMsLug2RM, jj);
		if (!strcmp("mslug2la07", s))		        AddGame(lpMsLug2RM, jj);
		if (!strcmp("mslug2la08", s))		        AddGame(lpMsLug2RM, jj);
		if (!strcmp("mslug2lb01", s))			    AddGame(lpMsLug2RM, jj);
		if (!strcmp("mslug2lb02", s))			    AddGame(lpMsLug2RM, jj);
		if (!strcmp("mslug2lb03", s))			    AddGame(lpMsLug2RM, jj);
		if (!strcmp("mslug2lb04", s))	            AddGame(lpMsLug2RM, jj);
		if (!strcmp("mslug2lb05", s))			    AddGame(lpMsLug2RM, jj);
		if (!strcmp("mslug2lb06", s))			    AddGame(lpMsLug2RM, jj);
		if (!strcmp("mslug2lb07", s))	            AddGame(lpMsLug2RM, jj);
		if (!strcmp("mslug2lb08", s))			    AddGame(lpMsLug2RM, jj);

        // Remix Extreme
		if (!strcmp("mslug2rma01", s))		        AddGame(lpMsLug2RE, jj);
		if (!strcmp("mslug2rma02", s))		        AddGame(lpMsLug2RE, jj);
		if (!strcmp("mslug2rmb01", s))			    AddGame(lpMsLug2RE, jj);
		if (!strcmp("mslug2rmb02", s))	            AddGame(lpMsLug2RE, jj);
	}
}

void CreateMSLUG3Folders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();


	LPTREEFOLDER lpFolder = treeFolders[parent_index];
    LPTREEFOLDER lpMsLug3CZ, lpMsLug3DS, lpMsLug3DSH, lpMsLug3DC, lpMsLug3EH, lpMsLug3OR, lpMslug3HC, lpMslug3HB, lpMsLug3SD, lpMsLug3SDH, lpMsLug3RM, lpMsLug3RE;

	// create our two subfolders
    lpMsLug3CZ = NewFolder("Cuztom", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug3DS = NewFolder("Darksoft", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug3DSH = NewFolder("Darksoft ~ Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug3DC = NewFolder("Decrypted C", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug3EH = NewFolder("Encrypted ~ Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug3OR = NewFolder("Original", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMslug3HC = NewFolder("Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMslug3HB = NewFolder("HomeBrew", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug3SD = NewFolder("NeoSD", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug3SDH = NewFolder("NeoSD ~ Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug3RM = NewFolder("Remix", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug3RE = NewFolder("Remix Extreme", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));

	AddFolder(lpMsLug3CZ);
	AddFolder(lpMsLug3DS);
	AddFolder(lpMsLug3DSH);
	AddFolder(lpMsLug3DC);
	AddFolder(lpMsLug3EH);
	AddFolder(lpMsLug3OR);
	AddFolder(lpMslug3HC);
	AddFolder(lpMslug3HB);
	AddFolder(lpMsLug3SD);
    AddFolder(lpMsLug3SDH);	
	AddFolder(lpMsLug3RM);
	AddFolder(lpMsLug3RE);

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const char *s = GetDriverGameName(jj);

		if (s == NULL || s[0] == '\0')
			continue;

        // Cuztom
		if (!strcmp("mslug3ctma01", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma02", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma03", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma04", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma05", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma06", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma07", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma08", s))		        AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma09", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma10", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma11", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma12", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma13", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma14", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma15", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma16", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma17", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma18", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctma19", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb01", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb02", s))		        AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb03", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb04", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb05", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb06", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb07", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb08", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb09", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb10", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb11", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb12", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb13", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb14", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb15", s))		        AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb16", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb17", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb18", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmb19", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc01", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc02", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc03", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc04", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc05", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc06", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc07", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc08", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc09", s))		        AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc10", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc11", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc12", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc13", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc14", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc15", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc16", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc17", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc18", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmc19", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmd01", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmd02", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmd03", s))		        AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmd04", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmd05", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmd06", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmd07", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmd08", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmd09", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmd10", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmd11", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmd12", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmd13", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmd14", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmd15", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmd16", s))		        AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctme01", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctme02", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctme03", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctme04", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctme05", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctme06", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctme07", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctme08", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctme09", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctme10", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctme11", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctme12", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctme13", s))		        AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctme14", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctme15", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctme16", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmf01", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmf02", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmf03", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmf04", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmf05", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmf06", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmf07", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmf08", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmf09", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmf10", s))		        AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmf11", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmf12", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmf13", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmf14", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmf15", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmf16", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmg01", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmg02", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmg03", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmg04", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmg05", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmg06", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmg07", s))		        AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmg08", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmg09", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmg10", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmg11", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmg12", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmg13", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmg14", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmg15", s))             AddGame(lpMsLug3CZ, jj);
		if (!strcmp("mslug3ctmg16", s))             AddGame(lpMsLug3CZ, jj);

        // Darksoft
		if (!strcmp("mslug3dd", s))		            AddGame(lpMsLug3DS, jj);
		if (!strcmp("mslug3hdd", s))			    AddGame(lpMsLug3DS, jj);
		if (!strcmp("mslug3ndd", s))			    AddGame(lpMsLug3DS, jj);
		if (!strcmp("mslug3b6dd", s))			    AddGame(lpMsLug3DS, jj);
		
        // Darksoft ~ Hack
		if (!strcmp("mslug3cqtdd", s))		        AddGame(lpMsLug3DSH, jj);
		if (!strcmp("mslug3chuanqidd", s))		    AddGame(lpMsLug3DSH, jj);
		if (!strcmp("mslug3cqdd", s))			    AddGame(lpMsLug3DSH, jj);
		if (!strcmp("mslug3cqtfbdd", s))	        AddGame(lpMsLug3DSH, jj);
		if (!strcmp("mslug3cqztdd", s))			    AddGame(lpMsLug3DSH, jj);
		if (!strcmp("mslug3dddd", s))			    AddGame(lpMsLug3DSH, jj);
		if (!strcmp("mslug3frdd", s))		        AddGame(lpMsLug3DSH, jj);
		if (!strcmp("mslug3gdd", s))			    AddGame(lpMsLug3DSH, jj);
		if (!strcmp("mslug3kidd", s))			    AddGame(lpMsLug3DSH, jj);
		if (!strcmp("mslug3nddd", s))			    AddGame(lpMsLug3DSH, jj);
		if (!strcmp("mslug3scdd", s))			    AddGame(lpMsLug3DSH, jj);
		
		// Decrypted C
		if (!strcmp("ms3b6boot", s))	    		AddGame(lpMsLug3DC, jj);
		if (!strcmp("mslug3d", s))			        AddGame(lpMsLug3DC, jj);
		if (!strcmp("mslug3hd", s))			        AddGame(lpMsLug3DC, jj);
		if (!strcmp("mslug3nd", s))		            AddGame(lpMsLug3DC, jj);
		if (!strcmp("mslug3he", s))			        AddGame(lpMsLug3DC, jj);
		if (!strcmp("mslug3b6d", s))			    AddGame(lpMsLug3DC, jj);
		if (!strcmp("mslug3b6e", s))			    AddGame(lpMsLug3DC, jj);

        // Encrypted ~ Hack
		if (!strcmp("mslug31v2", s))			    AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3burt", s))			    AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3eb", s))		            AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3eb14", s))			    AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3sd", s))	                AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3sc", s))			        AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3cq", s))			        AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3chuanqi", s))		    AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3cqtfb", s))			    AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3cqzt", s))			    AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3dbplus", s))			    AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3c", s))			        AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3fr", s))	                AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3g", s))			        AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3gw", s))			        AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3i", s))			        AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3dde", s))			    AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3ki", s))		            AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3lw", s))			        AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3se", s))			        AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3unity", s))			    AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3v", s))			        AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3zh", s))			        AddGame(lpMsLug3EH, jj);
		if (!strcmp("mslug3xe", s))			        AddGame(lpMsLug3EH, jj);

        // Original
		if (!strcmp("mslug3", s))			        AddGame(lpMsLug3OR, jj);
		if (!strcmp("mslug3a", s))			        AddGame(lpMsLug3OR, jj);
		if (!strcmp("mslug3h", s))			        AddGame(lpMsLug3OR, jj);
		if (!strcmp("mslug3b6", s))		            AddGame(lpMsLug3OR, jj);

        // Hack
		if (!strcmp("mslug3hc01", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc02", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc03", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc04", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc05", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc06", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc07", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc08", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc09", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc10", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc11", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc12", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc13", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc14", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc15", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc16", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc17", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc18", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc19", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc20", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc21", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc22", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc23", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc24", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc25", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc26", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc27", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc28", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc29", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc30", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc31", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc32", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc33", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc34", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc35", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc36", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc37", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc38", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc39", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc40", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc41", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc42", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc43", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc44", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc45", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc46", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc47", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc48", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc49", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc50", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc51", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc52", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc53", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc54", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc55", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc56", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc57", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc58", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc59", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc60", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc61", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc62", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc63", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc64", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc65", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc66", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc67", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc68", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc69", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc70", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc71", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc72", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc73", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc74", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc75", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc76", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc77", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc78", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc79", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc80", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc81", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc82", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc83", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc84", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc85", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc86", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc87", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc88", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc89", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc90", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc91", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc92", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc93", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc94", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc95", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc96", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc97", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc98", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc99", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3hc100", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3esl", s))		        AddGame(lpMslug3HC, jj);
		if (!strcmp("mslug3or", s))		            AddGame(lpMslug3HC, jj);

        // HomeBrew
		if (!strcmp("mslug3x", s))		            AddGame(lpMslug3HB, jj);
		if (!strcmp("mslug3xb", s))		            AddGame(lpMslug3HB, jj);
		if (!strcmp("mslug3x0", s))		            AddGame(lpMslug3HB, jj);
		if (!strcmp("mslug3x1", s))		            AddGame(lpMslug3HB, jj);
		if (!strcmp("mslug3x2", s))		            AddGame(lpMslug3HB, jj);
		if (!strcmp("mslug3x3", s))		            AddGame(lpMslug3HB, jj);
		if (!strcmp("neopang", s))		            AddGame(lpMslug3HB, jj);

        // NeoSD
		if (!strcmp("mslug3nsd", s))		        AddGame(lpMsLug3SD, jj);
		if (!strcmp("mslug3hnsd", s))			    AddGame(lpMsLug3SD, jj);
		if (!strcmp("mslug3ndnsd", s))			    AddGame(lpMsLug3SD, jj);
		if (!strcmp("mslug3b6nsd", s))			    AddGame(lpMsLug3SD, jj);

        // NeoSD ~ Hack
		if (!strcmp("mslug3cqtnds", s))		        AddGame(lpMsLug3SDH, jj);
		if (!strcmp("mslug3chuanqinds", s))		    AddGame(lpMsLug3SDH, jj);
		if (!strcmp("mslug3cqnds", s))			    AddGame(lpMsLug3SDH, jj);
		if (!strcmp("mslug3cqtfbnds", s))	        AddGame(lpMsLug3SDH, jj);
		if (!strcmp("mslug3cqztnds", s))		    AddGame(lpMsLug3SDH, jj);
		if (!strcmp("mslug3ddnds", s))			    AddGame(lpMsLug3SDH, jj);
		if (!strcmp("mslug3frnds", s))		        AddGame(lpMsLug3SDH, jj);
		if (!strcmp("mslug3gnds", s))			    AddGame(lpMsLug3SDH, jj);
		if (!strcmp("mslug3kinds", s))			    AddGame(lpMsLug3SDH, jj);
		if (!strcmp("mslug3ndnds", s))			    AddGame(lpMsLug3SDH, jj);
		if (!strcmp("mslug3scnds", s))			    AddGame(lpMsLug3SDH, jj);

        // Remix
		if (!strcmp("mslug3la01", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3la02", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3la03", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3la04", s))		        AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3la05", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3la06", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lb01", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lb02", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lb03", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lb04", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lb05", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lb06", s))		        AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lc01", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lc02", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lc03", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lc04", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lc05", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lc06", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3ld01", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3ld02", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3ld03", s))		        AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3ld04", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3ld05", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3ld06", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3le01", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3le02", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3le03", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3le04", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3le05", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3le06", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lf01", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lf02", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lf03", s))		        AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lf04", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lf05", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lf06", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lg01", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lg02", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lg03", s))		        AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lg04", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lg05", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lg06", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lh01", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lh02", s))		        AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lh03", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lh04", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lh05", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3lh06", s))	            AddGame(lpMsLug3RM, jj);
		if (!strcmp("mslug3li01", s))	            AddGame(lpMsLug3RM, jj);

        // Remix Extreme
		if (!strcmp("mslug3rma01", s))			    AddGame(lpMsLug3RE, jj);
		if (!strcmp("mslug3rmb01", s))			    AddGame(lpMsLug3RE, jj);
		if (!strcmp("mslug3rmc01", s))		        AddGame(lpMsLug3RE, jj);
		if (!strcmp("mslug3rmd01", s))			    AddGame(lpMsLug3RE, jj);
		if (!strcmp("mslug3rme01", s))			    AddGame(lpMsLug3RE, jj);
		if (!strcmp("mslug3rmf01", s))			    AddGame(lpMsLug3RE, jj);
		if (!strcmp("mslug3rmg01", s))			    AddGame(lpMsLug3RE, jj);
		if (!strcmp("mslug3rmh01", s))	            AddGame(lpMsLug3RE, jj);
		if (!strcmp("mslug3rmi01", s))			    AddGame(lpMsLug3RE, jj);
	}
}

void CreateMSLUG4Folders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();


	LPTREEFOLDER lpFolder = treeFolders[parent_index];
    LPTREEFOLDER lpMsLug4CZ, lpMsLug4DS, lpMsLug4DSH, lpMsLug4DC, lpMsLug4EH, lpMsLug4OR, lpMslug4HC, lpMsLug4SD, lpMsLug4SDH, lpMsLug4RM, lpMsLug4RE;

	// create our two subfolders
    lpMsLug4CZ = NewFolder("Cuztom", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug4DS = NewFolder("Darksoft", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug4DSH = NewFolder("Darksoft ~ Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug4DC = NewFolder("Decrypted C", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug4EH = NewFolder("Encrypted ~ Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug4OR = NewFolder("Original", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMslug4HC = NewFolder("Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug4SD = NewFolder("NeoSD", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug4SDH = NewFolder("NeoSD ~ Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug4RM = NewFolder("Remix", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug4RE = NewFolder("Remix Extreme", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));

	AddFolder(lpMsLug4CZ);
	AddFolder(lpMsLug4DS);
	AddFolder(lpMsLug4DSH);
	AddFolder(lpMsLug4DC);
	AddFolder(lpMsLug4EH);
	AddFolder(lpMsLug4OR);
	AddFolder(lpMslug4HC);
	AddFolder(lpMsLug4SD);
    AddFolder(lpMsLug4SDH);	
	AddFolder(lpMsLug4RM);
	AddFolder(lpMsLug4RE);

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const char *s = GetDriverGameName(jj);

		if (s == NULL || s[0] == '\0')
			continue;

        // Cuztom
		if (!strcmp("mslug4ctma01", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctma02", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctma03", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctma04", s))		        AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctma05", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctma06", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctma07", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctma08", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctma09", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctma10", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctma11", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctma12", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctma13", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmb03", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmb04", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmb05", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmb06", s))		        AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmb07", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmb08", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmb09", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmb10", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmb11", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmb12", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmb13", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmc03", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmc04", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmc05", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmc06", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmc07", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmc08", s))		        AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmc09", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmc10", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmc11", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmc12", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmc13", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmd03", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmd04", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmd05", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmd06", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmd07", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmd08", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmd09", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmd10", s))		        AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmd11", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmd12", s))             AddGame(lpMsLug4CZ, jj);
		if (!strcmp("mslug4ctmd13", s))             AddGame(lpMsLug4CZ, jj);

        // Darksoft
		if (!strcmp("ms4plusdd", s))			    AddGame(lpMsLug4DS, jj);
		if (!strcmp("mslug4dd", s))			        AddGame(lpMsLug4DS, jj);
		if (!strcmp("mslug4hdd", s))			    AddGame(lpMsLug4DS, jj);
		if (!strcmp("mslug4ndd", s))			    AddGame(lpMsLug4DS, jj);

        // Darksoft ~ Hack
		if (!strcmp("mslug4ammordd", s))	        AddGame(lpMsLug4DSH, jj);
		if (!strcmp("mslug4cqtdd", s))			    AddGame(lpMsLug4DSH, jj);
		if (!strcmp("mslug4cqidd", s))			    AddGame(lpMsLug4DSH, jj);
		if (!strcmp("mslug4dddd", s))			    AddGame(lpMsLug4DSH, jj);
		if (!strcmp("mslug4frdd", s))			    AddGame(lpMsLug4DSH, jj);
		if (!strcmp("mslug4kidd", s))			    AddGame(lpMsLug4DSH, jj);
		if (!strcmp("mslug4lwdd", s))		        AddGame(lpMsLug4DSH, jj);
		if (!strcmp("mslug4lwqdd", s))			    AddGame(lpMsLug4DSH, jj);

		// Decrypted C
		if (!strcmp("ms4boot", s))			        AddGame(lpMsLug4DC, jj);
		if (!strcmp("ms4plusa", s))	                AddGame(lpMsLug4DC, jj);
		if (!strcmp("ms4plusd", s))			        AddGame(lpMsLug4DC, jj);
		if (!strcmp("mslug4d", s))			        AddGame(lpMsLug4DC, jj);
		if (!strcmp("mslug4hd", s))			        AddGame(lpMsLug4DC, jj);
		if (!strcmp("mslug4nd", s))	                AddGame(lpMsLug4DC, jj);
		if (!strcmp("mslug4e", s))			        AddGame(lpMsLug4DC, jj);

        // Encrypted ~ Hack

		if (!strcmp("mslug4fr", s))			        AddGame(lpMsLug4EH, jj);
		if (!strcmp("mslug4lw", s))	                AddGame(lpMsLug4EH, jj);
		if (!strcmp("mslug4ammor", s))			    AddGame(lpMsLug4EH, jj);
		if (!strcmp("mslug4lwq", s))		        AddGame(lpMsLug4EH, jj);
		if (!strcmp("mslug4dde", s))			    AddGame(lpMsLug4EH, jj);
		if (!strcmp("mslug4ki", s))			        AddGame(lpMsLug4EH, jj);
		if (!strcmp("mslug4cqi", s))			    AddGame(lpMsLug4EH, jj);

        // Original
		if (!strcmp("ms4plus", s))			        AddGame(lpMsLug4OR, jj);
		if (!strcmp("mslug4", s))			        AddGame(lpMsLug4OR, jj);
		if (!strcmp("mslug4h", s))			        AddGame(lpMsLug4OR, jj);

        // Hack
		if (!strcmp("mslug4hc01", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc02", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc03", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc04", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc05", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc06", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc07", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc08", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc09", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc10", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc11", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc12", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc13", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc14", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc15", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc16", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc17", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc18", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc19", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc20", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc21", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc22", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc23", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc24", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc25", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc26", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc27", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc28", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc29", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc30", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc31", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc32", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc33", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc34", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc35", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc36", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc37", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc38", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc39", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc40", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc41", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc42", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc43", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc44", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc45", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc46", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc47", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc48", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc49", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc50", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc51", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc52", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc53", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc54", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc55", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc56", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc57", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc58", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc59", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc60", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc61", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc62", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc63", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc64", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc65", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc66", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc67", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc68", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc69", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc70", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc71", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc72", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc73", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc74", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc75", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc76", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc77", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc78", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc79", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc80", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc81", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc82", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc83", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc84", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc85", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc86", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc87", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc88", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc89", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc90", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc91", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc92", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc93", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc94", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc95", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc96", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc97", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc98", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc99", s))		        AddGame(lpMslug4HC, jj);
		if (!strcmp("mslug4hc100", s))		        AddGame(lpMslug4HC, jj);

        // NeoSD
		if (!strcmp("ms4plusnsd", s))			    AddGame(lpMsLug4SD, jj);
		if (!strcmp("mslug4nsd", s))			    AddGame(lpMsLug4SD, jj);
		if (!strcmp("mslug4hnsd", s))			    AddGame(lpMsLug4SD, jj);
		if (!strcmp("mslug4ndnsd", s))			    AddGame(lpMsLug4SD, jj);

        // NeoSD ~ Hack
		if (!strcmp("mslug4ammornds", s))	        AddGame(lpMsLug4SDH, jj);
		if (!strcmp("mslug4cqtnds", s))	            AddGame(lpMsLug4SDH, jj);
		if (!strcmp("mslug4cqinds", s))			    AddGame(lpMsLug4SDH, jj);
		if (!strcmp("mslug4ddnds", s))			    AddGame(lpMsLug4SDH, jj);
		if (!strcmp("mslug4frnds", s))			    AddGame(lpMsLug4SDH, jj);
		if (!strcmp("mslug4kinds", s))			    AddGame(lpMsLug4SDH, jj);
		if (!strcmp("mslug4lwnds", s))		        AddGame(lpMsLug4SDH, jj);
		if (!strcmp("mslug4lwqnds", s))			    AddGame(lpMsLug4SDH, jj);

        // Remix
		if (!strcmp("mslug4la01", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4la02", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4la03", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4la04", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4la05", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lb01", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lb02", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lb03", s))		        AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lb04", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lb05", s))	            AddGame(lpMsLug4RM, jj);
        if (!strcmp("mslug4lc01", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lc02", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lc03", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lc04", s))		        AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lc05", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4ld01", s))	            AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4ld02", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4ld03", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4ld04", s))		        AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4ld05", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4le01", s))	            AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4le02", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4le03", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4le04", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4le05", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lf01", s))		        AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lf02", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lf03", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lf04", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lf05", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lg01", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lg02", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lg03", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lg04", s))               AddGame(lpMsLug4RM, jj);
		if (!strcmp("mslug4lg05", s))	            AddGame(lpMsLug4RM, jj);

        // Remix Extreme
		if (!strcmp("mslug4rma01", s))			    AddGame(lpMsLug4RE, jj);
		if (!strcmp("mslug4rmb01", s))	            AddGame(lpMsLug4RE, jj);
		if (!strcmp("mslug4rmc01", s))			    AddGame(lpMsLug4RE, jj);
		if (!strcmp("mslug4rmd01", s))			    AddGame(lpMsLug4RE, jj);
		if (!strcmp("mslug4rme01", s))			    AddGame(lpMsLug4RE, jj);
		if (!strcmp("mslug4rmf01", s))			    AddGame(lpMsLug4RE, jj);
		if (!strcmp("mslug4rmg01", s))		        AddGame(lpMsLug4RE, jj);
		if (!strcmp("mslug4rmh01", s))			    AddGame(lpMsLug4RE, jj);
	}
}

void CreateMSLUG5Folders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();


	LPTREEFOLDER lpFolder = treeFolders[parent_index];
    LPTREEFOLDER lpMsLug5CZ, lpMsLug5DS, lpMsLug5DSH, lpMsLug5DC, lpMsLug5EH, lpMslug5F2, lpMsLug5OR, lpMslug5HC, lpMsLug5SD, lpMsLug5SDH, lpMsLug5RM, lpMsLug5RE;

	// create our two subfolders
    lpMsLug5CZ = NewFolder("Cuztom", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug5DS = NewFolder("Darksoft", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug5DSH = NewFolder("Darksoft ~ Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug5DC = NewFolder("Decrypted C", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug5EH = NewFolder("Encrypted ~ Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMslug5F2 = NewFolder("Fightcade 2", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug5OR = NewFolder("Original", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMslug5HC = NewFolder("Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug5SD = NewFolder("NeoSD", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug5SDH = NewFolder("NeoSD ~ Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLug5RM = NewFolder("Remix", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLug5RE = NewFolder("Remix Extreme", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));

	AddFolder(lpMsLug5CZ);
	AddFolder(lpMsLug5DS);
	AddFolder(lpMsLug5DSH);
	AddFolder(lpMsLug5DC);
	AddFolder(lpMsLug5EH);
	AddFolder(lpMslug5F2);
	AddFolder(lpMsLug5OR);
	AddFolder(lpMslug5HC);
	AddFolder(lpMsLug5SD);
    AddFolder(lpMsLug5SDH);	
	AddFolder(lpMsLug5RM);
	AddFolder(lpMsLug5RE);

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const char *s = GetDriverGameName(jj);

		if (s == NULL || s[0] == '\0')
			continue;

        // Cuztom
		if (!strcmp("mslug5ctma01", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctma02", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctma03", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctma04", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctma05", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctma06", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctma07", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctma08", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctma09", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctma10", s))		        AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctma11", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctma12", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctma13", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctma14", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctma15", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctma16", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctma17", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmb01", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmb02", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmb03", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmb04", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmb05", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmb06", s))		        AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmb07", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmb08", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmb09", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmb10", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmb11", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmb12", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmb13", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmb14", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmb15", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmb16", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmc01", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmc02", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmc03", s))		        AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmc04", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmc05", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmc06", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmc07", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmc08", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmc09", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmc10", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmc11", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmc12", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmc13", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmc14", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmc15", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmc16", s))		        AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd01", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd02", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd03", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd04", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd05", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd06", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd07", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd08", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd09", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd10", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd11", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd12", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd13", s))		        AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd14", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd15", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd16", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmd17", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme01", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme02", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme03", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme04", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme05", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme06", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme07", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme08", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme09", s))		        AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme10", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme11", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme12", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme13", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme14", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme15", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme16", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctme17", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf01", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf02", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf03", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf04", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf05", s))		        AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf06", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf07", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf08", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf09", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf10", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf11", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf12", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf13", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf14", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf15", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf16", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmf17", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg01", s))		        AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg02", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg03", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg04", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg05", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg06", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg07", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg08", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg09", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg10", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg11", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg12", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg13", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg14", s))		        AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg15", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg16", s))             AddGame(lpMsLug5CZ, jj);
		if (!strcmp("mslug5ctmg17", s))             AddGame(lpMsLug5CZ, jj);

        // Darksoft
		if (!strcmp("ms5pcbdd", s))			        AddGame(lpMsLug5DS, jj);
		if (!strcmp("ms5plusdd", s))			    AddGame(lpMsLug5DS, jj);
		if (!strcmp("mslug5dd", s))			        AddGame(lpMsLug5DS, jj);
		if (!strcmp("mslug5hdd", s))			    AddGame(lpMsLug5DS, jj);
		if (!strcmp("mslug5ndd", s))			    AddGame(lpMsLug5DS, jj);

        // Darksoft ~ Hack
		if (!strcmp("mslug5cqtdd", s))		    	AddGame(lpMsLug5DSH, jj);
		if (!strcmp("mslug5cqdd", s))			    AddGame(lpMsLug5DSH, jj);
		if (!strcmp("mslug5fdd", s))			    AddGame(lpMsLug5DSH, jj);
		if (!strcmp("mslug5frdd", s))			    AddGame(lpMsLug5DSH, jj);
		if (!strcmp("mslug5scdd", s))			    AddGame(lpMsLug5DSH, jj);
		if (!strcmp("mslug5wdd", s))			    AddGame(lpMsLug5DSH, jj);

		// Decrypted C
		if (!strcmp("ms5boot", s))		            AddGame(lpMsLug5DC, jj);
		if (!strcmp("ms5pcbd", s))			        AddGame(lpMsLug5DC, jj);
		if (!strcmp("ms5plusd", s))			        AddGame(lpMsLug5DC, jj);
		if (!strcmp("ms5plusnd", s))		        AddGame(lpMsLug5DC, jj);
		if (!strcmp("ms5pluse", s))			        AddGame(lpMsLug5DC, jj);
		if (!strcmp("mslug5d", s))			        AddGame(lpMsLug5DC, jj);
		if (!strcmp("mslug5hd", s))			        AddGame(lpMsLug5DC, jj);
		if (!strcmp("mslug5e", s))			        AddGame(lpMsLug5DC, jj);
		if (!strcmp("mslug5n", s))			        AddGame(lpMsLug5DC, jj);
		if (!strcmp("mslug5nd", s))			        AddGame(lpMsLug5DC, jj);
		if (!strcmp("mslug5nde", s))		        AddGame(lpMsLug5DC, jj);
		if (!strcmp("mslug5b2", s))			        AddGame(lpMsLug5DC, jj);
		if (!strcmp("mslug5b3", s))			        AddGame(lpMsLug5DC, jj);
		if (!strcmp("mslug5b4", s))			        AddGame(lpMsLug5DC, jj);
		if (!strcmp("mslug5b5", s))	                AddGame(lpMsLug5DC, jj);
		if (!strcmp("mslug5b6", s))			        AddGame(lpMsLug5DC, jj);
		if (!strcmp("mslug5b7", s))			        AddGame(lpMsLug5DC, jj);
		if (!strcmp("mslug5b8", s))			        AddGame(lpMsLug5DC, jj);

        // Encrypted ~ Hack
		if (!strcmp("ms5plush", s))			        AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug51v2", s))			    AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5ae", s))			        AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5bs", s))			        AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5burt", s))		        AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5ddd", s))			    AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5db", s))	                AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5c", s))			        AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5esr", s))			    AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5ex", s))		            AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5f", s))			        AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5g", s))			        AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5w", s))			        AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5unity", s))			    AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5sc", s))	                AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5sg", s))			        AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5cq", s))			        AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5ki", s))			        AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5l9a", s))			    AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5l9b", s))			    AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5mg", s))			        AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5rmk", s))			    AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5x", s))		            AddGame(lpMsLug5EH, jj);
		if (!strcmp("mslug5zh", s))			        AddGame(lpMsLug5EH, jj);

        // Original
		if (!strcmp("ms5pcb", s))			        AddGame(lpMsLug5OR, jj);
		if (!strcmp("ms5plus", s))			        AddGame(lpMsLug5OR, jj);
		if (!strcmp("mslug5", s))			        AddGame(lpMsLug5OR, jj);
		if (!strcmp("mslug5h", s))			        AddGame(lpMsLug5OR, jj);
		if (!strcmp("mslug5b1", s))			        AddGame(lpMsLug5OR, jj);

        // Hack
		if (!strcmp("mslug5hc01", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc02", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc03", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc04", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc05", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc06", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc07", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc08", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc09", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc10", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc11", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc12", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc13", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc14", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc15", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc16", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc17", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc18", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc19", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc20", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc21", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc22", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc23", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc24", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc25", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc26", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc27", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc28", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc29", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc30", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc31", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc32", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc33", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc34", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc35", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc36", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc37", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc38", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc39", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc40", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc41", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc42", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc43", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc44", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc45", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc46", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc47", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc48", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc49", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc50", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc51", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc52", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc53", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc54", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc55", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc56", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc57", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc58", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc59", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc60", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc61", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc62", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc63", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc64", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc65", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc66", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc67", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc68", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc69", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc70", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc71", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc72", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc73", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc74", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc75", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc76", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc77", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc78", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc79", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc80", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc81", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc82", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc83", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc84", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc85", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc86", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc87", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc88", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc89", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc90", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc91", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc92", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc93", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc94", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc95", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc96", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc97", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc98", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc99", s))		        AddGame(lpMslug5HC, jj);
		if (!strcmp("mslug5hc100", s))		        AddGame(lpMslug5HC, jj);

        // Fightcade 2
		if (!strcmp("mslug5sgf", s))			    AddGame(lpMslug5F2, jj);

        // NeoSD
		if (!strcmp("ms5pcbdnsd", s))			    AddGame(lpMsLug5SD, jj);
		if (!strcmp("ms5plusnsd", s))			    AddGame(lpMsLug5SD, jj);
		if (!strcmp("mslug5nsd", s))			    AddGame(lpMsLug5SD, jj);
		if (!strcmp("mslug5hnsd", s))			    AddGame(lpMsLug5SD, jj);
		if (!strcmp("mslug5ndnsd", s))			    AddGame(lpMsLug5SD, jj);

        // NeoSD ~ Hack
		if (!strcmp("mslug5cqtnds", s))			    AddGame(lpMsLug5SDH, jj);
		if (!strcmp("mslug5cqnds", s))			    AddGame(lpMsLug5SDH, jj);
		if (!strcmp("mslug5fnds", s))			    AddGame(lpMsLug5SDH, jj);
		if (!strcmp("mslug5frnds", s))			    AddGame(lpMsLug5SDH, jj);
		if (!strcmp("mslug5scnds", s))			    AddGame(lpMsLug5SDH, jj);
		if (!strcmp("mslug5wnds", s))			    AddGame(lpMsLug5SDH, jj);

        // Remix
		if (!strcmp("mslug5la01", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5la02", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5la03", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5lb01", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5lb02", s))		        AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5lb03", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5lc01", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5lc02", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5lc03", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5ld01", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5ld02", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5ld03", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5le01", s))		        AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5le02", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5le03", s))	            AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5lf01", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5lf02", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5lf03", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5lg01", s))	            AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5lg02", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5lg03", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5lh01", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5lh02", s))               AddGame(lpMsLug5RM, jj);
		if (!strcmp("mslug5lh03", s))		        AddGame(lpMsLug5RM, jj);

        // Remix Extreme
		if (!strcmp("mslug5rma01", s))			    AddGame(lpMsLug5RE, jj);
		if (!strcmp("mslug5rmb01", s))			    AddGame(lpMsLug5RE, jj);
		if (!strcmp("mslug5rmc01", s))			    AddGame(lpMsLug5RE, jj);
		if (!strcmp("mslug5rmd01", s))			    AddGame(lpMsLug5RE, jj);
		if (!strcmp("mslug5rme01", s))			    AddGame(lpMsLug5RE, jj);
		if (!strcmp("mslug5rmf01", s))			    AddGame(lpMsLug5RE, jj);
		if (!strcmp("mslug5rmg01", s))		        AddGame(lpMsLug5RE, jj);
		if (!strcmp("mslug5rmh01", s))			    AddGame(lpMsLug5RE, jj);
	}
}

void CreateMSLUGXFolders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();


	LPTREEFOLDER lpFolder = treeFolders[parent_index];
    LPTREEFOLDER lpMsLugXAP, lpMsLugXCZ, lpMsLugXDS, lpMsLugXDSH, lpMsLugXDC, lpMslugXF2, lpMsLugXOR, lpMslugXHC, lpMsLugXSD, lpMsLugXSDH, lpMsLugXRM, lpMsLugXRE;

	// create our two subfolders
    lpMsLugXAP = NewFolder("Apocalyptic Time", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLugXCZ = NewFolder("Cuztom", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLugXDS = NewFolder("Darksoft", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLugXDSH = NewFolder("Darksoft ~ Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLugXDC = NewFolder("Decrypted C", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMslugXF2 = NewFolder("Fightcade 2", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLugXOR = NewFolder("Original", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMslugXHC = NewFolder("Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLugXSD = NewFolder("NeoSD", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLugXSDH = NewFolder("NeoSD ~ Hack", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
	lpMsLugXRM = NewFolder("Remix", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpMsLugXRE = NewFolder("Remix Extreme", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));

	AddFolder(lpMsLugXAP);
	AddFolder(lpMsLugXCZ);
	AddFolder(lpMsLugXDS);
	AddFolder(lpMsLugXDSH);
	AddFolder(lpMsLugXDC);
	AddFolder(lpMslugXF2);
	AddFolder(lpMsLugXOR);
	AddFolder(lpMslugXHC);
	AddFolder(lpMsLugXSD);
    AddFolder(lpMsLugXSDH);	
	AddFolder(lpMsLugXRM);
	AddFolder(lpMsLugXRE);

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	for (jj = 0; jj < nGames; jj++)
	{
		const char *s = GetDriverGameName(jj);

		if (s == NULL || s[0] == '\0')
			continue;

        // Apocalyptic Time
		if (!strcmp("mslugxtst01", s))			    AddGame(lpMsLugXAP, jj);
		if (!strcmp("mslugxtao01", s))			    AddGame(lpMsLugXAP, jj);
		if (!strcmp("mslugxat01", s))			    AddGame(lpMsLugXAP, jj);
		if (!strcmp("mslugxat02", s))			    AddGame(lpMsLugXAP, jj);
		if (!strcmp("mslugxat03", s))			    AddGame(lpMsLugXAP, jj);
		if (!strcmp("mslugxat04", s))			    AddGame(lpMsLugXAP, jj);
		if (!strcmp("mslugxat05", s))			    AddGame(lpMsLugXAP, jj);
		if (!strcmp("mslugxat06", s))		        AddGame(lpMsLugXAP, jj);
		if (!strcmp("mslugxat07", s))			    AddGame(lpMsLugXAP, jj);
		if (!strcmp("mslugxat08", s))			    AddGame(lpMsLugXAP, jj);
		if (!strcmp("mslugxat09", s))			    AddGame(lpMsLugXAP, jj);
		if (!strcmp("mslugxat10", s))			    AddGame(lpMsLugXAP, jj);

        // Cuztom
		if (!strcmp("mslugxctma01", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctma02", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctma03", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctma04", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctma05", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctma06", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctma07", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctma08", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctma09", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctma10", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctma11", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctma12", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctma13", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctma14", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctma15", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctma16", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmb01", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmb02", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmb03", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmb04", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmb05", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmb06", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmb07", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmb08", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmb09", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmb10", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmb11", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmb12", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmb13", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmb14", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmb15", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmb16", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmc01", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmc02", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmc03", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmc04", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmc05", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmc06", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmc07", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmc08", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmc09", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmc10", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmc11", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmc12", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmc13", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmc14", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmc15", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmc16", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmd01", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmd02", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmd03", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmd04", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmd05", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmd06", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmd07", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmd08", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmd09", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmd10", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmd11", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmd12", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmd13", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmd14", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmd15", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmd16", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctme01", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctme02", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctme03", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctme04", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctme05", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctme06", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctme07", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctme08", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctme09", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctme10", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctme11", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctme12", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctme13", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctme14", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctme15", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctme16", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmf01", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmf02", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmf03", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmf04", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmf05", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmf06", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmf07", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmf08", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmf09", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmf10", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmf11", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmf12", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmf13", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmf14", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmf15", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmf16", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmg01", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmg02", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmg03", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmg04", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmg05", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmg06", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmg07", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmg08", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmg09", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmg10", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmg11", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmg12", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmg13", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmg14", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmg15", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmg16", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmh01", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmh02", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmh03", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmh04", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmh05", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmh06", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmh07", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmh08", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmh09", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmh10", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmh11", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmh12", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmh13", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmh14", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmh15", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmh16", s))             AddGame(lpMsLugXCZ, jj);
		if (!strcmp("mslugxctmi01", s))             AddGame(lpMsLugXCZ, jj);

        // Darksoft
		if (!strcmp("mslugxdd", s))		            AddGame(lpMsLugXDS, jj);

        // Darksoft ~ Hack
		if (!strcmp("mslugxcqtdd", s))	            AddGame(lpMsLugXDSH, jj);
		if (!strcmp("mslugxchuanqidd", s))	        AddGame(lpMsLugXDSH, jj);
		if (!strcmp("mslugxcqdd", s))			    AddGame(lpMsLugXDSH, jj);
		if (!strcmp("mslugxdddd", s))		        AddGame(lpMsLugXDSH, jj);
		if (!strcmp("mslugxfrdd", s))			    AddGame(lpMsLugXDSH, jj);
		if (!strcmp("mslugxlbdd", s))			    AddGame(lpMsLugXDSH, jj);
		if (!strcmp("mslugxscdd", s))			    AddGame(lpMsLugXDSH, jj);

		// Decrypted C
		if (!strcmp("mslugxb", s))			        AddGame(lpMsLugXDC, jj);

        // Original
		if (!strcmp("mslugx", s))			        AddGame(lpMsLugXOR, jj);

        // Hack
		if (!strcmp("mslugxhc01", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc02", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc03", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc04", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc05", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc06", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc07", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc08", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc09", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc10", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc11", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc12", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc13", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc14", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc15", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc16", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc17", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc18", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc19", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc20", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc21", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc22", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc23", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc24", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc25", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc26", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc27", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc28", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc29", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc30", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc31", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc32", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc33", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc34", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc35", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc36", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc37", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc38", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc39", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc40", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc41", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc42", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc43", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc44", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc45", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc46", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc47", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc48", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc49", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc50", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc51", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc52", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc53", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc54", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc55", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc56", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc57", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc58", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc59", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc60", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc61", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc62", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc63", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc64", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc65", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc66", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc67", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc68", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc69", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc70", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc71", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc72", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc73", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc74", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc75", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc76", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc77", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc78", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc79", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc80", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc81", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc82", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc83", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc84", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc85", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc86", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc87", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc88", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc89", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc90", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc91", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc92", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc93", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc94", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc95", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc96", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc97", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc98", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc99", s))		        AddGame(lpMslugXHC, jj);
		if (!strcmp("mslugxhc100", s))		        AddGame(lpMslugXHC, jj);

        // Fightcade 2
		if (!strcmp("mslugxsrf", s))		        AddGame(lpMslugXF2, jj);

        // NeoSD
		if (!strcmp("mslugxnsd", s))		        AddGame(lpMsLugXSD, jj);
	
        // NeoSD ~ Hack
		if (!strcmp("mslugxcqtnds", s))	            AddGame(lpMsLugXSDH, jj);
		if (!strcmp("mslugxchuanqinds", s))	        AddGame(lpMsLugXSDH, jj);
		if (!strcmp("mslugxcqnds", s))			    AddGame(lpMsLugXSDH, jj);
		if (!strcmp("mslugxddnds", s))		        AddGame(lpMsLugXSDH, jj);
		if (!strcmp("mslugxfrnds", s))			    AddGame(lpMsLugXSDH, jj);
		if (!strcmp("mslugxlbnds", s))			    AddGame(lpMsLugXSDH, jj);
		if (!strcmp("mslugxscnds", s))			    AddGame(lpMsLugXSDH, jj);

        // Remix
		if (!strcmp("mslugxla01", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxla02", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxla03", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxlb01", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxlb02", s))		        AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxlb03", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxlc01", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxlc02", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxlc03", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxld01", s))	            AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxld02", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxld03", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxle01", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxle02", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxle03", s))		        AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxlf01", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxlf02", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxlf03", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxlg01", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxlg02", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxlg03", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxlh01", s))	            AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxlh02", s))			    AddGame(lpMsLugXRM, jj);
		if (!strcmp("mslugxlh03", s))		        AddGame(lpMsLugXRM, jj);

        // Remix Extreme
		if (!strcmp("mslugxrma01", s))			    AddGame(lpMsLugXRE, jj);
		if (!strcmp("mslugxrmb01", s))			    AddGame(lpMsLugXRE, jj);
		if (!strcmp("mslugxrmc01", s))			    AddGame(lpMsLugXRE, jj);
		if (!strcmp("mslugxrmd01", s))			    AddGame(lpMsLugXRE, jj);
		if (!strcmp("mslugxrme01", s))			    AddGame(lpMsLugXRE, jj);
		if (!strcmp("mslugxrmf01", s))	            AddGame(lpMsLugXRE, jj);
		if (!strcmp("mslugxrmg01", s))			    AddGame(lpMsLugXRE, jj);
		if (!strcmp("mslugxrmh01", s))		        AddGame(lpMsLugXRE, jj);
	}
}

void CreateCOLLECTIONFolders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();


	LPTREEFOLDER lpFolder = treeFolders[parent_index];
    LPTREEFOLDER lpSerieEM, lpSerieER, lpSerieFP, lpSerieGB, lpSerieLD, lpSerieMF, lpSerieRI, lpSerieSD, lpSerieSR;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	// create our two subfolders
    lpSerieEM = NewFolder("Easy Mode", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerieER = NewFolder("Enemy Reset", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerieFP = NewFolder("FirePower", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerieGB = NewFolder("Green Blue", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerieLD = NewFolder("Legendary", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerieMF = NewFolder("Multifunction", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerieRI = NewFolder("Random Item", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerieSD = NewFolder("Speedrun", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerieSR = NewFolder("Survival", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));

	AddFolder(lpSerieEM);
	AddFolder(lpSerieER);
	AddFolder(lpSerieFP);
	AddFolder(lpSerieGB);
	AddFolder(lpSerieLD);
	AddFolder(lpSerieMF);	
	AddFolder(lpSerieRI);	
	AddFolder(lpSerieSD);
	AddFolder(lpSerieSR);

	for (jj = 0; jj < nGames; jj++)
	{
		const char *s = GetDriverGameName(jj);

		if (s == NULL || s[0] == '\0')
			continue;

        // Serie Easy Mode
		if (!strcmp("mslughc11", s))			        AddGame(lpSerieEM, jj);
		if (!strcmp("mslug2hc19", s))			        AddGame(lpSerieEM, jj);
		if (!strcmp("mslug3hc33", s))			        AddGame(lpSerieEM, jj);
		if (!strcmp("mslug4hc22", s))		            AddGame(lpSerieEM, jj);
		if (!strcmp("mslug5hc25", s))			        AddGame(lpSerieEM, jj);
		if (!strcmp("mslugxhc25", s))	                AddGame(lpSerieEM, jj);

        // Serie Enemy Reset
		if (!strcmp("mslug2hc01", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslug3hc14", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslug3hc15", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslug3hc17", s))		            AddGame(lpSerieER, jj);
		if (!strcmp("mslug3hc18", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslug3hc20", s))	                AddGame(lpSerieER, jj);
		if (!strcmp("mslug3hc36", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslug3hc37", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslug4hc03", s))		            AddGame(lpSerieER, jj);
		if (!strcmp("mslug4hc07", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslug4hc25", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslug5hc03", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslug5hc04", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslug5hc05", s))	                AddGame(lpSerieER, jj);
		if (!strcmp("mslug5hc07", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslug5hc10", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslug5hc13", s))	                AddGame(lpSerieER, jj);
		if (!strcmp("mslug5hc14", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslug5hc23", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslug5hc27", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslughc03", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslugxhc01", s))		            AddGame(lpSerieER, jj);
		if (!strcmp("mslugxhc02", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslugxhc03", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslugxhc07", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslugxhc15", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslugxhc19", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslugxhc20", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslugxhc33", s))			        AddGame(lpSerieER, jj);
		if (!strcmp("mslugxhc34", s))		            AddGame(lpSerieER, jj);

        // Serie Firepower
		if (!strcmp("mslughc16", s))			        AddGame(lpSerieFP, jj);
		if (!strcmp("mslug3hc11", s))			        AddGame(lpSerieFP, jj);
		if (!strcmp("mslug3hc37", s))			        AddGame(lpSerieFP, jj);
		if (!strcmp("mslug4hc25", s))		            AddGame(lpSerieFP, jj);
		if (!strcmp("mslug5hc13", s))			        AddGame(lpSerieFP, jj);
		if (!strcmp("mslugxhc10", s))	                AddGame(lpSerieFP, jj);
		if (!strcmp("mslugxhc11", s))	                AddGame(lpSerieFP, jj);
		if (!strcmp("mslugxhc12", s))	                AddGame(lpSerieFP, jj);
		if (!strcmp("mslugxhc34", s))	                AddGame(lpSerieFP, jj);

        // Serie Green Blue
		if (!strcmp("mslug2hc05", s))			        AddGame(lpSerieGB, jj);
		if (!strcmp("mslug3hc06", s))			        AddGame(lpSerieGB, jj);
		if (!strcmp("mslug3hc07", s))			        AddGame(lpSerieGB, jj);
		if (!strcmp("mslugxhc05", s))		            AddGame(lpSerieGB, jj);
		if (!strcmp("mslugxhc06", s))			        AddGame(lpSerieGB, jj);
		if (!strcmp("mslugxhc22", s))	                AddGame(lpSerieGB, jj);

        // Serie Legendary
		if (!strcmp("mslug2hc10", s))			        AddGame(lpSerieLD, jj);
		if (!strcmp("mslug3hc35", s))			        AddGame(lpSerieLD, jj);
		if (!strcmp("mslug3hc36", s))			        AddGame(lpSerieLD, jj);
		if (!strcmp("mslug3hc37", s))		            AddGame(lpSerieLD, jj);
		if (!strcmp("mslug3hc38", s))			        AddGame(lpSerieLD, jj);
		if (!strcmp("mslug3hc39", s))	                AddGame(lpSerieLD, jj);
		if (!strcmp("mslug4hc25", s))	                AddGame(lpSerieLD, jj);
		if (!strcmp("mslug5hc12", s))	                AddGame(lpSerieLD, jj);
		if (!strcmp("mslug5hc13", s))	                AddGame(lpSerieLD, jj);
		if (!strcmp("mslugxhc32", s))	                AddGame(lpSerieLD, jj);
		if (!strcmp("mslugxhc33", s))	                AddGame(lpSerieLD, jj);
		if (!strcmp("mslugxhc34", s))	                AddGame(lpSerieLD, jj);

        // Serie Multifunction
		if (!strcmp("mslughc04", s))			        AddGame(lpSerieMF, jj);
		if (!strcmp("mslug2hc09", s))			        AddGame(lpSerieMF, jj);
		if (!strcmp("mslug3hc03", s))			        AddGame(lpSerieMF, jj);
		if (!strcmp("mslug4hc12", s))		            AddGame(lpSerieMF, jj);
		if (!strcmp("mslug5hc17", s))			        AddGame(lpSerieMF, jj);
		if (!strcmp("mslugxhc13", s))	                AddGame(lpSerieMF, jj);

        // Serie Random Item
		if (!strcmp("mslug2hc10", s))			        AddGame(lpSerieRI, jj);
		if (!strcmp("mslug3hc03", s))			        AddGame(lpSerieRI, jj);
		if (!strcmp("mslug3hc35", s))			        AddGame(lpSerieRI, jj);
		if (!strcmp("mslug3hc36", s))		            AddGame(lpSerieRI, jj);
		if (!strcmp("mslug3hc37", s))			        AddGame(lpSerieRI, jj);
		if (!strcmp("mslug4hc08", s))	                AddGame(lpSerieRI, jj);
		if (!strcmp("mslug4hc09", s))	                AddGame(lpSerieRI, jj);
		if (!strcmp("mslug4hc25", s))	                AddGame(lpSerieRI, jj);
		if (!strcmp("mslug5hc07", s))	                AddGame(lpSerieRI, jj);
		if (!strcmp("mslug5hc12", s))	                AddGame(lpSerieRI, jj);
		if (!strcmp("mslug5hc13", s))	                AddGame(lpSerieRI, jj);
		if (!strcmp("mslughc02", s))	                AddGame(lpSerieRI, jj);
		if (!strcmp("mslughc15", s))	                AddGame(lpSerieRI, jj);
		if (!strcmp("mslugxhc13", s))	                AddGame(lpSerieRI, jj);
		if (!strcmp("mslugxhc26", s))	                AddGame(lpSerieRI, jj);
		if (!strcmp("mslugxhc32", s))	                AddGame(lpSerieRI, jj);
		if (!strcmp("mslugxhc34", s))	                AddGame(lpSerieRI, jj);

        // Serie Speedrun
		if (!strcmp("mslughc07", s))			        AddGame(lpSerieSD, jj);
		if (!strcmp("mslug2hc03", s))			        AddGame(lpSerieSD, jj);
		if (!strcmp("mslug3hc24", s))			        AddGame(lpSerieSD, jj);
		if (!strcmp("mslug4hc11", s))		            AddGame(lpSerieSD, jj);
		if (!strcmp("mslug5hc18", s))			        AddGame(lpSerieSD, jj);
		if (!strcmp("mslugxhc04", s))	                AddGame(lpSerieSD, jj);

        // Serie Survival
		if (!strcmp("mslug3hc39", s))			        AddGame(lpSerieSR, jj);
		if (!strcmp("mslug3hc40", s))			        AddGame(lpSerieSR, jj);
	    if (!strcmp("mslug3esl", s))			        AddGame(lpSerieSR, jj);
	}
}

void CreateUPDATEFolders(int parent_index)
{
	int jj;
	int nGames = GetNumGames();

	LPTREEFOLDER lpFolder = treeFolders[parent_index];
    LPTREEFOLDER lpSerie26, lpSerie25, lpSerie24, lpSerie23, lpSerie22, lpSerie21, lpSerie20, lpSerie19, lpSerie18, lpSerie17, lpSerie16, lpSerie15, lpSerie14, lpSerie13, lpSerie12, lpSerie11, lpSerie10, lpSerie08, lpSerie07, lpSerie06, lpSerie05;

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,FALSE);

	// create our two subfolders
    lpSerie26 = NewFolder("2026", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie25 = NewFolder("2025", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie24 = NewFolder("2024", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie23 = NewFolder("2023", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie22 = NewFolder("2022", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie21 = NewFolder("2021", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie20 = NewFolder("2020", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie19 = NewFolder("2019", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie18 = NewFolder("2018", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie17 = NewFolder("2017", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie16 = NewFolder("2016", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie15 = NewFolder("2015", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie14 = NewFolder("2014", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie13 = NewFolder("2013", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie12 = NewFolder("2012", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie11 = NewFolder("2011", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie10 = NewFolder("2010", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie08 = NewFolder("2008", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie07 = NewFolder("2007", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie06 = NewFolder("2006", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
    lpSerie05 = NewFolder("2005", next_folder_id++, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));

	AddFolder(lpSerie26);
	AddFolder(lpSerie25);
	AddFolder(lpSerie24);
	AddFolder(lpSerie23);
	AddFolder(lpSerie22);
	AddFolder(lpSerie21);
	AddFolder(lpSerie20);
	AddFolder(lpSerie19);
	AddFolder(lpSerie18);
	AddFolder(lpSerie17);
	AddFolder(lpSerie16);
	AddFolder(lpSerie15);
	AddFolder(lpSerie14);
	AddFolder(lpSerie13);
	AddFolder(lpSerie12);
	AddFolder(lpSerie11);
	AddFolder(lpSerie10);
	AddFolder(lpSerie08);
	AddFolder(lpSerie07);
	AddFolder(lpSerie06);
	AddFolder(lpSerie05);

	for (jj = 0; jj < nGames; jj++)
	{
		const char *s = GetDriverGameName(jj);

		if (s == NULL || s[0] == '\0')
			continue;

        // 2026
		if (!strcmp("mslughc110", s))			        AddGame(lpSerie26, jj);
		if (!strcmp("mslughc110", s))			        AddGame(lpSerie26, jj);
		if (!strcmp("mslughc110", s))			        AddGame(lpSerie26, jj);
		if (!strcmp("mslughc110", s))		            AddGame(lpSerie26, jj);
		if (!strcmp("mslughc110", s))			        AddGame(lpSerie26, jj);
		if (!strcmp("mslughc110", s))	                AddGame(lpSerie26, jj);

        // 2025
		if (!strcmp("mslugdyf1", s))			        AddGame(lpSerie25, jj);
		if (!strcmp("mslughc03", s))			        AddGame(lpSerie25, jj);
		if (!strcmp("mslug3esl", s))			        AddGame(lpSerie25, jj);
		if (!strcmp("mslug3hc36", s))		            AddGame(lpSerie25, jj);
		if (!strcmp("mslug3hc37", s))			        AddGame(lpSerie25, jj);
		if (!strcmp("mslug3hc38", s))	                AddGame(lpSerie25, jj);
		if (!strcmp("mslug3hc39", s))			        AddGame(lpSerie25, jj);
		if (!strcmp("mslug4hc25", s))			        AddGame(lpSerie25, jj);
		if (!strcmp("mslug4hc26", s))		            AddGame(lpSerie25, jj);
		if (!strcmp("mslug5hc13", s))			        AddGame(lpSerie25, jj);
		if (!strcmp("mslug5hc14", s))			        AddGame(lpSerie25, jj);
		if (!strcmp("mslug5hc30", s))			        AddGame(lpSerie25, jj);
		if (!strcmp("mslugxhc31", s))			        AddGame(lpSerie25, jj);
		if (!strcmp("mslugxhc34", s))	                AddGame(lpSerie25, jj);
		if (!strcmp("mslugxhc35", s))			        AddGame(lpSerie25, jj);

        // 2024
		if (!strcmp("mslughc13", s))			        AddGame(lpSerie24, jj);
		if (!strcmp("mslughc14", s))			        AddGame(lpSerie24, jj);
		if (!strcmp("mslughc15", s))			        AddGame(lpSerie24, jj);
		if (!strcmp("mslug3or", s))		                AddGame(lpSerie24, jj);
		if (!strcmp("mslug3hc23", s))			        AddGame(lpSerie24, jj);
		if (!strcmp("mslug3hc40", s))	                AddGame(lpSerie24, jj);
		if (!strcmp("mslug5hc07", s))	                AddGame(lpSerie24, jj);
		if (!strcmp("mslug5hc08", s))	                AddGame(lpSerie24, jj);
		if (!strcmp("mslugxhc26", s))	                AddGame(lpSerie24, jj);
        if (!strcmp("mslugxhc33", s))	                AddGame(lpSerie24, jj);

        // 2023
		if (!strcmp("mslughc06", s))			        AddGame(lpSerie23, jj);
		if (!strcmp("mslughc12", s))			        AddGame(lpSerie23, jj);
		if (!strcmp("mslug3hc20", s))			        AddGame(lpSerie23, jj);
		if (!strcmp("mslug3hc28", s))		            AddGame(lpSerie23, jj);
		if (!strcmp("mslug3hc34", s))			        AddGame(lpSerie23, jj);
		if (!strcmp("mslug3hc35", s))	                AddGame(lpSerie23, jj);
		if (!strcmp("mslug4hc03", s))			        AddGame(lpSerie23, jj);
		if (!strcmp("mslug4hc04", s))		            AddGame(lpSerie23, jj);
		if (!strcmp("mslug4hc05", s))			        AddGame(lpSerie23, jj);
		if (!strcmp("mslug4hc23", s))	                AddGame(lpSerie23, jj);
		if (!strcmp("mslug5hc10", s))		            AddGame(lpSerie23, jj);
		if (!strcmp("mslug5hc12", s))			        AddGame(lpSerie23, jj);
		if (!strcmp("mslug5hc20", s))	                AddGame(lpSerie23, jj);
		if (!strcmp("mslugxhc03", s))		            AddGame(lpSerie23, jj);
		if (!strcmp("mslugxhc07", s))			        AddGame(lpSerie23, jj);
		if (!strcmp("mslugxhc13", s))	                AddGame(lpSerie23, jj);
		if (!strcmp("mslugxhc30", s))			        AddGame(lpSerie23, jj);
		if (!strcmp("mslugxhc32", s))		            AddGame(lpSerie23, jj);
		if (!strcmp("mslugxat10", s))			        AddGame(lpSerie23, jj);

        // 2022
		if (!strcmp("mslug3x", s))			            AddGame(lpSerie22, jj);
		if (!strcmp("mslughc05", s))			        AddGame(lpSerie22, jj);
		if (!strcmp("mslug2hc08", s))			        AddGame(lpSerie22, jj);
		if (!strcmp("mslug2hc14", s))		            AddGame(lpSerie22, jj);
		if (!strcmp("mslug3hc02", s))			        AddGame(lpSerie22, jj);
		if (!strcmp("mslug3hc03", s))	                AddGame(lpSerie22, jj);
		if (!strcmp("mslug3hc05", s))	                AddGame(lpSerie22, jj);
		if (!strcmp("mslug3hc26", s))	                AddGame(lpSerie22, jj);
		if (!strcmp("mslug3hc27", s))	                AddGame(lpSerie22, jj);
		if (!strcmp("mslug4hc02", s))	                AddGame(lpSerie22, jj);
		if (!strcmp("mslug4hc08", s))	                AddGame(lpSerie22, jj);
		if (!strcmp("mslug4hc09", s))	                AddGame(lpSerie22, jj);
		if (!strcmp("mslug4hc13", s))			        AddGame(lpSerie22, jj);
		if (!strcmp("mslug4hc14", s))			        AddGame(lpSerie22, jj);
		if (!strcmp("mslug5hc03", s))			        AddGame(lpSerie22, jj);
		if (!strcmp("mslug5hc11", s))		            AddGame(lpSerie22, jj);
		if (!strcmp("mslug5hc21", s))			        AddGame(lpSerie22, jj);
		if (!strcmp("mslug5hc22", s))	                AddGame(lpSerie22, jj);
		if (!strcmp("mslug5hc26", s))	                AddGame(lpSerie22, jj);
		if (!strcmp("mslug5hc27", s))	                AddGame(lpSerie22, jj);
		if (!strcmp("mslug5hc28", s))	                AddGame(lpSerie22, jj);
		if (!strcmp("mslugxhc14", s))	                AddGame(lpSerie22, jj);
		if (!strcmp("mslugxhc24", s))	                AddGame(lpSerie22, jj);

        // 2021
		if (!strcmp("mslughc02", s))			        AddGame(lpSerie21, jj);
		if (!strcmp("mslughc07", s))			        AddGame(lpSerie21, jj);
		if (!strcmp("mslug2hc03", s))			        AddGame(lpSerie21, jj);
		if (!strcmp("mslug2hc05", s))		            AddGame(lpSerie21, jj);
		if (!strcmp("mslug2hc06", s))			        AddGame(lpSerie21, jj);
		if (!strcmp("mslug2at01", s))	                AddGame(lpSerie21, jj);
		if (!strcmp("mslug3hc11", s))			        AddGame(lpSerie21, jj);
		if (!strcmp("mslug3hc24", s))			        AddGame(lpSerie21, jj);
		if (!strcmp("mslug4hc11", s))			        AddGame(lpSerie21, jj);
		if (!strcmp("mslug4hc17", s))		            AddGame(lpSerie21, jj);
		if (!strcmp("mslug5hc09", s))			        AddGame(lpSerie21, jj);
		if (!strcmp("mslug5hc18", s))	                AddGame(lpSerie21, jj);
		if (!strcmp("mslugxsrf", s))	                AddGame(lpSerie21, jj);
		if (!strcmp("mslugxhc04", s))	                AddGame(lpSerie21, jj);
		if (!strcmp("mslugxhc10", s))	                AddGame(lpSerie21, jj);
		if (!strcmp("mslugxhc12", s))	                AddGame(lpSerie21, jj);
		if (!strcmp("mslugxhc19", s))	                AddGame(lpSerie21, jj);
		if (!strcmp("mslugxhc20", s))	                AddGame(lpSerie21, jj);
		if (!strcmp("mslugxat08", s))	                AddGame(lpSerie21, jj);
		if (!strcmp("mslugxat09", s))	                AddGame(lpSerie21, jj);

        // 2020
		if (!strcmp("mslug3hc12", s))			        AddGame(lpSerie20, jj);
		if (!strcmp("mslug3hc14", s))			        AddGame(lpSerie20, jj);
		if (!strcmp("mslug4hc07", s))			        AddGame(lpSerie20, jj);
		if (!strcmp("mslug5hc02", s))		            AddGame(lpSerie20, jj);
		if (!strcmp("mslug5hc19", s))			        AddGame(lpSerie20, jj);
		if (!strcmp("mslugxhc11", s))	                AddGame(lpSerie20, jj);
		if (!strcmp("mslugxhc21", s))	                AddGame(lpSerie20, jj);
		if (!strcmp("mslugxhc22", s))	                AddGame(lpSerie20, jj);
		if (!strcmp("mslugxhc23", s))	                AddGame(lpSerie20, jj);
		if (!strcmp("mslugxtst01", s))	                AddGame(lpSerie20, jj);
		if (!strcmp("mslugxtao01", s))	                AddGame(lpSerie20, jj);
		if (!strcmp("mslugxat01", s))	                AddGame(lpSerie20, jj);
		if (!strcmp("mslugxat02", s))	                AddGame(lpSerie20, jj);
		if (!strcmp("mslugxat03", s))	                AddGame(lpSerie20, jj);
		if (!strcmp("mslugxat04", s))	                AddGame(lpSerie20, jj);
		if (!strcmp("mslugxat05", s))	                AddGame(lpSerie20, jj);
		if (!strcmp("mslugxat06", s))	                AddGame(lpSerie20, jj);
		if (!strcmp("mslugxat07", s))	                AddGame(lpSerie20, jj);

        // 2019
		if (!strcmp("mslug2hc02", s))			        AddGame(lpSerie19, jj);
		if (!strcmp("mslug3hc04", s))			        AddGame(lpSerie19, jj);
		if (!strcmp("mslug3hc17", s))			        AddGame(lpSerie19, jj);
		if (!strcmp("mslug3hc18", s))		            AddGame(lpSerie19, jj);
		if (!strcmp("mslug3hc22", s))			        AddGame(lpSerie19, jj);
		if (!strcmp("mslug3hc25", s))	                AddGame(lpSerie19, jj);
		if (!strcmp("mslug4hc06", s))			        AddGame(lpSerie19, jj);
		if (!strcmp("mslug4hc10", s))			        AddGame(lpSerie19, jj);
		if (!strcmp("mslug5hc15", s))			        AddGame(lpSerie19, jj);
		if (!strcmp("mslug5hc23", s))		            AddGame(lpSerie19, jj);
		if (!strcmp("mslug5hc29", s))			        AddGame(lpSerie19, jj);
		if (!strcmp("mslug5hc31", s))	                AddGame(lpSerie19, jj);
		if (!strcmp("mslugxhc17", s))	                AddGame(lpSerie19, jj);

        // 2018
		if (!strcmp("mslughc04", s))			        AddGame(lpSerie18, jj);
		if (!strcmp("mslughc11", s))			        AddGame(lpSerie18, jj);
		if (!strcmp("mslug2hc07", s))			        AddGame(lpSerie18, jj);
		if (!strcmp("mslug2hc10", s))		            AddGame(lpSerie18, jj);
		if (!strcmp("mslug2hc13", s))			        AddGame(lpSerie18, jj);
		if (!strcmp("mslug2hc19", s))	                AddGame(lpSerie18, jj);
		if (!strcmp("mslug3hc10", s))			        AddGame(lpSerie18, jj);
		if (!strcmp("mslug3hc33", s))			        AddGame(lpSerie18, jj);
		if (!strcmp("mslug4hc22", s))		            AddGame(lpSerie18, jj);
		if (!strcmp("mslug5hc16", s))			        AddGame(lpSerie18, jj);
		if (!strcmp("mslug5hc25", s))	                AddGame(lpSerie18, jj);
		if (!strcmp("mslugxhc08", s))	                AddGame(lpSerie18, jj);
		if (!strcmp("mslugxhc18", s))	                AddGame(lpSerie18, jj);
		if (!strcmp("mslugxhc25", s))	                AddGame(lpSerie18, jj);

        // 2017
		if (!strcmp("mslughc10", s))			        AddGame(lpSerie17, jj);
		if (!strcmp("mslug2hc09", s))			        AddGame(lpSerie17, jj);
		if (!strcmp("mslug3hc08", s))			        AddGame(lpSerie17, jj);
		if (!strcmp("mslug3hc09", s))		            AddGame(lpSerie17, jj);
		if (!strcmp("mslug3hc15", s))			        AddGame(lpSerie17, jj);
		if (!strcmp("mslug3hc19", s))	                AddGame(lpSerie17, jj);
		if (!strcmp("mslug3hc21", s))			        AddGame(lpSerie17, jj);
		if (!strcmp("mslug4hc12", s))			        AddGame(lpSerie17, jj);
		if (!strcmp("mslug5hc17", s))			        AddGame(lpSerie17, jj);

        // 2016
		if (!strcmp("mslughc01", s))			        AddGame(lpSerie16, jj);
		if (!strcmp("mslughc08", s))			        AddGame(lpSerie16, jj);
		if (!strcmp("mslughc09", s))			        AddGame(lpSerie16, jj);
		if (!strcmp("mslug2hc12", s))		            AddGame(lpSerie16, jj);
		if (!strcmp("mslug2hc15", s))			        AddGame(lpSerie16, jj);
		if (!strcmp("mslug3hc30", s))	                AddGame(lpSerie16, jj);
		if (!strcmp("mslug5hc01", s))			        AddGame(lpSerie16, jj);
		if (!strcmp("mslugxhc09", s))			        AddGame(lpSerie16, jj);

        // 2015
		if (!strcmp("mslug2t", s))			            AddGame(lpSerie15, jj);
		if (!strcmp("mslug2hc11", s))			        AddGame(lpSerie15, jj);
		if (!strcmp("mslug2hc16", s))			        AddGame(lpSerie15, jj);
		if (!strcmp("mslug4hc18", s))		            AddGame(lpSerie15, jj);
		if (!strcmp("mslug5l9a", s))			        AddGame(lpSerie15, jj);
		if (!strcmp("mslug5l9b", s))	                AddGame(lpSerie15, jj);
		if (!strcmp("mslug5hc04", s))			        AddGame(lpSerie15, jj);
		if (!strcmp("mslug5hc05", s))			        AddGame(lpSerie15, jj);
		if (!strcmp("mslug5hc06", s))			        AddGame(lpSerie15, jj);
		if (!strcmp("mslugxhc02", s))			        AddGame(lpSerie15, jj);

        // 2014
		if (!strcmp("mslug2hc01", s))	                AddGame(lpSerie14, jj);
		if (!strcmp("mslug3hc07", s))			        AddGame(lpSerie14, jj);
		if (!strcmp("mslugxhc06", s))			        AddGame(lpSerie14, jj);
		if (!strcmp("mslugxhc16", s))		            AddGame(lpSerie14, jj);

        // 2013
		if (!strcmp("mslughc16", s))	                AddGame(lpSerie13, jj);
		if (!strcmp("mslug3hc06", s))			        AddGame(lpSerie13, jj);
		if (!strcmp("mslug3hc13", s))			        AddGame(lpSerie13, jj);
		if (!strcmp("mslug4hc01", s))		            AddGame(lpSerie13, jj);
		if (!strcmp("mslug4hc19", s))	                AddGame(lpSerie13, jj);
		if (!strcmp("mslugxhc01", s))			        AddGame(lpSerie13, jj);
		if (!strcmp("mslugxhc05", s))			        AddGame(lpSerie13, jj);
		if (!strcmp("mslugxhc15", s))		            AddGame(lpSerie13, jj);

        // 2012
		if (!strcmp("mslug3hc01", s))	                AddGame(lpSerie12, jj);
		if (!strcmp("mslug3hc16", s))			        AddGame(lpSerie12, jj);
		if (!strcmp("mslug3hc29", s))			        AddGame(lpSerie12, jj);
		if (!strcmp("mslug3hc31", s))		            AddGame(lpSerie12, jj);
		if (!strcmp("mslug4hc16", s))	                AddGame(lpSerie12, jj);
		if (!strcmp("mslug4hc24", s))			        AddGame(lpSerie12, jj);
		if (!strcmp("mslugxhc27", s))			        AddGame(lpSerie12, jj);

        // 2011
		if (!strcmp("mslug2hc18", s))	                AddGame(lpSerie11, jj);

        // 2010
		if (!strcmp("neopang", s))	                    AddGame(lpSerie10, jj);

        // 2008
		if (!strcmp("mslug2hc04", s))	                AddGame(lpSerie08, jj);

        // 2007
		if (!strcmp("mslug4hc21", s))	                AddGame(lpSerie07, jj);

        // 2006
		if (!strcmp("mslug2hc17", s))	                AddGame(lpSerie06, jj);
		if (!strcmp("mslug3hc32", s))			        AddGame(lpSerie06, jj);
		if (!strcmp("mslug4hc20", s))			        AddGame(lpSerie06, jj);
		if (!strcmp("mslug5hc24", s))		            AddGame(lpSerie06, jj);
		if (!strcmp("mslugxhc28", s))	                AddGame(lpSerie06, jj);

        // 2005
		if (!strcmp("mslug4hc21", s))	                AddGame(lpSerie05, jj);
		if (!strcmp("mslugxhc29", s))			        AddGame(lpSerie05, jj);
	}
}

//#endif
