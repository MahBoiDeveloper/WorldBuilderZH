// WBQtBridge.h -- the seam between the MFC application and the Qt world.
//
// This header is deliberately a plain facade: NO Qt and NO MFC types, so the big
// MFC translation unit (WorldBuilder.cpp) can call into Qt without ever including a
// Qt header (which would drag Qt's keywords/macros into an MFC+windows.h compile).
// All the Qt mixing lives in WBQtBridge.cpp and the vendored qmfcapp.cpp.
//
// Phase 1 of the MFC -> Qt migration: bring up a Qt event loop that coexists with
// MFC's, and show one proof window. See qt/3rdparty/qtwinmigrate for the loop merge.
#ifndef WB_QT_BRIDGE_H
#define WB_QT_BRIDGE_H

// Create the QApplication (hooked into MFC's existing message loop) and show the
// Phase 1 proof window. Safe to call once, after the MFC main window exists.
void WBQt_Startup(void);

// Destroy the proof window and the QApplication. Must be called explicitly from
// CWorldBuilderApp::ExitInstance: the app's global dtor calls _exit(0) right after,
// so static/atexit teardown never runs.
void WBQt_Shutdown(void);

#endif // WB_QT_BRIDGE_H
