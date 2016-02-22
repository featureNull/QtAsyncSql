#include "mainwindow.h"
#include <QApplication>

#include "Database/ConnectionManager.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	Database::ConnectionManager *mgr = Database::ConnectionManager::createInstance();
	mgr->setType("QSQLITE");
	mgr->setDatabaseName(QApplication::applicationDirPath() + "/data/Northwind.sl3");


	MainWindow w;
	w.show();

	int ret = a.exec();

	Database::ConnectionManager::destroyInstance();

	return ret;
}
