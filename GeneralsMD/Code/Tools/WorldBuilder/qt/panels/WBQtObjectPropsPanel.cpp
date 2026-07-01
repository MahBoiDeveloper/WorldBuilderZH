// WBQtObjectPropsPanel.cpp -- see WBQtObjectPropsPanel.h.
#include "WBQtObjectPropsPanel.h"
#include "WBQtObjectPropsBridge.h"

#include <QComboBox>
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
	resize(320, 200);

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
	root->addStretch(1);

	// Seed from the current selection under the guard.
	pushRefresh();

	connect(m_name, SIGNAL(editingFinished()), this, SLOT(onNameChanged()));
	connect(m_team, SIGNAL(currentIndexChanged(int)), this, SLOT(onTeamChanged(int)));

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

// --- Forward push (MapObjectProps refresh -> widget), Qt side of WBQtObjectPropsBridge.h -----
extern "C" void WBQtObjectProps_PushRefresh(void)
{
	if (WBQtObjectPropsPanel::instance() != NULL)
	{
		WBQtObjectPropsPanel::instance()->pushRefresh();
	}
}
