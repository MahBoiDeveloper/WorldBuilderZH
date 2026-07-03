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
#include "Common/DataChunk.h"
#include "Common/FileSystem.h"
#include "GameLogic/SidesList.h"
#include "qt/panels/WBQtTeamsBridge.h"
#include <vector>

#ifdef RTS_HAS_QT

static CTeamsDialog *s_qtDlg = NULL;
static Int s_qtPrevCurTeam = 0;

// ================= De-bridged (windowless) view model -- branch qt-debridge =================
// The Teams dialog window is never Create()d; these statics mirror what updateUI /
// UpdateTeamsList seeded into the hidden player listbox, teams CListCtrl and command buttons,
// rebuilt from m_sides by qtMRefresh(). Copies of the teamsdialog.cpp file-static helpers.

static const char *QTM_NEUTRAL_NAME_STR = "(neutral)";
static const Int QTM_K_LOCAL_TEAMS_VERSION_1 = 1;

namespace
{
	AsciiString qtmPlayerNameForUI(SidesList &sides, int i)
	{
		AsciiString b = sides.getSideInfo(i)->getDict()->getAsciiString(TheKey_playerName);
		if (b.isEmpty())
		{
			b = QTM_NEUTRAL_NAME_STR;
		}
		return b;
	}

	AsciiString qtmTeamNameForUI(SidesList &sides, int i)
	{
		TeamsInfo *ti = sides.getTeamInfo(i);
		if (sides.isPlayerDefaultTeam(ti))
		{
			AsciiString n;
			n.format("(default team)");
			return n;
		}
		return ti->getDict()->getAsciiString(TheKey_teamName);
	}

	Bool qtmIsPlayerDefaultTeamIndex(SidesList &sides, Int i)
	{
		return sides.isPlayerDefaultTeam(sides.getTeamInfo(i));
	}

	// == the file-local stream wrapper teamsdialog.cpp uses for the export chunk writer.
	class QtmMFCFileOutputStream : public OutputStream
	{
	protected:
		CFile *m_file;
	public:
		QtmMFCFileOutputStream(CFile *pFile) : m_file(pFile) {}
		virtual Int write(const void *pData, Int numBytes)
		{
			Int numBytesWritten = 0;
			try {
				m_file->Write(pData, numBytes);
				numBytesWritten = numBytes;
			} catch(...) {
			}
			return numBytesWritten;
		}
	};
}

struct QtmTeamRow { AsciiString cols[6]; int teamIndex; int selected; };
static int s_qtCurPlayer = -1;	// == the hidden IDC_PLAYER_LIST GetCurSel (starts unselected)
static std::vector<AsciiString> s_qtPlayerNames;
static std::vector<QtmTeamRow> s_qtTeamRows;
static Bool s_qtNewEnabled = false, s_qtDeleteEnabled = false, s_qtCopyEnabled = false, s_qtMoveEnabled = false;

