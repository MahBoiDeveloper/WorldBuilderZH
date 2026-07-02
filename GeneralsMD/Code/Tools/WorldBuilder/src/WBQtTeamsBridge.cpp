// WBQtTeamsBridge.cpp -- MFC side of the Qt Teams-dialog seam (Tier 3b-2). Plain MFC TU (no Qt
// include). Defines CTeamsDialog's qt* member functions here (member functions may be defined
// in any TU): the dialog is created HIDDEN as the model owner and the Qt dialog drives its real
// controls / real handlers, so the working-copy m_sides, the team surgery, the MFC team
// property sheet and the fix-owner validation are reused verbatim. Whole body guarded by
// RTS_HAS_QT so the OFF build compiles it to an empty object.
#include "StdAfx.h"
#include "resource.h"
#include "Lib/BaseType.h"
#include "WorldBuilder.h"
#include "WorldBuilderDoc.h"
#include "CUndoable.h"
#include "teamsdialog.h"
#include "TeamIdentity.h"
#include "TeamReinforcement.h"
#include "TeamBehavior.h"
#include "TeamGeneric.h"
#include "Common/WellKnownKeys.h"
#include "GameLogic/SidesList.h"
#include "qt/panels/WBQtTeamsBridge.h"

#ifdef RTS_HAS_QT

static CTeamsDialog *s_qtDlg = NULL;
static Int s_qtPrevCurTeam = 0;	// remembers the selected team across sessions (== thePrevCurTeam)

static void copyOut(const char *str, char *buf, int cap)
{
	if (buf == NULL || cap <= 0)
	{
		return;
	}
	strncpy(buf, str ? str : "", cap - 1);
	buf[cap - 1] = 0;
}

// ================= CTeamsDialog qt* member functions =================

CTeamsDialog *CTeamsDialog::qtInstance(void)
{
	return s_qtDlg;
}

CTeamsDialog *CTeamsDialog::qtOpen(void)
{
	if (s_qtDlg == NULL)
	{
		s_qtDlg = new CTeamsDialog();
		// Create() runs OnInitDialog (m_sides copy, columns, player list, the fix-owner
		// validation -- which may pop modals); the template has no WS_VISIBLE, so the dialog
		// stays hidden.
		s_qtDlg->Create(CTeamsDialog::IDD, AfxGetMainWnd());
		// Restore the last-selected team (the MFC static that does this is file-local to
		// teamsdialog.cpp, so the Qt path keeps its own).
		s_qtDlg->m_curTeam = s_qtPrevCurTeam;
		s_qtDlg->updateUI(REBUILD_ALL);
	}
	return s_qtDlg;
}

void CTeamsDialog::qtClose(int accepted)
{
	if (s_qtDlg != NULL)
	{
		if (accepted != 0)
		{
			s_qtDlg->qtCommit();
		}
		s_qtDlg->DestroyWindow();
		delete s_qtDlg;
		s_qtDlg = NULL;
	}
}

void CTeamsDialog::qtCommit(void)
{
	// == OnOK minus the modal close: preserve the map selection around the one-undoable commit.
	std::vector<Coord3D> selectedPositions;
	for (MapObject* pObj = MapObject::getFirstMapObject(); pObj; pObj = pObj->getNext()) {
		if (pObj->isSelected())
			selectedPositions.push_back(*pObj->getLocation());
	}

	Bool modified = m_sides.validateSides();
	(void)modified;
	DEBUG_ASSERTLOG(!modified,("had to clean up sides in CTeamsDialog::qtCommit"));

	CWorldBuilderDoc* pDoc = CWorldBuilderDoc::GetActiveDoc();
	SidesListUndoable *pUndo = new SidesListUndoable(m_sides, pDoc);
	pDoc->AddAndDoUndoable(pUndo);
	REF_PTR_RELEASE(pUndo); // belongs to pDoc now.

	s_qtPrevCurTeam = m_curTeam;

	for (MapObject* pObjb = MapObject::getFirstMapObject(); pObjb; pObjb = pObjb->getNext()) {
		pObjb->setSelected(false);
		for (std::vector<Coord3D>::size_type i = 0; i < selectedPositions.size(); ++i) {
			if (*pObjb->getLocation() == selectedPositions[i]) {
				pObjb->setSelected(true);
				break; // no need to check the rest
			}
		}
	}
}

int CTeamsDialog::qtPlayerCount(void)
{
	CListBox *players = (CListBox *)GetDlgItem(IDC_PLAYER_LIST);
	return (players != NULL) ? players->GetCount() : 0;
}

