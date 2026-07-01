// WBQtObjectPropsPanel.h -- Qt replacement for the MFC MapObjectProps dialog.
//
// The big object-properties panel: edits the Dict of the selected map object(s) via the hidden
// MFC MapObjectProps dialog (TheMapObjectProps), which stays the owner of the working Dict(s) and
// the toggle-OFF fallback. This is the RTS_HAS_QT path -- a top-level Qt::Tool window owned by the
// shared QWinWidget bridge. It has no state of its own; every control reads/writes through the
// WBQtObjectProps_* facade. MapObjectProps::updateTheUI() re-seeds it via WBQtObjectProps_PushRefresh().
//
// Phase 1: General section only (object name, owning team). Later phases add the Logical, Visual,
// Sound and build-with-upgrades sections.
#ifndef WB_QT_OBJECTPROPS_PANEL_H
#define WB_QT_OBJECTPROPS_PANEL_H

#include <QWidget>

class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;

class WBQtObjectPropsPanel : public QWidget
{
	Q_OBJECT

public:
	explicit WBQtObjectPropsPanel(QWidget *owner);

	// Re-seed every control from the current MFC selection (WBQtObjectProps_PushRefresh).
	void pushRefresh();

	static WBQtObjectPropsPanel *instance() { return s_instance; }

private slots:
	void onNameChanged();
	void onTeamChanged(int index);

private:
	void rebuildTeams();	// repopulate the team combo from the bridge

	QLabel    *m_selectionLabel;	// "No Selection" / "N objects" / the object name
	QGroupBox *m_generalBox;
	QLineEdit *m_name;
	QComboBox *m_team;

	bool m_updating;	// re-entrancy guard, mirrors MFC MapObjectProps::m_updating

	static WBQtObjectPropsPanel *s_instance;
};

#endif // WB_QT_OBJECTPROPS_PANEL_H
