// WBQtObjectPropsPanel.cpp -- see WBQtObjectPropsPanel.h.
#include "WBQtObjectPropsPanel.h"
#include "WBQtObjectPropsBridge.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

WBQtObjectPropsPanel *WBQtObjectPropsPanel::s_instance = NULL;

WBQtObjectPropsPanel::WBQtObjectPropsPanel(QWidget *owner)
	: QWidget(owner, Qt::Tool),
	  m_updating(false)
{
	setWindowTitle("Object Properties");
	resize(340, 520);

	QVBoxLayout *root = new QVBoxLayout(this);

	// Which object(s) the panel is editing.
	m_selectionLabel = new QLabel("No Selection", this);
	root->addWidget(m_selectionLabel);

	// General section: object name (single-select only) + owning team.
	m_generalBox = new QGroupBox("General", this);
	QVBoxLayout *genLay = new QVBoxLayout(m_generalBox);

	QHBoxLayout *nameRow = new QHBoxLayout();
	nameRow->addWidget(new QLabel("Name:", m_generalBox));
	m_name = new QLineEdit(m_generalBox);
	nameRow->addWidget(m_name, 1);
	genLay->addLayout(nameRow);

	QHBoxLayout *teamRow = new QHBoxLayout();
	teamRow->addWidget(new QLabel("Team:", m_generalBox));
	m_team = new QComboBox(m_generalBox);
	teamRow->addWidget(m_team, 1);
	genLay->addLayout(teamRow);

	root->addWidget(m_generalBox);

	// Logical section. Matches the MFC dialog: a left column of combos (Initial Health, Max HP,
	// Aggressiveness, Veterancy) and a right column of the seven object flags.
	m_logicalBox = new QGroupBox("Logical", this);
	QHBoxLayout *logLay = new QHBoxLayout(m_logicalBox);

	// Left column: the four combos.
	QGridLayout *leftGrid = new QGridLayout();

	leftGrid->addWidget(new QLabel("Initial Health %", m_logicalBox), 0, 0);
	QHBoxLayout *healthRow = new QHBoxLayout();
	m_health = new QComboBox(m_logicalBox);
	m_health->addItem("Dead");
	m_health->addItem("25%");
	m_health->addItem("50%");
	m_health->addItem("75%");
	m_health->addItem("100%");
	m_health->addItem("Other");
	healthRow->addWidget(m_health);
	m_healthEdit = new QLineEdit(m_logicalBox);
	m_healthEdit->setMaximumWidth(48);
	healthRow->addWidget(m_healthEdit);
	leftGrid->addLayout(healthRow, 0, 1);

	leftGrid->addWidget(new QLabel("Max HP", m_logicalBox), 1, 0);
	m_maxHPs = new QComboBox(m_logicalBox);
	m_maxHPs->setEditable(true);
	m_maxHPs->addItem("Default For Unit");
	leftGrid->addWidget(m_maxHPs, 1, 1);

	leftGrid->addWidget(new QLabel("Aggressiveness:", m_logicalBox), 2, 0);
	m_aggressiveness = new QComboBox(m_logicalBox);
	m_aggressiveness->addItem("Sleep");
	m_aggressiveness->addItem("Passive");
	m_aggressiveness->addItem("Normal");
	m_aggressiveness->addItem("Alert");
	m_aggressiveness->addItem("Aggressive");
	leftGrid->addWidget(m_aggressiveness, 2, 1);

	leftGrid->addWidget(new QLabel("Veterancy:", m_logicalBox), 3, 0);
	m_veterancy = new QComboBox(m_logicalBox);
	m_veterancy->addItem("Normal");
	m_veterancy->addItem("Veteran");
	m_veterancy->addItem("Elite");
	m_veterancy->addItem("Heroic");
	leftGrid->addWidget(m_veterancy, 3, 1);
	logLay->addLayout(leftGrid, 1);

	// Right column: the seven object flags, in the MFC order.
	QVBoxLayout *flagCol = new QVBoxLayout();
	m_enabled        = new QCheckBox("Enabled", m_logicalBox);
	m_unsellable     = new QCheckBox("Unsellable", m_logicalBox);
	m_targetable     = new QCheckBox("Targetable", m_logicalBox);
	m_indestructible = new QCheckBox("Indestructible", m_logicalBox);
	m_recruitableAI  = new QCheckBox("AI Recruitable", m_logicalBox);
	m_powered        = new QCheckBox("Powered", m_logicalBox);
	m_selectable     = new QCheckBox("Selectable", m_logicalBox);
	m_selectable->setTristate(true);
	flagCol->addWidget(m_enabled);
	flagCol->addWidget(m_unsellable);
	flagCol->addWidget(m_targetable);
	flagCol->addWidget(m_indestructible);
	flagCol->addWidget(m_recruitableAI);
	flagCol->addWidget(m_powered);
	flagCol->addWidget(m_selectable);
	flagCol->addStretch(1);
	logLay->addLayout(flagCol);

	root->addWidget(m_logicalBox);

	// Distance box: Stopping / Targeting / Shroud, matching the MFC "Distance" groupbox. Note the
	// "Targeting" field edits the object's vision/visual range (TheKey_objectVisualRange).
	m_distanceBox = new QGroupBox("Distance", this);
	QHBoxLayout *distLay = new QHBoxLayout(m_distanceBox);
	distLay->addWidget(new QLabel("Stopping:", m_distanceBox));
	m_stopping = new QLineEdit(m_distanceBox);
	m_stopping->setMaximumWidth(48);
	distLay->addWidget(m_stopping);
	distLay->addWidget(new QLabel("Targeting:", m_distanceBox));
	m_targeting = new QLineEdit(m_distanceBox);
	m_targeting->setMaximumWidth(48);
	distLay->addWidget(m_targeting);
	distLay->addWidget(new QLabel("Shroud:", m_distanceBox));
	m_shroud = new QLineEdit(m_distanceBox);
	m_shroud->setMaximumWidth(48);
	distLay->addWidget(m_shroud);
	distLay->addStretch(1);
	root->addWidget(m_distanceBox);
	root->addStretch(1);

	// Seed from the current selection under the guard.
	pushRefresh();

	connect(m_name, SIGNAL(editingFinished()), this, SLOT(onNameChanged()));
	connect(m_team, SIGNAL(currentIndexChanged(int)), this, SLOT(onTeamChanged(int)));
	connect(m_health, SIGNAL(currentIndexChanged(int)), this, SLOT(onHealthChanged(int)));
	connect(m_healthEdit, SIGNAL(editingFinished()), this, SLOT(onHealthEditChanged()));
	connect(m_maxHPs, SIGNAL(editTextChanged(const QString &)), this, SLOT(onMaxHPsChanged()));
	connect(m_aggressiveness, SIGNAL(currentIndexChanged(int)), this, SLOT(onAggressivenessChanged(int)));
	connect(m_veterancy, SIGNAL(currentIndexChanged(int)), this, SLOT(onVeterancyChanged(int)));
	connect(m_enabled, SIGNAL(clicked()), this, SLOT(onFlagToggled()));
	connect(m_indestructible, SIGNAL(clicked()), this, SLOT(onFlagToggled()));
	connect(m_unsellable, SIGNAL(clicked()), this, SLOT(onFlagToggled()));
	connect(m_targetable, SIGNAL(clicked()), this, SLOT(onFlagToggled()));
	connect(m_powered, SIGNAL(clicked()), this, SLOT(onFlagToggled()));
	connect(m_recruitableAI, SIGNAL(clicked()), this, SLOT(onFlagToggled()));
	connect(m_selectable, SIGNAL(clicked()), this, SLOT(onFlagToggled()));
	connect(m_targeting, SIGNAL(editingFinished()), this, SLOT(onTargetingChanged()));
	connect(m_shroud, SIGNAL(editingFinished()), this, SLOT(onShroudChanged()));
	connect(m_stopping, SIGNAL(editingFinished()), this, SLOT(onStoppingChanged()));

	s_instance = this;
}

