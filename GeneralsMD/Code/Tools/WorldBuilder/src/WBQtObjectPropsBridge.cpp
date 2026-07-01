// WBQtObjectPropsBridge.cpp -- MFC side of the Qt Object Properties facade.
//
// These extern "C" reverse callbacks (declared in qt/panels/WBQtObjectPropsBridge.h) forward to
// the MapObjectProps::qt* statics. The whole body is behind RTS_HAS_QT; with Qt OFF this TU is
// empty, exactly like the other WBQt*Bridge.cpp files, so the MFC-only build is unchanged.

#include "StdAfx.h"

#ifdef RTS_HAS_QT

#include "mapobjectprops.h"
#include "qt/panels/WBQtObjectPropsBridge.h"

extern "C" int WBQtObjectProps_HasSelection(void)
{
	return MapObjectProps::qtHasSelection();
}

extern "C" int WBQtObjectProps_GetSelCount(void)
{
	return MapObjectProps::qtGetSelCount();
}

extern "C" int WBQtObjectProps_GetName(char *out, int cap)
{
	return MapObjectProps::qtGetName(out, cap);
}

extern "C" void WBQtObjectProps_SetName(const char *name)
{
	MapObjectProps::qtSetName(name);
}

extern "C" int WBQtObjectProps_GetTeamCount(void)
{
	return MapObjectProps::qtGetTeamCount();
}

extern "C" int WBQtObjectProps_GetTeamName(int i, char *out, int cap)
{
	return MapObjectProps::qtGetTeamName(i, out, cap);
}

extern "C" int WBQtObjectProps_GetCurTeam(void)
{
	return MapObjectProps::qtGetCurTeam();
}

extern "C" void WBQtObjectProps_SetTeam(int i)
{
	MapObjectProps::qtSetTeam(i);
}

#endif // RTS_HAS_QT
