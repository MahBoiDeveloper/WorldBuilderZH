// WBQtBridge.cpp -- implements the MFC <-> Qt seam declared in WBQtBridge.h.
//
// This file (and the vendored qmfcapp.cpp) are the only places Qt is touched in
// WorldBuilder during Phase 1. MFC keeps owning the message loop; Qt is pumped from
// inside CWinApp::Run() via QMfcApp::pluginInstance's WH_GETMESSAGE hook, so we never
// call QMfcApp::run() (which would replace the loop).
#include "WBQtBridge.h"

#include "qmfcapp.h"
#include "WBQtTestWindow.h"
#include "WBQtTheme.h"

#include <QApplication>

// Owns the Phase 1 proof window for the life of the process.
static WBQtTestWindow *g_wbQtTestWindow = NULL;

void WBQt_Startup(void)
{
	// pluginInstance(0): WorldBuilder is the executable itself, not a DLL, so there is
	// no module to pin -- 0 is the documented same-executable usage. This creates the
	// QApplication and installs the message hook that drives Qt from MFC's loop.
	QMfcApp::pluginInstance(0);

	// Apply dark-mode support before any Qt window is created so it inherits the theme.
	WBQtTheme::applyApplicationTheme();

	if (g_wbQtTestWindow == NULL)
	{
		g_wbQtTestWindow = new WBQtTestWindow();
	}
	g_wbQtTestWindow->show();
}

void WBQt_Shutdown(void)
{
	if (g_wbQtTestWindow != NULL)
	{
		delete g_wbQtTestWindow;
		g_wbQtTestWindow = NULL;
	}

	// Explicit teardown: the global CWorldBuilderApp dtor calls _exit(0) immediately
	// after ExitInstance returns, so nothing static/atexit would ever run. Deleting
	// qApp here uninstalls the WH_GETMESSAGE hook and releases Qt cleanly first.
	delete qApp;
}
