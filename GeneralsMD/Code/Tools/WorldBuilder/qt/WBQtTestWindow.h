// WBQtTestWindow.h -- Phase 1 proof-of-life window for the MFC -> Qt migration.
//
// A standalone, top-level Qt window shown alongside the running MFC WorldBuilder.
// It touches nothing MFC on purpose: if it paints and its controls respond while the
// MFC UI stays live, the Qt and MFC event loops are confirmed coexisting in one
// process. It also hosts the theme selector (System/Dark/Light) so dark-mode support
// is demonstrably selectable. This whole file is replaced by real migrated UI later.
//
// Pure Qt (no windows.h / no afx), so the normal Qt keywords are used directly.
#ifndef WB_QT_TEST_WINDOW_H
#define WB_QT_TEST_WINDOW_H

#include <QMainWindow>

class WBQtTestWindow : public QMainWindow
{
	Q_OBJECT

public:
	WBQtTestWindow(QWidget *parent = NULL);

private slots:
	void onPingClicked();
	void onThemeChanged(int index);
};

#endif // WB_QT_TEST_WINDOW_H