void CTeamsDialog::qtPlayerName(int i, char *buf, int cap)
{
	if (buf != NULL && cap > 0)
	{
		buf[0] = 0;
	}
	CListBox *players = (CListBox *)GetDlgItem(IDC_PLAYER_LIST);
	if (players != NULL && i >= 0 && i < players->GetCount())
	{
		CString text;
		players->GetText(i, text);
		copyOut((LPCTSTR)text, buf, cap);
	}
}

int CTeamsDialog::qtPlayerCurSel(void)
{
	CListBox *players = (CListBox *)GetDlgItem(IDC_PLAYER_LIST);
	return (players != NULL) ? players->GetCurSel() : -1;
}

void CTeamsDialog::qtSelectPlayer(int i)
{
	CListBox *players = (CListBox *)GetDlgItem(IDC_PLAYER_LIST);
	if (players != NULL)
	{
		players->SetCurSel(i);
	}
	OnSelchangePlayerList();
}

int CTeamsDialog::qtTeamRowCount(void)
{
	CListCtrl *pList = (CListCtrl *)GetDlgItem(IDC_TEAMS_LIST);
	return (pList != NULL) ? pList->GetItemCount() : 0;
}

void CTeamsDialog::qtTeamRowText(int row, int col, char *buf, int cap)
{
	if (buf != NULL && cap > 0)
	{
		buf[0] = 0;
	}
	CListCtrl *pList = (CListCtrl *)GetDlgItem(IDC_TEAMS_LIST);
	if (pList != NULL && row >= 0 && row < pList->GetItemCount())
	{
		CString text = pList->GetItemText(row, col);
		copyOut((LPCTSTR)text, buf, cap);
	}
}

int CTeamsDialog::qtTeamRowSelected(int row)
{
	CListCtrl *pList = (CListCtrl *)GetDlgItem(IDC_TEAMS_LIST);
	if (pList != NULL && row >= 0 && row < pList->GetItemCount())
	{
		return (pList->GetItemState(row, LVIS_SELECTED) & LVIS_SELECTED) ? 1 : 0;
	}
	return 0;
}

void CTeamsDialog::qtSelectTeamRow(int row)
{
	CListCtrl *pList = (CListCtrl *)GetDlgItem(IDC_TEAMS_LIST);
	if (pList == NULL || row < 0 || row >= pList->GetItemCount())
	{
		return;
	}
	// == a click on the row: make it the only selected item, then run the click handler
	// (which reads the selection and sets m_curTeam from the row's item data).
	pList->SetItemState(-1, 0, LVIS_SELECTED);
	pList->SetItemState(row, LVIS_SELECTED, LVIS_SELECTED);
	LRESULT result = 0;
	OnClickTeamsList(NULL, &result);
}

int CTeamsDialog::qtIsCtrlEnabled(int ctrlId)
{
	CWnd *ctrl = GetDlgItem(ctrlId);
	return (ctrl != NULL && ctrl->IsWindowEnabled()) ? 1 : 0;
}

void CTeamsDialog::qtNewTeam(void)
{
	// == OnNewteam minus the OnEditTemplate pop: the Qt side opens the Qt team sheet on the
	// new team after this returns.
	Int num = 1;
	AsciiString tname;
	do
	{
		tname.format("team%04d",num++);
	}
	while (m_sides.findTeamInfo(tname));

	AsciiString oname = m_sides.getTeamInfo(m_curTeam)->getDict()->getAsciiString(TheKey_teamOwner);

	Dict d;
	d.setAsciiString(TheKey_teamName, tname);
	d.setAsciiString(TheKey_teamOwner, oname);	// owned by the parent of whatever is selected.
	d.setBool(TheKey_teamIsSingleton, false);

	m_sides.addTeam(&d);
	Int i;
	if (m_sides.findTeamInfo(tname, &i)) {
		m_curTeam = i;
	}
	updateUI(REBUILD_ALL);
}

void *CTeamsDialog::qtCurTeamDict(void)
{
	if (m_curTeam >= 0 && m_curTeam < m_sides.getNumTeams())
	{
		return m_sides.getTeamInfo(m_curTeam)->getDict();
	}
	return NULL;
}

void *CTeamsDialog::qtSides(void)
{
	return &m_sides;
}

