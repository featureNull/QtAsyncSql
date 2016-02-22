#pragma once

#include <QMetaType>
#include <QSqlRecord>
#include <QVector>
#include <QVariant>
#include <QSqlError>


namespace Database {

// class forward decls's
class SqlTaskPrivate;

/**
* @brief Represent a AsyncQuery result.
* @details The query result is retreived via the getter functions. If an sql error
* occured AsyncQueryResult is not isValid() and the error can retrieved with
* error().
*
*/
class AsyncQueryResult
{
friend class SqlTaskPrivate;

public:
	AsyncQueryResult();
	virtual ~AsyncQueryResult();
	AsyncQueryResult(const AsyncQueryResult&);
	AsyncQueryResult& operator=(const AsyncQueryResult& other);

	/**
	 * @brief Returns \c true if no error occured in the query.
	 */
	bool isValid() const;

	/**
	 * @brief Retrieve the sql error of the query.
	 */
	QSqlError error() const;

	/**
	 * @brief Returns the head record to retrieve column names of the table.
	 */
	QSqlRecord headRecord() const;

	/**
	 * @brief Returns the number of rows in the result.
	 */
	int  count() const;

	/**
	 * @brief Returns the QSqlRecord of given row.
	 */
	QSqlRecord record(int row) const;

	/**
	 * @brief Returns the value of given row and columns.
	 * @details If row or col is invalid a empty QVariatn is returned.
	 */
	QVariant value(int row, int col) const;

	/**
	 * @brief Returns the value of given row and column name.
	 * @details If row or col is invalid a empty QVariatn is returned.
	 */
	QVariant value(int row, const QString &col) const;

	/**
	 * @brief Returns internal raw data structure of result.
	 */
	QVector<QVector<QVariant>> data() const { return _data; }

private:
	QVector<QVector<QVariant>> _data;
	QSqlRecord _record;
	QSqlError _error;
};

}	//	namespace

Q_DECLARE_METATYPE(Database::AsyncQueryResult)