void WBQtObjectPropsPanel::rebuildTeams()
{
	// Caller has set m_updating (repopulation must not fire onTeamChanged).
	m_team->clear();
	const int cap = 256;
	char buf[cap];
	int count = WBQtObjectProps_GetTeamCount();
	for (int i = 0; i < count; ++i)
	{
		if (WBQtObjectProps_GetTeamName(i, buf, cap))
		{
			m_team->addItem(QString::fromLatin1(buf));
		}
		else
		{
			m_team->addItem(QString());
		}
	}
}

void WBQtObjectPropsPanel::pushRefresh()
{
	m_updating = true;

	int selCount = WBQtObjectProps_GetSelCount();
	bool single = (WBQtObjectProps_HasSelection() != 0);

	const int cap = 256;
	char buf[cap];

	// Selection summary + name field. The name is single-select only (matches the MFC panel).
	if (selCount == 0)
	{
		m_selectionLabel->setText("No Selection");
	}
	else if (selCount > 1)
	{
		m_selectionLabel->setText(QString("%1 objects selected").arg(selCount));
	}
	else if (WBQtObjectProps_GetName(buf, cap) && buf[0] != 0)
	{
		m_selectionLabel->setText(QString::fromLatin1(buf));
	}
	else
	{
		m_selectionLabel->setText("1 object selected");
	}

	if (single && WBQtObjectProps_GetName(buf, cap))
	{
		m_name->setText(QString::fromLatin1(buf));
	}
	else
	{
		m_name->clear();
	}
	m_name->setEnabled(single);

	// Team combo: always available for any selection (applies to all selected objects).
	rebuildTeams();
	int curTeam = WBQtObjectProps_GetCurTeam();
	if (curTeam >= 0 && curTeam < m_team->count())
	{
		m_team->setCurrentIndex(curTeam);
	}
	m_team->setEnabled(selCount > 0);
	m_generalBox->setEnabled(selCount > 0);

	// --- Logical section ---------------------------------------------------------------------
	// Health: pick the matching preset or fall to "Other" + the explicit value.
	int health = WBQtObjectProps_GetHealthPercent();
	int healthIndex = -1;
	if (health == 0)        { healthIndex = 0; }
	else if (health == 25)  { healthIndex = 1; }
	else if (health == 50)  { healthIndex = 2; }
	else if (health == 75)  { healthIndex = 3; }
	else if (health == 100) { healthIndex = 4; }
	if (healthIndex >= 0)
	{
		m_health->setCurrentIndex(healthIndex);
		m_healthEdit->clear();
		m_healthEdit->setEnabled(false);
	}
	else
	{
		m_health->setCurrentIndex(5);	// Other
		m_healthEdit->setText(QString::number(health));
		m_healthEdit->setEnabled(true);
	}

	// Max hit points: rebuild so the explicit value is the second entry (like the MFC combo).
	m_maxHPs->clear();
	m_maxHPs->addItem("Default For Unit");
	int hps = WBQtObjectProps_GetMaxHPs();
	if (hps > 0)
	{
		m_maxHPs->addItem(QString::number(hps));
		m_maxHPs->setCurrentIndex(1);
	}
	else
	{
		m_maxHPs->setCurrentIndex(0);
	}

	// Aggressiveness value (-2..2) maps to combo index 0..4.
	int agg = WBQtObjectProps_GetAggressiveness();
	int aggIndex = agg + 2;
	if (aggIndex >= 0 && aggIndex < m_aggressiveness->count())
	{
		m_aggressiveness->setCurrentIndex(aggIndex);
	}

	int vet = WBQtObjectProps_GetVeterancy();
	if (vet >= 0 && vet < m_veterancy->count())
	{
		m_veterancy->setCurrentIndex(vet);
	}

	m_enabled->setChecked(WBQtObjectProps_GetFlag(WBQT_OBJPROP_FLAG_ENABLED) != 0);
	m_indestructible->setChecked(WBQtObjectProps_GetFlag(WBQT_OBJPROP_FLAG_INDESTRUCTIBLE) != 0);
	m_unsellable->setChecked(WBQtObjectProps_GetFlag(WBQT_OBJPROP_FLAG_UNSELLABLE) != 0);
	m_targetable->setChecked(WBQtObjectProps_GetFlag(WBQT_OBJPROP_FLAG_TARGETABLE) != 0);
	m_powered->setChecked(WBQtObjectProps_GetFlag(WBQT_OBJPROP_FLAG_POWERED) != 0);
	m_recruitableAI->setChecked(WBQtObjectProps_GetFlag(WBQT_OBJPROP_FLAG_RECRUITABLEAI) != 0);
	// Selectable is tri-state: 2 == the "default" (key absent) partial state.
	int sel = WBQtObjectProps_GetFlag(WBQT_OBJPROP_FLAG_SELECTABLE);
	m_selectable->setCheckState(sel == 2 ? Qt::PartiallyChecked
		: (sel == 1 ? Qt::Checked : Qt::Unchecked));

	// Distance box. "Targeting" is the vision/visual range; blank when unset (0), like the MFC edits.
	int targeting = WBQtObjectProps_GetVisionDistance();
	m_targeting->setText(targeting > 0 ? QString::number(targeting) : QString());
	int shroud = WBQtObjectProps_GetShroudClearingDistance();
	m_shroud->setText(shroud > 0 ? QString::number(shroud) : QString());
	m_stopping->setText(QString::number(WBQtObjectProps_GetStoppingDistance(), 'g'));

	m_logicalBox->setEnabled(selCount > 0);
	m_distanceBox->setEnabled(selCount > 0);

	m_updating = false;
}

