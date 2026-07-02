// WBQtMapFileBridge.cpp -- MFC side of the Qt Open/Save Map seam (Tier 3d). Plain MFC TU (no
// Qt include). Open Map: OpenMap's qt* member functions are defined here (member functions may
// be defined in any TU); the dialog is created HIDDEN so its enumeration/search/packed-.big
// logic runs verbatim, with a TOpenMapInfo sentinel distinguishing "completed" picks from
// packed-mode drills. Save Map: faithful free-function ports of the small populate/overwrite
// logic. Whole body guarded by RTS_HAS_QT so the OFF build compiles it to an empty object.
#include "StdAfx.h"
#include "resource.h"
#include "Lib/BaseType.h"
#include "WorldBuilder.h"
#include "OpenMap.h"
#include "SaveMap.h"
#include "Common/GlobalData.h"
#include "qt/panels/WBQtMapFileBridge.h"

#ifdef RTS_HAS_QT

static void copyOut(const char *str, char *buf, int cap)
{
	if (buf == NULL || cap <= 0)
	{
		return;
	}
	strncpy(buf, str ? str : "", cap - 1);
	buf[cap - 1] = 0;
}

extern "C" int WBQtMapFileData_RadiosVisible(void)
{
	// == the MFC pages hiding System/User outside debug/internal builds.
#if defined(_DEBUG) || defined(_INTERNAL)
	return 1;
#else
	return 0;
#endif
}

// ================= Open Map (hidden-dialog driven) =================

static OpenMap *s_qtOpenMap = NULL;
static TOpenMapInfo s_qtOpenInfo;

OpenMap *OpenMap::qtInstance(void)
{
	return s_qtOpenMap;
}

OpenMap *OpenMap::qtOpen(void)
{
	if (s_qtOpenMap == NULL)
	{
		s_qtOpenInfo.filename = "";
		s_qtOpenInfo.browse = false;
		s_qtOpenMap = new OpenMap(&s_qtOpenInfo);
		// Create() runs OnInitDialog (radio seed + initial population); no WS_VISIBLE in the
		// template, so the dialog stays hidden.
		s_qtOpenMap->Create(OpenMap::IDD, AfxGetMainWnd());
	}
	return s_qtOpenMap;
}

void OpenMap::qtClose(void)
{
	if (s_qtOpenMap != NULL)
	{
		s_qtOpenMap->DestroyWindow();
		delete s_qtOpenMap;
		s_qtOpenMap = NULL;
	}
}

int OpenMap::qtListCount(void)
{
	CListBox *pList = (CListBox *)GetDlgItem(IDC_OPEN_LIST);
	return (pList != NULL) ? pList->GetCount() : 0;
}

void OpenMap::qtListItem(int i, char *buf, int cap)
{
	if (buf != NULL && cap > 0)
	{
		buf[0] = 0;
	}
	CListBox *pList = (CListBox *)GetDlgItem(IDC_OPEN_LIST);
	if (pList != NULL && i >= 0 && i < pList->GetCount())
	{
		CString text;
		pList->GetText(i, text);
		copyOut((LPCTSTR)text, buf, cap);
	}
}

int OpenMap::qtListCurSel(void)
{
	CListBox *pList = (CListBox *)GetDlgItem(IDC_OPEN_LIST);
	return (pList != NULL) ? pList->GetCurSel() : -1;
}

int OpenMap::qtOkEnabled(void)
{
	CWnd *pOk = GetDlgItem(IDOK);
	return (pOk != NULL && pOk->IsWindowEnabled()) ? 1 : 0;
}

int OpenMap::qtGetMode(void)
{
	if (m_packedMode != PM_OFF)
	{
		return WB_QT_MAPMODE_PACKED;
	}
	return m_usingSystemDir ? WB_QT_MAPMODE_SYSTEM : WB_QT_MAPMODE_USER;
}

