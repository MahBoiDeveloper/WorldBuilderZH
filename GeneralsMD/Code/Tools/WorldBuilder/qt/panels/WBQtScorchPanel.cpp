// WBQtScorchPanel.cpp -- see WBQtScorchPanel.h.
#include "WBQtScorchPanel.h"
#include "WBQtScorchBridge.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

WBQtScorchPanel *WBQtScorchPanel::s_instance = NULL;

// The size popup-slider in the MFC panel runs 0..256 (see ScorchOptions::GetPopSliderInfo); the
// edit box stored a Real (objectRadius). Match that range with a double spin box.
static const double kScorchSizeMin = 0.0;
static const double kScorchSizeMax = 256.0;

WBQtScorchPanel::WBQtScorchPanel(QWidget *owner)
	: QWidget(owner, Qt::Tool),
	  m_updating(false)
{
	setWindowTitle("Scorch Options");

	QVBoxLayout *root = new QVBoxLayout(this);

	// The "Scorch" group box, matching the MFC dialog's single group.
	QGroupBox *scorchBox = new QGroupBox("Scorch", this);
	QVBoxLayout *scorchLay = new QVBoxLayout(scorchBox);

	// Scorch-type combo (IDC_SCORCHTYPE). Populated from the fixed .rc entry list via the bridge.
	m_type = new QComboBox(scorchBox);
	scorchLay->addWidget(m_type);

	// Scorch size (IDC_SIZE_EDIT + IDC_SIZE_POPUP collapse into one spin box).
	QHBoxLayout *sizeRow = new QHBoxLayout();
	sizeRow->addWidget(new QLabel("Scorch Size:", scorchBox));
	m_size = new QDoubleSpinBox(scorchBox);
	m_size->setRange(kScorchSizeMin, kScorchSizeMax);
	m_size->setDecimals(2);
	m_size->setSingleStep(1.0);
	sizeRow->addWidget(m_size, 1);
	scorchLay->addLayout(sizeRow);

	root->addWidget(scorchBox);
	root->addStretch(1);

	// Seed the combo + values under the guard so nothing echoes back while we populate.
	m_updating = true;
	const int cap = 256;
	char nameBuf[cap];
	int typeCount = WBQtScorch_GetTypeCount();
	for (int i = 0; i < typeCount; ++i)
	{
		if (WBQtScorch_GetTypeName(i, nameBuf, cap))
		{
			m_type->addItem(QString::fromLatin1(nameBuf));
		}
		else
		{
			m_type->addItem(QString());
		}
	}
	int curType = WBQtScorch_GetType();
	if (curType >= 0 && curType < m_type->count())
	{
		m_type->setCurrentIndex(curType);
	}
	m_size->setValue(WBQtScorch_GetSize());
	m_updating = false;

	connect(m_type, SIGNAL(currentIndexChanged(int)), this, SLOT(onTypeChanged(int)));
	connect(m_size, SIGNAL(valueChanged(double)), this, SLOT(onSizeChanged(double)));

	s_instance = this;
}

void WBQtScorchPanel::onTypeChanged(int index)
{
	if (m_updating)
	{
		return;
	}
	WBQtScorch_SetType(index);
}

void WBQtScorchPanel::onSizeChanged(double v)
{
	if (m_updating)
	{
		return;
	}
	WBQtScorch_SetSize(v);
}

void WBQtScorchPanel::pushRefresh()
{
	// ScorchOptions::update() re-read the statics from the selected scorch; mirror them.
	m_updating = true;
	int curType = WBQtScorch_GetType();
	if (curType >= 0 && curType < m_type->count())
	{
		m_type->setCurrentIndex(curType);
	}
	m_size->setValue(WBQtScorch_GetSize());
	m_updating = false;
}

// --- Forward push function (MFC ScorchOptions::update -> widget), Qt-side of the bridge -----
extern "C" void WBQtScorch_PushRefresh(void)
{
	if (WBQtScorchPanel::instance() != NULL)
	{
		WBQtScorchPanel::instance()->pushRefresh();
	}
}