int CTeamsDialog::qtCurTeamIsDefault(void)
{
	if (m_curTeam >= 0 && m_curTeam < m_sides.getNumTeams())
	{
		return m_sides.isPlayerDefaultTeam(m_sides.getTeamInfo(m_curTeam)) ? 1 : 0;
	}
	return 1;	// nothing selected -> treat as un-editable
}

void CTeamsDialog::qtDeleteTeam(void)
{
	OnDeleteteam();
}

void CTeamsDialog::qtCopyTeam(void)
{
	OnCopyteam();
}

void CTeamsDialog::qtEditTeam(void)
{
	// Guard like OnDblclkTeamsList: the default team has no editable template.
	if (m_curTeam >= 0 && !m_sides.isPlayerDefaultTeam(m_sides.getTeamInfo(m_curTeam)))
	{
		OnEditTemplate();
	}
}

void CTeamsDialog::qtSelectTeamMembers(void)
{
	OnSelectTeamMembers();
}

void CTeamsDialog::qtMoveUpTeam(void)
{
	OnMoveUpTeam();
}

void CTeamsDialog::qtMoveDownTeam(void)
{
	OnMoveDownTeam();
}

void CTeamsDialog::qtExportTeams(void)
{
	OnExportTeams();
}

void CTeamsDialog::qtImportTeams(void)
{
	OnImportTeams();
}

// ================= the C facade =================

extern "C" void WBQtTeamsData_Open(void)
{
	CTeamsDialog::qtOpen();
}

extern "C" void WBQtTeamsData_Close(int accepted)
{
	CTeamsDialog::qtClose(accepted);
}

extern "C" int WBQtTeamsData_GetPlayerCount(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	return (dlg != NULL) ? dlg->qtPlayerCount() : 0;
}

extern "C" void WBQtTeamsData_GetPlayerName(int i, char *buf, int cap)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtPlayerName(i, buf, cap);
	}
}

extern "C" int WBQtTeamsData_GetPlayerIndex(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	return (dlg != NULL) ? dlg->qtPlayerCurSel() : -1;
}

extern "C" void WBQtTeams_SelectPlayer(int i)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtSelectPlayer(i);
	}
}

extern "C" int WBQtTeamsData_GetTeamRowCount(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	return (dlg != NULL) ? dlg->qtTeamRowCount() : 0;
}

extern "C" void WBQtTeamsData_GetTeamRowText(int row, int col, char *buf, int cap)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtTeamRowText(row, col, buf, cap);
	}
}

extern "C" int WBQtTeamsData_GetTeamRowSelected(int row)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	return (dlg != NULL) ? dlg->qtTeamRowSelected(row) : 0;
}

extern "C" void WBQtTeams_SelectTeamRow(int row)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtSelectTeamRow(row);
	}
}

extern "C" int WBQtTeamsData_GetNewEnabled(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	return (dlg != NULL) ? dlg->qtIsCtrlEnabled(IDC_NEWTEAM) : 0;
}

extern "C" int WBQtTeamsData_GetDeleteEnabled(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	return (dlg != NULL) ? dlg->qtIsCtrlEnabled(IDC_DELETETEAM) : 0;
}

extern "C" int WBQtTeamsData_GetCopyEnabled(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	return (dlg != NULL) ? dlg->qtIsCtrlEnabled(IDC_COPYTEAM) : 0;
}

extern "C" int WBQtTeamsData_GetMoveEnabled(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	return (dlg != NULL) ? dlg->qtIsCtrlEnabled(IDC_MOVEUPTEAM) : 0;
}

extern "C" void WBQtTeams_NewTeam(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtNewTeam();
	}
}

extern "C" void WBQtTeams_DeleteTeam(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtDeleteTeam();
	}
}

extern "C" void WBQtTeams_CopyTeam(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtCopyTeam();
	}
}

extern "C" void WBQtTeams_EditTeam(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtEditTeam();
	}
}

extern "C" void WBQtTeams_SelectTeamMembers(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtSelectTeamMembers();
	}
}

extern "C" void WBQtTeams_MoveUpTeam(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtMoveUpTeam();
	}
}

extern "C" void WBQtTeams_MoveDownTeam(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtMoveDownTeam();
	}
}

extern "C" void WBQtTeams_ExportTeams(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtExportTeams();
	}
}

extern "C" void WBQtTeams_ImportTeams(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	if (dlg != NULL)
	{
		dlg->qtImportTeams();
	}
}