void WBQtObjectPropsPanel::onNameChanged()
{
	if (m_updating)
	{
		return;
	}
	WBQtObjectProps_SetName(m_name->text().toLatin1().constData());
}

void WBQtObjectPropsPanel::onTeamChanged(int index)
{
	if (m_updating)
	{
		return;
	}
	if (index >= 0)
	{
		WBQtObjectProps_SetTeam(index);
	}
}

// --- Logical section slots ------------------------------------------------------------------

void WBQtObjectPropsPanel::onHealthChanged(int index)
{
	if (m_updating)
	{
		return;
	}
	// Index 0..4 == 0/25/50/75/100%; index 5 == Other (take the value from the edit box).
	int value = 100;
	switch (index)
	{
		case 0: value = 0;   break;
		case 1: value = 25;  break;
		case 2: value = 50;  break;
		case 3: value = 75;  break;
		case 4: value = 100; break;
		case 5:
		{
			// Enable the Other edit; if it's empty use 100 for now (the user will type a value,
			// which fires onHealthEditChanged). Don't overwrite an existing value.
			bool ok = false;
			int typed = m_healthEdit->text().toInt(&ok);
			value = ok ? typed : 100;
			break;
		}
		default: value = 100; break;
	}
	m_updating = true;
	m_healthEdit->setEnabled(index == 5);
	if (index != 5)
	{
		m_healthEdit->clear();
	}
	m_updating = false;
	WBQtObjectProps_SetHealthPercent(value);
}

