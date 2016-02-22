#pragma once

#include <QMainWindow>

#include <Database/AsyncQueryResult.h>
#include <Database/AsyncQuery.h>

namespace Ui {
class MainWindow;
}

namespace Database {
	class AsyncQueryModel;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

public slots:
	void onBusyChanged(bool busy);

	void onComboBoxChanged (const QString &index);
	void onExec4Queries(bool clicked);
	void onClearClicked(bool clicked);
	void onExec4QueriesDone(const Database::AsyncQueryResult& result);
	void onSliderChanged(int value);

private:
	Ui::MainWindow *ui;

	QString str;

	Database::AsyncQueryModel *_tableModel;
	Database::AsyncQueryModel *_queryModel;
	Database::AsyncQuery *_aQuery;
	Database::AsyncQueryModel *_sliderModel;
};

