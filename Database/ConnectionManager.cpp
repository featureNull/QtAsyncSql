#include "ConnectionManager.h"
#include <QSqlError>


namespace Database {

ConnectionManager *ConnectionManager::_instance = nullptr;
QMutex ConnectionManager::_instanceMutex;

ConnectionManager::ConnectionManager(QObject* parent /*= nullptr */)
	: QObject(parent), logger("Database.ConnectionManager")
{
	_port = -1;
	_precisionPolicy = QSql::LowPrecisionDouble;
	_type = "QMYSQL";
}

ConnectionManager::~ConnectionManager()
{
	closeAll();
}

ConnectionManager *ConnectionManager::createInstance()
{
	return instance();
}

ConnectionManager *ConnectionManager::instance()
{
	QMutexLocker locker(&_instanceMutex);
	if (_instance == nullptr) {
		_instance = new ConnectionManager();
	}
	return _instance;
}

void ConnectionManager::destroyInstance()
{
	QMutexLocker locker(&_instanceMutex);
	if (_instance != nullptr) {
		delete _instance; _instance = nullptr;
	}
}

void ConnectionManager::setType(QString type)
{
	QMutexLocker locker(&_mutex);
	_type = type;
}

QString ConnectionManager::type()
{
	QMutexLocker locker(&_mutex);
	return _type;
}

void ConnectionManager::setHostName(const QString & host)
{
	QMutexLocker locker(&_mutex);
	_hostName = host;
}

QString	ConnectionManager::hostName() const
{
	QMutexLocker locker(&_mutex);
	return _hostName;
}

void ConnectionManager::setPort(int port)
{
	QMutexLocker locker(&_mutex);
	_port = port;
}

int	ConnectionManager::port() const
{
	QMutexLocker locker(&_mutex);
	return _port;
}

void ConnectionManager::setDatabaseName(const QString& name)
{
	QMutexLocker locker(&_mutex);
	_databaseName = name;
}

QString ConnectionManager::databaseName() const
{
	QMutexLocker locker(&_mutex);
	return _databaseName;
}


void ConnectionManager::setUserName(const QString & name)
{
	QMutexLocker locker(&_mutex);
	_userName = name;
}

QString	ConnectionManager::userName() const
{
	QMutexLocker locker(&_mutex);
	return _userName;
}

void ConnectionManager::setNumericalPrecisionPolicy(
	QSql::NumericalPrecisionPolicy precisionPolicy)
{
	QMutexLocker locker(&_mutex);
	_precisionPolicy = precisionPolicy;
}

QSql::NumericalPrecisionPolicy
	ConnectionManager::numericalPrecisionPolicy() const
{
	QMutexLocker locker(&_mutex);
	return _precisionPolicy;
}

void ConnectionManager::setPassword(const QString & password)
{
	QMutexLocker locker(&_mutex);
	_password = password;
}

QString	ConnectionManager::password() const
{
	QMutexLocker locker(&_mutex);
	return _password;
}

int ConnectionManager::connectionCount() const
{
	QMutexLocker locker(&_mutex);
	return _conns.count();
}

bool ConnectionManager::connectionExists(QThread* t /*= QThread::currentThread()*/) const
{
	QMutexLocker locker(&_mutex);
	return _conns.contains(t);
}

bool ConnectionManager::open()
{
	QMutexLocker locker(&_mutex);

	QThread* curThread = QThread::currentThread();

	if (_conns.contains(curThread)) {
		qCWarning(logger) << "ConnectionManager::open: "
			"there is a open connection";
		return true;
	}

	QString conname = QString("CNM0x%1") .arg((qlonglong)curThread, 0, 16);
	QSqlDatabase dbconn = QSqlDatabase::addDatabase(_type, conname);
	dbconn.setHostName(_hostName);
	dbconn.setDatabaseName(_databaseName);
	dbconn.setUserName(_userName);
	dbconn.setPassword(_password);
	dbconn.setPort(_port);

	bool ok = dbconn.open();

	if (ok != true) {
		qCCritical(logger) << "ConnectionManager::open: con= " << conname
			<< ": Connection error=" << dbconn.lastError().text();
		return false;
	}

	_conns.insert(curThread, dbconn);

	return true;
}

QSqlDatabase ConnectionManager::threadConnection() const
{
	QMutexLocker locker(&_mutex);
	QThread* curThread = QThread::currentThread();
	QSqlDatabase ret = _conns.value(curThread, QSqlDatabase());
	return ret;
}

void ConnectionManager::dump()
{
	qCInfo(logger) << "Database connections:" << _conns;
}

void ConnectionManager::closeAll()
{
	QMutexLocker locker(&_mutex);
	/// @attention es koennte sein, dass das nicht geht, weil falscher thread

	while (_conns.count()) {
		QThread* t = _conns.firstKey();
		QSqlDatabase db = _conns.take(t);
		db.close();
	}
}

void ConnectionManager::closeOne(QThread* t)
{
	QMutexLocker locker(&_mutex);
	/// @attention es koennte sein, dass das nicht geht, wenn falscher thread

	if (!_conns.contains(t)) {
		qCWarning(logger) << "closeOne no Connection open for thread " << t;
		return;
	}

	QSqlDatabase db = _conns.take(t);
	db.close();
}

}	//	namespace
