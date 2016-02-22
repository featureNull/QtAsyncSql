# QtAsyncSql
QT based classes to support asynchronous and threaded SQL queries.
 
##Introduction

###Overview
Qt provides QSqlQuery class for synchronous database access. Often asynchronous and threaded database access is desired. This implementation provides a implementation f or asynchronous database access using the Qt Api.

### Features 
* Asynchronous SQL queries with SIGNAL/SLOT mechanism.
* Database access from distinct threads. 
(Fills the gap of Qt's Database Api: *"A connection can only be used from within the thread that created it."* See http://doc.qt.io/qt-5/threads-modules.html#threads-and-the-sql-module).
* Fast Parallel query execution. 
AsyncQueries internally are distributed via QRunnable tasks in a QThreadPool which is designed to optimally leverage the available number of cores on your hardware. There is no massive thread generation if a lot of queries are started.
* Different execution modes Parallel, Fifo and SkipPrevious.

### Make
To build the demo application simply run  `qmake && make` or open `QtAsyncSql.pro` file in qtCreator. 
For the implementation **Qt5.5** and **c++11** compiler was used (Note that the source should be adaptable without much effort to **Qt4**  and lower c++ standards.). 


###Example Usage
All relevant classes (can be found in the folder Database) are embedded in the namspace `Database`. In the main function the ConnectionManager need to be created, set up and destroyed:
```cpp
int main(int argc, char *argv[])
{
	Database::ConnectionManager *mgr = Database::ConnectionManager::createInstance();
	mgr->setType("QSQLITE");	//set the driver
	mgr->setDatabaseName( "/data/Northwind.sl3"); //set database name
	mgr->setHostname(...) //set host

	//... do the main loop

	Database::ConnectionManager::destroyInstance();
}
```
Next in any thread an AsyncQuery can be created, configured and started:
```cpp
	Database::AsyncQuery query = new Database::AsyncQuery();
	connect (query, SIGNAL(execDone(Database::AsyncQueryResult)),
			 this, SLOT(onExecDone(Database::AsyncQueryResult)));
	query->startExec("SELECT * FROM Companies");
	//...execution continous immediatly
```

When database access is finished the SIGNAL is triggered and the SLOT prints the result table:

```
void MainWindow::onExecDone(const Database::AsyncQueryResult &result)
{
	if (!result.isValid()) {
		qDebug() << "SqlError" << result.error().text();
	} else {
		int columns = result.headRecord().count();
		for (int row = 0; row < result.count(); row++) {
			for (int col = 0; col < columns; col++) {
				qDebug() << result.value(row, col).toString();
			}
		}
	}
}
```
### Demo Application
The QtAsyncSql Demo application (build the application) demonstrates all provided features.

##Details
This section describes the implemented interface. For further details it is refered to the comments in the header files.

###ConnectionManager Class
Maintains the database connection for asynchrone queries. Internally several connection are opened to access the database from different threads.

###AsyncQuery Class
Asynchronous queries are started via:
```
void startExec(const QString &query);
```
 There is also support for prepared statements with value binding.  (Note that dabase preparation is currently done each time startExec() is called -> need to be fixed.)
```
void prepare(const QString &query);
void bindValue(const QString &placeholder, const QVariant &val);
void startExec();
```
Following signals are provided:
```
void execDone(const Database::AsyncQueryResult& result); // query has finished execution
void busyChanged(bool busy); // busy indicator
```

#### Modes
* **Mode_Parallel** (default)
 All queries for this object are started immediately and run in parallel. The order in which subsequent queries are executed and finished can not be guaranteed. Each query is started as soon as possible. 
* **Mode_Fifo**
Subsquent queries for this object are started in a Fifo fashion. A Subsequent query waits until the last query is finished. This guarantees the order of query sequences. 
* **Mode_SkipPrevious**
 Same as **Mode_Fifo**, but if a previous startExec call is not executed yet it is skipped and overwritten by the currrent query. E.g. if a graphical slider is bound to a sql query heavy database access can be ommited by using this mode.

####Convenience Functions
If a query should be executed just once AsynQuery provides 2 static convenience functions (`static void startExecOnce(...)`) where no explicit object needs to be created
```
//signal slot style
 Database::AsyncQuery::startExecOnce("SELECT name FROM sqlite_master WHERE type='table'",
	this, SLOT(myExecDoneHandler(const Database::AsyncQueryResult &)));
//and lambda style
Database::AsyncQuery::startExecOnce("SELECT name FROM sqlite_master WHERE type='table'",
	 [=](const Database::AsyncQueryResult& res) {
		//do handling directly here in lambda
	});
```


##AsyncQueryResult Class
The query result is retreived via the getter functions. If an sql error occured AsyncQueryResult is not valid and the error can be retrieved.

##AsyncQueryModel Class
The AsyncQueryModel class implementents a QtAbstractTableModel for asynchronous queries which can be used with a QTableView to show the query results.