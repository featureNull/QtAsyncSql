#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMovie>

#include "Database/ConnectionManager.h"
#include "Database/AsyncQuery.h"
#include "Database/AsynqQueryModel.h"
#include "QComboBox"


#include "QSqlQuery"
#include "QStringListModel"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, _tableModel(nullptr)
{
	ui->setupUi(this);

	//Setup Tables tab
	connect (ui->cbTables, SIGNAL(currentIndexChanged(QString)),
			 this, SLOT(onComboBoxChanged(QString)));

	//fill combobox with table Strings use lambda expression
	Database::AsyncQuery::startExecOnce(
				"SELECT name FROM sqlite_master WHERE type='table'",
				[=](const Database::AsyncQueryResult& res) {
		for (int i = 0; i < res.count(); i++) {
			ui->cbTables->addItem(res.value(i, "name").toString());
		}
		if (res.count() > 0) {
			ui->cbTables->setCurrentIndex(0);
		}
	});

	_tableModel = new Database::AsyncQueryModel(this);
	ui->tvTables->setModel(_tableModel);

	connect (_tableModel->asyncQuery(), SIGNAL(busyChanged(bool)),
			 this, SLOT(onBusyChanged(bool)));

	//setup SqlStatement tab
	ui->teQuery->setText(QString()
				+"SELECT o.OrderID, c.CompanyName, e.FirstName, e.LastName\n"
				+"FROM Orders o\n"
				+"    JOIN Employees e ON (e.EmployeeID = o.EmployeeID)\n"
				+"    JOIN Customers c ON (c.CustomerID = o.CustomerID)\n"
				+"WHERE o.ShippedDate > o.RequiredDate AND o.OrderDate > '1-Jan-1998'\n"
				+"ORDER BY c.CompanyName;\n"
				);


	_queryModel = new Database::AsyncQueryModel(this);
	ui->tvQuery->setModel(_queryModel);

	connect (_queryModel->asyncQuery(), SIGNAL(busyChanged(bool)),
			 this, SLOT(onBusyChanged(bool)));
	connect (ui->btnQuery, &QPushButton::clicked,
			 [=] {
		_queryModel->startExec(ui->teQuery->toPlainText());
	});


	//setup mode tab
	connect (ui->btnExec4Queries, SIGNAL(clicked(bool)),
			 this, SLOT(onExec4Queries(bool)));
	connect (ui->btnClearResults, SIGNAL(clicked(bool)),
			 this, SLOT(onClearClicked(bool)));


	_aQuery = new Database::AsyncQuery(this);
	connect (_aQuery, SIGNAL(execDone(Database::AsyncQueryResult)),
			 this, SLOT(onExec4QueriesDone(Database::AsyncQueryResult)));
	connect (_aQuery, SIGNAL(busyChanged(bool)),
			 this, SLOT(onBusyChanged(bool)));


	_sliderModel = new Database::AsyncQueryModel(this);
	ui->tvSlider->setModel(_sliderModel);

	connect (_sliderModel->asyncQuery(), SIGNAL(busyChanged(bool)),
			 this, SLOT(onBusyChanged(bool)));

	connect (ui->slUnitPrice, SIGNAL(valueChanged(int)),
			 this, SLOT(onSliderChanged(int)));

}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::onBusyChanged(bool busy)
{
	if (busy) {
		ui->pbLoad->setMaximum(0);
		ui->pbLoad->setValue(0);
	} else {
		ui->pbLoad->setMaximum(1);
		ui->pbLoad->setValue(1);
	}
}

void MainWindow::onComboBoxChanged(const QString &index)
{
	_tableModel->startExec("SELECT * FROM '" + index + "'");
}

void MainWindow::onClearClicked(bool clicked)
{
	Q_UNUSED(clicked);
	ui->lblCategroies->setText("---");
	ui->lblCustomers->setText("---");
	ui->lblEmployees->setText("---");
	ui->lblOrders->setText("---");
}

void MainWindow::onExec4Queries(bool clicked)
{
	Q_UNUSED(clicked)
	onClearClicked(true);
	if (ui->rbParallel->isChecked()) {
		_aQuery->setMode(Database::AsyncQuery::Mode_Parallel);
	} else if (ui->rbFifo->isChecked()) {
		_aQuery->setMode(Database::AsyncQuery::Mode_Fifo);
	} else if (ui->rbSkipPrevious->isChecked()) {
		_aQuery->setMode(Database::AsyncQuery::Mode_SkipPrevious);
	}

	if (ui->cbDelay->isChecked()) {
		_aQuery->setDelayMs(500);
	} else {
		_aQuery->setDelayMs(0);
	}

	_aQuery->startExec("SELECT COUNT(*) AS NumCategories FROM Categories");
	_aQuery->startExec("SELECT COUNT(*) AS NumCustomers FROM Customers");
	_aQuery->startExec("SELECT COUNT(*) AS NumEmployees FROM Employees");
	_aQuery->startExec("SELECT COUNT(*) AS NumOrders FROM Orders");
}

void MainWindow::onExec4QueriesDone(const Database::AsyncQueryResult &result)
{
	if (result.headRecord().fieldName(0) == "NumCategories") {
		ui->lblCategroies->setText(result.value(0,0).toString());
	} else if (result.headRecord().fieldName(0) == "NumCustomers") {
		ui->lblCustomers->setText(result.value(0,0).toString());
	} else if (result.headRecord().fieldName(0) == "NumEmployees") {
		ui->lblEmployees->setText(result.value(0,0).toString());
	} else if (result.headRecord().fieldName(0) == "NumOrders") {
		ui->lblOrders->setText(result.value(0,0).toString());
	}
}

void MainWindow::onSliderChanged(int value)
{
	ui->lblSlider->setText(QString::number(value));
	Database::AsyncQuery *aQuery = _sliderModel->asyncQuery();

	if (ui->rbParallel->isChecked()) {
		aQuery->setMode(Database::AsyncQuery::Mode_Parallel);
	} else if (ui->rbFifo->isChecked()) {
		aQuery->setMode(Database::AsyncQuery::Mode_Fifo);
	} else if (ui->rbSkipPrevious->isChecked()) {
		aQuery->setMode(Database::AsyncQuery::Mode_SkipPrevious);
	}

	if (ui->cbDelay->isChecked()) {
		aQuery->setDelayMs(500);
	} else {
		aQuery->setDelayMs(0);
	}

	aQuery->prepare("SELECT * FROM Products WHERE UnitPrice < :price");
	aQuery->bindValue(":price", value);
	aQuery->startExec();
}