// == updateUI minus the controls: validate + clamp, optionally rebuild the row model
// (REBUILD_TEAMS == UpdateTeamsList's derivation), then the derived button enables.
void CTeamsDialog::qtMRefresh(int rebuildRows)
{
	m_sides.validateSides();

	if (m_curTeam < 0)
	{
		m_curTeam = 0;
	}
	if (m_curTeam >= m_sides.getNumTeams())
	{
		m_curTeam = m_sides.getNumTeams() - 1;
	}

	if (rebuildRows)
	{
		s_qtPlayerNames.clear();
		Int p;
		for (p = 0; p < m_sides.getNumSides(); p++)
		{
			s_qtPlayerNames.push_back(qtmPlayerNameForUI(m_sides, p));
		}

		s_qtTeamRows.clear();
		if (s_qtCurPlayer >= 0 && s_qtCurPlayer < m_sides.getNumSides())
		{
			AsciiString playerName = qtmPlayerNameForUI(m_sides, s_qtCurPlayer);
			Bool selected = false;
			Int i;
			for (i = 0; i < m_sides.getNumTeams(); i++)
			{
				TeamsInfo *ti = m_sides.getTeamInfo(i);
				if (ti->getDict()->getAsciiString(TheKey_teamOwner) == playerName.str())
				{
					QtmTeamRow row;
					Bool exists;
					row.cols[0] = qtmTeamNameForUI(m_sides, i);
					row.cols[1] = ti->getDict()->getAsciiString(TheKey_teamOnCreateScript, &exists);
					row.cols[2] = ti->getDict()->getAsciiString(TheKey_teamProductionCondition, &exists);
					row.cols[3].format("%d", ti->getDict()->getInt(TheKey_teamProductionPriority, &exists));
					row.cols[4] = ti->getDict()->getAsciiString(TheKey_teamHome, &exists);
					row.cols[5].format("%d", i);
					row.teamIndex = i;
					row.selected = 0;
					if (m_curTeam == i)
					{
						selected = true;
						row.selected = 1;
					}
					s_qtTeamRows.push_back(row);
				}
			}
			if (!selected)
			{
				m_curTeam = -1;
				if (!s_qtTeamRows.empty())
				{
					m_curTeam = s_qtTeamRows[0].teamIndex;
					s_qtTeamRows[0].selected = 1;
				}
			}
		}
	}

	// == updateUI's button-enable pass (derived; there are no controls to disable).
	Bool isDefault = true;
	if (m_curTeam >= 0)
	{
		isDefault = qtmIsPlayerDefaultTeamIndex(m_sides, m_curTeam);
	}
	s_qtDeleteEnabled = !isDefault;
	s_qtCopyEnabled = !isDefault;
	s_qtMoveEnabled = !isDefault;
	s_qtNewEnabled = (s_qtCurPlayer > 0);
}	// remembers the selected team across sessions (== thePrevCurTeam)

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
		// De-bridged (qt-debridge): never Create() the window. Seed the model like
		// OnInitDialog (m_sides copy, fix-owner validation -- which may pop the Qt-seamed
		// modals) and build the view model directly. The player list starts unselected,
		// exactly like the hidden listbox did.
		s_qtDlg->m_updating = 0;
		s_qtDlg->m_sides = *TheSidesList;
		s_qtDlg->m_curTeam = s_qtPrevCurTeam;
		s_qtDlg->m_expanded = TRUE;
		s_qtCurPlayer = -1;
		s_qtDlg->validateTeamOwners();
		s_qtDlg->qtMRefresh(1);
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
	return (int)s_qtPlayerNames.size();
}

void CTeamsDialog::qtPlayerName(int i, char *buf, int cap)
{
	if (buf != NULL && cap > 0)
	{
		buf[0] = 0;
	}
	if (i >= 0 && i < (int)s_qtPlayerNames.size())
	{
		copyOut(s_qtPlayerNames[i].str(), buf, cap);
	}
}

int CTeamsDialog::qtPlayerCurSel(void)
{
	return s_qtCurPlayer;
}

void CTeamsDialog::qtSelectPlayer(int i)
{
	// == OnSelchangePlayerList minus the listbox: refilter the team rows for this player.
	s_qtCurPlayer = i;
	qtMRefresh(1);
}

int CTeamsDialog::qtTeamRowCount(void)
{
	return (int)s_qtTeamRows.size();
}

void CTeamsDialog::qtTeamRowText(int row, int col, char *buf, int cap)
{
	if (buf != NULL && cap > 0)
	{
		buf[0] = 0;
	}
	if (row >= 0 && row < (int)s_qtTeamRows.size() && col >= 0 && col < 6)
	{
		copyOut(s_qtTeamRows[row].cols[col].str(), buf, cap);
	}
}

int CTeamsDialog::qtTeamRowSelected(int row)
{
	if (row >= 0 && row < (int)s_qtTeamRows.size())
	{
		return s_qtTeamRows[row].selected;
	}
	return 0;
}

void CTeamsDialog::qtSelectTeamRow(int row)
{
	if (row < 0 || row >= (int)s_qtTeamRows.size())
	{
		return;
	}
	// == a click on the row: only the selection + button enables change (the per-click
	// light path from the Teams-lag fix, now fully model-side).
	for (size_t i = 0; i < s_qtTeamRows.size(); i++)
	{
		s_qtTeamRows[i].selected = 0;
	}
	s_qtTeamRows[row].selected = 1;
	m_curTeam = s_qtTeamRows[row].teamIndex;
	qtMRefresh(0);
}

