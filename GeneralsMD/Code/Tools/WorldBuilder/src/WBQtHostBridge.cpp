// WBQtHostBridge.cpp -- the MFC side of the Phase 2 viewport-host seam.
//
// This is a plain MFC translation unit (no Qt include). It implements the reverse
// callback that the Qt viewport host (qt/WBQtBridge.cpp) fires on resize, so the D3D
// device tracks the on-screen host size. It lives on the MFC side because
// reset3dEngineDisplaySize is engine/MFC-facing; the Qt static lib resolves this symbol
// against the exe at the final link (extern "C" keeps the name stable).
//
// The whole body is guarded by RTS_HAS_QT, so in the default (Qt-off) build this
// compiles to an empty object and the MFC build is unchanged.
#include "StdAfx.h"
#include "MainFrm.h"
#include "WorldBuilderDoc.h"
#include "wbview3d.h"
#include "qt/WBQtBridge.h"

#ifdef RTS_HAS_QT
extern "C" void WBQt_OnViewportHostResized(int width, int height)
{
	if (width <= 0 || height <= 0)
	{
		return;
	}

	WbView3d *p3d = CWorldBuilderDoc::GetActive3DView();
	if (p3d != NULL)
	{
		// Idempotent: reset3dEngineDisplaySize early-outs when the size is unchanged and
		// no-ops until the device is inited, so an early/duplicate call is harmless.
		p3d->reset3dEngineDisplaySize(width, height);
	}
}
#endif
