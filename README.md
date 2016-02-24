# QtAsyncSql
QT based classes to support asynchronous and threaded SQL queries.
 
##Introduction

###Overview
Qt provides QSqlQuery class for synchronous database access. Often asynchronous and threaded database access is desired. QtAsyncSql provides a implementation for asynchronous database access using the Qt Api.

### Features 
* Asynchronous SQL queries with Qt's SIGNAL and SLOT mechanism.
* Database access from distinct threads. 
(Closes the gap of Qt's Database Api: *"A connection can only be used from within the thread that created it."* See http://doc.qt.io/qt-5/threads-modules.html#threads-and-the-sql-module).
* Fast parallel query execution. 
AsyncQueries internally are distributed via QRunnable tasks in a QThreadPool which is designed to optimally leverage the available number of cores on your hardware. There is no massive thread generation if a lot of queries are started.
* Different execution modes *Parallel*, *Fifo* and *SkipPrevious*.

### Make
To build the demo application simply run  `qmake && make` or open `QtAsyncSql.pro` file in qtCreator. 
For the implementation **Qt5.5** and **c++11** compiler was used (Note that the source should be adaptable without much effort to **Qt4**  and lower c++ standards.). 


###Example Usage
All relevant classes (can be found in the folder Database) are embedded in the namespace `Database`. In the main function the ConnectionManager need to be created, set up and destroyed:
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

```cpp
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
Maintains the database connection for asynchrone queries. Internally several connections are opened to access the database from different threads.

###AsyncQuery Class
Asynchronous queries are started via:
```cpp
void startExec(const QString &query);
```
 There is also support for prepared statements with value binding.  (Note that dabase preparation is currently done each time startExec() is called -> need to be fixed.)
```cpp
void prepare(const QString &query);
void bindValue(const QString &placeholder, const QVariant &val);
void startExec();
```
Following signals are provided:
```cpp
void execDone(const Database::AsyncQueryResult& result); // query has finished execution
void busyChanged(bool busy); // busy indicator
```

#### Modes
The different modes define how subsequent `startExec(...)` calls are handled.

* **Mode_Parallel** (default)
 All queries started by the AsyncQuery object are started immediately and run in parallel. The order in which subsequent queries are executed and finished can not be guaranteed. Each query is started as soon as possible. 
* **Mode_Fifo**
Subsquent queries for the AsyncQuery object are started in a Fifo fashion. A Subsequent query waits until the last query is finished. This guarantees the order of query sequences. 
* **Mode_SkipPrevious**
 Same as **Mode_Fifo**, but if a previous `startExec(...)` call is not executed yet it is skipped and overwritten by the currrent query. E.g. if a graphical slider is bound to a sql query heavy database access can be ommited by using this mode (see the demo application).

####Convenience Functions
If a query should be executed just once AsynQuery provides 2 static convenience functions (`static void startExecOnce
(...)`) where no explicit object needs to be created.
```cpp
static void startExecOnce(const QString& query, QObject* receiver,const char* member);
template <typename Func1>
	static inline void startExecOnce(const QString& query, Func1 slot)
```
Which can be used as follows:

```cpp
//signal slot style
 Database::AsyncQuery::startExecOnce("SELECT name FROM sqlite_master WHERE type='table'",
	this, SLOT(myExecDoneHandler(const Database::AsyncQueryResult &)));
//and lambda style
Database::AsyncQuery::startExecOnce("SELECT name FROM sqlite_master WHERE type='table'",
	 [=](const Database::AsyncQueryResult& res) {
		//do handling directly here in lambda
	});
```

####Others
Block the calling thread until all started queries are executed (allow synchronous execution):
```
bool waitDone(ulong msTimout = ULONG_MAX);
```
Delay each query execution for `ms` milliseconds before it is started. This does not block the calling thread, but only the internall query thread  (is mainly used for testing, see the demo):
```cpp
void setDelayMs(ulong ms);
```


###AsyncQueryResult Class
The query result is retreived via the getter functions. If an sql error occured AsyncQueryResult is not valid and the error can be retrieved.

###AsyncQueryModel Class
The AsyncQueryModel class implementents a QtAbstractTableModel for asynchronous queries which can be used with a QTableView to show the query results.

Create the model and bind it to a view:
```cpp
Database::AsyncQueryModel *queryModel = new Database::AsyncQueryModel();
QTableView *view = new QTableView();
view->setModel(queryModel);
view->show();
```
Start query directly:
```cpp
queryModel->startExec("SELECT * FROM Companies"); //updates the bound views
```
Or via the model's AsyncQuery object:
```cpp
Database::AsyncQuery *query = queryModel->asyncQuery();
query->prepare("SELECT * FROM Products WHERE UnitPrice < :price");
query->bindValue(":price", value);
query->startExec(); //updates the bound views
```