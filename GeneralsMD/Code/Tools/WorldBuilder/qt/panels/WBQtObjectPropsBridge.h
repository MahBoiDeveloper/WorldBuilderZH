// WBQtObjectPropsBridge.h -- self-contained opaque facade for the Qt Object Properties panel.
//
// Like the other per-panel bridge headers, this carries ONLY int/double/char* (no Qt or MFC
// types) so the MFC mapobjectprops.cpp TU can drive the Qt panel and the Qt panel can drive the
// MFC MapObjectProps dialog, without either side including the other's headers.
//
// MapObjectProps is a SELECTED-OBJECT panel: the hidden MFC dialog (TheMapObjectProps) stays the
// owner of the working Dict(s) -- m_dictToEdit for the current object, m_allSelectedDicts for a
// multi-select. The Qt panel reads state through the Get* funcs and writes through the Set* funcs,
// which set the hidden MFC control's state then call the real MFC _XToDict handler so the
// DictItemUndoable / multi-select path is reused unchanged.
//
// Phase 1: selection state + General section (object name, owning team).
#ifndef WB_QT_OBJECTPROPS_BRIDGE_H
#define WB_QT_OBJECTPROPS_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// --- Reverse: Qt widget -> MFC dialog (implemented MFC-side, MapObjectProps::qt*) ----------
// Selection. HasSelection is non-zero when a single non-waypoint/light object is selected;
// GetSelCount counts all selected editable objects (a multi-select edits every one).
int  WBQtObjectProps_HasSelection(void);
int  WBQtObjectProps_GetSelCount(void);

// Object name (General section). GetName copies the current object's name into out (cap bytes),
// returns non-zero if a Dict is being edited. SetName writes the hidden MFC edit box then runs
// _NameToDict (single-select only, like the MFC panel).
int  WBQtObjectProps_GetName(char *out, int cap);
void WBQtObjectProps_SetName(const char *name);

// Owning-team combo. GetTeamCount/GetTeamName give the team list (with the "(neutral)" relabel);
// GetCurTeam returns the index of the current object's team (-1 if none/unknown); SetTeam writes
// the hidden MFC combo then runs _TeamToDict (applies to every selected object).
int  WBQtObjectProps_GetTeamCount(void);
int  WBQtObjectProps_GetTeamName(int i, char *out, int cap);
int  WBQtObjectProps_GetCurTeam(void);
void WBQtObjectProps_SetTeam(int i);

// --- Forward: MFC dialog -> Qt widget (implemented Qt-side, WBQtObjectPropsBridge.cpp) ------
// MapObjectProps::updateTheUI() calls this after re-seeding its controls from the new selection;
// it re-reads the panel from the Get* funcs above (no-op when the Qt panel isn't open).
void WBQtObjectProps_PushRefresh(void);

// --- Forward: open/close the Qt panel (implemented Qt-side) ---------------------------------
// The generic options-panel host in WBQtOptionsPanels.cpp creates/shows the panel; these are the
// registry hooks it uses. (Declared here so the registry TU sees them without the shared header.)

#ifdef __cplusplus
}
#endif

#endif // WB_QT_OBJECTPROPS_BRIDGE_H
