// WBQtTerrainModalDialog.h -- the native Qt "missing terrain texture" picker (Tier 5b),
// rebuilding TerrainModal over the WBQtTerrainModalBridge catalog; the swatch preview
// reuses the WBQtTerrainMaterial pixel bridge. Run via WBQtTerrainModal_Run.
#ifndef WB_QT_TERRAINMODAL_DIALOG_H
#define WB_QT_TERRAINMODAL_DIALOG_H

#include <QDialog>

class QLabel;
class QTreeWidget;
class QTreeWidgetItem;

class WBQtTerrainModalDialog : public QDialog
{
	Q_OBJECT
public:
	WBQtTerrainModalDialog(const QString &missingPath, QWidget *parent = 0);

	int pickedIndex() const { return m_picked; }

private slots:
	void onCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
	void selectTexClass(int texClass);
	void refreshPreview(int texClass);

	int m_picked;
	QTreeWidget *m_tree;
	QLabel *m_nameLabel;
	QLabel *m_preview;
};

#endif // WB_QT_TERRAINMODAL_DIALOG_H