int CTeamsDialog::qtIsCtrlEnabled(int ctrlId)
{
	switch (ctrlId)
	{
		case IDC_NEWTEAM:      return s_qtNewEnabled ? 1 : 0;
		case IDC_DELETETEAM:   return s_qtDeleteEnabled ? 1 : 0;
		case IDC_COPYTEAM:     return s_qtCopyEnabled ? 1 : 0;
		case IDC_MOVEUPTEAM:
		case IDC_MOVEDOWNTEAM: return s_qtMoveEnabled ? 1 : 0;
		default:               return 1;
	}
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
	qtMRefresh(1);
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
	// == OnDeleteteam minus the updateUI tail.
	if (m_curTeam < 0)
	{
		return;
	}
	if (qtmIsPlayerDefaultTeamIndex(m_sides, m_curTeam))
	{
		return;	// should not be allowed (button disabled)
	}
	AsciiString tname = m_sides.getTeamInfo(m_curTeam)->getDict()->getAsciiString(TheKey_teamName);
	Int count = MapObject::countMapObjectsWithOwner(tname);
	if (count > 0)
	{
		CString msg;
		msg.Format(IDS_REMOVING_INUSE_TEAM, count);
		if (::AfxMessageBox(msg, MB_YESNO) == IDNO)
		{
			return;
		}
	}
	m_sides.removeTeam(m_curTeam);
	qtMRefresh(1);
}

void CTeamsDialog::qtCopyTeam(void)
{
	// == OnCopyteam minus the updateUI tail.
	Dict d = *m_sides.getTeamInfo(m_curTeam)->getDict();
	AsciiString origName = d.getAsciiString(TheKey_teamName);
	Int num = 1;
	AsciiString tname;
	do
	{
		tname.format("%s.%2d", origName.str(), num++);
	}
	while (m_sides.findTeamInfo(tname));
	d.setAsciiString(TheKey_teamName, tname);
	m_sides.addTeam(&d);
	qtMRefresh(1);
}

void CTeamsDialog::qtEditTeam(void)
{
	// De-bridged: the Qt team sheet (WBQtTeamSheet_Open) IS the editor now; the MFC
	// property-sheet path would need the window. This facade entry is unused by the Qt
	// dialog and intentionally does nothing.
}

void CTeamsDialog::qtSelectTeamMembers(void)
{
	OnSelectTeamMembers();	// pure m_sides + map walk + info box; no controls
}

void CTeamsDialog::qtMoveUpTeam(void)
{
	// == OnMoveUpTeam with findPrevTeamIndex resolved against the view rows instead of the
	// hidden list's col-5 walk (the rows carry the same real team indexes).
	if (m_curTeam <= 0)
	{
		return;
	}
	int prevIndex = -1;
	for (size_t i = 1; i < s_qtTeamRows.size(); i++)
	{
		if (s_qtTeamRows[i].teamIndex == m_curTeam)
		{
			prevIndex = s_qtTeamRows[i - 1].teamIndex;
			break;
		}
	}
	if (prevIndex < 0)
	{
		return;
	}
	Dict *currentTeam = m_sides.getTeamInfo(m_curTeam)->getDict();
	Dict *prevTeam = m_sides.getTeamInfo(prevIndex)->getDict();
	std::swap(*currentTeam, *prevTeam);
	m_curTeam = prevIndex;
	qtMRefresh(1);
}

void CTeamsDialog::qtMoveDownTeam(void)
{
	// == OnMoveDownTeam, view-row resolved.
	if (m_curTeam >= m_sides.getNumTeams() - 1)
	{
		return;
	}
	int nextIndex = -1;
	for (size_t i = 0; i + 1 < s_qtTeamRows.size(); i++)
	{
		if (s_qtTeamRows[i].teamIndex == m_curTeam)
		{
			nextIndex = s_qtTeamRows[i + 1].teamIndex;
			break;
		}
	}
	if (nextIndex < 0 || nextIndex >= m_sides.getNumTeams())
	{
		return;
	}
	Dict *currentTeam = m_sides.getTeamInfo(m_curTeam)->getDict();
	Dict *nextTeam = m_sides.getTeamInfo(nextIndex)->getDict();
	std::swap(*currentTeam, *nextTeam);
	m_curTeam = nextIndex;
	qtMRefresh(1);
}

