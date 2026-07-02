// WBQtPickUnitBridge.h -- opaque facade for the Qt "Pick A Unit" / "Replace Missing Unit"
// dialogs (Tier 3e), the native rebuilds of PickUnitDialog/ReplaceUnitDialog. The MFC side
// (src/WBQtPickUnitBridge.cpp) supplies the filtered template catalog and the shared 128x128
// preview render. The _Run entry points return -1 when Qt is not up yet (a map opened from the
// command line validates its objects BEFORE WBQt_Startup) so callers fall back to the MFC
// dialogs. BuildListTool's modeless pick panel stays MFC.
#ifndef WB_QT_PICKUNIT_BRIDGE_H
#define WB_QT_PICKUNIT_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// ============ MFC -> Qt (implemented in qt/panels/WBQtPickUnitDialog.cpp) ============

// Run the Qt "Pick A Unit" modal (== PickUnitDialog::DoModal). allowable is the list of
// EditorSortingType values to include. Returns 1 on OK (nameOut = the picked template name;
// may be empty when a folder was selected -- callers no-op, like getPickedThing() returning
// NULL), 0 on cancel, -1 when Qt is unavailable (fall back to the MFC dialog).
int WBQtPickUnit_Run(void *frameHwnd, const int *allowable, int allowCount, int factionOnly,
	char *nameOut, int nameCap);

// Run the Qt "Replace Missing Unit" modal (== ReplaceUnitDialog::DoModal). Returns 1 on OK,
// 2 on "Continue without replacing..." (== IDIGNORE), 0 on cancel, -1 when Qt is unavailable.
int WBQtReplaceUnit_Run(void *frameHwnd, const char *missingName, const int *allowable,
	int allowCount, int factionOnly, char *nameOut, int nameCap);

// ============ Qt -> MFC (implemented in src/WBQtPickUnitBridge.cpp) ============

// Build the filtered template catalog (== PickUnitDialog::OnInitDialog's allowable-sorting +
// factionOnly==isBuildableItem filter); returns the row count readable via GetInfo.
int WBQtPickUnitData_Build(const int *allowable, int allowCount, int factionOnly);
int WBQtPickUnitData_GetInfo(int i, char *nameOut, int nameCap, char *sideOut, int sideCap,
	char *sortingOut, int sortingCap, int *isTestOut);

// Render the named template through the shared MFC preview path
// (ObjectPreview::qtRenderTemplatePreview): 128x128 BGR, bottom-up rows.
int WBQtPickUnit_RenderPreview(const char *templateName, unsigned char *bgrOut, int cap);

// Persist the dialog position (== PickUnitDialog::OnMove -> the PickUnitWindow Top/Left
// profile shared with BuildListTool's pick panel).
void WBQtPickUnit_SavePos(int top, int left);

#ifdef __cplusplus
}
#endif

#endif // WB_QT_PICKUNIT_BRIDGE_H
