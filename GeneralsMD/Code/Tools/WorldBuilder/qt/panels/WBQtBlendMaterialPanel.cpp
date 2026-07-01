// WBQtBlendMaterialPanel.cpp -- see WBQtBlendMaterialPanel.h.
#include "WBQtBlendMaterialPanel.h"
#include "WBQtBlendMaterialBridge.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

WBQtBlendMaterialPanel *WBQtBlendMaterialPanel::s_instance = NULL;

WBQtBlendMaterialPanel::WBQtBlendMaterialPanel(QWidget *owner)
	: QWidget(owner, Qt::Tool),
	  m_updating(false)
{
	setWindowTitle("Blend Material Options");

	QVBoxLayout *root = new QVBoxLayout(this);

	// Tile Gap Options: the two "add one tile between gaps" checkboxes (IDC_HVGAP / IDC_DGAP).
	QGroupBox *gapBox = new QGroupBox("Tile Gap Options", this);
	QVBoxLayout *gapLay = new QVBoxLayout(gapBox);
	m_hvGap = new QCheckBox("Add one tile between horizontal and vertical gaps", gapBox);
	m_dGap = new QCheckBox("Add one tile between diagonal gaps", gapBox);
	gapLay->addWidget(m_hvGap);
	gapLay->addWidget(m_dGap);
	root->addWidget(gapBox);

	// Advanced Blend Options: the re-revalidate-blends checkbox (IDC_REVALIDATEBLENDS).
	QGroupBox *advBox = new QGroupBox("Advanced Blend Options", this);
	QVBoxLayout *advLay = new QVBoxLayout(advBox);
	m_revalBlends = new QCheckBox("Re-revalidate Blends (3x3 around cursor)", advBox);
	advLay->addWidget(m_revalBlends);
	root->addWidget(advBox);

	// Developer note (static text in the MFC dialog).
	QGroupBox *noteBox = new QGroupBox("Developer Note:", this);
	QVBoxLayout *noteLay = new QVBoxLayout(noteBox);
	noteLay->addWidget(new QLabel("Hold shift then click at a texture to unblend them", noteBox));
	root->addWidget(noteBox);

	// Advanced Mirror Options: the four mirror toggles (IDC_TOGGLE_MIRROR/X/Y/XY). These drive
	// AutoEdgeOutTool's mirror statics, the same tool the Feather panel's mirror row drives.
	QGroupBox *mirrorBox = new QGroupBox("Advanced Mirror Options", this);
	QVBoxLayout *mirrorLay = new QVBoxLayout(mirrorBox);
	mirrorLay->addWidget(new QLabel(
		"Warning: On large maps, this may result in noticeable slowdowns, especially near the edges.",
		mirrorBox));
	m_mirror = new QCheckBox("Toggle", mirrorBox);
	m_mirrorX = new QCheckBox("Mirror X", mirrorBox);
	m_mirrorY = new QCheckBox("Mirror Y", mirrorBox);
	m_mirrorXY = new QCheckBox("Diagonal", mirrorBox);
	mirrorLay->addWidget(m_mirror);
	mirrorLay->addWidget(m_mirrorX);
	mirrorLay->addWidget(m_mirrorY);
	mirrorLay->addWidget(m_mirrorXY);
	root->addWidget(mirrorBox);

	root->addStretch(1);

	// Seed from the current tool state under the guard so it doesn't echo back to the tool.
	m_updating = true;
	m_hvGap->setChecked(WBQtBlendMaterial_GetHorizVertGap() != 0);
	m_dGap->setChecked(WBQtBlendMaterial_GetDiagGap() != 0);
	m_revalBlends->setChecked(WBQtBlendMaterial_GetRevalBlends() != 0);
	m_mirror->setChecked(WBQtBlendMaterial_GetMirror() != 0);
	m_mirrorX->setChecked(WBQtBlendMaterial_GetMirrorX() != 0);
	m_mirrorY->setChecked(WBQtBlendMaterial_GetMirrorY() != 0);
	m_mirrorXY->setChecked(WBQtBlendMaterial_GetMirrorXY() != 0);
	m_updating = false;

	// Checkboxes use clicked() so the programmatic setChecked() during seeding doesn't fire
	// the tool.
	connect(m_hvGap, SIGNAL(clicked()), this, SLOT(onHorizVertGapToggled()));
	connect(m_dGap, SIGNAL(clicked()), this, SLOT(onDiagGapToggled()));
	connect(m_revalBlends, SIGNAL(clicked()), this, SLOT(onRevalBlendsToggled()));
	connect(m_mirror, SIGNAL(clicked()), this, SLOT(onMirror()));
	connect(m_mirrorX, SIGNAL(clicked()), this, SLOT(onMirrorX()));
	connect(m_mirrorY, SIGNAL(clicked()), this, SLOT(onMirrorY()));
	connect(m_mirrorXY, SIGNAL(clicked()), this, SLOT(onMirrorXY()));

	s_instance = this;
}

// The three gap checkboxes push their explicit state (the setters also refresh the tooltip).
void WBQtBlendMaterialPanel::onHorizVertGapToggled()
{
	if (m_updating)
	{
		return;
	}
	WBQtBlendMaterial_SetHorizVertGap(m_hvGap->isChecked() ? 1 : 0);
}

void WBQtBlendMaterialPanel::onDiagGapToggled()
{
	if (m_updating)
	{
		return;
	}
	WBQtBlendMaterial_SetDiagGap(m_dGap->isChecked() ? 1 : 0);
}

void WBQtBlendMaterialPanel::onRevalBlendsToggled()
{
	if (m_updating)
	{
		return;
	}
	WBQtBlendMaterial_SetRevalBlends(m_revalBlends->isChecked() ? 1 : 0);
}

// The four mirror checkboxes toggle the tool statics, matching the MFC OnToggleMirror*.
void WBQtBlendMaterialPanel::onMirror()
{
	WBQtBlendMaterial_ToggleMirror();
}

void WBQtBlendMaterialPanel::onMirrorX()
{
	WBQtBlendMaterial_ToggleMirrorX();
}

void WBQtBlendMaterialPanel::onMirrorY()
{
	WBQtBlendMaterial_ToggleMirrorY();
}

void WBQtBlendMaterialPanel::onMirrorXY()
{
	WBQtBlendMaterial_ToggleMirrorXY();
}