void CTeamsDialog::qtExportTeams(void)
{
	// == OnExportTeams minus the player-listbox read (the view model holds the selection).
	Int selectedPlayer = s_qtCurPlayer;
	if (selectedPlayer < 1)
	{
		AfxMessageBox("Please select a valid player first!", MB_OK | MB_ICONWARNING);
		return;
	}
	AsciiString selectedPlayerName = qtmPlayerNameForUI(m_sides, selectedPlayer);

	CFileDialog dlg(FALSE, ".teams", "exportedteams.teams",
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"Team Export (*.teams)|*.teams||");
	if (dlg.DoModal() == IDCANCEL)
	{
		return;
	}
	CString path = dlg.GetPathName();
	try {
		CFile f(path,
			CFile::modeCreate | CFile::modeWrite |
			CFile::shareDenyWrite | CFile::typeBinary);
		QtmMFCFileOutputStream stream(&f);
		DataChunkOutput out(&stream);
		out.openDataChunk("ScriptTeams", QTM_K_LOCAL_TEAMS_VERSION_1);
		int exportedCount = 0;
		for (int i = 0; i < m_sides.getNumTeams(); i++)
		{
			TeamsInfo *ti = m_sides.getTeamInfo(i);
			if (!ti) { continue; }
			if (m_sides.isPlayerDefaultTeam(ti)) { continue; }
			Dict *d = ti->getDict();
			if (!d) { continue; }
			AsciiString teamOwner = d->getAsciiString(TheKey_teamOwner);
			if (teamOwner == selectedPlayerName.str())
			{
				out.writeDict(*d);
				exportedCount++;
			}
		}
		out.closeDataChunk();
		if (exportedCount == 0)
		{
			AfxMessageBox("No teams found for the selected player!", MB_OK | MB_ICONWARNING);
		}
		else
		{
			CString msg;
			msg.Format("Successfully exported %d team(s) from player '%s' to:\n%s",
				exportedCount, selectedPlayerName.str(), path);
			AfxMessageBox(msg, MB_OK | MB_ICONINFORMATION);
		}
	} catch(...) {
		AfxMessageBox("Error writing team export file!", MB_OK | MB_ICONERROR);
	}
}

void CTeamsDialog::qtImportTeams(void)
{
	// == OnImportTeams minus the player-listbox read and the updateUI tail.
	Int selectedPlayer = s_qtCurPlayer;
	if (selectedPlayer < 1)
	{
		AfxMessageBox("Please select a valid player first!", MB_OK | MB_ICONWARNING);
		return;
	}
	m_importTargetPlayer = qtmPlayerNameForUI(m_sides, selectedPlayer);

	CFileDialog dlg(TRUE, ".teams", NULL,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		"Team Export (*.teams)|*.teams||");
	if (dlg.DoModal() == IDCANCEL)
	{
		return;
	}
	CString path = dlg.GetPathName();
	try {
		CachedFileInputStream in;
		if (!in.open(AsciiString(path)))
		{
			AfxMessageBox("Could not open team file!", MB_OK | MB_ICONERROR);
			return;
		}
		DataChunkInput reader(&in);
		reader.registerParser(
			AsciiString("ScriptTeams"),
			AsciiString::TheEmptyString,
			ParseTeamsDataChunk
		);
		if (!reader.parse(this))
		{
			AfxMessageBox("Error parsing team export file!\nFile may be corrupt or incompatible.",
				MB_OK | MB_ICONERROR);
			return;
		}
		validateTeamOwners();
		qtMRefresh(1);
		AfxMessageBox("Teams imported successfully!", MB_OK | MB_ICONINFORMATION);
	} catch(...) {
		AfxMessageBox("Exception occurred while importing teams!", MB_OK | MB_ICONERROR);
	}
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
	s_qtPageIdentity->Create(TeamIdentity::IDD, AfxGetMainWnd());	// hidden child of the frame (the Teams dialog is windowless now)

	s_qtPageReinforcement = new TeamReinforcement();
	s_qtPageReinforcement->setTeamDict(teamDict);
	s_qtPageReinforcement->Create(TeamReinforcement::IDD, AfxGetMainWnd());

	s_qtPageBehavior = new TeamBehavior();
	s_qtPageBehavior->setTeamDict(teamDict);
	s_qtPageBehavior->Create(TeamBehavior::IDD, AfxGetMainWnd());

	s_qtPageGeneric = new TeamGeneric();
	s_qtPageGeneric->setTeamDict(teamDict);
	s_qtPageGeneric->Create(TeamGeneric::IDD, AfxGetMainWnd());

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