// ================= the Qt team property sheet (Tier 3b-3) =================
// Four HIDDEN Team* pages (they are CDialogs) bound to the current team's dict in the hidden
// CTeamsDialog's working copy. Their OnInitDialog seeds the controls and every handler writes
// the dict live, so the Qt sheet drives them generically: write the control, then send the
// real WM_COMMAND notification -- the page's own message map dispatches to the right handler.
// The unit-pick "..." buttons still pop the MFC PickUnitDialog (owner = the active Qt sheet).

static TeamIdentity *s_qtPageIdentity = NULL;
static TeamReinforcement *s_qtPageReinforcement = NULL;
static TeamBehavior *s_qtPageBehavior = NULL;
static TeamGeneric *s_qtPageGeneric = NULL;

static CDialog *qtTeamPage(int page)
{
	switch (page)
	{
		case WB_QT_TEAMPAGE_IDENTITY:		return s_qtPageIdentity;
		case WB_QT_TEAMPAGE_REINFORCEMENT:	return s_qtPageReinforcement;
		case WB_QT_TEAMPAGE_BEHAVIOR:		return s_qtPageBehavior;
		case WB_QT_TEAMPAGE_GENERIC:		return s_qtPageGeneric;
		default:							return NULL;
	}
}

extern "C" int WBQtTeamSheet_Open(void)
{
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	if (dlg == NULL || s_qtPageIdentity != NULL)
	{
		return 0;
	}
	if (dlg->qtCurTeamIsDefault() != 0)
	{
		return 0;	// default teams have no editable template (== the double-click guard)
	}
	Dict *teamDict = static_cast<Dict *>(dlg->qtCurTeamDict());
	SidesList *sides = static_cast<SidesList *>(dlg->qtSides());
	if (teamDict == NULL)
	{
		return 0;
	}

	// == OnEditTemplate's page setup, but Create()'d hidden (children of the hidden Teams
	// dialog; the page templates are WS_CHILD with no WS_VISIBLE).
	s_qtPageIdentity = new TeamIdentity();
	s_qtPageIdentity->setTeamDict(teamDict);
	s_qtPageIdentity->setSidesList(sides);
	s_qtPageIdentity->Create(TeamIdentity::IDD, dlg);

	s_qtPageReinforcement = new TeamReinforcement();
	s_qtPageReinforcement->setTeamDict(teamDict);
	s_qtPageReinforcement->Create(TeamReinforcement::IDD, dlg);

	s_qtPageBehavior = new TeamBehavior();
	s_qtPageBehavior->setTeamDict(teamDict);
	s_qtPageBehavior->Create(TeamBehavior::IDD, dlg);

	s_qtPageGeneric = new TeamGeneric();
	s_qtPageGeneric->setTeamDict(teamDict);
	s_qtPageGeneric->Create(TeamGeneric::IDD, dlg);

	return 1;
}

extern "C" void WBQtTeamSheet_Close(void)
{
	// Page edits already landed in the working copy live (the MFC sheet behaved the same:
	// its DoModal result was ignored); just tear the hidden pages down. The Teams dialog
	// refreshes its list after (the Qt side calls updateUI via a selection re-push).
	if (s_qtPageIdentity != NULL)
	{
		s_qtPageIdentity->DestroyWindow();
		delete s_qtPageIdentity;
		s_qtPageIdentity = NULL;
	}
	if (s_qtPageReinforcement != NULL)
	{
		s_qtPageReinforcement->DestroyWindow();
		delete s_qtPageReinforcement;
		s_qtPageReinforcement = NULL;
	}
	if (s_qtPageBehavior != NULL)
	{
		s_qtPageBehavior->DestroyWindow();
		delete s_qtPageBehavior;
		s_qtPageBehavior = NULL;
	}
	if (s_qtPageGeneric != NULL)
	{
		s_qtPageGeneric->DestroyWindow();
		delete s_qtPageGeneric;
		s_qtPageGeneric = NULL;
	}
	CTeamsDialog *dlg = CTeamsDialog::qtInstance();
	if (dlg != NULL)
	{
		// Rebuild the hidden teams list (the sheet may have renamed/re-owned the team):
		// re-pushing the player selection runs updateUI(REBUILD_ALL).
		dlg->qtSelectPlayer(dlg->qtPlayerCurSel());
	}
}

static void qtPageNotify(CDialog *page, int ctrlId, int code)
{
	CWnd *ctrl = page->GetDlgItem(ctrlId);
	if (ctrl != NULL)
	{
		page->SendMessage(WM_COMMAND, MAKEWPARAM(ctrlId, code), (LPARAM)ctrl->GetSafeHwnd());
	}
}

