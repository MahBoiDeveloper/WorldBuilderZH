// WBQtTestWindow.cpp -- see WBQtTestWindow.h.
#include "WBQtTestWindow.h"
#include "WBQtTheme.h"

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

WBQtTestWindow::WBQtTestWindow(QWidget *parent)
	: QMainWindow(parent)
{
	setWindowTitle("WorldBuilder Qt (Phase 1)");

	QWidget *central = new QWidget(this);
	QVBoxLayout *layout = new QVBoxLayout(central);

	QLabel *label = new QLabel("Qt event loop is live inside WorldBuilder.", central);
	layout->addWidget(label);

	// Theme selector: System / Dark / Light, persisted and applied live.
	QComboBox *theme = new QComboBox(central);
	theme->addItem("System (follow Windows)", (int)WBQtTheme::ModeSystem);
	theme->addItem("Dark", (int)WBQtTheme::ModeDark);
	theme->addItem("Light", (int)WBQtTheme::ModeLight);
	theme->setCurrentIndex((int)WBQtTheme::mode());	// items are in Mode order
	// Connect AFTER setting the initial index so construction doesn't re-trigger apply.
	connect(theme, SIGNAL(currentIndexChanged(int)), this, SLOT(onThemeChanged(int)));

	QFormLayout *form = new QFormLayout();
	form->addRow("Theme:", theme);
	layout->addLayout(form);

	QPushButton *button = new QPushButton("Ping", central);
	layout->addWidget(button);
	connect(button, SIGNAL(clicked()), this, SLOT(onPingClicked()));

	setCentralWidget(central);
	resize(360, 170);
}

void WBQtTestWindow::onPingClicked()
{
	QMessageBox::information(this, "WorldBuilder Qt",
		"Pong -- Qt is processing events alongside MFC.");
}

void WBQtTestWindow::onThemeChanged(int index)
{
	// Combo item order matches the Mode enum (System=0, Dark=1, Light=2).
	WBQtTheme::setMode((WBQtTheme::Mode)index);
}