void WBQtObjectPropsPanel::onHealthEditChanged()
{
	if (m_updating)
	{
		return;
	}
	// Only meaningful while "Other" is selected.
	if (m_health->currentIndex() != 5)
	{
		return;
	}
	bool ok = false;
	int typed = m_healthEdit->text().toInt(&ok);
	if (ok)
	{
		WBQtObjectProps_SetHealthPercent(typed);
	}
}

void WBQtObjectPropsPanel::onMaxHPsChanged()
{
	if (m_updating)
	{
		return;
	}
	QString text = m_maxHPs->currentText();
	if (text == "Default For Unit")
	{
		WBQtObjectProps_SetMaxHPs(-1);
		return;
	}
	bool ok = false;
	int hps = text.toInt(&ok);
	WBQtObjectProps_SetMaxHPs(ok ? hps : -1);
}

void WBQtObjectPropsPanel::onAggressivenessChanged(int index)
{
	if (m_updating)
	{
		return;
	}
	if (index >= 0)
	{
		// Combo index 0..4 -> value -2..2.
		WBQtObjectProps_SetAggressiveness(index - 2);
	}
}

void WBQtObjectPropsPanel::onVeterancyChanged(int index)
{
	if (m_updating)
	{
		return;
	}
	if (index >= 0)
	{
		WBQtObjectProps_SetVeterancy(index);
	}
}

void WBQtObjectPropsPanel::applyFlag(int flagId, QCheckBox *box)
{
	int state = 0;
	if (box->checkState() == Qt::PartiallyChecked)
	{
		state = 2;	// Selectable's tri-state "default"
	}
	else if (box->checkState() == Qt::Checked)
	{
		state = 1;
	}
	WBQtObjectProps_SetFlag(flagId, state);
}

void WBQtObjectPropsPanel::onFlagToggled()
{
	if (m_updating)
	{
		return;
	}
	QObject *src = sender();
	if (src == m_enabled)             { applyFlag(WBQT_OBJPROP_FLAG_ENABLED, m_enabled); }
	else if (src == m_indestructible) { applyFlag(WBQT_OBJPROP_FLAG_INDESTRUCTIBLE, m_indestructible); }
	else if (src == m_unsellable)     { applyFlag(WBQT_OBJPROP_FLAG_UNSELLABLE, m_unsellable); }
	else if (src == m_targetable)     { applyFlag(WBQT_OBJPROP_FLAG_TARGETABLE, m_targetable); }
	else if (src == m_powered)        { applyFlag(WBQT_OBJPROP_FLAG_POWERED, m_powered); }
	else if (src == m_recruitableAI)  { applyFlag(WBQT_OBJPROP_FLAG_RECRUITABLEAI, m_recruitableAI); }
	else if (src == m_selectable)     { applyFlag(WBQT_OBJPROP_FLAG_SELECTABLE, m_selectable); }
}

void WBQtObjectPropsPanel::onTargetingChanged()
{
	if (m_updating)
	{
		return;
	}
	// "Targeting" edits the object's vision/visual range. Blank == unset (0).
	bool ok = false;
	int v = m_targeting->text().toInt(&ok);
	WBQtObjectProps_SetVisionDistance(ok ? v : 0);
}

void WBQtObjectPropsPanel::onShroudChanged()
{
	if (m_updating)
	{
		return;
	}
	bool ok = false;
	int v = m_shroud->text().toInt(&ok);
	WBQtObjectProps_SetShroudClearingDistance(ok ? v : 0);
}

void WBQtObjectPropsPanel::onStoppingChanged()
{
	if (m_updating)
	{
		return;
	}
	bool ok = false;
	double v = m_stopping->text().toDouble(&ok);
	if (ok)
	{
		WBQtObjectProps_SetStoppingDistance(v);
	}
}

// --- Forward push (MapObjectProps refresh -> widget), Qt side of WBQtObjectPropsBridge.h -----
extern "C" void WBQtObjectProps_PushRefresh(void)
{
	if (WBQtObjectPropsPanel::instance() != NULL)
	{
		WBQtObjectPropsPanel::instance()->pushRefresh();
	}
}
