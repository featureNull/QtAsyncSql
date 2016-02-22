#pragma once

#include "AsyncQueryResult.h"

#include <QObject>
#include <QString>
#include <QSqlError>
#include <QLoggingCategory>
#include <QWaitCondition>
#include <QMutex>
#include <QQueue>

namespace Database {

// class forward decl's
class SqlTaskPrivate;

/**
 * @brief Class to run a asynchron sql query.
 *
 * @details This class provides functionalities to execute sql queries in a
 * asynchrounous way. The interface is similar to the Qt's synchronous QSqlQuery
 * http://doc.qt.io/qt-5/qsqlquery.html
 *
 * Create a AsyncQuery, connect a handler to the
 * execDone(const Database::AsyncQueryResult &result) signal and start the query
 * with startExec(const QString &query). The query is started in a proper thread and
 * the connected slot is called when finished. Queries are internally maintained in
 * a QThreadPool. By using the QThreadPool the execution of queries is optimized to
 * the available cores on the cpu and threads are not blindly generated.
 *
 * QSqlDatabase's can be only be used from within the thread that created it. This
 * class provides a solution to run queries also from  different threads
 * (http://doc.qt.io/qt-5/threads-modules.html#threads-and-the-sql-module).
 *
 */
class AsyncQuery : public QObject
{
	friend class SqlTaskPrivate;
	Q_OBJECT

public:
	/**
	 * @brief The Mode defines how subsequent queries, triggered with
	 * startExec() or startExec(const QString &query) are handled.
	 */

	typedef enum Mode {
		/** All queries for this object are started immediately and run in parallel.
		 * The order in which subsequent queries are executed and finished can not
		 * be guaranteed. Each query is started as soon as possible.
		 */
		Mode_Parallel,
		/** Subsquent queries for this object are started in a Fifo fashion.
		 * A Subsequent query waits until the last query is finished.
		 * This guarantees the order of query sequences.
		 */
		Mode_Fifo,
		/** Same as Mode_Fifo, but if a previous startExec call is not executed
		 * yet it is skipped and overwritten by the current query. E.g. if a
		 * graphical slider is bound to a sql query heavy database access can be
		 * ommited by using this mode.
		 */
		Mode_SkipPrevious,
	} Mode;

	explicit AsyncQuery(QObject* parent = nullptr);
	virtual ~AsyncQuery();

	/**
	 * @brief Set the mode how subsequent queries are executed.
	 */
	void setMode(AsyncQuery::Mode mode);
	AsyncQuery::Mode mode();

	/**
	 * @brief Are there any queries running.
	 */
	bool isRunning() const;

	/**
	 * @brief Retrieve the result of the last query
	 */
	AsyncQueryResult result() const;

	/**
	 * @brief Prepare a AsyncQuery
	 */
	void prepare(const QString &query);

	/**
	 * @brief BindValue for prepared query.
	 */
	void bindValue(const QString &placeholder, const QVariant &val);

	/**
	 * @brief Start a prepared query execution set with prepare(const QString &query);
	 */
	void startExec(); //start

	/**
	 * @brief Start the execution of the query.
	 */
	void startExec(const QString & query);

	/**
	 * @brief Wait for query is finished
	 * @details This function blocks the calling thread until query is finsihed. Using
	 * this function provides same functionallity as Qt's synchron QSqlQuery.
	 */
	bool waitDone(ulong msTimout = ULONG_MAX);

	/**
	 * @brief Convinience function to start a AsyncQuery once with given slot as result
	 * handler.
	 * @details Sample Usage:
	 * \code{.cpp}
	 * Database::AsyncQuery::startExecOnce(
	 *        "SELECT name FROM sqlite_master WHERE type='table'",
	 *        this, SLOT(myExecDoneHandler(const Database::AsyncQueryResult &)));
	 * \endcode
	 */
	static void startExecOnce(const QString& query, QObject* receiver,const char* member);

	/**
	 * @brief Convinience function to start a AsyncQuery once with given lambda function
	 * as result handler.
	 * @details Sample Usage:
	 * \code{.cpp}
	 * Database::AsyncQuery::startExecOnce(
	 *        "SELECT name FROM sqlite_master WHERE type='table'",
	 *        [=](const Database::AsyncQueryResult& res) {
	 *            //do handling here
	 * });
	 * \endcode
	 */
	template <typename Func1>
	static inline void startExecOnce(const QString& query, Func1 slot)
	{
		AsyncQuery * q = new AsyncQuery();
		q->_deleteOnDone =true;
		connect(q, &AsyncQuery::execDone, slot);
		q->startExec(query);
	}	

	/**
	 * @brief Set delay to execute query. Mainly used for testing.
	 * @details The executing query thread sleeps ms before query is executed.
	 */
	void setDelayMs(ulong ms);

signals:
	/**
	 * @brief Is emited when asynchronous query is done.
	 */
	void execDone(const Database::AsyncQueryResult& result);
	/**
	 * @brief Is emited if asynchronous query running status changes.
	 */
	void busyChanged(bool busy);

private:
	typedef struct QueuedQuery {
		bool isPrepared;
		QString query;
		QMap <QString, QVariant> boundValues;
	} QueuedQuery;

	void startExecIntern();
	/* use only in locked area */
	void incTaskCount();
	void decTaskCount();

	// asynchronous callbacks
	// attention lives in the context of QRunable
	void taskCallback(const AsyncQueryResult& result);


private:
	QLoggingCategory logger;

	QWaitCondition _waitcondition;
	mutable QMutex _mutex;
	bool _deleteOnDone;
	ulong _delayMs;
	Mode _mode;
	int _taskCnt;

	AsyncQueryResult _result;
	QQueue <QueuedQuery> _ququ;
	QueuedQuery _curQuery;

};

}