void OpenMap::qtSetMode(int mode)
{
	CButton *pSystem = (CButton *)GetDlgItem(IDC_SYSTEMMAPS);
	CButton *pUser = (CButton *)GetDlgItem(IDC_USERMAPS);
	CButton *pPacked = (CButton *)GetDlgItem(IDC_PACKED_MAPS);
	if (pSystem != NULL)
	{
		pSystem->SetCheck(mode == WB_QT_MAPMODE_SYSTEM ? 1 : 0);
	}
	if (pUser != NULL)
	{
		pUser->SetCheck(mode == WB_QT_MAPMODE_USER ? 1 : 0);
	}
	if (pPacked != NULL)
	{
		pPacked->SetCheck(mode == WB_QT_MAPMODE_PACKED ? 1 : 0);
	}
	switch (mode)
	{
		case WB_QT_MAPMODE_SYSTEM:
			OnSystemMaps();
			break;
		case WB_QT_MAPMODE_PACKED:
			OnPackedMaps();
			break;
		default:
			OnUserMaps();
			break;
	}
}

void OpenMap::qtSearch(const char *text)
{
	CWnd *pEdit = GetDlgItem(IDC_MAP_SEARCH_EDIT);
	if (pEdit != NULL)
	{
		pEdit->SetWindowText(text ? text : "");
	}
	OnSearchMap();
}

void OpenMap::qtResetSearch(void)
{
	CWnd *pEdit = GetDlgItem(IDC_MAP_SEARCH_EDIT);
	if (pEdit != NULL)
	{
		pEdit->SetWindowText("");
	}
	OnResetSearch();
}

int OpenMap::qtPick(int row)
{
	CListBox *pList = (CListBox *)GetDlgItem(IDC_OPEN_LIST);
	if (pList != NULL)
	{
		pList->SetCurSel(row);
	}
	// Sentinel: every COMPLETING path in OnOK sets a filename or the browse flag; the packed
	// drill-into-archive path touches neither (it relists and stays open).
	s_qtOpenInfo.filename = "";
	s_qtOpenInfo.browse = false;
	OnOK();
	return (s_qtOpenInfo.browse || !s_qtOpenInfo.filename.IsEmpty()) ? 1 : 0;
}

int OpenMap::qtBrowsePick(void)
{
	// == OnBrowse: packed mode pops the .big chooser (owner = the active Qt dialog) then
	// relists -- not a completion; normal mode completes with the browse fallback.
	s_qtOpenInfo.filename = "";
	s_qtOpenInfo.browse = false;
	OnBrowse();
	return (s_qtOpenInfo.browse || !s_qtOpenInfo.filename.IsEmpty()) ? 1 : 0;
}

extern "C" void WBQtOpenMapData_Open(void)
{
	OpenMap::qtOpen();
}

extern "C" void WBQtOpenMapData_Close(void)
{
	OpenMap::qtClose();
}

extern "C" int WBQtOpenMapData_ListCount(void)
{
	OpenMap *dlg = OpenMap::qtInstance();
	return (dlg != NULL) ? dlg->qtListCount() : 0;
}

extern "C" void WBQtOpenMapData_ListItem(int i, char *buf, int cap)
{
	OpenMap *dlg = OpenMap::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtListItem(i, buf, cap);
	}
}

extern "C" int WBQtOpenMapData_ListCurSel(void)
{
	OpenMap *dlg = OpenMap::qtInstance();
	return (dlg != NULL) ? dlg->qtListCurSel() : -1;
}

extern "C" int WBQtOpenMapData_OkEnabled(void)
{
	OpenMap *dlg = OpenMap::qtInstance();
	return (dlg != NULL) ? dlg->qtOkEnabled() : 0;
}

extern "C" int WBQtOpenMapData_GetMode(void)
{
	OpenMap *dlg = OpenMap::qtInstance();
	return (dlg != NULL) ? dlg->qtGetMode() : WB_QT_MAPMODE_USER;
}

extern "C" void WBQtOpenMap_SetMode(int mode)
{
	OpenMap *dlg = OpenMap::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtSetMode(mode);
	}
}

extern "C" void WBQtOpenMap_Search(const char *text)
{
	OpenMap *dlg = OpenMap::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtSearch(text);
	}
}

extern "C" void WBQtOpenMap_ResetSearch(void)
{
	OpenMap *dlg = OpenMap::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtResetSearch();
	}
}

extern "C" int WBQtOpenMap_Pick(int row)
{
	OpenMap *dlg = OpenMap::qtInstance();
	return (dlg != NULL) ? dlg->qtPick(row) : 0;
}

