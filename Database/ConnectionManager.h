#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QThread>
#include <QMutex>
#include <QSql>
#include <QSqlDatabase>

#include <QLoggingCategory>


namespace Database {

/**
 * @brief Maintains the database connection for asynchrone queries.
 *
 * @details Set up the ConnectionManager (database type, name, etc...) at programm
 * startup with createInstance() and setting the appropriate values to the instance. The
 * ConnectionManager instance provides the same interface as QSqlDatabase for initializing
 * the connection. A asynchrone query AsyncQuery inernally uses the configured instance
 * and opens a connection for each thread.
 *
 * Before application shutdown the instance have to be destroyed with destroyInstance().
 *
 * @note All functions are thread save and reentrant
 */
class ConnectionManager : public QObject
{
	Q_OBJECT

	/** Number if open connections. */
	Q_PROPERTY(int connectionCount READ connectionCount NOTIFY connectionCountChanged)

public:
	/**
	 * @brief Call createInstance for initialization.
	 * @return The ConnectionManager instance.
	 */
	static ConnectionManager *createInstance();

	/**
	 * @brief Get the instance.
	 * @details If the instance is not created it will be created.
	 * @return The ConnectionManager instance.
	 */
	static ConnectionManager *instance();

	/**
	 * @brief Delete the ConnectionManager instance.
	 */
	static void destroyInstance();

	/**
	 * @name Wrapper methods around QSqlDatabase
	 * @note Each connection which was already opened (e.g. by the AsyncQuery) are not
	 * reopened on change.
	 */
	///@{

	/**
	 * @brief Set the database driver. Valid values are e.g. "QMYSQL", "QSQLITE", ...
	 * @see http://doc.qt.io/qt-5/sql-driver.html
	 */
	void setType(QString type);
	QString type();

	void setHostName(const QString & host);
	QString	hostName() const;

	void setPort(int port);
	int	port() const;

	void setDatabaseName(const QString& name);
	QString databaseName() const;

	void setUserName(const QString & name);
	QString	userName() const;

	void setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy);
	QSql::NumericalPrecisionPolicy	numericalPrecisionPolicy() const;

	void setPassword(const QString & password);
	QString	password() const;
	///@}

	///@{
	/**
	  * @name Connection maintainance. Basically for AsyncQuery internal usage.
	  */

	/**
	 * @brief Number of open connections.
	 */
	int connectionCount() const;

	/**
	 * @brief Returns \c true, if a database connection for thread t exists.
	 * @param t [optional] thread Object.
	 */
	bool connectionExists(QThread* t = QThread::currentThread()) const;

	/**
	 * @brief Opens a database connection for current thread.
	 * @returns \c true on success
	 */
	bool open();

	/**
	 * @brief Check if a connection exists for current thread.
	 * @note If no connection exists QSqlDatabase::isValid() is \c false
	 */
	QSqlDatabase threadConnection() const;

	/** @brief Dump all connections to tracelog */
	void dump();

	/**
	 * @brief Close all open connections.
	 */
	void closeAll();

	/**
	 * @brief Close connection for thread t.
	 * @note If connection does not exists nothing happens.
	 */
	void closeOne(QThread* t);
	///@}

signals:
	/**
	 * @brief Is emitted if the number of connections is changed.
	 */
	void connectionCountChanged(int);

private:
	ConnectionManager(QObject* parent = nullptr);
	virtual ~ConnectionManager();

	//the static instance
	static ConnectionManager *_instance;
	static QMutex _instanceMutex;

	mutable QMutex _mutex;
	QMap<QThread*, QSqlDatabase> _conns;

	QString	_hostName;
	int	_port;
	QString	_userName;
	QString _databaseName;
	QSql::NumericalPrecisionPolicy	_precisionPolicy;
	QString	_password;
	QString _type;

	QLoggingCategory logger;
};

} // namespace