extern "C" void WBQtTeamPage_GetText(int page, int ctrlId, char *buf, int cap)
{
	if (buf != NULL && cap > 0)
	{
		buf[0] = 0;
	}
	CDialog *pPage = qtTeamPage(page);
	if (pPage != NULL)
	{
		CWnd *ctrl = pPage->GetDlgItem(ctrlId);
		if (ctrl != NULL)
		{
			CString text;
			ctrl->GetWindowText(text);
			copyOut((LPCTSTR)text, buf, cap);
		}
	}
}

extern "C" void WBQtTeamPage_SetText(int page, int ctrlId, const char *text, int notify)
{
	CDialog *pPage = qtTeamPage(page);
	if (pPage == NULL)
	{
		return;
	}
	CWnd *ctrl = pPage->GetDlgItem(ctrlId);
	if (ctrl == NULL)
	{
		return;
	}
	ctrl->SetWindowText(text ? text : "");
	if (notify == WB_QT_TEAMNOTIFY_CHANGE)
	{
		qtPageNotify(pPage, ctrlId, EN_CHANGE);
	}
	else if (notify == WB_QT_TEAMNOTIFY_KILLFOCUS)
	{
		qtPageNotify(pPage, ctrlId, EN_KILLFOCUS);
	}
}

extern "C" int WBQtTeamPage_GetCheck(int page, int ctrlId)
{
	CDialog *pPage = qtTeamPage(page);
	if (pPage != NULL)
	{
		CButton *button = (CButton *)pPage->GetDlgItem(ctrlId);
		return (button != NULL && button->GetCheck() == 1) ? 1 : 0;
	}
	return 0;
}

extern "C" void WBQtTeamPage_SetCheck(int page, int ctrlId, int check)
{
	CDialog *pPage = qtTeamPage(page);
	if (pPage == NULL)
	{
		return;
	}
	CButton *button = (CButton *)pPage->GetDlgItem(ctrlId);
	if (button != NULL)
	{
		button->SetCheck(check ? 1 : 0);
		qtPageNotify(pPage, ctrlId, BN_CLICKED);
	}
}

extern "C" int WBQtTeamPage_IsEnabled(int page, int ctrlId)
{
	CDialog *pPage = qtTeamPage(page);
	if (pPage != NULL)
	{
		CWnd *ctrl = pPage->GetDlgItem(ctrlId);
		return (ctrl != NULL && ctrl->IsWindowEnabled()) ? 1 : 0;
	}
	return 0;
}

extern "C" int WBQtTeamPage_ComboCount(int page, int ctrlId)
{
	CDialog *pPage = qtTeamPage(page);
	if (pPage != NULL)
	{
		CComboBox *combo = (CComboBox *)pPage->GetDlgItem(ctrlId);
		return (combo != NULL) ? combo->GetCount() : 0;
	}
	return 0;
}

extern "C" void WBQtTeamPage_ComboItem(int page, int ctrlId, int i, char *buf, int cap)
{
	if (buf != NULL && cap > 0)
	{
		buf[0] = 0;
	}
	CDialog *pPage = qtTeamPage(page);
	if (pPage != NULL)
	{
		CComboBox *combo = (CComboBox *)pPage->GetDlgItem(ctrlId);
		if (combo != NULL && i >= 0 && i < combo->GetCount())
		{
			CString text;
			combo->GetLBText(i, text);
			copyOut((LPCTSTR)text, buf, cap);
		}
	}
}

extern "C" void WBQtTeamPage_ComboSelectText(int page, int ctrlId, const char *text, int notify)
{
	CDialog *pPage = qtTeamPage(page);
	if (pPage == NULL)
	{
		return;
	}
	CComboBox *combo = (CComboBox *)pPage->GetDlgItem(ctrlId);
	if (combo == NULL)
	{
		return;
	}
	combo->SelectString(-1, text ? text : "");
	if (notify == WB_QT_TEAMNOTIFY_SELCHANGE)
	{
		qtPageNotify(pPage, ctrlId, CBN_SELCHANGE);
	}
	else if (notify == WB_QT_TEAMNOTIFY_SELENDOK)
	{
		qtPageNotify(pPage, ctrlId, CBN_SELENDOK);
	}
}

extern "C" void WBQtTeamPage_ClickButton(int page, int ctrlId)
{
	CDialog *pPage = qtTeamPage(page);
	if (pPage != NULL)
	{
		qtPageNotify(pPage, ctrlId, BN_CLICKED);
	}
}

#endif // RTS_HAS_QT
