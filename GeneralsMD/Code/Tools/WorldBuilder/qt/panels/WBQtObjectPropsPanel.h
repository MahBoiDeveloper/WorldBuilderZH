// WBQtObjectPropsPanel.h -- Qt replacement for the MFC MapObjectProps dialog.
//
// The big object-properties panel: edits the Dict of the selected map object(s) via the hidden
// MFC MapObjectProps dialog (TheMapObjectProps), which stays the owner of the working Dict(s) and
// the toggle-OFF fallback. This is the RTS_HAS_QT path -- a top-level Qt::Tool window owned by the
// shared QWinWidget bridge. It has no state of its own; every control reads/writes through the
// WBQtObjectProps_* facade. MapObjectProps::updateTheUI() re-seeds it via WBQtObjectProps_PushRefresh().
//
// Phase 1: General section (object name, owning team).
// Phase 2: Logical section (starting health, hit points, aggressiveness, veterancy, the seven
// object flags, and the vision / shroud / stopping distances).
// Later phases add the Visual, Sound and build-with-upgrades sections.
#ifndef WB_QT_OBJECTPROPS_PANEL_H
#define WB_QT_OBJECTPROPS_PANEL_H

#include <QWidget>

class QCheckBox;
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
	// Logical section.
	void onHealthChanged(int index);
	void onHealthEditChanged();
	void onMaxHPsChanged();
	void onAggressivenessChanged(int index);
	void onVeterancyChanged(int index);
	void onFlagToggled();
	void onTargetingChanged();
	void onShroudChanged();
	void onStoppingChanged();

private:
	void rebuildTeams();	// repopulate the team combo from the bridge
	void applyFlag(int flagId, QCheckBox *box);

	QLabel    *m_selectionLabel;	// "No Selection" / "N objects" / the object name
	QGroupBox *m_generalBox;
	QLineEdit *m_name;
	QComboBox *m_team;

	// Logical section.
	QGroupBox      *m_logicalBox;
	QComboBox      *m_health;		// 0% / 25% / 50% / 75% / 100% / Other
	QLineEdit      *m_healthEdit;	// the "Other" value (enabled only when Other is selected)
	QComboBox      *m_maxHPs;		// "Default For Unit" or an explicit value
	QComboBox      *m_aggressiveness;
	QComboBox      *m_veterancy;
	QCheckBox      *m_enabled;
	QCheckBox      *m_indestructible;
	QCheckBox      *m_unsellable;
	QCheckBox      *m_targetable;
	QCheckBox      *m_powered;
	QCheckBox      *m_recruitableAI;
	QCheckBox      *m_selectable;	// tri-state (default = partially checked)
	QGroupBox      *m_distanceBox;
	QLineEdit      *m_stopping;		// Stopping distance (real; edit like the MFC ES_AUTOHSCROLL)
	QLineEdit      *m_targeting;	// "Targeting" == the object's vision/visual range (int)
	QLineEdit      *m_shroud;		// Shroud clearing distance (int)

	bool m_updating;	// re-entrancy guard, mirrors MFC MapObjectProps::m_updating

	static WBQtObjectPropsPanel *s_instance;
};

#endif // WB_QT_OBJECTPROPS_PANEL_H
