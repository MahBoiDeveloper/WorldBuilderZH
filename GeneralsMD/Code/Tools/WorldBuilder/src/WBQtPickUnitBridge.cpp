// WBQtPickUnitBridge.cpp -- the MFC side of the Qt Pick Unit / Replace Missing Unit seam.
// Plain MFC TU (no Qt include); whole body guarded by RTS_HAS_QT so the OFF build compiles it
// to an empty object. Supplies the template catalog filtered exactly like
// PickUnitDialog::OnInitDialog (allowable editor sortings + factionOnly==isBuildableItem) and
// the shared 128x128 preview render (ObjectPreview::qtRenderTemplatePreview).
#define DEFINE_EDITOR_SORTING_NAMES		// instantiate EditorSortingNames[] in this TU

#include "StdAfx.h"
#include "resource.h"
#include "Lib/BaseType.h"
#include "PickUnitDialog.h"
#include "ObjectPreview.h"
#include "Common/ThingTemplate.h"
#include "Common/ThingFactory.h"
#include "Common/ThingSort.h"
#include "qt/panels/WBQtPickUnitBridge.h"

#ifdef RTS_HAS_QT

#include <vector>

// The MFC preview is a fixed 128x128 BGR image (see PREVIEW_WIDTH/HEIGHT in ObjectPreview.cpp).
#define WBQT_PREVIEW_W 256	// the 2x Qt render (see ObjectPreview::qtRenderTemplatePreview)
#define WBQT_PREVIEW_H 256

static void copyOut(const AsciiString &s, char *buf, int cap)
{
	if (buf == NULL || cap <= 0)
	{
		return;
	}
	strncpy(buf, s.str(), cap - 1);
	buf[cap - 1] = 0;
}

static std::vector<const ThingTemplate *> s_qtPickTemplates;

extern "C" int WBQtPickUnitData_Build(const int *allowable, int allowCount, int factionOnly)
{
	Bool allowed[ES_NUM_SORTING_TYPES];
	int i;
	for (i = 0; i < ES_NUM_SORTING_TYPES; i++)
	{
		allowed[i] = false;
	}
	for (i = 0; i < allowCount; i++)
	{
		if (allowable != NULL && allowable[i] >= 0 && allowable[i] < ES_NUM_SORTING_TYPES)
		{
			allowed[allowable[i]] = true;
		}
	}
	s_qtPickTemplates.clear();
	const ThingTemplate *tTemplate;
	for( tTemplate = TheThingFactory->firstTemplate();
			 tTemplate;
			 tTemplate = tTemplate->friend_getNextTemplate() )
	{
		// == PickUnitDialog::IsAllowableType(sort, isBuildableItem).
		if (factionOnly && !tTemplate->isBuildableItem())
		{
			continue;
		}
		EditorSortingType sort = tTemplate->getEditorSorting();
		if (sort < 0 || sort >= ES_NUM_SORTING_TYPES || !allowed[sort])
		{
			continue;
		}
		s_qtPickTemplates.push_back(tTemplate);
	}
	return (int)s_qtPickTemplates.size();
}

extern "C" int WBQtPickUnitData_GetInfo(int i, char *nameOut, int nameCap, char *sideOut, int sideCap,
	char *sortingOut, int sortingCap, int *isTestOut)
{
	if (i < 0 || i >= (int)s_qtPickTemplates.size())
	{
		return 0;
	}
	const ThingTemplate *thingTemplate = s_qtPickTemplates[i];
	copyOut(thingTemplate->getName(), nameOut, nameCap);
	copyOut(thingTemplate->getDefaultOwningSide(), sideOut, sideCap);
	EditorSortingType sorting = thingTemplate->getEditorSorting();
	if (sorting >= ES_FIRST && sorting < ES_NUM_SORTING_TYPES)
	{
		copyOut(AsciiString(EditorSortingNames[sorting]), sortingOut, sortingCap);
	}
	else
	{
		// == addObject's fallthrough when no sorting matches.
		copyOut(AsciiString("UNSORTED"), sortingOut, sortingCap);
	}
	if (isTestOut != NULL)
	{
		*isTestOut = (sorting == ES_TEST) ? 1 : 0;
	}
	return 1;
}

extern "C" int WBQtPickUnit_RenderPreview(const char *templateName, unsigned char *bgrOut, int cap)
{
	if (bgrOut == NULL || cap < WBQT_PREVIEW_W * WBQT_PREVIEW_H * 3)
	{
		return 0;
	}
	if (templateName == NULL || templateName[0] == 0 || TheThingFactory == NULL)
	{
		return 0;
	}
	const ThingTemplate *tt = TheThingFactory->findTemplate(AsciiString(templateName));
	const UnsignedByte *data = ObjectPreview::qtRenderTemplatePreview(tt);
	if (data == NULL)
	{
		return 0;
	}
	memcpy(bgrOut, data, WBQT_PREVIEW_W * WBQT_PREVIEW_H * 3);
	return 1;
}

extern "C" void WBQtPickUnit_SavePos(int top, int left)
{
	// == PickUnitDialog::OnMove: keep the shared PickUnitWindow profile position in sync so
	// BuildListTool's pick panel (still MFC) opens where the user last left a pick dialog.
	::AfxGetApp()->WriteProfileInt(BUILD_PICK_PANEL_SECTION, "Top", top);
	::AfxGetApp()->WriteProfileInt(BUILD_PICK_PANEL_SECTION, "Left", left);
}

#endif // RTS_HAS_QT
