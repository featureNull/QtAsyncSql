#include "AsyncQuery.h"
#include "ConnectionManager.h"

#include <QRunnable>
#include <QElapsedTimer>
#include <QSqlQuery>
#include <QThreadPool>
#include <QQueue>


namespace Database {

class SqlTaskPrivate : public QRunnable
{
public:
	SqlTaskPrivate(AsyncQuery *instance, AsyncQuery::QueuedQuery query,
				   ulong delayMs = 0);

	void run() override;

private:
	AsyncQuery* _instance;
	AsyncQuery::QueuedQuery _query;
	ulong _delayMs;

};

SqlTaskPrivate::SqlTaskPrivate(AsyncQuery *instance, AsyncQuery::QueuedQuery query,
							   ulong delayMs)
	: _instance(instance)
	, _query(query)
	, _delayMs(delayMs)
{
}

void SqlTaskPrivate::run()
{
	QElapsedTimer timer;
	timer.start();

	Q_ASSERT(_instance);

	ConnectionManager* conmgr = ConnectionManager::instance();
	if (!conmgr->connectionExists()) {
		bool ret = conmgr->open();
		Q_ASSERT(ret);
	}

	QSqlDatabase db = conmgr->threadConnection();

	//delay query
	if (_delayMs > 0) {
		QThread::currentThread()->msleep(_delayMs);
	}

	AsyncQueryResult result;
	QSqlQuery query = QSqlQuery(db);
	bool succ = true;
	if (_query.isPrepared) {
		succ = query.prepare(_query.query);
		//bind values
		QMapIterator<QString, QVariant> i(_query.boundValues);
		while (i.hasNext()) {
			i.next();
			query.bindValue(i.key(), i.value());
		}
	}
	if (succ) {
		if (_query.isPrepared) {
			query.exec();
		}
		else {
			query.exec(_query.query);
		}
	}

	result._record = query.record();
	result._error = query.lastError();
	int cols = result._record.count();

	while (query.next()) {
		QVector<QVariant> currow(cols);

		for (int ii = 0; ii < cols; ii++) {
			if (query.isNull(ii)) {
				currow[ii] = QVariant();
			}
			else {
				currow[ii] = query.value(ii);
			}
		}
		result._data.append(currow);
	}

	//send result
	_instance->taskCallback(result);
}

/****************************************************************************************/
/*                                          AsyncQuery                                  */
/****************************************************************************************/


AsyncQuery::AsyncQuery(QObject* parent /* = nullptr */)
	: QObject(parent), logger("Database.AsyncQuery")
	, _deleteOnDone(false)
	, _delayMs(0)
	, _mode(Mode_Parallel)
	, _taskCnt(0)
{
}

AsyncQuery::~AsyncQuery()
{
}

void AsyncQuery::setMode(AsyncQuery::Mode mode)
{
	QMutexLocker locker(&_mutex);
	_mode = mode;
}

AsyncQuery::Mode AsyncQuery::mode()
{
	QMutexLocker locker(&_mutex);
	return _mode;
}

bool AsyncQuery::isRunning() const
{
	QMutexLocker lock(&_mutex);
	return (_taskCnt > 0);
}

AsyncQueryResult AsyncQuery::result() const
{
	QMutexLocker lock(&_mutex);
	return _result;
}

void AsyncQuery::prepare(const QString &query)
{
	_curQuery.query = query;
}

void AsyncQuery::bindValue(const QString &placeholder, const QVariant &val)
{
	_curQuery.boundValues[placeholder] = val;
}

void AsyncQuery::startExec()
{
	_curQuery.isPrepared = true;
	startExecIntern();
}

void AsyncQuery::startExec(const QString &query)
{
	_curQuery.isPrepared = false;
	_curQuery.query = query;
	startExecIntern();

}

bool AsyncQuery::waitDone(ulong msTimout)
{
	QMutexLocker lock(&_mutex);
	if (_taskCnt > 0)
		return _waitcondition.wait(&_mutex, msTimout);
	else
		return true;
}

void AsyncQuery::startExecOnce(const QString &query, QObject *receiver, const char *member)
{
	AsyncQuery *q = new AsyncQuery();
	q->_deleteOnDone = true;
	connect(q, SIGNAL(execDone(Database::AsyncQueryResult)),
			receiver, member);
	q->startExec(query);
}

void AsyncQuery::setDelayMs(ulong ms)
{
	QMutexLocker locker(&_mutex);
	_delayMs = ms;
}

void AsyncQuery::startExecIntern()
{
	QMutexLocker lock(&_mutex);
	if (_mode == Mode_Parallel) {
		QThreadPool* pool = QThreadPool::globalInstance();
		SqlTaskPrivate* task = new SqlTaskPrivate(this, _curQuery, _delayMs);
		incTaskCount();
		pool->start(task);
	} else {
		if (_taskCnt == 0) {
			QThreadPool* pool = QThreadPool::globalInstance();
			SqlTaskPrivate* task = new SqlTaskPrivate(this, _curQuery, _delayMs);
			incTaskCount();
			pool->start(task);
		} else {
			if (_mode == Mode_Fifo) {
				_ququ.enqueue(_curQuery);
			} else {
				_ququ.clear();
				_ququ.enqueue(_curQuery);
			}
		}
	}
}

void AsyncQuery::incTaskCount()
{
	if (_taskCnt == 0) {
		emit busyChanged(true);
	}
	_taskCnt++;
}

void AsyncQuery::decTaskCount()
{
	if (_taskCnt == 1) {
		emit busyChanged(false);
	}
	_taskCnt--;

}

void AsyncQuery::taskCallback(const AsyncQueryResult& result)
{
	_mutex.lock();
	Q_ASSERT(_taskCnt > 0);
	_result = result;
	if (_mode != Mode_Parallel && !_ququ.isEmpty()) {
		//start next query if queue not empty
		QueuedQuery query = _ququ.dequeue();
		QThreadPool* pool = QThreadPool::globalInstance();
		SqlTaskPrivate* task = new SqlTaskPrivate(this, query, _delayMs);
		pool->start(task);
	} else {
		decTaskCount();
	}

	_waitcondition.wakeAll();
	_mutex.unlock();

	emit execDone(result);

	if (_deleteOnDone) {
		// note delete later should be thread save
		deleteLater();
	}
}
}