extern "C" int WBQtOpenMap_BrowsePick(void)
{
	OpenMap *dlg = OpenMap::qtInstance();
	return (dlg != NULL) ? dlg->qtBrowsePick() : 0;
}

extern "C" void WBQtOpenMapData_GetResult(char *filenameOut, int cap, int *browseOut)
{
	copyOut((LPCTSTR)s_qtOpenInfo.filename, filenameOut, cap);
	if (browseOut != NULL)
	{
		*browseOut = s_qtOpenInfo.browse ? 1 : 0;
	}
}

// ================= Save Map (native; data + confirmation ports) =================

static CStringArray s_qtSaveMaps;

extern "C" int WBQtSaveMapData_GetUseSystemDir(void)
{
	// == the SaveMap ctor's profile read (FALSE outside debug/internal).
#if defined(_DEBUG) || defined(_INTERNAL)
	return ::AfxGetApp()->GetProfileInt(MAP_OPENSAVE_PANEL_SECTION, "UseSystemDir", TRUE) ? 1 : 0;
#else
	return 0;
#endif
}

extern "C" int WBQtSaveMapData_Enumerate(int systemMaps)
{
	// == SaveMap::populateMapListbox's folder walk (a map counts when
	// Maps\<name>\<name>.map exists), including the profile write.
#if defined(_DEBUG) || defined(_INTERNAL)
	::AfxGetApp()->WriteProfileInt(MAP_OPENSAVE_PANEL_SECTION, "UseSystemDir", systemMaps ? TRUE : FALSE);
#endif
	s_qtSaveMaps.RemoveAll();

	HANDLE hFindFile = 0;
	WIN32_FIND_DATA findData;
	char dirBuf[_MAX_PATH];
	char findBuf[_MAX_PATH];
	char fileBuf[_MAX_PATH];

	if (systemMaps)
		strcpy(dirBuf, ".\\Maps\\");
	else
		sprintf(dirBuf, "%sMaps\\", TheGlobalData->getPath_UserData().str());
	int len = strlen(dirBuf);
	if (len > 0 && dirBuf[len - 1] != '\\') {
		dirBuf[len++] = '\\';
		dirBuf[len] = 0;
	}
	strcpy(findBuf, dirBuf);
	strcat(findBuf, "*.*");

	hFindFile = FindFirstFile(findBuf, &findData);
	if (hFindFile != INVALID_HANDLE_VALUE) {
		do {
			if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
				continue;
			if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				continue;
			}
			strcpy(fileBuf, dirBuf);
			strcat(fileBuf, findData.cFileName);
			strcat(fileBuf, "\\");
			strcat(fileBuf, findData.cFileName);
			strcat(fileBuf, ".map");
			try {
				CFileStatus status;
				if (CFile::GetStatus(fileBuf, status)) {
					if (!(status.m_attribute & CFile::directory)) {
						s_qtSaveMaps.Add(findData.cFileName);
					}
				}
			} catch(...) {}
		} while (FindNextFile(hFindFile, &findData));
		if (hFindFile) FindClose(hFindFile);
	}
	return (int)s_qtSaveMaps.GetSize();
}

extern "C" void WBQtSaveMapData_GetMapName(int i, char *buf, int cap)
{
	if (buf != NULL && cap > 0)
	{
		buf[0] = 0;
	}
	if (i >= 0 && i < s_qtSaveMaps.GetSize())
	{
		copyOut((LPCTSTR)s_qtSaveMaps[i], buf, cap);
	}
}

extern "C" int WBQtSaveMap_ConfirmOverwrite(const char *filename, int systemMaps)
{
	// == SaveMap::OnOK's existence check + IDS_REPLACEFILE yes/no prompt.
	CString testName;
	if (systemMaps)
		testName = ".\\Maps\\";
	else
	{
		testName = TheGlobalData->getPath_UserData().str();
		testName = testName + "Maps\\";
	}
	CString name(filename ? filename : "");
	testName += name + "\\" + name + ".map";
	CFileStatus status;
	if (CFile::GetStatus(testName, status)) {
		CString warn;
		warn.Format(IDS_REPLACEFILE, LPCTSTR(testName));
		Int ret = ::AfxMessageBox(warn, MB_YESNO);
		if (ret == IDNO) {
			return 0;
		}
	}
	return 1;
}

#endif // RTS_HAS_QT
